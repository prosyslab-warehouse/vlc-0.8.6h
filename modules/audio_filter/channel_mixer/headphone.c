/*****************************************************************************
 * headphone.c : headphone virtual spatialization channel mixer module
 *               -> gives the feeling of a real room with a simple headphone
 *****************************************************************************
 * Copyright (C) 2002-2006 the VideoLAN team
 * $Id: 2c174571220ecf1b4c1166ffa3b6a0d26c73b19e $
 *
 * Authors: Boris Dorès <babal@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <stdlib.h>                                      /* malloc(), free() */
#include <string.h>
#include <math.h>                                        /* sqrt */

#include <vlc/vlc.h>
#include "audio_output.h"
#include "aout_internal.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  Create    ( vlc_object_t * );
static void Destroy   ( vlc_object_t * );

static void DoWork    ( aout_instance_t *, aout_filter_t *, aout_buffer_t *,
                        aout_buffer_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define MODULE_DESCRIPTION N_ ( \
     "This effect gives you the feeling that you are standing in a room " \
     "with a complete 7.1 speaker set when using only a headphone, " \
     "providing a more realistic sound experience. It should also be " \
     "more comfortable and less tiring when listening to music for " \
     "long periods of time.\nIt works with any source format from mono " \
     "to 7.1.")

#define HEADPHONE_DIM_TEXT N_("Characteristic dimension")
#define HEADPHONE_DIM_LONGTEXT N_( \
     "Distance between front left speaker and listener in meters.")

#define HEADPHONE_COMPENSATE_TEXT N_("Compensate delay")
#define HEADPHONE_COMPENSATE_LONGTEXT N_( \
     "The delay which is introduced by the physical algorithm may "\
     "sometimes be disturbing for the synchronization between lips-movement "\
     "and speech. In case, turn this on to compensate.")

#define HEADPHONE_DOLBY_TEXT N_("No decoding of Dolby Surround")
#define HEADPHONE_DOLBY_LONGTEXT N_( \
     "Dolby Surround encoded streams won't be decoded before being " \
     "processed by this filter. Enabling this setting is not recommended.")

vlc_module_begin();
    set_description( _("Headphone virtual spatialization effect") );
    set_shortname( _("Headphone effect") );
    set_category( CAT_AUDIO );
    set_subcategory( SUBCAT_AUDIO_AFILTER );

    add_integer( "headphone-dim", 10, NULL, HEADPHONE_DIM_TEXT,
                 HEADPHONE_DIM_LONGTEXT, VLC_FALSE );
    add_bool( "headphone-compensate", 0, NULL, HEADPHONE_COMPENSATE_TEXT,
              HEADPHONE_COMPENSATE_LONGTEXT, VLC_TRUE );
    add_bool( "headphone-dolby", 0, NULL, HEADPHONE_DOLBY_TEXT,
              HEADPHONE_DOLBY_LONGTEXT, VLC_TRUE );

    set_capability( "audio filter", 0 );
    set_callbacks( Create, Destroy );
    add_shortcut( "headphone" );
vlc_module_end();


/*****************************************************************************
 * Internal data structures
 *****************************************************************************/
struct atomic_operation_t
{
    int i_source_channel_offset;
    int i_dest_channel_offset;
    unsigned int i_delay;/* in sample unit */
    double d_amplitude_factor;
};

struct aout_filter_sys_t
{
    size_t i_overflow_buffer_size;/* in bytes */
    byte_t * p_overflow_buffer;
    unsigned int i_nb_atomic_operations;
    struct atomic_operation_t * p_atomic_operations;
};

/*****************************************************************************
 * Init: initialize internal data structures
 * and computes the needed atomic operations
 *****************************************************************************/
/* x and z represent the coordinates of the virtual speaker
 *  relatively to the center of the listener's head, measured in meters :
 *
 *  left              right
 *Z
 *-
 *a          head
 *x
 *i
 *s
 *  rear left    rear right
 *
 *          x-axis
 *  */
static void ComputeChannelOperations ( struct aout_filter_sys_t * p_data
        , unsigned int i_rate , unsigned int i_next_atomic_operation
        , int i_source_channel_offset , double d_x , double d_z
        , double d_compensation_length , double d_channel_amplitude_factor )
{
    double d_c = 340; /*sound celerity (unit: m/s)*/
    double d_compensation_delay = (d_compensation_length-0.1) / d_c * i_rate;

    /* Left ear */
    p_data->p_atomic_operations[i_next_atomic_operation]
        .i_source_channel_offset = i_source_channel_offset;
    p_data->p_atomic_operations[i_next_atomic_operation]
        .i_dest_channel_offset = 0;/* left */
    p_data->p_atomic_operations[i_next_atomic_operation]
        .i_delay = (int)( sqrt( (-0.1-d_x)*(-0.1-d_x) + (0-d_z)*(0-d_z) )
                          / d_c * i_rate - d_compensation_delay );
    if ( d_x < 0 )
    {
        p_data->p_atomic_operations[i_next_atomic_operation]
            .d_amplitude_factor = d_channel_amplitude_factor * 1.1 / 2;
    }
    else if ( d_x > 0 )
    {
        p_data->p_atomic_operations[i_next_atomic_operation]
            .d_amplitude_factor = d_channel_amplitude_factor * 0.9 / 2;
    }
    else
    {
        p_data->p_atomic_operations[i_next_atomic_operation]
            .d_amplitude_factor = d_channel_amplitude_factor / 2;
    }

    /* Right ear */
    p_data->p_atomic_operations[i_next_atomic_operation + 1]
        .i_source_channel_offset = i_source_channel_offset;
    p_data->p_atomic_operations[i_next_atomic_operation + 1]
        .i_dest_channel_offset = 1;/* right */
    p_data->p_atomic_operations[i_next_atomic_operation + 1]
        .i_delay = (int)( sqrt( (0.1-d_x)*(0.1-d_x) + (0-d_z)*(0-d_z) )
                          / d_c * i_rate - d_compensation_delay );
    if ( d_x < 0 )
    {
        p_data->p_atomic_operations[i_next_atomic_operation + 1]
            .d_amplitude_factor = d_channel_amplitude_factor * 0.9 / 2;
    }
    else if ( d_x > 0 )
    {
        p_data->p_atomic_operations[i_next_atomic_operation + 1]
            .d_amplitude_factor = d_channel_amplitude_factor * 1.1 / 2;
    }
    else
    {
        p_data->p_atomic_operations[i_next_atomic_operation + 1]
            .d_amplitude_factor = d_channel_amplitude_factor / 2;
    }
}

static int Init ( aout_filter_t * p_filter , struct aout_filter_sys_t * p_data
        , unsigned int i_nb_channels , uint32_t i_physical_channels
        , unsigned int i_rate )
{
    double d_x = config_GetInt ( p_filter , "headphone-dim" );
    double d_z = d_x;
    double d_z_rear = -d_x/3;
    double d_min = 0;
    unsigned int i_next_atomic_operation;
    int i_source_channel_offset;
    unsigned int i;

    if ( p_data == NULL )
    {
        msg_Dbg ( p_filter, "passing a null pointer as argument" );
        return 0;
    }

    if ( config_GetInt ( p_filter , "headphone-compensate" ) )
    {
        /* minimal distance to any speaker */
        if ( i_physical_channels & AOUT_CHAN_REARCENTER )
        {
            d_min = d_z_rear;
        }
        else
        {
            d_min = d_z;
        }
    }

    /* Number of elementary operations */
    p_data->i_nb_atomic_operations = i_nb_channels * 2;
    if ( i_physical_channels & AOUT_CHAN_CENTER )
    {
        p_data->i_nb_atomic_operations += 2;
    }
    p_data->p_atomic_operations = malloc ( sizeof(struct atomic_operation_t)
            * p_data->i_nb_atomic_operations );
    if ( p_data->p_atomic_operations == NULL )
    {
        msg_Err( p_filter, "out of memory" );
        return -1;
    }

    /* For each virtual speaker, computes elementary wave propagation time
     * to each ear */
    i_next_atomic_operation = 0;
    i_source_channel_offset = 0;
    if ( i_physical_channels & AOUT_CHAN_LEFT )
    {
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , -d_x , d_z , d_min , 2.0 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }
    if ( i_physical_channels & AOUT_CHAN_RIGHT )
    {
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , d_x , d_z , d_min , 2.0 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }
    if ( i_physical_channels & AOUT_CHAN_MIDDLELEFT )
    {
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , -d_x , 0 , d_min , 1.5 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }
    if ( i_physical_channels & AOUT_CHAN_MIDDLERIGHT )
    {
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , d_x , 0 , d_min , 1.5 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }
    if ( i_physical_channels & AOUT_CHAN_REARLEFT )
    {
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , -d_x , d_z_rear , d_min , 1.5 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }
    if ( i_physical_channels & AOUT_CHAN_REARRIGHT )
    {
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , d_x , d_z_rear , d_min , 1.5 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }
    if ( i_physical_channels & AOUT_CHAN_REARCENTER )
    {
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , 0 , -d_z , d_min , 1.5 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }
    if ( i_physical_channels & AOUT_CHAN_CENTER )
    {
        /* having two center channels increases the spatialization effect */
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , d_x / 5.0 , d_z , d_min , 0.75 / i_nb_channels );
        i_next_atomic_operation += 2;
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , -d_x / 5.0 , d_z , d_min , 0.75 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }
    if ( i_physical_channels & AOUT_CHAN_LFE )
    {
        ComputeChannelOperations ( p_data , i_rate
                , i_next_atomic_operation , i_source_channel_offset
                , 0 , d_z_rear , d_min , 5.0 / i_nb_channels );
        i_next_atomic_operation += 2;
        i_source_channel_offset++;
    }

    /* Initialize the overflow buffer
     * we need it because the process induce a delay in the samples */
    p_data->i_overflow_buffer_size = 0;
    for ( i = 0 ; i < p_data->i_nb_atomic_operations ; i++ )
    {
        if ( p_data->i_overflow_buffer_size
                < p_data->p_atomic_operations[i].i_delay * 2 * sizeof (float) )
        {
            p_data->i_overflow_buffer_size
                = p_data->p_atomic_operations[i].i_delay * 2 * sizeof (float);
        }
    }
    p_data->p_overflow_buffer = malloc ( p_data->i_overflow_buffer_size );
    if ( p_data->p_atomic_operations == NULL )
    {
        msg_Err( p_filter, "out of memory" );
        return -1;
    }
    memset ( p_data->p_overflow_buffer , 0 , p_data->i_overflow_buffer_size );

    /* end */
    return 0;
}

/*****************************************************************************
 * Create: allocate headphone downmixer
 *****************************************************************************/
static int Create( vlc_object_t *p_this )
{
    aout_filter_t * p_filter = (aout_filter_t *)p_this;
    vlc_bool_t b_fit = VLC_TRUE;

    /* Activate this filter only with stereo devices */
    if ( p_filter->output.i_physical_channels
            != (AOUT_CHAN_LEFT|AOUT_CHAN_RIGHT) )
    {
        msg_Dbg( p_filter, "filter discarded (incompatible format)" );
        return VLC_EGENERIC;
    }

    /* Request a specific format if not already compatible */
    if ( p_filter->input.i_original_channels
            != p_filter->output.i_original_channels )
    {
        b_fit = VLC_FALSE;
        p_filter->input.i_original_channels =
                                        p_filter->output.i_original_channels;
    }
    if ( p_filter->input.i_format != VLC_FOURCC('f','l','3','2')
          || p_filter->output.i_format != VLC_FOURCC('f','l','3','2') )
    {
        b_fit = VLC_FALSE;
        p_filter->input.i_format = VLC_FOURCC('f','l','3','2');
        p_filter->output.i_format = VLC_FOURCC('f','l','3','2');
    }
    if ( p_filter->input.i_rate != p_filter->output.i_rate )
    {
        b_fit = VLC_FALSE;
        p_filter->input.i_rate = p_filter->output.i_rate;
    }
    if ( p_filter->input.i_physical_channels == (AOUT_CHAN_LEFT|AOUT_CHAN_RIGHT)
          && ( p_filter->input.i_original_channels & AOUT_CHAN_DOLBYSTEREO )
          && ! config_GetInt ( p_filter , "headphone-dolby" ) )
    {
        b_fit = VLC_FALSE;
        p_filter->input.i_physical_channels = AOUT_CHAN_LEFT | AOUT_CHAN_RIGHT |
                                              AOUT_CHAN_CENTER |
                                              AOUT_CHAN_REARLEFT |
                                              AOUT_CHAN_REARRIGHT;
    }
    if ( ! b_fit )
    {
        msg_Dbg( p_filter, "requesting specific format" );
        return VLC_EGENERIC;
    }

    /* Allocate the memory needed to store the module's structure */
    p_filter->p_sys = malloc( sizeof(struct aout_filter_sys_t) );
    if ( p_filter->p_sys == NULL )
    {
        msg_Err( p_filter, "out of memory" );
        return VLC_EGENERIC;
    }
    p_filter->p_sys->i_overflow_buffer_size = 0;
    p_filter->p_sys->p_overflow_buffer = NULL;
    p_filter->p_sys->i_nb_atomic_operations = 0;
    p_filter->p_sys->p_atomic_operations = NULL;

    if ( Init( p_filter , p_filter->p_sys
                , aout_FormatNbChannels ( &p_filter->input )
                , p_filter->input.i_physical_channels
                , p_filter->input.i_rate ) < 0 )
    {
        return VLC_EGENERIC;
    }

    p_filter->pf_do_work = DoWork;
    p_filter->b_in_place = 0;

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Destroy: deallocate resources associated with headphone downmixer
 *****************************************************************************/
static void Destroy( vlc_object_t *p_this )
{
    aout_filter_t * p_filter = (aout_filter_t *)p_this;

    if ( p_filter->p_sys != NULL )
    {
        if ( p_filter->p_sys->p_overflow_buffer != NULL )
        {
            free ( p_filter->p_sys->p_overflow_buffer );
        }
        if ( p_filter->p_sys->p_atomic_operations != NULL )
        {
            free ( p_filter->p_sys->p_atomic_operations );
        }
        free ( p_filter->p_sys );
        p_filter->p_sys = NULL;
    }
}

/*****************************************************************************
 * DoWork: convert a buffer
 *****************************************************************************/
static void DoWork( aout_instance_t * p_aout, aout_filter_t * p_filter,
                    aout_buffer_t * p_in_buf, aout_buffer_t * p_out_buf )
{
    int i_input_nb = aout_FormatNbChannels( &p_filter->input );
    int i_output_nb = aout_FormatNbChannels( &p_filter->output );

    float * p_in = (float*) p_in_buf->p_buffer;
    byte_t * p_out;
    byte_t * p_overflow;
    byte_t * p_slide;

    size_t i_overflow_size;     /* in bytes */
    size_t i_out_size;          /* in bytes */

    unsigned int i, j;

    int i_source_channel_offset;
    int i_dest_channel_offset;
    unsigned int i_delay;
    double d_amplitude_factor;


    /* out buffer characterisitcs */
    p_out_buf->i_nb_samples = p_in_buf->i_nb_samples;
    p_out_buf->i_nb_bytes = p_in_buf->i_nb_bytes * i_output_nb / i_input_nb;
    p_out = p_out_buf->p_buffer;
    i_out_size = p_out_buf->i_nb_bytes;

    if ( p_filter->p_sys != NULL )
    {
        /* Slide the overflow buffer */
        p_overflow = p_filter->p_sys->p_overflow_buffer;
        i_overflow_size = p_filter->p_sys->i_overflow_buffer_size;

        memset ( p_out , 0 , i_out_size );
        if ( i_out_size > i_overflow_size )
            memcpy ( p_out , p_overflow , i_overflow_size );
        else
            memcpy ( p_out , p_overflow , i_out_size );

        p_slide = p_filter->p_sys->p_overflow_buffer;
        while ( p_slide < p_overflow + i_overflow_size )
        {
            if ( p_slide + i_out_size < p_overflow + i_overflow_size )
            {
                memset ( p_slide , 0 , i_out_size );
                if ( p_slide + 2 * i_out_size < p_overflow + i_overflow_size )
                    memcpy ( p_slide , p_slide + i_out_size , i_out_size );
                else
                    memcpy ( p_slide , p_slide + i_out_size
                      , p_overflow + i_overflow_size - ( p_slide + i_out_size ) );
            }
            else
            {
                memset ( p_slide , 0 , p_overflow + i_overflow_size - p_slide );
            }
            p_slide += i_out_size;
        }

        /* apply the atomic operations */
        for ( i = 0 ; i < p_filter->p_sys->i_nb_atomic_operations ; i++ )
        {
            /* shorter variable names */
            i_source_channel_offset
                = p_filter->p_sys->p_atomic_operations[i].i_source_channel_offset;
            i_dest_channel_offset
                = p_filter->p_sys->p_atomic_operations[i].i_dest_channel_offset;
            i_delay = p_filter->p_sys->p_atomic_operations[i].i_delay;
            d_amplitude_factor
                = p_filter->p_sys->p_atomic_operations[i].d_amplitude_factor;

            if ( p_out_buf->i_nb_samples > i_delay )
            {
                /* current buffer coefficients */
                for ( j = 0 ; j < p_out_buf->i_nb_samples - i_delay ; j++ )
                {
                    ((float*)p_out)[ (i_delay+j)*i_output_nb + i_dest_channel_offset ]
                        += p_in[ j * i_input_nb + i_source_channel_offset ]
                           * d_amplitude_factor;
                }

                /* overflow buffer coefficients */
                for ( j = 0 ; j < i_delay ; j++ )
                {
                    ((float*)p_overflow)[ j*i_output_nb + i_dest_channel_offset ]
                        += p_in[ (p_out_buf->i_nb_samples - i_delay + j)
                           * i_input_nb + i_source_channel_offset ]
                           * d_amplitude_factor;
                }
            }
            else
            {
                /* overflow buffer coefficients only */
                for ( j = 0 ; j < p_out_buf->i_nb_samples ; j++ )
                {
                    ((float*)p_overflow)[ (i_delay - p_out_buf->i_nb_samples + j)
                        * i_output_nb + i_dest_channel_offset ]
                        += p_in[ j * i_input_nb + i_source_channel_offset ]
                           * d_amplitude_factor;
                }
            }
        }
    }
    else
    {
        memset ( p_out , 0 , i_out_size );
    }
}

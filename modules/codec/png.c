/*****************************************************************************
 * png.c: png decoder module making use of libpng.
 *****************************************************************************
 * Copyright (C) 1999-2001 the VideoLAN team
 * $Id: 2c13d0219771eaf7e477e90bffc154176c50b06a $
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
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
#include <vlc/vlc.h>
#include <vlc/decoder.h>

#include <png.h>

/*****************************************************************************
 * decoder_sys_t : png decoder descriptor
 *****************************************************************************/
struct decoder_sys_t
{
    vlc_bool_t b_error;
};

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  OpenDecoder   ( vlc_object_t * );
static void CloseDecoder  ( vlc_object_t * );

static picture_t *DecodeBlock  ( decoder_t *, block_t ** );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin();
    set_category( CAT_INPUT );
    set_subcategory( SUBCAT_INPUT_VCODEC );
    set_description( _("PNG video decoder") );
    set_capability( "decoder", 1000 );
    set_callbacks( OpenDecoder, CloseDecoder );
    add_shortcut( "png" );
vlc_module_end();

/*****************************************************************************
 * OpenDecoder: probe the decoder and return score
 *****************************************************************************/
static int OpenDecoder( vlc_object_t *p_this )
{
    decoder_t *p_dec = (decoder_t*)p_this;
    decoder_sys_t *p_sys;

    if( p_dec->fmt_in.i_codec != VLC_FOURCC('p','n','g',' ') &&
        p_dec->fmt_in.i_codec != VLC_FOURCC('M','P','N','G') )
    {
        return VLC_EGENERIC;
    }

    /* Allocate the memory needed to store the decoder's structure */
    if( ( p_dec->p_sys = p_sys =
          (decoder_sys_t *)malloc(sizeof(decoder_sys_t)) ) == NULL )
    {
        msg_Err( p_dec, "out of memory" );
        return VLC_EGENERIC;
    }

    /* Set output properties */
    p_dec->fmt_out.i_cat = VIDEO_ES;
    p_dec->fmt_out.i_codec = VLC_FOURCC('R','V','3','2');

    /* Set callbacks */
    p_dec->pf_decode_video = DecodeBlock;

    return VLC_SUCCESS;
}

static void user_read( png_structp p_png, png_bytep data, png_size_t i_length )
{
    block_t *p_block = (block_t *)png_get_io_ptr( p_png );
    png_size_t i_read = __MIN( p_block->i_buffer, (int)i_length );
    memcpy( data, p_block->p_buffer, i_length );
    p_block->p_buffer += i_length;
    p_block->i_buffer -= i_length;

    if( i_length != i_read ) png_error( p_png, "not enough data" );
}

static void user_error( png_structp p_png, png_const_charp error_msg )
{
    decoder_t *p_dec = (decoder_t *)png_get_error_ptr( p_png );
    p_dec->p_sys->b_error = VLC_TRUE;
    msg_Err( p_dec, error_msg );
}

static void user_warning( png_structp p_png, png_const_charp warning_msg )
{
    decoder_t *p_dec = (decoder_t *)png_get_error_ptr( p_png );
    msg_Warn( p_dec, warning_msg );
}

/****************************************************************************
 * DecodeBlock: the whole thing
 ****************************************************************************
 * This function must be fed with a complete compressed frame.
 ****************************************************************************/
static picture_t *DecodeBlock( decoder_t *p_dec, block_t **pp_block )
{
    decoder_sys_t *p_sys = p_dec->p_sys;
    block_t *p_block;
    picture_t *p_pic = 0;

    png_uint_32 i_width, i_height;
    int i_color_type, i_interlace_type, i_compression_type, i_filter_type;
    int i_bit_depth, i;

    png_structp p_png;
    png_infop p_info, p_end_info;
    png_bytep *p_row_pointers = NULL;

    if( !pp_block || !*pp_block ) return NULL;

    p_block = *pp_block;
    p_sys->b_error = VLC_FALSE;

    p_png = png_create_read_struct( PNG_LIBPNG_VER_STRING, 0, 0, 0 );
    if( p_png == NULL )
    {
        block_Release( p_block ); *pp_block = NULL;
        return NULL;
    }
    
    p_info = png_create_info_struct( p_png );
    if( p_info == NULL )
    {
        png_destroy_read_struct( &p_png, png_infopp_NULL, png_infopp_NULL );
        block_Release( p_block ); *pp_block = NULL;
        return NULL;
    }

    p_end_info = png_create_info_struct( p_png );
    if( p_end_info == NULL )
    {
        png_destroy_read_struct( &p_png, &p_info, png_infopp_NULL );
        block_Release( p_block ); *pp_block = NULL;
        return NULL;
    }
 
    /* libpng longjmp's there in case of error */
    if( setjmp( png_jmpbuf( p_png ) ) )
        goto error;

    png_set_read_fn( p_png, (void *)p_block, user_read );
    png_set_error_fn( p_png, (void *)p_dec, user_error, user_warning );

    png_read_info( p_png, p_info );
    if( p_sys->b_error ) goto error;

    png_get_IHDR( p_png, p_info, &i_width, &i_height,
                  &i_bit_depth, &i_color_type, &i_interlace_type,
                  &i_compression_type, &i_filter_type);
    if( p_sys->b_error ) goto error;

    /* Set output properties */
    p_dec->fmt_out.i_codec = VLC_FOURCC('R','V','3','2');
    p_dec->fmt_out.video.i_width = i_width;
    p_dec->fmt_out.video.i_height = i_height;
    p_dec->fmt_out.video.i_aspect = VOUT_ASPECT_FACTOR * i_width / i_height;

    if( i_color_type == PNG_COLOR_TYPE_PALETTE )
        png_set_palette_to_rgb( p_png );

    if( i_color_type == PNG_COLOR_TYPE_GRAY ||
        i_color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
          png_set_gray_to_rgb( p_png );

    /* Strip to 8 bits per channel */
    if( i_bit_depth == 16 ) png_set_strip_16( p_png );

    if( png_get_valid( p_png, p_info, PNG_INFO_tRNS ) )
    {
        png_set_tRNS_to_alpha( p_png );
    }
    else if( !(i_color_type & PNG_COLOR_MASK_ALPHA) )
    {
        p_dec->fmt_out.i_codec = VLC_FOURCC('R','V','2','4');
    }
    if( i_color_type & PNG_COLOR_MASK_COLOR &&
        p_dec->fmt_out.i_codec != VLC_FOURCC('R','V','2','4') )
    {
        /* Invert colors */
        png_set_bgr( p_png );
    }

    /* Get a new picture */
    p_pic = p_dec->pf_vout_buffer_new( p_dec );
    if( !p_pic ) goto error;

    /* Decode picture */
    p_row_pointers = malloc( sizeof(png_bytep) * i_height );
    for( i = 0; i < (int)i_height; i++ )
        p_row_pointers[i] = p_pic->p->p_pixels + p_pic->p->i_pitch * i;

    png_read_image( p_png, p_row_pointers );
    if( p_sys->b_error ) goto error;
    png_read_end( p_png, p_end_info );
    if( p_sys->b_error ) goto error;

    png_destroy_read_struct( &p_png, &p_info, &p_end_info );
    free( p_row_pointers );

    p_pic->date = p_block->i_pts > 0 ? p_block->i_pts : p_block->i_dts;

    block_Release( p_block ); *pp_block = NULL;
    return p_pic;

 error:

    if( p_row_pointers ) free( p_row_pointers );
    png_destroy_read_struct( &p_png, &p_info, &p_end_info );
    block_Release( p_block ); *pp_block = NULL;
    return NULL;
}

/*****************************************************************************
 * CloseDecoder: png decoder destruction
 *****************************************************************************/
static void CloseDecoder( vlc_object_t *p_this )
{
    decoder_t *p_dec = (decoder_t *)p_this;
    decoder_sys_t *p_sys = p_dec->p_sys;

    free( p_sys );
}

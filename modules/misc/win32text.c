/*****************************************************************************
 * win32text.c : Text drawing routines using the TextOut win32 API
 *****************************************************************************
 * Copyright (C) 2002 - 2005 the VideoLAN team
 * $Id: 9c6d2204eee0399536e1777ab75d535e70cf019c $
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
#include <stdlib.h>                                      /* malloc(), free() */
#include <string.h>

#include <vlc/vlc.h>
#include <vlc/vout.h>
#include "vlc_osd.h"
#include "vlc_block.h"
#include "vlc_filter.h"

#include <math.h>

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  Create ( vlc_object_t * );
static void Destroy( vlc_object_t * );

/* The RenderText call maps to pf_render_string, defined in vlc_filter.h */
static int RenderText( filter_t *, subpicture_region_t *,
                       subpicture_region_t * );

static int Render( filter_t *, subpicture_region_t *, uint8_t *, int, int);
static int SetFont( filter_t *, int );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define FONT_TEXT N_("Font")
#define FONT_LONGTEXT N_("Filename for the font you want to use")
#define FONTSIZE_TEXT N_("Font size in pixels")
/// \bug [String] extra space
#define FONTSIZE_LONGTEXT N_("This is the default size of the fonts " \
     "that will be rendered on the video. " \
     "If set to something different than 0 this option will override the " \
     "relative font size. " )
#define OPACITY_TEXT N_("Opacity")
#define OPACITY_LONGTEXT N_("The opacity (inverse of transparency) of the " \
     "text that will be rendered on the video. 0 = transparent, " \
     "255 = totally opaque. " )
#define COLOR_TEXT N_("Text default color")
#define COLOR_LONGTEXT N_("The color of the text that will be rendered on "\
  "the video. This must be an hexadecimal (like HTML colors). The first two "\
  "chars are for red, then green, then blue. #000000 = black, #FF0000 = red,"\
  " #00FF00 = green, #FFFF00 = yellow (red + green), #FFFFFF = white" )
#define FONTSIZER_TEXT N_("Relative font size")
#define FONTSIZER_LONGTEXT N_("This is the relative default size of the " \
  "fonts that will be rendered on the video. If absolute font size is set, "\
   "relative size will be overriden." )

static int   pi_sizes[] = { 20, 18, 16, 12, 6 };
static char *ppsz_sizes_text[] = { N_("Smaller"), N_("Small"), N_("Normal"),
                                   N_("Large"), N_("Larger") };
static int pi_color_values[] = {
  0x00000000, 0x00808080, 0x00C0C0C0, 0x00FFFFFF, 0x00800000,
  0x00FF0000, 0x00FF00FF, 0x00FFFF00, 0x00808000, 0x00008000, 0x00008080,
  0x0000FF00, 0x00800080, 0x00000080, 0x000000FF, 0x0000FFFF };

static char *ppsz_color_descriptions[] = {
  N_("Black"), N_("Gray"), N_("Silver"), N_("White"), N_("Maroon"),
  N_("Red"), N_("Fuchsia"), N_("Yellow"), N_("Olive"), N_("Green"), N_("Teal"),
  N_("Lime"), N_("Purple"), N_("Navy"), N_("Blue"), N_("Aqua") };

vlc_module_begin();
    set_shortname( _("Text renderer"));
    set_description( _("Win32 font renderer") );
    set_category( CAT_VIDEO );
    set_subcategory( SUBCAT_VIDEO_SUBPIC );

    add_integer( "win32text-fontsize", 0, NULL, FONTSIZE_TEXT,
                 FONTSIZE_LONGTEXT, VLC_TRUE );

    /* opacity valid on 0..255, with default 255 = fully opaque */
    add_integer_with_range( "win32-opacity", 255, 0, 255, NULL,
        OPACITY_TEXT, OPACITY_LONGTEXT, VLC_FALSE );

    /* hook to the color values list, with default 0x00ffffff = white */
    add_integer( "win32text-color", 0x00FFFFFF, NULL, COLOR_TEXT,
                 COLOR_LONGTEXT, VLC_TRUE );
        change_integer_list( pi_color_values, ppsz_color_descriptions, 0 );

    add_integer( "win32text-rel-fontsize", 16, NULL, FONTSIZER_TEXT,
                 FONTSIZER_LONGTEXT, VLC_FALSE );
        change_integer_list( pi_sizes, ppsz_sizes_text, 0 );

    set_capability( "text renderer", 50 );
    add_shortcut( "text" );
    set_callbacks( Create, Destroy );
vlc_module_end();

/*****************************************************************************
 * filter_sys_t: win32text local data
 *****************************************************************************/
struct filter_sys_t
{
    uint8_t        i_font_opacity;
    int            i_font_color;
    int            i_font_size;

    int            i_default_font_size;
    int            i_display_height;

    HDC hcdc;
    HFONT hfont;
    HFONT hfont_bak;
    int i_logpy;
};

static uint8_t pi_gamma[16] =
  {0x00, 0x41, 0x52, 0x63, 0x84, 0x85, 0x96, 0xa7, 0xb8, 0xc9,
   0xca, 0xdb, 0xdc, 0xed, 0xee, 0xff};

/*****************************************************************************
 * Create: creates the module
 *****************************************************************************/
static int Create( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;
    filter_sys_t *p_sys;
    char *psz_fontfile = NULL;
    vlc_value_t val;
    HDC hdc;

    /* Allocate structure */
    p_filter->p_sys = p_sys = malloc( sizeof( filter_sys_t ) );
    if( !p_sys )
    {
        msg_Err( p_filter, "out of memory" );
        return VLC_ENOMEM;
    }
    p_sys->i_font_size = 0;
    p_sys->i_display_height = 0;

    var_Create( p_filter, "win32text-font",
                VLC_VAR_STRING | VLC_VAR_DOINHERIT );
    var_Create( p_filter, "win32text-fontsize",
                VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Create( p_filter, "win32text-rel-fontsize",
                VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Create( p_filter, "win32text-opacity",
                VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Get( p_filter, "win32text-opacity", &val );
    p_sys->i_font_opacity = __MAX( __MIN( val.i_int, 255 ), 0 );
    var_Create( p_filter, "win32text-color",
                VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Get( p_filter, "win32text-color", &val );
    p_sys->i_font_color = __MAX( __MIN( val.i_int, 0xFFFFFF ), 0 );

    p_sys->hfont = p_sys->hfont_bak = 0;
    hdc = GetDC( NULL );
    p_sys->hcdc = CreateCompatibleDC( hdc );
    p_sys->i_logpy = GetDeviceCaps( hdc, LOGPIXELSY );
    ReleaseDC( NULL, hdc );
    SetBkMode( p_sys->hcdc, TRANSPARENT );

    var_Get( p_filter, "win32text-fontsize", &val );
    p_sys->i_default_font_size = val.i_int;
    if( SetFont( p_filter, 0 ) != VLC_SUCCESS ) goto error;

    if( psz_fontfile ) free( psz_fontfile );
    p_filter->pf_render_text = RenderText;
    return VLC_SUCCESS;

 error:
    if( psz_fontfile ) free( psz_fontfile );
    free( p_sys );
    return VLC_EGENERIC;
}

/*****************************************************************************
 * Destroy: destroy the module
 *****************************************************************************/
static void Destroy( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;
    filter_sys_t *p_sys = p_filter->p_sys;

    if( p_sys->hfont_bak ) SelectObject( p_sys->hcdc, p_sys->hfont_bak );
    if( p_sys->hfont ) DeleteObject( p_sys->hfont );
    DeleteDC( p_sys->hcdc );
    free( p_sys );
}

/*****************************************************************************
 * Render: place string in picture
 *****************************************************************************
 * This function merges the previously rendered win32text glyphs into a picture
 *****************************************************************************/
static int Render( filter_t *p_filter, subpicture_region_t *p_region,
                   uint8_t *p_bitmap, int i_width, int i_height )
{
    uint8_t *p_dst;
    video_format_t fmt;
    int i, i_pitch;
    subpicture_region_t *p_region_tmp;
    vlc_bool_t b_outline = VLC_TRUE;

    /* Create a new subpicture region */
    memset( &fmt, 0, sizeof(video_format_t) );
    fmt.i_chroma = VLC_FOURCC('Y','U','V','P');
    fmt.i_width = fmt.i_visible_width = i_width + (b_outline ? 4 : 0);
    fmt.i_height = fmt.i_visible_height = i_height + (b_outline ? 4 : 0);
    fmt.i_x_offset = fmt.i_y_offset = 0;
    p_region_tmp = spu_CreateRegion( p_filter, &fmt );
    if( !p_region_tmp )
    {
        msg_Err( p_filter, "cannot allocate SPU region" );
        return VLC_EGENERIC;
    }

    /* Build palette */
    fmt.p_palette->i_entries = 16;
    for( i = 0; i < fmt.p_palette->i_entries; i++ )
    {
        fmt.p_palette->palette[i][0] = pi_gamma[i];
        fmt.p_palette->palette[i][1] = 128;
        fmt.p_palette->palette[i][2] = 128;
        fmt.p_palette->palette[i][3] = pi_gamma[i];
    }

    p_region->fmt = p_region_tmp->fmt;
    p_region->picture = p_region_tmp->picture;
    free( p_region_tmp );

    p_dst = p_region->picture.Y_PIXELS;
    i_pitch = p_region->picture.Y_PITCH;

    if( b_outline )
    {
        memset( p_dst, 0, i_pitch * fmt.i_height );
        p_dst += p_region->picture.Y_PITCH * 2 + 2;
    }

    for( i = 0; i < i_height; i++ )
    {
        memcpy( p_dst, p_bitmap, i_width );
        p_bitmap += (i_width+3) & ~3;
        p_dst += i_pitch;
    }

    /* Outlining (find something better than nearest neighbour filtering ?) */
    if( b_outline )
    {
        uint8_t *p_top = p_dst; /* Use 1st line as a cache */
        uint8_t left, current;
        int x, y;

        p_dst = p_region->picture.Y_PIXELS;

        for( y = 1; y < (int)fmt.i_height - 1; y++ )
        {
            memcpy( p_top, p_dst, fmt.i_width );
            p_dst += i_pitch;
            left = 0;

            for( x = 1; x < (int)fmt.i_width - 1; x++ )
            {
                current = p_dst[x];
                p_dst[x] = ( 4 * (int)p_dst[x] + left + p_top[x] + p_dst[x+1] +
                             p_dst[x + i_pitch]) / 8;
                left = current;
            }
        }
        memset( p_top, 0, fmt.i_width );
    }

    return VLC_SUCCESS;
}

static int RenderText( filter_t *p_filter, subpicture_region_t *p_region_out,
                       subpicture_region_t *p_region_in )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    int i_font_color, i_font_alpha, i_font_size;
    uint8_t *p_bitmap;
    TCHAR *psz_string;
    int i, i_width, i_height;
    HBITMAP bitmap, bitmap_bak;
    BITMAPINFO *p_bmi;
    RECT rect = {0};
    SIZE size;

    /* Sanity check */
    if( !p_region_in || !p_region_out ) return VLC_EGENERIC;
#ifdef UNICODE
    psz_string = malloc( (strlen( p_region_in->psz_text )+1) * sizeof(TCHAR) );
    if( mbstowcs( psz_string, p_region_in->psz_text,
                  strlen( p_region_in->psz_text ) * sizeof(TCHAR) ) < 0 )
    {
        free( psz_string );
        return VLC_EGENERIC;
    }
#else
    psz_string = strdup( p_region_in->psz_text );
#endif
    if( !psz_string || !*psz_string ) return VLC_EGENERIC;

    if( p_region_in->p_style )
    {
        i_font_color = __MAX( __MIN( p_region_in->p_style->i_font_color, 0xFFFFFF ), 0 );
        i_font_alpha = __MAX( __MIN( p_region_in->p_style->i_font_alpha, 255 ), 0 );
        i_font_size  = __MAX( __MIN( p_region_in->p_style->i_font_size, 255 ), 0 );
    }
    else
    {
        i_font_color = p_sys->i_font_color;
        i_font_alpha = 255 - p_sys->i_font_opacity;
        i_font_size = p_sys->i_default_font_size;
    }

    SetFont( p_filter, i_font_size );

    SetTextColor( p_sys->hcdc, RGB( (i_font_color >> 16) & 0xff,
                  (i_font_color >> 8) & 0xff, i_font_color & 0xff) );

    GetTextExtentExPoint( p_sys->hcdc, psz_string, _tcslen(psz_string),
                          0, 0, 0, &size );
    i_width = rect.right = size.cx; i_height = rect.bottom = size.cy;

    p_bmi = malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*16);
    memset( p_bmi, 0, sizeof(BITMAPINFOHEADER) );
    p_bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    p_bmi->bmiHeader.biWidth = (i_width+3) & ~3;
    p_bmi->bmiHeader.biHeight = - i_height;
    p_bmi->bmiHeader.biPlanes = 1;
    p_bmi->bmiHeader.biBitCount = 8;
    p_bmi->bmiHeader.biCompression = BI_RGB;
    p_bmi->bmiHeader.biClrUsed = 16;

    for( i = 0; i < 16; i++ )
    {
        p_bmi->bmiColors[i].rgbBlue =
            p_bmi->bmiColors[i].rgbGreen =
                p_bmi->bmiColors[i].rgbRed = pi_gamma[i];
    }

    bitmap = CreateDIBSection( p_sys->hcdc, p_bmi, DIB_RGB_COLORS,
                               (void **)&p_bitmap, NULL, 0 );
    if( !bitmap )
    {
        msg_Err( p_filter, "could not create bitmap" );
        return VLC_EGENERIC;
    }

    bitmap_bak = SelectObject( p_sys->hcdc, bitmap );
    FillRect( p_sys->hcdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH) );

    //TextOut( p_sys->hcdc, 0, 0, psz_string, strlen(psz_string) );
    if( !DrawText( p_sys->hcdc, psz_string, -1, &rect, 0 ) )
    {
        msg_Err( p_filter, "could not draw text" );
    }

    p_region_out->i_x = p_region_in->i_x;
    p_region_out->i_y = p_region_in->i_y;
    Render( p_filter, p_region_out, p_bitmap, i_width, i_height );

    SelectObject( p_sys->hcdc, bitmap_bak );
    DeleteObject( bitmap );
    return VLC_SUCCESS;
}

static int SetFont( filter_t *p_filter, int i_size )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    LOGFONT logfont;

    if( i_size && i_size == p_sys->i_font_size ) return VLC_SUCCESS;

    if( !i_size )
    {
        vlc_value_t val;

        if( !p_sys->i_default_font_size &&
            p_sys->i_display_height == (int)p_filter->fmt_out.video.i_height )
            return VLC_SUCCESS;

        if( p_sys->i_default_font_size )
        {
            i_size = p_sys->i_default_font_size;
        }
        else
        {
            var_Get( p_filter, "win32text-rel-fontsize", &val );
            i_size = (int)p_filter->fmt_out.video.i_height / val.i_int;
            p_filter->p_sys->i_display_height =
                p_filter->fmt_out.video.i_height;
        }
        if( i_size <= 0 )
        {
            msg_Warn( p_filter, "invalid fontsize, using 12" );
            i_size = 12;
        }

        msg_Dbg( p_filter, "using fontsize: %i", i_size );
    }

    p_sys->i_font_size = i_size;

    if( p_sys->hfont_bak ) SelectObject( p_sys->hcdc, p_sys->hfont_bak );
    if( p_sys->hfont ) DeleteObject( p_sys->hfont );

    i_size = i_size * (int64_t)p_sys->i_logpy / 72;

    logfont.lfHeight = i_size;
    logfont.lfWidth = 0;
    logfont.lfEscapement = 0;
    logfont.lfOrientation = 0;
    logfont.lfWeight = 0;
    logfont.lfItalic = FALSE;
    logfont.lfUnderline = FALSE;
    logfont.lfStrikeOut = FALSE;
    logfont.lfCharSet = ANSI_CHARSET;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = ANTIALIASED_QUALITY;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    memcpy( logfont.lfFaceName, _T("Arial"), sizeof(_T("Arial")) );

    p_sys->hfont = CreateFontIndirect( &logfont );

    p_sys->hfont_bak = SelectObject( p_sys->hcdc, p_sys->hfont );

    return VLC_SUCCESS;
}

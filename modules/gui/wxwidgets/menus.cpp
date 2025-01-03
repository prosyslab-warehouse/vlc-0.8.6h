/*****************************************************************************
 * menus.cpp : wxWidgets plugin for vlc
 *****************************************************************************
 * Copyright (C) 2000-2004 the VideoLAN team
 * $Id: 18d1328d2b3ceaeefc33d87c83907a05ecfd06a1 $
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

#include <vlc/vlc.h>
#include <vlc/intf.h>

#include "wxwidgets.hpp"
#include "interface.hpp"

#include <wx/dynarray.h>
WX_DEFINE_ARRAY(int, ArrayOfInts);
WX_DEFINE_ARRAY_PTR(const char *, ArrayOfStrings);


class wxMenuItemExt: public wxMenuItem
{
public:
    /* Constructor */
    wxMenuItemExt( wxMenu* parentMenu, int id, const wxString& text,
                   const wxString& helpString, wxItemKind kind,
                   char *_psz_var, int _i_object_id, vlc_value_t _val,
                   int _i_val_type );
    virtual ~wxMenuItemExt();

    char *psz_var;
    int  i_val_type;
    int  i_object_id;
    vlc_value_t val;
};

class Menu: public wxMenu
{
public:
    /* Constructor */
    Menu( intf_thread_t *p_intf, int i_start_id );
    virtual ~Menu();

    void Populate( ArrayOfStrings &, ArrayOfInts &);
    void Clear();

private:
    wxMenu *CreateDummyMenu();
    void   CreateMenuItem( wxMenu *, const char *, vlc_object_t * );
    wxMenu *CreateChoicesMenu( const char *, vlc_object_t *, bool );

    DECLARE_EVENT_TABLE();

    intf_thread_t *p_intf;

    int i_start_id;
    int i_item_id;
};

/*****************************************************************************
 * Event Table.
 *****************************************************************************/
enum
{
    /* menu items */
    MenuDummy_Event = wxID_HIGHEST + 1000,
    OpenFileSimple_Event = wxID_HIGHEST + 1100,
    OpenFile_Event,
    OpenDirectory_Event,
    OpenDisc_Event,
    OpenNet_Event,
    OpenCapture_Event,
    MediaInfo_Event,
    Messages_Event,
    Preferences_Event,
    Play_Event,
    Pause_Event,
    Previous_Event,
    Next_Event,
    Stop_Event,
    FirstAutoGenerated_Event = wxID_HIGHEST + 1999,
    SettingsMenu_Events = wxID_HIGHEST + 5000,
    AudioMenu_Events = wxID_HIGHEST + 2000,
    VideoMenu_Events = wxID_HIGHEST + 3000,
    NavigMenu_Events = wxID_HIGHEST + 4000,
    PopupMenu_Events = wxID_HIGHEST + 6000,
    Hotkeys_Events = wxID_HIGHEST + 7000
};

BEGIN_EVENT_TABLE(Menu, wxMenu)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MenuEvtHandler, wxEvtHandler)
    EVT_MENU(OpenFileSimple_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(OpenFile_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(OpenDirectory_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(OpenDisc_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(OpenNet_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(OpenCapture_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(MediaInfo_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(Messages_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(Preferences_Event, MenuEvtHandler::OnShowDialog)
    EVT_MENU(-1, MenuEvtHandler::OnMenuEvent)
END_EVENT_TABLE()

/*****************************************************************************
 * Static menu helpers
 *****************************************************************************/
wxMenu *OpenStreamMenu( intf_thread_t *p_intf )
{
    wxMenu *menu = new wxMenu;
    menu->Append( OpenFileSimple_Event, wxU(_("Quick &Open File...")) );
    menu->Append( OpenFile_Event, wxU(_("Open &File...")) );
    menu->Append( OpenDirectory_Event, wxU(_("Open D&irectory...")) );
    menu->Append( OpenDisc_Event, wxU(_("Open &Disc...")) );
    menu->Append( OpenNet_Event, wxU(_("Open &Network Stream...")) );
    menu->Append( OpenCapture_Event, wxU(_("Open &Capture Device...")) );
    return menu;
}

wxMenu *MiscMenu( intf_thread_t *p_intf )
{
    wxMenu *menu = new wxMenu;
    menu->Append( MediaInfo_Event, wxU(_("Media &Info...")) );
    menu->Append( Messages_Event, wxU(_("&Messages...")) );
    menu->Append( Preferences_Event, wxU(_("&Preferences...")) );
    return menu;
}

/*****************************************************************************
 * Builders for the dynamic menus
 *****************************************************************************/
#define PUSH_VAR( var ) rs_varnames.Add( var ); \
                        ri_objects.Add( p_object->i_object_id )

int InputAutoMenuBuilder( vlc_object_t *p_object, ArrayOfInts &ri_objects,
                          ArrayOfStrings &rs_varnames )
{
    PUSH_VAR( "bookmark");
    PUSH_VAR( "title" );
    PUSH_VAR ("chapter" );
    PUSH_VAR( "program" );
    PUSH_VAR( "navigation" );
    PUSH_VAR( "dvd_menus" );
    return VLC_SUCCESS;
}

int VideoAutoMenuBuilder( vlc_object_t *p_object, ArrayOfInts &ri_objects,
                          ArrayOfStrings &rs_varnames )
{
    PUSH_VAR( "fullscreen" );
    PUSH_VAR( "zoom" );
    PUSH_VAR( "deinterlace" );
    PUSH_VAR( "aspect-ratio" );
    PUSH_VAR( "crop" );
    PUSH_VAR( "video-on-top" );
    PUSH_VAR( "directx-wallpaper" );
    PUSH_VAR( "video-snapshot" );

    vlc_object_t *p_dec_obj = (vlc_object_t *)vlc_object_find( p_object,
                                                 VLC_OBJECT_DECODER,
                                                 FIND_PARENT );
    if( p_dec_obj != NULL )
    {
        vlc_object_t *p_object = p_dec_obj;
        PUSH_VAR( "ffmpeg-pp-q" );
        vlc_object_release( p_dec_obj );
    }
    return VLC_SUCCESS;
}

int AudioAutoMenuBuilder( vlc_object_t *p_object, ArrayOfInts &ri_objects,
                          ArrayOfStrings &rs_varnames )
{
    PUSH_VAR( "audio-device" );
    PUSH_VAR( "audio-channels" );
    PUSH_VAR( "visual" );
    PUSH_VAR( "equalizer" );
    return VLC_SUCCESS;
}

int IntfAutoMenuBuilder( intf_thread_t *p_intf, ArrayOfInts &ri_objects,
                         ArrayOfStrings &rs_varnames, bool is_popup)
{
    /* vlc_object_find is needed because of the dialogs provider case */
    vlc_object_t *p_object;
    p_object = (vlc_object_t *)vlc_object_find( p_intf, VLC_OBJECT_INTF,
                                                FIND_PARENT );
    if( p_object != NULL )
    {
        if( is_popup )
        {
#ifndef WIN32
            PUSH_VAR( "intf-switch" );
#endif
        }
        else
        {
            PUSH_VAR( "intf-switch" );
        }
        PUSH_VAR( "intf-add" );
        PUSH_VAR( "intf-skins" );
        vlc_object_release( p_object );
    }
    return VLC_SUCCESS;
}

#undef PUSH_VAR
/*****************************************************************************
 * Popup menus
 *****************************************************************************/
#define PUSH_VAR( var ) as_varnames.Add( var ); \
                        ai_objects.Add( p_object->i_object_id )

#define PUSH_SEPARATOR if( ai_objects.GetCount() != i_last_separator ) { \
                            ai_objects.Add( 0 ); \
                            as_varnames.Add( "" ); \
                            i_last_separator = ai_objects.GetCount(); }

#define POPUP_BOILERPLATE \
    unsigned int i_last_separator = 0; \
    ArrayOfInts ai_objects; \
    ArrayOfStrings as_varnames; \
    playlist_t *p_playlist = (playlist_t *) vlc_object_find( p_intf, \
                                          VLC_OBJECT_PLAYLIST, FIND_ANYWHERE );\
    if( !p_playlist ) \
        return; \
    input_thread_t *p_input = p_playlist->p_input

#define CREATE_POPUP    \
    Menu popupmenu( p_intf, PopupMenu_Events ); \
    popupmenu.Populate( as_varnames, ai_objects ); \
    p_intf->p_sys->p_popup_menu = &popupmenu; \
    p_parent->PopupMenu( &popupmenu, pos.x, pos.y ); \
    p_intf->p_sys->p_popup_menu = NULL; \
    i_last_separator = 0 /* stop compiler warning */

#define POPUP_STATIC_ENTRIES \
    if( p_input != NULL ) \
    { \
        vlc_value_t val; \
        popupmenu.InsertSeparator( 0 ); \
        if (!minimal) \
        { \
        popupmenu.Insert( 0, Stop_Event, wxU(_("Stop")) ); \
        popupmenu.Insert( 0, Previous_Event, wxU(_("Previous")) ); \
        popupmenu.Insert( 0, Next_Event, wxU(_("Next")) ); \
        } \
         \
        var_Get( p_input, "state", &val ); \
        if( val.i_int == PAUSE_S ) \
            popupmenu.Insert( 0, Play_Event, wxU(_("Play")) ); \
        else \
            popupmenu.Insert( 0, Pause_Event, wxU(_("Pause")) ); \
         \
        vlc_object_release( p_input ); \
    } \
    else \
    { \
        if( p_playlist && p_playlist->i_size ) \
        { \
            popupmenu.InsertSeparator( 0 ); \
            popupmenu.Insert( 0, Play_Event, wxU(_("Play")) ); \
        } \
        if( p_playlist ) vlc_object_release( p_playlist ); \
    } \
    \
    popupmenu.Append( MenuDummy_Event, wxU(_("Miscellaneous")), \
                      MiscMenu( p_intf ), wxT("") )


void VideoPopupMenu( intf_thread_t *p_intf, wxWindow *p_parent,
                     const wxPoint& pos )
{
    POPUP_BOILERPLATE;
    if( p_input )
    {
        vlc_object_yield( p_input );
        as_varnames.Add( "video-es" );
        ai_objects.Add( p_input->i_object_id );
        as_varnames.Add( "spu-es" );
        ai_objects.Add( p_input->i_object_id );
        vlc_object_t *p_vout = (vlc_object_t *)vlc_object_find( p_input,
                                                VLC_OBJECT_VOUT, FIND_CHILD );
        if( p_vout )
        {
            VideoAutoMenuBuilder( p_vout, ai_objects, as_varnames );
            vlc_object_release( p_vout );
        }
        vlc_object_release( p_input );
    }
    vlc_object_release( p_playlist );
    CREATE_POPUP;
}

void AudioPopupMenu( intf_thread_t *p_intf, wxWindow *p_parent,
                     const wxPoint& pos )
{
    POPUP_BOILERPLATE;
    if( p_input )
    {
        vlc_object_yield( p_input );
        as_varnames.Add( "audio-es" );
        ai_objects.Add( p_input->i_object_id );
        vlc_object_t *p_aout = (vlc_object_t *)vlc_object_find( p_input,
                                             VLC_OBJECT_AOUT, FIND_ANYWHERE );
        if( p_aout )
        {
            AudioAutoMenuBuilder( p_aout, ai_objects, as_varnames );
            vlc_object_release( p_aout );
        }
        vlc_object_release( p_input );
    }
    vlc_object_release( p_playlist );
    CREATE_POPUP;
}

/* Navigation stuff, and general */
void MiscPopupMenu( intf_thread_t *p_intf, wxWindow *p_parent,
                    const wxPoint& pos )
{
    int minimal = 0;
    POPUP_BOILERPLATE;
    if( p_input )
    {
        vlc_object_yield( p_input );
        as_varnames.Add( "audio-es" );
        InputAutoMenuBuilder( VLC_OBJECT(p_input), ai_objects, as_varnames );
        PUSH_SEPARATOR;
    }
    IntfAutoMenuBuilder( p_intf, ai_objects, as_varnames, true );

    Menu popupmenu( p_intf, PopupMenu_Events );
    popupmenu.Populate( as_varnames, ai_objects );

    POPUP_STATIC_ENTRIES;
    popupmenu.Append( MenuDummy_Event, wxU(_("Open")),
                      OpenStreamMenu( p_intf ), wxT("") );

    p_intf->p_sys->p_popup_menu = &popupmenu;
    p_parent->PopupMenu( &popupmenu, pos.x, pos.y );
    p_intf->p_sys->p_popup_menu = NULL;
    vlc_object_release( p_playlist );
}

void PopupMenu( intf_thread_t *p_intf, wxWindow *p_parent,
                const wxPoint& pos )
{
    int minimal = config_GetInt( p_intf, "wx-minimal" );
    POPUP_BOILERPLATE;
    if( p_input )
    {
        vlc_object_yield( p_input );
        InputAutoMenuBuilder( VLC_OBJECT(p_input), ai_objects, as_varnames );

        /* Video menu */
        PUSH_SEPARATOR;
        as_varnames.Add( "video-es" );
        ai_objects.Add( p_input->i_object_id );
        as_varnames.Add( "spu-es" );
        ai_objects.Add( p_input->i_object_id );
        vlc_object_t *p_vout = (vlc_object_t *)vlc_object_find( p_input,
                                                VLC_OBJECT_VOUT, FIND_CHILD );
        if( p_vout )
        {
            VideoAutoMenuBuilder( p_vout, ai_objects, as_varnames );
            vlc_object_release( p_vout );
        }
        /* Audio menu */
        PUSH_SEPARATOR
        as_varnames.Add( "audio-es" );
        ai_objects.Add( p_input->i_object_id );
        vlc_object_t *p_aout = (vlc_object_t *)vlc_object_find( p_input,
                                             VLC_OBJECT_AOUT, FIND_ANYWHERE );
        if( p_aout )
        {
            AudioAutoMenuBuilder( p_aout, ai_objects, as_varnames );
            vlc_object_release( p_aout );
        }
    }

    /* Interface menu */
    PUSH_SEPARATOR
    IntfAutoMenuBuilder( p_intf, ai_objects, as_varnames, true );

    /* Build menu */
    Menu popupmenu( p_intf, PopupMenu_Events );
    popupmenu.Populate( as_varnames, ai_objects );
    POPUP_STATIC_ENTRIES;

    if (!minimal)
    {
        popupmenu.Append( MenuDummy_Event, wxU(_("Open")),
                          OpenStreamMenu( p_intf ), wxT("") );
    }
    p_intf->p_sys->p_popup_menu = &popupmenu;
    p_parent->PopupMenu( &popupmenu, pos.x, pos.y );
    p_intf->p_sys->p_popup_menu = NULL;
    vlc_object_release( p_playlist );
}

/*****************************************************************************
 * Auto menus
 *****************************************************************************/
wxMenu *AudioMenu( intf_thread_t *_p_intf, wxWindow *p_parent, wxMenu *p_menu )
{
    vlc_object_t *p_object;
    ArrayOfInts ai_objects;
    ArrayOfStrings as_varnames;

    p_object = (vlc_object_t *)vlc_object_find( _p_intf, VLC_OBJECT_INPUT,
                                                FIND_ANYWHERE );
    if( p_object != NULL )
    {
        PUSH_VAR( "audio-es" );
        vlc_object_release( p_object );
    }

    p_object = (vlc_object_t *)vlc_object_find( _p_intf, VLC_OBJECT_AOUT,
                                                FIND_ANYWHERE );
    if( p_object )
    {
        AudioAutoMenuBuilder( p_object, ai_objects, as_varnames );
        vlc_object_release( p_object );
    }

    /* Build menu */
    Menu *p_vlc_menu = (Menu *)p_menu;
    if( !p_vlc_menu )
        p_vlc_menu = new Menu( _p_intf, AudioMenu_Events );
    else
        p_vlc_menu->Clear();

    p_vlc_menu->Populate(  as_varnames, ai_objects );

    return p_vlc_menu;
}

wxMenu *VideoMenu( intf_thread_t *_p_intf, wxWindow *p_parent, wxMenu *p_menu )
{
    vlc_object_t *p_object;
    ArrayOfInts ai_objects;
    ArrayOfStrings as_varnames;

    p_object = (vlc_object_t *)vlc_object_find( _p_intf, VLC_OBJECT_INPUT,
                                                FIND_ANYWHERE );
    if( p_object != NULL )
    {
        PUSH_VAR( "video-es" );
        PUSH_VAR( "spu-es" );
        vlc_object_release( p_object );
    }

    p_object = (vlc_object_t *)vlc_object_find( _p_intf, VLC_OBJECT_VOUT,
                                                FIND_ANYWHERE );
    if( p_object != NULL )
    {
        VideoAutoMenuBuilder( p_object, ai_objects, as_varnames );
        vlc_object_release( p_object );
    }

    /* Build menu */
    Menu *p_vlc_menu = (Menu *)p_menu;
    if( !p_vlc_menu )
        p_vlc_menu = new Menu( _p_intf, VideoMenu_Events );
    else
        p_vlc_menu->Clear();

    p_vlc_menu->Populate(  as_varnames, ai_objects );
    return p_vlc_menu;
}

wxMenu *NavigMenu( intf_thread_t *_p_intf, wxWindow *p_parent, wxMenu *p_menu )
{
    vlc_object_t *p_object;
    ArrayOfInts ai_objects;
    ArrayOfStrings as_varnames;

    p_object = (vlc_object_t *)vlc_object_find( _p_intf, VLC_OBJECT_INPUT,
                                                FIND_ANYWHERE );
    if( p_object != NULL )
    {
        InputAutoMenuBuilder( p_object, ai_objects, as_varnames );
        PUSH_VAR( "prev-title"); PUSH_VAR ( "next-title" );
        PUSH_VAR( "prev-chapter"); PUSH_VAR( "next-chapter" );
        vlc_object_release( p_object );
    }

    /* Build menu */
    Menu *p_vlc_menu = (Menu *)p_menu;
    if( !p_vlc_menu )
        p_vlc_menu = new Menu( _p_intf, NavigMenu_Events );
    else
        p_vlc_menu->Clear();

    p_vlc_menu->Populate( as_varnames, ai_objects );

    return p_vlc_menu;
}

wxMenu *SettingsMenu( intf_thread_t *_p_intf, wxWindow *p_parent,
                      wxMenu *p_menu )
{
    vlc_object_t *p_object;
    ArrayOfInts ai_objects;
    ArrayOfStrings as_varnames;

    p_object = (vlc_object_t *)vlc_object_find( _p_intf, VLC_OBJECT_INTF,
                                                FIND_PARENT );
    if( p_object != NULL )
    {
        PUSH_VAR( "intf-switch" );
        PUSH_VAR( "intf-add" );
        vlc_object_release( p_object );
    }

    /* Build menu */
    Menu *p_vlc_menu = (Menu *)p_menu;
    if( !p_vlc_menu )
        p_vlc_menu = new Menu( _p_intf, SettingsMenu_Events );
    else
        p_vlc_menu->Clear();

    p_vlc_menu->Populate( as_varnames, ai_objects );

    return p_vlc_menu;
}

/*****************************************************************************
 * Constructor.
 *****************************************************************************/
Menu::Menu( intf_thread_t *_p_intf, int _i_start_id ) : wxMenu( )
{
    /* Initializations */
    p_intf = _p_intf;
    i_start_id = _i_start_id;
}

Menu::~Menu()
{
}

/*****************************************************************************
 * Public methods.
 *****************************************************************************/
void Menu::Populate( ArrayOfStrings & ras_varnames,
                     ArrayOfInts & rai_objects )
{
    vlc_object_t *p_object;
    vlc_bool_t b_section_empty = VLC_FALSE;
    int i;

    i_item_id = i_start_id;

    for( i = 0; i < (int)rai_objects.GetCount() ; i++ )
    {
        if( !ras_varnames[i] || !*ras_varnames[i] )
        {
            if( b_section_empty )
            {
                Append( MenuDummy_Event + i, wxU(_("Empty")) );
                Enable( MenuDummy_Event + i, FALSE );
            }
            AppendSeparator();
            b_section_empty = VLC_TRUE;
            continue;
        }

        if( rai_objects[i] == 0  )
        {
            Append( MenuDummy_Event, wxU(ras_varnames[i]) );
            b_section_empty = VLC_FALSE;
            continue;
        }

        p_object = (vlc_object_t *)vlc_object_get( p_intf,
                                                   rai_objects[i] );
        if( p_object == NULL ) continue;

        b_section_empty = VLC_FALSE;
        CreateMenuItem( this, ras_varnames[i], p_object );
        vlc_object_release( p_object );
    }

    /* Special case for empty menus */
    if( GetMenuItemCount() == 0 || b_section_empty )
    {
        Append( MenuDummy_Event + i, wxU(_("Empty")) );
        Enable( MenuDummy_Event + i, FALSE );
    }
}

/* Work-around helper for buggy wxGTK */
static void RecursiveDestroy( wxMenu *menu )
{
    wxMenuItemList::Node *node = menu->GetMenuItems().GetFirst();
    for( ; node; )
    {
        wxMenuItem *item = node->GetData();
        node = node->GetNext();

        /* Delete the submenus */
        wxMenu *submenu = item->GetSubMenu();
        if( submenu )
        {
            RecursiveDestroy( submenu );
        }
        menu->Delete( item );
    }
}

void Menu::Clear( )
{
    RecursiveDestroy( this );
}

/*****************************************************************************
 * Private methods.
 *****************************************************************************/
static bool IsMenuEmpty( const char *psz_var, vlc_object_t *p_object,
                         bool b_root = TRUE )
{
    vlc_value_t val, val_list;
    int i_type, i_result, i;

    /* Check the type of the object variable */
    i_type = var_Type( p_object, psz_var );

    /* Check if we want to display the variable */
    if( !(i_type & VLC_VAR_HASCHOICE) ) return FALSE;

    var_Change( p_object, psz_var, VLC_VAR_CHOICESCOUNT, &val, NULL );
    if( val.i_int == 0 ) return TRUE;

    if( (i_type & VLC_VAR_TYPE) != VLC_VAR_VARIABLE )
    {
        /* Very evil hack ! intf-switch can have only one value */
        if( !strcmp( psz_var, "intf-switch" ) ) return FALSE;
        if( val.i_int == 1 && b_root ) return TRUE;
        else return FALSE;
    }

    /* Check children variables in case of VLC_VAR_VARIABLE */
    if( var_Change( p_object, psz_var, VLC_VAR_GETLIST, &val_list, NULL ) < 0 )
    {
        return TRUE;
    }

    for( i = 0, i_result = TRUE; i < val_list.p_list->i_count; i++ )
    {
        if( !IsMenuEmpty( val_list.p_list->p_values[i].psz_string,
                          p_object, FALSE ) )
        {
            i_result = FALSE;
            break;
        }
    }

    /* clean up everything */
    var_Change( p_object, psz_var, VLC_VAR_FREELIST, &val_list, NULL );

    return i_result;
}

void Menu::CreateMenuItem( wxMenu *menu, const char *psz_var,
                           vlc_object_t *p_object )
{
    wxMenuItemExt *menuitem;
    vlc_value_t val, text;
    int i_type;

    /* Check the type of the object variable */
    i_type = var_Type( p_object, psz_var );

    switch( i_type & VLC_VAR_TYPE )
    {
    case VLC_VAR_VOID:
    case VLC_VAR_BOOL:
    case VLC_VAR_VARIABLE:
    case VLC_VAR_STRING:
    case VLC_VAR_INTEGER:
    case VLC_VAR_FLOAT:
        break;
    default:
        /* Variable doesn't exist or isn't handled */
        return;
    }

    /* Make sure we want to display the variable */
    if( IsMenuEmpty( psz_var, p_object ) )  return;

    /* Get the descriptive name of the variable */
    var_Change( p_object, psz_var, VLC_VAR_GETTEXT, &text, NULL );

    if( i_type & VLC_VAR_HASCHOICE )
    {
        menu->Append( MenuDummy_Event,
                      wxU(text.psz_string ? text.psz_string : psz_var),
                      CreateChoicesMenu( psz_var, p_object, TRUE ),
                      wxT("")/* Nothing for now (maybe use a GETLONGTEXT) */ );

        if( text.psz_string ) free( text.psz_string );
        return;
    }


    switch( i_type & VLC_VAR_TYPE )
    {
    case VLC_VAR_VOID:
        var_Get( p_object, psz_var, &val );
        menuitem = new wxMenuItemExt( menu, ++i_item_id,
                                      wxU(text.psz_string ?
                                        text.psz_string : psz_var),
                                      wxT(""), wxITEM_NORMAL, strdup(psz_var),
                                      p_object->i_object_id, val, i_type );
        menu->Append( menuitem );
        break;

    case VLC_VAR_BOOL:
        var_Get( p_object, psz_var, &val );
        val.b_bool = !val.b_bool;
        menuitem = new wxMenuItemExt( menu, ++i_item_id,
                                      wxU(text.psz_string ?
                                        text.psz_string : psz_var),
                                      wxT(""), wxITEM_CHECK, strdup(psz_var),
                                      p_object->i_object_id, val, i_type );
        menu->Append( menuitem );
        Check( i_item_id, val.b_bool ? FALSE : TRUE );
        break;
    }

    if( text.psz_string ) free( text.psz_string );
}

wxMenu *Menu::CreateChoicesMenu( const char *psz_var, vlc_object_t *p_object,
                                 bool b_root )
{
    vlc_value_t val, val_list, text_list;
    int i_type, i;

    /* Check the type of the object variable */
    i_type = var_Type( p_object, psz_var );

    /* Make sure we want to display the variable */
    if( IsMenuEmpty( psz_var, p_object, b_root ) ) return NULL;

    switch( i_type & VLC_VAR_TYPE )
    {
    case VLC_VAR_VOID:
    case VLC_VAR_BOOL:
    case VLC_VAR_VARIABLE:
    case VLC_VAR_STRING:
    case VLC_VAR_INTEGER:
    case VLC_VAR_FLOAT:
        break;
    default:
        /* Variable doesn't exist or isn't handled */
        return NULL;
    }

    if( var_Change( p_object, psz_var, VLC_VAR_GETLIST,
                    &val_list, &text_list ) < 0 )
    {
        return NULL;
    }

    wxMenu *menu = new wxMenu;
    for( i = 0; i < val_list.p_list->i_count; i++ )
    {
        vlc_value_t another_val;
        wxMenuItemExt *menuitem;

        switch( i_type & VLC_VAR_TYPE )
        {
        case VLC_VAR_VARIABLE:
          menu->Append( MenuDummy_Event,
                        wxU(text_list.p_list->p_values[i].psz_string ?
                        text_list.p_list->p_values[i].psz_string :
                        val_list.p_list->p_values[i].psz_string),
                        CreateChoicesMenu(
                            val_list.p_list->p_values[i].psz_string,
                            p_object, FALSE ), wxT("") );
          break;

        case VLC_VAR_STRING:
          var_Get( p_object, psz_var, &val );

          another_val.psz_string =
              strdup(val_list.p_list->p_values[i].psz_string);
          menuitem =
              new wxMenuItemExt( menu, ++i_item_id,
                                 wxU(text_list.p_list->p_values[i].psz_string ?
                                 text_list.p_list->p_values[i].psz_string :
                                 another_val.psz_string), wxT(""),
                                 i_type & VLC_VAR_ISCOMMAND ?
                                   wxITEM_NORMAL : wxITEM_RADIO,
                                 strdup(psz_var),
                                 p_object->i_object_id, another_val, i_type );

          menu->Append( menuitem );

          if( !(i_type & VLC_VAR_ISCOMMAND) && val.psz_string &&
              !strcmp( val.psz_string,
                       val_list.p_list->p_values[i].psz_string ) )
              menu->Check( i_item_id, TRUE );

          if( val.psz_string ) free( val.psz_string );
          break;

        case VLC_VAR_INTEGER:
          var_Get( p_object, psz_var, &val );

          menuitem =
              new wxMenuItemExt( menu, ++i_item_id,
                                 text_list.p_list->p_values[i].psz_string ?
                                 (wxString)wxU(
                                   text_list.p_list->p_values[i].psz_string) :
                                 wxString::Format(wxT("%d"),
                                 val_list.p_list->p_values[i].i_int), wxT(""),
                                 i_type & VLC_VAR_ISCOMMAND ?
                                   wxITEM_NORMAL : wxITEM_RADIO,
                                 strdup(psz_var),
                                 p_object->i_object_id,
                                 val_list.p_list->p_values[i], i_type );

          menu->Append( menuitem );

          if( !(i_type & VLC_VAR_ISCOMMAND) &&
              val_list.p_list->p_values[i].i_int == val.i_int )
              menu->Check( i_item_id, TRUE );
          break;

        case VLC_VAR_FLOAT:
          var_Get( p_object, psz_var, &val );

          menuitem =
              new wxMenuItemExt( menu, ++i_item_id,
                                 text_list.p_list->p_values[i].psz_string ?
                                 (wxString)wxU(
                                   text_list.p_list->p_values[i].psz_string) :
                                 wxString::Format(wxT("%.2f"),
                                 val_list.p_list->p_values[i].f_float),wxT(""),
                                 i_type & VLC_VAR_ISCOMMAND ?
                                   wxITEM_NORMAL : wxITEM_RADIO,
                                 strdup(psz_var),
                                 p_object->i_object_id,
                                 val_list.p_list->p_values[i], i_type );

          menu->Append( menuitem );

          if( !(i_type & VLC_VAR_ISCOMMAND) &&
              val_list.p_list->p_values[i].f_float == val.f_float )
              menu->Check( i_item_id, TRUE );
          break;

        default:
          break;
        }
    }

    /* clean up everything */
    var_Change( p_object, psz_var, VLC_VAR_FREELIST, &val_list, &text_list );

    return menu;
}

/*****************************************************************************
 * A small helper class which intercepts all popup menu events
 *****************************************************************************/
MenuEvtHandler::MenuEvtHandler( intf_thread_t *_p_intf,
                                Interface *_p_main_interface )
{
    /* Initializations */
    p_intf = _p_intf;
    p_main_interface = _p_main_interface;
}

MenuEvtHandler::~MenuEvtHandler()
{
}

void MenuEvtHandler::OnShowDialog( wxCommandEvent& event )
{
    if( p_intf->p_sys->pf_show_dialog )
    {
        int i_id;

        switch( event.GetId() )
        {
        case OpenFileSimple_Event:
            i_id = INTF_DIALOG_FILE_SIMPLE;
            break;
        case OpenFile_Event:
            i_id = INTF_DIALOG_FILE;
            break;
        case OpenDirectory_Event:
            i_id = INTF_DIALOG_DIRECTORY;
            break;
        case OpenDisc_Event:
            i_id = INTF_DIALOG_DISC;
            break;
        case OpenNet_Event:
            i_id = INTF_DIALOG_NET;
            break;
        case OpenCapture_Event:
            i_id = INTF_DIALOG_CAPTURE;
            break;
        case MediaInfo_Event:
            i_id = INTF_DIALOG_FILEINFO;
            break;
        case Messages_Event:
            i_id = INTF_DIALOG_MESSAGES;
            break;
        case Preferences_Event:
            i_id = INTF_DIALOG_PREFS;
            break;
        default:
            i_id = INTF_DIALOG_FILE;
            break;

        }

        p_intf->p_sys->pf_show_dialog( p_intf, i_id, 1, 0 );
    }
}

void MenuEvtHandler::OnMenuEvent( wxCommandEvent& event )
{
    wxMenuItem *p_menuitem = NULL;
    int i_hotkey_event = p_intf->p_sys->i_first_hotkey_event;
    int i_hotkeys = p_intf->p_sys->i_hotkeys;

    if( event.GetId() >= Play_Event && event.GetId() <= Stop_Event )
    {
        input_thread_t *p_input;
        playlist_t * p_playlist =
            (playlist_t *)vlc_object_find( p_intf, VLC_OBJECT_PLAYLIST,
                                           FIND_ANYWHERE );
        if( !p_playlist ) return;

        switch( event.GetId() )
        {
        case Play_Event:
        case Pause_Event:
            p_input =
                (input_thread_t *)vlc_object_find( p_intf, VLC_OBJECT_INPUT,
                                                   FIND_ANYWHERE );
            if( !p_input ) playlist_Play( p_playlist );
            else
            {
                vlc_value_t val;
                var_Get( p_input, "state", &val );
                if( val.i_int != PAUSE_S ) val.i_int = PAUSE_S;
                else val.i_int = PLAYING_S;
                var_Set( p_input, "state", val );
                vlc_object_release( p_input );
            }
            break;
        case Stop_Event:
            playlist_Stop( p_playlist );
            break;
        case Previous_Event:
            playlist_Prev( p_playlist );
            break;
        case Next_Event:
            playlist_Next( p_playlist );
            break;
        }

        vlc_object_release( p_playlist );
        return;
    }

    /* Check if this is an auto generated menu item */
    if( event.GetId() < FirstAutoGenerated_Event )
    {
        event.Skip();
        return;
    }

    /* Check if this is an hotkey event */
    if( event.GetId() >= i_hotkey_event &&
        event.GetId() < i_hotkey_event + i_hotkeys )
    {
        vlc_value_t val;

        val.i_int =
            p_intf->p_vlc->p_hotkeys[event.GetId() - i_hotkey_event].i_key;

        /* Get the key combination and send it to the hotkey handler */
        var_Set( p_intf->p_vlc, "key-pressed", val );
        return;
    }

    if( !p_main_interface ||
        (p_menuitem = p_main_interface->GetMenuBar()->FindItem(event.GetId()))
        == NULL )
    {
        if( p_intf->p_sys->p_popup_menu )
        {
            p_menuitem =
                p_intf->p_sys->p_popup_menu->FindItem( event.GetId() );
        }
    }

    if( p_menuitem )
    {
        wxMenuItemExt *p_menuitemext = (wxMenuItemExt *)p_menuitem;
        vlc_object_t *p_object;

        p_object = (vlc_object_t *)vlc_object_get( p_intf,
                                       p_menuitemext->i_object_id );
        if( p_object == NULL ) return;

        wxMutexGuiLeave(); // We don't want deadlocks
        var_Set( p_object, p_menuitemext->psz_var, p_menuitemext->val );
        //wxMutexGuiEnter();

        vlc_object_release( p_object );
    }
    else
        event.Skip();
}

/*****************************************************************************
 * A small helper class which encapsulate wxMenuitem with some other useful
 * things.
 *****************************************************************************/
wxMenuItemExt::wxMenuItemExt( wxMenu* parentMenu, int id, const wxString& text,
    const wxString& helpString, wxItemKind kind,
    char *_psz_var, int _i_object_id, vlc_value_t _val, int _i_val_type ):
    wxMenuItem( parentMenu, id, text, helpString, kind )
{
    /* Initializations */
    psz_var = _psz_var;
    i_val_type = _i_val_type;
    i_object_id = _i_object_id;
    val = _val;
};

wxMenuItemExt::~wxMenuItemExt()
{
    if( psz_var ) free( psz_var );
    if( ((i_val_type & VLC_VAR_TYPE) == VLC_VAR_STRING)
        && val.psz_string ) free( val.psz_string );
};

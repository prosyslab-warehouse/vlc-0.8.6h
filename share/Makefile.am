NULL =

EXTRA_DIST = $(DIST_rsrc) $(DIST_fonts) \
	$(skins2_default_vlt_FILES)

nobase_dist_data_DATA = \
	$(DIST_applications) \
	$(NULL)

pkgdatadir = $(datadir)/vlc

dist_pkgdata_DATA = \
	$(DIST_pda) \
	$(DIST_icons16) \
	$(DIST_icons32) \
	$(DIST_icons48) \
	$(DIST_icons128) \
	$(NULL)

nobase_dist_pkgdata_DATA = \
	$(DIST_skins2) \
	$(DIST_http) \
	$(DIST_osdmenu_default) \
	$(DIST_osdmenu_dvd) \
	$(NULL)

nobase_pkgdata_DATA = skins2/default.vlt

skins2/default.vlt: $(skins2_default_vlt_FILES)

MOSTLYCLEANFILES = $(nobase_pkgdata_DATA)

DIST_fonts = \
	$(NULL)

DIST_rsrc = \
	newres.h \
	resource.h \
	vlc_wince_rc.rc \
	vlc_beos.rsrc \
	vlc_win32_rc.rc \
	$(NULL)

DIST_pda = \
        pda-forwardb16x16.xpm \
        pda-openb16x16.xpm \
        pda-pauseb16x16.xpm \
        pda-playb16x16.xpm \
        pda-playlistb16x16.xpm \
        pda-preferencesb16x16.xpm \
        pda-rewindb16x16.xpm \
        pda-stopb16x16.xpm \
        $(NULL)

DIST_icons16 = \
	vlc16x16.png \
	vlc16x16.xpm \
	$(NULL)

DIST_icons32 = \
	vlc32x32.png \
	vlc32x32.xpm \
	$(NULL)

DIST_icons48 = \
	vlc48x48.ico \
	vlc48x48.png \
	$(NULL)

DIST_icons128 = \
	vlc128x128.png \
	$(NULL)

DIST_skins2 = \
	skins2/fonts/FreeSans.ttf \
	skins2/fonts/FreeSansBold.ttf \
	skins2/skin.dtd \
	skins2/skin.catalog \
	skins2/winamp2.xml \
	$(NULL)

skins2_default_vlt_FILES = \
        skins2/default/main/pause_over.png \
        skins2/default/main/main_up.png \
        skins2/default/main/timeslider.png \
        skins2/default/main/main_down.png \
        skins2/default/main/main.png \
        skins2/default/main/dvd_prevtitle.png \
        skins2/default/main/dvd_nexttitle.png \
        skins2/default/main/dvd_nextchapter.png \
        skins2/default/main/pause_up.png \
        skins2/default/main/main_over.png \
        skins2/default/main/pause_down.png \
        skins2/default/main/dvd_prevchapter.png \
        skins2/default/main/vol_bg.png \
        skins2/default/main/vol_mask.png \
        skins2/default/main/vol_slider.png \
        skins2/default/main/dvd_menu.png \
        skins2/default/switch.png \
        skins2/default/font.otf \
        skins2/default/credits.png \
        skins2/default/theme.xml \
        skins2/default/playlist/plscroller.png \
        skins2/default/playlist/playlist_down.png \
        skins2/default/playlist/playtreeglyphs.png \
        skins2/default/playlist/playlist_up.png \
        skins2/default/playlist/playlist_over.png \
        skins2/default/playlist/playlist.png \
        skins2/default/minimal.png \
        skins2/default/equalizer.png

DIST_http = \
	http/.hosts \
	http/dialogs/.hosts \
	http/dialogs/browse \
	http/dialogs/footer \
	http/dialogs/input \
	http/dialogs/main \
	http/dialogs/mosaic \
	http/dialogs/playlist \
	http/dialogs/sout \
	http/dialogs/vlm \
	http/favicon.ico \
	http/images/delete.png \
	http/images/delete_small.png \
	http/images/eject.png \
	http/images/empty.png \
	http/images/fullscreen.png \
	http/images/help.png \
	http/images/info.png \
	http/images/loop.png \
	http/images/minus.png \
	http/images/next.png \
	http/images/pause.png \
	http/images/play.png \
	http/images/playlist.png \
	http/images/playlist_small.png \
	http/images/plus.png \
	http/images/prev.png \
	http/images/refresh.png \
	http/images/repeat.png \
	http/images/sd.png \
	http/images/shuffle.png \
	http/images/slider_bar.png \
	http/images/slider_left.png \
	http/images/slider_point.png \
	http/images/slider_right.png \
	http/images/snapshot.png \
	http/images/slow.png \
	http/images/sort.png \
	http/images/sout.png \
	http/images/speaker.png \
	http/images/speaker_mute.png \
	http/images/stop.png \
	http/images/vlc16x16.png \
	http/images/volume_down.png \
	http/images/volume_up.png \
	http/images/white.png \
	http/images/white_cross_small.png \
	http/index.html \
	http/js/functions.js \
	http/js/mosaic.js \
	http/js/vlm.js \
	http/mosaic.html \
	http/old/.hosts \
	http/old/admin/.access \
	http/old/admin/browse.html \
	http/old/admin/dboxfiles.html \
	http/old/admin/index.html \
	http/old/cone_minus.png \
	http/old/cone_plus.png \
	http/old/index.html \
	http/old/info.html \
	http/old/style.css \
	http/old/vlm/edit.html \
	http/old/vlm/index.html \
	http/old/vlm/new.html \
	http/old/vlm/show.html \
	http/old/webcam.html \
	http/requests/browse.xml \
	http/requests/playlist.xml \
	http/requests/status.xml \
	http/requests/vlm.xml \
	http/requests/vlm_cmd.xml \
	http/requests/readme \
	http/style.css \
	http/iehacks.css \
	http/vlm.html \
	http/vlm_export.html \
	$(NULL)

DIST_osdmenu_default = \
	osdmenu/default.cfg \
	osdmenu/default/unselected.png \
	osdmenu/default/selection/bw.png \
	osdmenu/default/selection/esc.png \
	osdmenu/default/selection/fw.png \
	osdmenu/default/selection/volume.png \
	osdmenu/default/selection/next.png \
	osdmenu/default/selection/play_pause.png \
	osdmenu/default/selection/previous.png \
	osdmenu/default/selection/stop.png \
	osdmenu/default/selected/bw.png \
	osdmenu/default/selected/next.png \
	osdmenu/default/selected/play_pause.png \
	osdmenu/default/selected/fw.png \
	osdmenu/default/selected/stop.png \
	osdmenu/default/selected/previous.png \
	osdmenu/default/selected/esc.png \
	osdmenu/default/selected/volume.png \
	osdmenu/default/volume/volume_00.png \
	osdmenu/default/volume/volume_01.png \
	osdmenu/default/volume/volume_02.png \
	osdmenu/default/volume/volume_03.png \
	osdmenu/default/volume/volume_04.png \
	osdmenu/default/volume/volume_05.png \
	osdmenu/default/volume/volume_06.png \
	osdmenu/default/volume/volume_07.png \
	osdmenu/default/volume/volume_08.png \
	osdmenu/default/volume/volume_09.png \
	osdmenu/default/volume/volume_10.png \
	$(NULL)

DIST_osdmenu_dvd = \
	osdmenu/dvd.cfg \
	osdmenu/dvd/selection/bw.png \
	osdmenu/dvd/selection/esc.png \
	osdmenu/dvd/selection/fw.png \
	osdmenu/dvd/selection/mute.png \
	osdmenu/dvd/selection/next.png \
	osdmenu/dvd/selection/pause.png \
	osdmenu/dvd/selection/play.png \
	osdmenu/dvd/selection/previous.png \
	osdmenu/dvd/selection/slow.png \
	osdmenu/dvd/selection/stop.png \
	osdmenu/dvd/selected/bw.png \
	osdmenu/dvd/selected/next.png \
	osdmenu/dvd/selected/play.png \
	osdmenu/dvd/selected/fw.png \
	osdmenu/dvd/selected/stop.png \
	osdmenu/dvd/selected/pause.png \
	osdmenu/dvd/selected/previous.png \
	osdmenu/dvd/selected/esc.png \
	osdmenu/dvd/selected/mute.png \
	osdmenu/dvd/selected/slow.png \
	osdmenu/dvd/selected/volume.png \
	osdmenu/dvd/volume/volume00.png \
	osdmenu/dvd/volume/volume01.png \
	osdmenu/dvd/volume/volume02.png \
	osdmenu/dvd/volume/volume03.png \
	osdmenu/dvd/volume/volume04.png \
	osdmenu/dvd/volume/volume05.png \
	osdmenu/dvd/unselect/barroff.png \
	$(NULL)

DIST_applications = \
	applications/vlc.desktop \
	$(NULL)

skins2/default.vlt:
	mkdir -p skins2
	tar cvvzf skins2/default.vlt --exclude .svn -C $(srcdir)/skins2 default/

.PHONY: $(nobase_pkgdata_DATA)


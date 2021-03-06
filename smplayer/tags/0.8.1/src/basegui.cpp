/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2012 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "basegui.h"

#include "filedialog.h"
#include <QMessageBox>
#include <QLabel>
#include <QMenu>
#include <QFileInfo>
#include <QApplication>
#include <QMenuBar>
#include <QHBoxLayout>
#include <QCursor>
#include <QTimer>
#include <QStyle>
#include <QRegExp>
#include <QStatusBar>
#include <QActionGroup>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopServices>
#include <QInputDialog>

#include <cmath>

#include "mplayerwindow.h"
#include "desktopinfo.h"
#include "helper.h"
#include "paths.h"
#include "colorutils.h"
#include "global.h"
#include "translator.h"
#include "images.h"
#include "preferences.h"
#include "discname.h"
#include "timeslider.h"
#include "logwindow.h"
#include "playlist.h"
#include "filepropertiesdialog.h"
#include "eqslider.h"
#include "videoequalizer.h"
#include "audioequalizer.h"
#include "inputdvddirectory.h"
#include "inputmplayerversion.h"
#include "inputurl.h"
#include "recents.h"
#include "urlhistory.h"
#include "about.h"
#include "errordialog.h"
#include "timedialog.h"
#include "clhelp.h"
#include "mplayerversion.h"

#ifdef FIND_SUBTITLES
#include "findsubtitleswindow.h"
#endif

#ifdef VIDEOPREVIEW
#include "videopreview.h"
#endif

#include "config.h"
#include "actionseditor.h"

#include "tvlist.h"

#include "preferencesdialog.h"
#ifndef NO_USE_INI_FILES
#include "prefgeneral.h"
#endif
#include "prefinterface.h"
#include "prefinput.h"
#include "prefadvanced.h"
#include "prefplaylist.h"

#include "myaction.h"
#include "myactiongroup.h"
#include "playlist.h"

#include "constants.h"

#include "extensions.h"

#ifdef Q_OS_WIN
#include "deviceinfo.h"
#include <QSysInfo>
#endif

using namespace Global;

BaseGui::BaseGui( QWidget* parent, Qt::WindowFlags flags ) 
	: QMainWindow( parent, flags ),
		near_top(false),
		near_bottom(false)
{
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	/* Disable screensaver by event */
	just_stopped = false;
#endif
	ignore_show_hide_events = false;

	arg_close_on_finish = -1;
	arg_start_in_fullscreen = -1;

	setWindowTitle( "SMPlayer" );

	// Not created objects
	popup = 0;
	pref_dialog = 0;
	file_dialog = 0;
	clhelp_window = 0;
#ifdef FIND_SUBTITLES
	find_subs_dialog = 0;
#endif
#ifdef VIDEOPREVIEW
	video_preview = 0;
#endif

	// Create objects:
	createPanel();
	setCentralWidget(panel);

	createMplayerWindow();
	createCore();
	createPlaylist();
	createVideoEqualizer();
	createAudioEqualizer();

	// Mouse Wheel
	connect( this, SIGNAL(wheelUp()),
             core, SLOT(wheelUp()) );
	connect( this, SIGNAL(wheelDown()),
             core, SLOT(wheelDown()) );
	connect( mplayerwindow, SIGNAL(wheelUp()),
             core, SLOT(wheelUp()) );
	connect( mplayerwindow, SIGNAL(wheelDown()),
             core, SLOT(wheelDown()) );

	// Set style before changing color of widgets:
	// Set style
#if STYLE_SWITCHING
	qDebug( "Style name: '%s'", qApp->style()->objectName().toUtf8().data() );
	qDebug( "Style class name: '%s'", qApp->style()->metaObject()->className() );

	default_style = qApp->style()->objectName();
	if (!pref->style.isEmpty()) {
		qApp->setStyle( pref->style );
	}
#endif

#ifdef LOG_MPLAYER
	mplayer_log_window = new LogWindow(0);
#endif
#ifdef LOG_SMPLAYER
	smplayer_log_window = new LogWindow(0);
#endif

	createActions();
	createMenus();
#if AUTODISABLE_ACTIONS
	setActionsEnabled(false);
#endif

#if !DOCK_PLAYLIST
	connect(playlist, SIGNAL(visibilityChanged(bool)),
            showPlaylistAct, SLOT(setChecked(bool)) );
#endif

	retranslateStrings();

	setAcceptDrops(true);

	resize(pref->default_size);

	panel->setFocus();

	initializeGui();
}

void BaseGui::initializeGui() {
	if (pref->compact_mode) toggleCompactMode(TRUE);
	changeStayOnTop(pref->stay_on_top);

#if ALLOW_CHANGE_STYLESHEET
	changeStyleSheet(pref->iconset);
#endif

	updateRecents();

	// Call loadActions() outside initialization of the class.
	// Otherwise DefaultGui (and other subclasses) doesn't exist, and 
	// its actions are not loaded
	QTimer::singleShot(20, this, SLOT(loadActions()));

	// Single instance
	/* Deleted */
}

#ifdef SINGLE_INSTANCE
void BaseGui::handleMessageFromOtherInstances(const QString& message) {
	qDebug("BaseGui::handleMessageFromOtherInstances: '%s'", message.toUtf8().constData());

	int pos = message.indexOf(' ');
	if (pos > -1) {
		QString command = message.left(pos);
		QString arg = message.mid(pos+1);
		qDebug("command: '%s'", command.toUtf8().constData());
		qDebug("arg: '%s'", arg.toUtf8().constData());

		if (command == "open_file") {
			open(arg);
		} 
		else
		if (command == "open_files") {
			QStringList file_list = arg.split(" <<sep>> ");
			openFiles(file_list);
		}
		else
		if (command == "add_to_playlist") {
			QStringList file_list = arg.split(" <<sep>> ");
			playlist->addFiles(file_list);
		}
		else
		if (command == "action") {
			processFunction(arg);
		}
		else
		if (command == "load_sub") {
			setInitialSubtitle(arg); 
			if (core->state() != Core::Stopped) {
				core->loadSub(arg);
			}
		}
	}
}
#endif

BaseGui::~BaseGui() {
	delete core; // delete before mplayerwindow, otherwise, segfault...
#ifdef LOG_MPLAYER
	delete mplayer_log_window;
#endif
#ifdef LOG_SMPLAYER
	delete smplayer_log_window;
#endif

	delete favorites;
	delete tvlist;
	delete radiolist;

//#if !DOCK_PLAYLIST
	if (playlist) {
		delete playlist;
		playlist = 0;
	}
//#endif

#ifdef FIND_SUBTITLES
	if (find_subs_dialog) {
		delete find_subs_dialog;
		find_subs_dialog = 0; // Necessary?
	}
#endif

#ifdef VIDEOPREVIEW
	if (video_preview) {
		delete video_preview;
	}
#endif
}

void BaseGui::createActions() {
	// Menu File
	openFileAct = new MyAction( QKeySequence("Ctrl+F"), this, "open_file" );
	connect( openFileAct, SIGNAL(triggered()),
             this, SLOT(openFile()) );

	openDirectoryAct = new MyAction( this, "open_directory" );
	connect( openDirectoryAct, SIGNAL(triggered()),
             this, SLOT(openDirectory()) );

	openPlaylistAct = new MyAction( this, "open_playlist" );
	connect( openPlaylistAct, SIGNAL(triggered()),
             playlist, SLOT(load()) );

	openVCDAct = new MyAction( this, "open_vcd" );
	connect( openVCDAct, SIGNAL(triggered()),
             this, SLOT(openVCD()) );

	openAudioCDAct = new MyAction( this, "open_audio_cd" );
	connect( openAudioCDAct, SIGNAL(triggered()),
             this, SLOT(openAudioCD()) );

	openDVDAct = new MyAction( this, "open_dvd" );
	connect( openDVDAct, SIGNAL(triggered()),
             this, SLOT(openDVD()) );

	openDVDFolderAct = new MyAction( this, "open_dvd_folder" );
	connect( openDVDFolderAct, SIGNAL(triggered()),
             this, SLOT(openDVDFromFolder()) );

	openURLAct = new MyAction( QKeySequence("Ctrl+U"), this, "open_url" );
	connect( openURLAct, SIGNAL(triggered()),
             this, SLOT(openURL()) );

	exitAct = new MyAction( QKeySequence("Ctrl+X"), this, "close" );
	connect( exitAct, SIGNAL(triggered()), this, SLOT(closeWindow()) );

	clearRecentsAct = new MyAction( this, "clear_recents" );
	connect( clearRecentsAct, SIGNAL(triggered()), this, SLOT(clearRecentsList()) );

	// Favorites
	favorites = new Favorites(Paths::configPath() + "/favorites.m3u8", this);
	favorites->menuAction()->setObjectName( "favorites_menu" );
	addAction(favorites->editAct());
	addAction(favorites->jumpAct());
	addAction(favorites->nextAct());
	addAction(favorites->previousAct());
	connect(favorites, SIGNAL(activated(QString)), this, SLOT(openFavorite(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString &, const QString &)),
            favorites, SLOT(getCurrentMedia(const QString &, const QString &)));

	// TV and Radio
	tvlist = new TVList(pref->check_channels_conf_on_startup, 
                        TVList::TV, Paths::configPath() + "/tv.m3u8", this);
	tvlist->menuAction()->setObjectName( "tv_menu" );
	addAction(tvlist->editAct());
	addAction(tvlist->jumpAct());
	addAction(tvlist->nextAct());
	addAction(tvlist->previousAct());
	tvlist->nextAct()->setShortcut( Qt::Key_H );
	tvlist->previousAct()->setShortcut( Qt::Key_L );
	tvlist->nextAct()->setObjectName("next_tv");
	tvlist->previousAct()->setObjectName("previous_tv");
	tvlist->editAct()->setObjectName("edit_tv_list");
	tvlist->jumpAct()->setObjectName("jump_tv_list");
	connect(tvlist, SIGNAL(activated(QString)), this, SLOT(open(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString &, const QString &)),
            tvlist, SLOT(getCurrentMedia(const QString &, const QString &)));

	radiolist = new TVList(pref->check_channels_conf_on_startup, 
                           TVList::Radio, Paths::configPath() + "/radio.m3u8", this);
	radiolist->menuAction()->setObjectName( "radio_menu" );
	addAction(radiolist->editAct());
	addAction(radiolist->jumpAct());
	addAction(radiolist->nextAct());
	addAction(radiolist->previousAct());
	radiolist->nextAct()->setShortcut( Qt::SHIFT | Qt::Key_H );
	radiolist->previousAct()->setShortcut( Qt::SHIFT | Qt::Key_L );
	radiolist->nextAct()->setObjectName("next_radio");
	radiolist->previousAct()->setObjectName("previous_radio");
	radiolist->editAct()->setObjectName("edit_radio_list");
	radiolist->jumpAct()->setObjectName("jump_radio_list");
	connect(radiolist, SIGNAL(activated(QString)), this, SLOT(open(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString &, const QString &)),
            radiolist, SLOT(getCurrentMedia(const QString &, const QString &)));


	// Menu Play
	playAct = new MyAction( this, "play" );
	connect( playAct, SIGNAL(triggered()),
             core, SLOT(play()) );

	playOrPauseAct = new MyAction( Qt::Key_MediaPlay, this, "play_or_pause" );
	connect( playOrPauseAct, SIGNAL(triggered()),
             core, SLOT(play_or_pause()) );

	pauseAct = new MyAction( Qt::Key_Space, this, "pause" );
	connect( pauseAct, SIGNAL(triggered()),
             core, SLOT(pause()) );

	pauseAndStepAct = new MyAction( this, "pause_and_frame_step" );
	connect( pauseAndStepAct, SIGNAL(triggered()),
             core, SLOT(pause_and_frame_step()) );

	stopAct = new MyAction( Qt::Key_MediaStop, this, "stop" );
	connect( stopAct, SIGNAL(triggered()),
             core, SLOT(stop()) );

	frameStepAct = new MyAction( Qt::Key_Period, this, "frame_step" );
	connect( frameStepAct, SIGNAL(triggered()),
             core, SLOT(frameStep()) );

	rewind1Act = new MyAction( Qt::Key_Left, this, "rewind1" );
	connect( rewind1Act, SIGNAL(triggered()),
             core, SLOT(srewind()) );

	rewind2Act = new MyAction( Qt::Key_Down, this, "rewind2" );
	connect( rewind2Act, SIGNAL(triggered()),
             core, SLOT(rewind()) );

	rewind3Act = new MyAction( Qt::Key_PageDown, this, "rewind3" );
	connect( rewind3Act, SIGNAL(triggered()),
             core, SLOT(fastrewind()) );

	forward1Act = new MyAction( Qt::Key_Right, this, "forward1" );
	connect( forward1Act, SIGNAL(triggered()),
             core, SLOT(sforward()) );

	forward2Act = new MyAction( Qt::Key_Up, this, "forward2" );
	connect( forward2Act, SIGNAL(triggered()),
             core, SLOT(forward()) );

	forward3Act = new MyAction( Qt::Key_PageUp, this, "forward3" );
	connect( forward3Act, SIGNAL(triggered()),
             core, SLOT(fastforward()) );

	setAMarkerAct = new MyAction( this, "set_a_marker" );
	connect( setAMarkerAct, SIGNAL(triggered()),
             core, SLOT(setAMarker()) );

	setBMarkerAct = new MyAction( this, "set_b_marker" );
	connect( setBMarkerAct, SIGNAL(triggered()),
             core, SLOT(setBMarker()) );

	clearABMarkersAct = new MyAction( this, "clear_ab_markers" );
	connect( clearABMarkersAct, SIGNAL(triggered()),
             core, SLOT(clearABMarkers()) );

	repeatAct = new MyAction( this, "repeat" );
	repeatAct->setCheckable( true );
	connect( repeatAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleRepeat(bool)) );

	gotoAct = new MyAction( QKeySequence("Ctrl+J"), this, "jump_to" );
	connect( gotoAct, SIGNAL(triggered()),
             this, SLOT(showGotoDialog()) );

	// Submenu Speed
	normalSpeedAct = new MyAction( Qt::Key_Backspace, this, "normal_speed" );
	connect( normalSpeedAct, SIGNAL(triggered()),
             core, SLOT(normalSpeed()) );

	halveSpeedAct = new MyAction( Qt::Key_BraceLeft, this, "halve_speed" );
	connect( halveSpeedAct, SIGNAL(triggered()),
             core, SLOT(halveSpeed()) );

	doubleSpeedAct = new MyAction( Qt::Key_BraceRight, this, "double_speed" );
	connect( doubleSpeedAct, SIGNAL(triggered()),
             core, SLOT(doubleSpeed()) );

	decSpeed10Act = new MyAction( Qt::Key_BracketLeft, this, "dec_speed" );
	connect( decSpeed10Act, SIGNAL(triggered()),
             core, SLOT(decSpeed10()) );

	incSpeed10Act = new MyAction( Qt::Key_BracketRight, this, "inc_speed" );
	connect( incSpeed10Act, SIGNAL(triggered()),
             core, SLOT(incSpeed10()) );

	decSpeed4Act = new MyAction( this, "dec_speed_4" );
	connect( decSpeed4Act, SIGNAL(triggered()),
             core, SLOT(decSpeed4()) );

	incSpeed4Act = new MyAction( this, "inc_speed_4" );
	connect( incSpeed4Act, SIGNAL(triggered()),
             core, SLOT(incSpeed4()) );

	decSpeed1Act = new MyAction( this, "dec_speed_1" );
	connect( decSpeed1Act, SIGNAL(triggered()),
             core, SLOT(decSpeed1()) );

	incSpeed1Act = new MyAction( this, "inc_speed_1" );
	connect( incSpeed1Act, SIGNAL(triggered()),
             core, SLOT(incSpeed1()) );


	// Menu Video
	fullscreenAct = new MyAction( Qt::Key_F, this, "fullscreen" );
	fullscreenAct->setCheckable( true );
	connect( fullscreenAct, SIGNAL(toggled(bool)),
             this, SLOT(toggleFullscreen(bool)) );

	compactAct = new MyAction( QKeySequence("Ctrl+C"), this, "compact" );
	compactAct->setCheckable( true );
	connect( compactAct, SIGNAL(toggled(bool)),
             this, SLOT(toggleCompactMode(bool)) );

	videoEqualizerAct = new MyAction( QKeySequence("Ctrl+E"), this, "video_equalizer" );
	videoEqualizerAct->setCheckable( true );
	connect( videoEqualizerAct, SIGNAL(toggled(bool)),
             this, SLOT(showVideoEqualizer(bool)) );

	// Single screenshot
	screenshotAct = new MyAction( Qt::Key_S, this, "screenshot" );
	connect( screenshotAct, SIGNAL(triggered()),
             core, SLOT(screenshot()) );

	// Multiple screenshots
	screenshotsAct = new MyAction( QKeySequence("Shift+D"), this, "multiple_screenshots" );
	connect( screenshotsAct, SIGNAL(triggered()),
             core, SLOT(screenshots()) );

#ifdef VIDEOPREVIEW
	videoPreviewAct = new MyAction( this, "video_preview" );
	connect( videoPreviewAct, SIGNAL(triggered()),
             this, SLOT(showVideoPreviewDialog()) );
#endif

	flipAct = new MyAction( this, "flip" );
	flipAct->setCheckable( true );
	connect( flipAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleFlip(bool)) );

	mirrorAct = new MyAction( this, "mirror" );
	mirrorAct->setCheckable( true );
	connect( mirrorAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleMirror(bool)) );


	// Submenu filter
	postProcessingAct = new MyAction( this, "postprocessing" );
	postProcessingAct->setCheckable( true );
	connect( postProcessingAct, SIGNAL(toggled(bool)),
             core, SLOT(togglePostprocessing(bool)) );

	phaseAct = new MyAction( this, "autodetect_phase" );
	phaseAct->setCheckable( true );
	connect( phaseAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleAutophase(bool)) );

	deblockAct = new MyAction( this, "deblock" );
	deblockAct->setCheckable( true );
	connect( deblockAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleDeblock(bool)) );

	deringAct = new MyAction( this, "dering" );
	deringAct->setCheckable( true );
	connect( deringAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleDering(bool)) );

	gradfunAct = new MyAction( this, "gradfun" );
	gradfunAct->setCheckable( true );
	connect( gradfunAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleGradfun(bool)) );


	addNoiseAct = new MyAction( this, "add_noise" );
	addNoiseAct->setCheckable( true );
	connect( addNoiseAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleNoise(bool)) );

	addLetterboxAct = new MyAction( this, "add_letterbox" );
	addLetterboxAct->setCheckable( true );
	connect( addLetterboxAct, SIGNAL(toggled(bool)),
             core, SLOT(changeLetterbox(bool)) );

	upscaleAct = new MyAction( this, "upscaling" );
	upscaleAct->setCheckable( true );
	connect( upscaleAct, SIGNAL(toggled(bool)),
             core, SLOT(changeUpscale(bool)) );


	// Menu Audio
	audioEqualizerAct = new MyAction( this, "audio_equalizer" );
	audioEqualizerAct->setCheckable( true );
	connect( audioEqualizerAct, SIGNAL(toggled(bool)),
             this, SLOT(showAudioEqualizer(bool)) );

	muteAct = new MyAction( Qt::Key_M, this, "mute" );
	muteAct->setCheckable( true );
	connect( muteAct, SIGNAL(toggled(bool)),
             core, SLOT(mute(bool)) );

#if USE_MULTIPLE_SHORTCUTS
	decVolumeAct = new MyAction( this, "decrease_volume" );
	decVolumeAct->setShortcuts( ActionsEditor::stringToShortcuts("9,/") );
#else
	decVolumeAct = new MyAction( Qt::Key_9, this, "dec_volume" );
#endif
	connect( decVolumeAct, SIGNAL(triggered()),
             core, SLOT(decVolume()) );

#if USE_MULTIPLE_SHORTCUTS
	incVolumeAct = new MyAction( this, "increase_volume" );
	incVolumeAct->setShortcuts( ActionsEditor::stringToShortcuts("0,*") );
#else
	incVolumeAct = new MyAction( Qt::Key_0, this, "inc_volume" );
#endif
	connect( incVolumeAct, SIGNAL(triggered()),
             core, SLOT(incVolume()) );

	decAudioDelayAct = new MyAction( Qt::Key_Minus, this, "dec_audio_delay" );
	connect( decAudioDelayAct, SIGNAL(triggered()),
             core, SLOT(decAudioDelay()) );

	incAudioDelayAct = new MyAction( Qt::Key_Plus, this, "inc_audio_delay" );
	connect( incAudioDelayAct, SIGNAL(triggered()),
             core, SLOT(incAudioDelay()) );

	audioDelayAct = new MyAction( this, "audio_delay" );
	connect( audioDelayAct, SIGNAL(triggered()),
             this, SLOT(showAudioDelayDialog()) );

	loadAudioAct = new MyAction( this, "load_audio_file" );
	connect( loadAudioAct, SIGNAL(triggered()),
             this, SLOT(loadAudioFile()) );

	unloadAudioAct = new MyAction( this, "unload_audio_file" );
	connect( unloadAudioAct, SIGNAL(triggered()),
             core, SLOT(unloadAudioFile()) );


	// Submenu Filters
	extrastereoAct = new MyAction( this, "extrastereo_filter" );
	extrastereoAct->setCheckable( true );
	connect( extrastereoAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleExtrastereo(bool)) );

	karaokeAct = new MyAction( this, "karaoke_filter" );
	karaokeAct->setCheckable( true );
	connect( karaokeAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleKaraoke(bool)) );

	volnormAct = new MyAction( this, "volnorm_filter" );
	volnormAct->setCheckable( true );
	connect( volnormAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleVolnorm(bool)) );


	// Menu Subtitles
	loadSubsAct = new MyAction( this, "load_subs" );
	connect( loadSubsAct, SIGNAL(triggered()),
             this, SLOT(loadSub()) );

	unloadSubsAct = new MyAction( this, "unload_subs" );
	connect( unloadSubsAct, SIGNAL(triggered()),
             core, SLOT(unloadSub()) );

	decSubDelayAct = new MyAction( Qt::Key_Z, this, "dec_sub_delay" );
	connect( decSubDelayAct, SIGNAL(triggered()),
             core, SLOT(decSubDelay()) );

	incSubDelayAct = new MyAction( Qt::Key_X, this, "inc_sub_delay" );
	connect( incSubDelayAct, SIGNAL(triggered()),
             core, SLOT(incSubDelay()) );

	subDelayAct = new MyAction( this, "sub_delay" );
	connect( subDelayAct, SIGNAL(triggered()),
             this, SLOT(showSubDelayDialog()) );

	decSubPosAct = new MyAction( Qt::Key_R, this, "dec_sub_pos" );
	connect( decSubPosAct, SIGNAL(triggered()),
             core, SLOT(decSubPos()) );
	incSubPosAct = new MyAction( Qt::Key_T, this, "inc_sub_pos" );
	connect( incSubPosAct, SIGNAL(triggered()),
             core, SLOT(incSubPos()) );

	decSubScaleAct = new MyAction( Qt::SHIFT | Qt::Key_R, this, "dec_sub_scale" );
	connect( decSubScaleAct, SIGNAL(triggered()),
             core, SLOT(decSubScale()) );

	incSubScaleAct = new MyAction( Qt::SHIFT | Qt::Key_T, this, "inc_sub_scale" );
	connect( incSubScaleAct, SIGNAL(triggered()),
             core, SLOT(incSubScale()) );
    
	decSubStepAct = new MyAction( Qt::Key_G, this, "dec_sub_step" );
	connect( decSubStepAct, SIGNAL(triggered()),
             core, SLOT(decSubStep()) );

	incSubStepAct = new MyAction( Qt::Key_Y, this, "inc_sub_step" );
	connect( incSubStepAct, SIGNAL(triggered()),
             core, SLOT(incSubStep()) );

	useAssAct = new MyAction(this, "use_ass_lib");
	useAssAct->setCheckable(true);
	connect( useAssAct, SIGNAL(toggled(bool)), core, SLOT(changeUseAss(bool)) );

	useForcedSubsOnlyAct = new MyAction(this, "use_forced_subs_only");
	useForcedSubsOnlyAct->setCheckable(true);
	connect( useForcedSubsOnlyAct, SIGNAL(toggled(bool)), core, SLOT(toggleForcedSubsOnly(bool)) );

	subVisibilityAct = new MyAction(Qt::Key_V, this, "subtitle_visibility");
	subVisibilityAct->setCheckable(true);
	connect( subVisibilityAct, SIGNAL(toggled(bool)), core, SLOT(changeSubVisibility(bool)) );

#ifdef FIND_SUBTITLES
	showFindSubtitlesDialogAct = new MyAction( this, "show_find_sub_dialog" );
	connect( showFindSubtitlesDialogAct, SIGNAL(triggered()), 
             this, SLOT(showFindSubtitlesDialog()) );

	openUploadSubtitlesPageAct = new MyAction( this, "upload_subtitles" );		//turbos
	connect( openUploadSubtitlesPageAct, SIGNAL(triggered()),					//turbos
             this, SLOT(openUploadSubtitlesPage()) );							//turbos
#endif

	// Menu Options
	showPlaylistAct = new MyAction( QKeySequence("Ctrl+L"), this, "show_playlist" );
	showPlaylistAct->setCheckable( true );
	connect( showPlaylistAct, SIGNAL(toggled(bool)),
             this, SLOT(showPlaylist(bool)) );

	showPropertiesAct = new MyAction( QKeySequence("Ctrl+I"), this, "show_file_properties" );
	connect( showPropertiesAct, SIGNAL(triggered()),
             this, SLOT(showFilePropertiesDialog()) );

	showPreferencesAct = new MyAction( QKeySequence("Ctrl+P"), this, "show_preferences" );
	connect( showPreferencesAct, SIGNAL(triggered()),
             this, SLOT(showPreferencesDialog()) );

	showTubeBrowserAct = new MyAction( Qt::Key_F11, this, "show_tube_browser" );
	connect( showTubeBrowserAct, SIGNAL(triggered()),
             this, SLOT(showTubeBrowser()) );

	// Submenu Logs
#ifdef LOG_MPLAYER
	showLogMplayerAct = new MyAction( QKeySequence("Ctrl+M"), this, "show_mplayer_log" );
	connect( showLogMplayerAct, SIGNAL(triggered()),
             this, SLOT(showMplayerLog()) );
#endif

#ifdef LOG_SMPLAYER
	showLogSmplayerAct = new MyAction( QKeySequence("Ctrl+S"), this, "show_smplayer_log" );
	connect( showLogSmplayerAct, SIGNAL(triggered()),
             this, SLOT(showLog()) );
#endif

	// Menu Help
	showFAQAct = new MyAction( this, "faq" );
	connect( showFAQAct, SIGNAL(triggered()),
             this, SLOT(helpFAQ()) );

	showCLOptionsAct = new MyAction( this, "cl_options" );
	connect( showCLOptionsAct, SIGNAL(triggered()),
             this, SLOT(helpCLOptions()) );

	showCheckUpdatesAct = new MyAction( this, "check_updates" );
	connect( showCheckUpdatesAct, SIGNAL(triggered()),
             this, SLOT(helpCheckUpdates()) );

	donateAct = new MyAction( this, "donate" );
	connect( donateAct, SIGNAL(triggered()),
             this, SLOT(helpDonate()) );

	aboutQtAct = new MyAction( this, "about_qt" );
	connect( aboutQtAct, SIGNAL(triggered()),
             this, SLOT(helpAboutQt()) );

	aboutThisAct = new MyAction( this, "about_smplayer" );
	connect( aboutThisAct, SIGNAL(triggered()),
             this, SLOT(helpAbout()) );

	// Playlist
	playNextAct = new MyAction(Qt::Key_Greater, this, "play_next");
	connect( playNextAct, SIGNAL(triggered()), playlist, SLOT(playNext()) );

	playPrevAct = new MyAction(Qt::Key_Less, this, "play_prev");
	connect( playPrevAct, SIGNAL(triggered()), playlist, SLOT(playPrev()) );


	// Move video window and zoom
	moveUpAct = new MyAction(Qt::ALT | Qt::Key_Up, this, "move_up");
	connect( moveUpAct, SIGNAL(triggered()), mplayerwindow, SLOT(moveUp()) );

	moveDownAct = new MyAction(Qt::ALT | Qt::Key_Down, this, "move_down");
	connect( moveDownAct, SIGNAL(triggered()), mplayerwindow, SLOT(moveDown()) );

	moveLeftAct = new MyAction(Qt::ALT | Qt::Key_Left, this, "move_left");
	connect( moveLeftAct, SIGNAL(triggered()), mplayerwindow, SLOT(moveLeft()) );

	moveRightAct = new MyAction(Qt::ALT | Qt::Key_Right, this, "move_right");
	connect( moveRightAct, SIGNAL(triggered()), mplayerwindow, SLOT(moveRight()) );

	incZoomAct = new MyAction(Qt::Key_E, this, "inc_zoom");
	connect( incZoomAct, SIGNAL(triggered()), core, SLOT(incZoom()) );

	decZoomAct = new MyAction(Qt::Key_W, this, "dec_zoom");
	connect( decZoomAct, SIGNAL(triggered()), core, SLOT(decZoom()) );

	resetZoomAct = new MyAction(Qt::SHIFT | Qt::Key_E, this, "reset_zoom");
	connect( resetZoomAct, SIGNAL(triggered()), core, SLOT(resetZoom()) );

	autoZoomAct = new MyAction(Qt::SHIFT | Qt::Key_W, this, "auto_zoom");
	connect( autoZoomAct, SIGNAL(triggered()), core, SLOT(autoZoom()) );

	autoZoom169Act = new MyAction(Qt::SHIFT | Qt::Key_A, this, "zoom_169");
	connect( autoZoom169Act, SIGNAL(triggered()), core, SLOT(autoZoomFor169()) );

	autoZoom235Act = new MyAction(Qt::SHIFT | Qt::Key_S, this, "zoom_235");
	connect( autoZoom235Act, SIGNAL(triggered()), core, SLOT(autoZoomFor235()) );

#if USE_MPLAYER_PANSCAN
	incPanscanAct = new MyAction(Qt::SHIFT | Qt::Key_M, this, "inc_panscan");
	connect( incPanscanAct, SIGNAL(triggered()), core, SLOT(incPanscan()) );

	decPanscanAct = new MyAction(Qt::SHIFT | Qt::Key_N, this, "dec_panscan");
	connect( decPanscanAct, SIGNAL(triggered()), core, SLOT(decPanscan()) );
#endif


	// Actions not in menus or buttons
	// Volume 2
#if !USE_MULTIPLE_SHORTCUTS
	decVolume2Act = new MyAction( Qt::Key_Slash, this, "dec_volume2" );
	connect( decVolume2Act, SIGNAL(triggered()), core, SLOT(decVolume()) );

	incVolume2Act = new MyAction( Qt::Key_Asterisk, this, "inc_volume2" );
	connect( incVolume2Act, SIGNAL(triggered()), core, SLOT(incVolume()) );
#endif
	// Exit fullscreen
	exitFullscreenAct = new MyAction( Qt::Key_Escape, this, "exit_fullscreen" );
	connect( exitFullscreenAct, SIGNAL(triggered()), this, SLOT(exitFullscreen()) );

	nextOSDAct = new MyAction( Qt::Key_O, this, "next_osd");
	connect( nextOSDAct, SIGNAL(triggered()), core, SLOT(nextOSD()) );

	decContrastAct = new MyAction( Qt::Key_1, this, "dec_contrast");
	connect( decContrastAct, SIGNAL(triggered()), core, SLOT(decContrast()) );

	incContrastAct = new MyAction( Qt::Key_2, this, "inc_contrast");
	connect( incContrastAct, SIGNAL(triggered()), core, SLOT(incContrast()) );

	decBrightnessAct = new MyAction( Qt::Key_3, this, "dec_brightness");
	connect( decBrightnessAct, SIGNAL(triggered()), core, SLOT(decBrightness()) );

	incBrightnessAct = new MyAction( Qt::Key_4, this, "inc_brightness");
	connect( incBrightnessAct, SIGNAL(triggered()), core, SLOT(incBrightness()) );

	decHueAct = new MyAction(Qt::Key_5, this, "dec_hue");
	connect( decHueAct, SIGNAL(triggered()), core, SLOT(decHue()) );

	incHueAct = new MyAction( Qt::Key_6, this, "inc_hue");
	connect( incHueAct, SIGNAL(triggered()), core, SLOT(incHue()) );

	decSaturationAct = new MyAction( Qt::Key_7, this, "dec_saturation");
	connect( decSaturationAct, SIGNAL(triggered()), core, SLOT(decSaturation()) );

	incSaturationAct = new MyAction( Qt::Key_8, this, "inc_saturation");
	connect( incSaturationAct, SIGNAL(triggered()), core, SLOT(incSaturation()) );

	decGammaAct = new MyAction( this, "dec_gamma");
	connect( decGammaAct, SIGNAL(triggered()), core, SLOT(decGamma()) );

	incGammaAct = new MyAction( this, "inc_gamma");
	connect( incGammaAct, SIGNAL(triggered()), core, SLOT(incGamma()) );

	nextVideoAct = new MyAction( this, "next_video");
	connect( nextVideoAct, SIGNAL(triggered()), core, SLOT(nextVideo()) );

	nextAudioAct = new MyAction( Qt::Key_K, this, "next_audio");
	connect( nextAudioAct, SIGNAL(triggered()), core, SLOT(nextAudio()) );

	nextSubtitleAct = new MyAction( Qt::Key_J, this, "next_subtitle");
	connect( nextSubtitleAct, SIGNAL(triggered()), core, SLOT(nextSubtitle()) );

	nextChapterAct = new MyAction( Qt::Key_At, this, "next_chapter");
	connect( nextChapterAct, SIGNAL(triggered()), core, SLOT(nextChapter()) );

	prevChapterAct = new MyAction( Qt::Key_Exclam, this, "prev_chapter");
	connect( prevChapterAct, SIGNAL(triggered()), core, SLOT(prevChapter()) );

	doubleSizeAct = new MyAction( Qt::CTRL | Qt::Key_D, this, "toggle_double_size");
	connect( doubleSizeAct, SIGNAL(triggered()), core, SLOT(toggleDoubleSize()) );

	resetVideoEqualizerAct = new MyAction( this, "reset_video_equalizer");
	connect( resetVideoEqualizerAct, SIGNAL(triggered()), video_equalizer, SLOT(reset()) );

	resetAudioEqualizerAct = new MyAction( this, "reset_audio_equalizer");
	connect( resetAudioEqualizerAct, SIGNAL(triggered()), audio_equalizer, SLOT(reset()) );

	showContextMenuAct = new MyAction( this, "show_context_menu");
	connect( showContextMenuAct, SIGNAL(triggered()), 
             this, SLOT(showPopupMenu()) );

	nextAspectAct = new MyAction( Qt::Key_A, this, "next_aspect");
	connect( nextAspectAct, SIGNAL(triggered()), 
             core, SLOT(nextAspectRatio()) );

	nextWheelFunctionAct = new MyAction(this, "next_wheel_function");
	connect( nextWheelFunctionAct, SIGNAL(triggered()),
			 core, SLOT(nextWheelFunction()) );

	showFilenameAct = new MyAction(Qt::SHIFT | Qt::Key_I, this, "show_filename");
	connect( showFilenameAct, SIGNAL(triggered()), core, SLOT(showFilenameOnOSD()) );

	toggleDeinterlaceAct = new MyAction(Qt::Key_D, this, "toggle_deinterlacing");
	connect( toggleDeinterlaceAct, SIGNAL(triggered()), core, SLOT(toggleDeinterlace()) );


	// Group actions

	// OSD
	osdGroup = new MyActionGroup(this);
	osdNoneAct = new MyActionGroupItem(this, osdGroup, "osd_none", Preferences::None);
	osdSeekAct = new MyActionGroupItem(this, osdGroup, "osd_seek", Preferences::Seek);
	osdTimerAct = new MyActionGroupItem(this, osdGroup, "osd_timer", Preferences::SeekTimer);
	osdTotalAct = new MyActionGroupItem(this, osdGroup, "osd_total", Preferences::SeekTimerTotal);
	connect( osdGroup, SIGNAL(activated(int)), core, SLOT(changeOSD(int)) );

	// Denoise
	denoiseGroup = new MyActionGroup(this);
	denoiseNoneAct = new MyActionGroupItem(this, denoiseGroup, "denoise_none", MediaSettings::NoDenoise);
	denoiseNormalAct = new MyActionGroupItem(this, denoiseGroup, "denoise_normal", MediaSettings::DenoiseNormal);
	denoiseSoftAct = new MyActionGroupItem(this, denoiseGroup, "denoise_soft", MediaSettings::DenoiseSoft);
	connect( denoiseGroup, SIGNAL(activated(int)), core, SLOT(changeDenoise(int)) );

	// Unsharp group
	unsharpGroup = new MyActionGroup(this);
	unsharpNoneAct = new MyActionGroupItem(this, unsharpGroup, "unsharp_off", 0);
	blurAct = new MyActionGroupItem(this, unsharpGroup, "blur", 1);
	sharpenAct = new MyActionGroupItem(this, unsharpGroup, "sharpen", 2);
	connect( unsharpGroup, SIGNAL(activated(int)), core, SLOT(changeUnsharp(int)) );

	// Video size
	sizeGroup = new MyActionGroup(this);
	size50 = new MyActionGroupItem(this, sizeGroup, "5&0%", "size_50", 50);
	size75 = new MyActionGroupItem(this, sizeGroup, "7&5%", "size_75", 75);
	size100 = new MyActionGroupItem(this, sizeGroup, "&100%", "size_100", 100);
	size125 = new MyActionGroupItem(this, sizeGroup, "1&25%", "size_125", 125);
	size150 = new MyActionGroupItem(this, sizeGroup, "15&0%", "size_150", 150);
	size175 = new MyActionGroupItem(this, sizeGroup, "1&75%", "size_175", 175);
	size200 = new MyActionGroupItem(this, sizeGroup, "&200%", "size_200", 200);
	size300 = new MyActionGroupItem(this, sizeGroup, "&300%", "size_300", 300);
	size400 = new MyActionGroupItem(this, sizeGroup, "&400%", "size_400", 400);
	size100->setShortcut( Qt::CTRL | Qt::Key_1 );
	size200->setShortcut( Qt::CTRL | Qt::Key_2 );
	connect( sizeGroup, SIGNAL(activated(int)), core, SLOT(changeSize(int)) );
	// Make all not checkable
	QList <QAction *> size_list = sizeGroup->actions();
	for (int n=0; n < size_list.count(); n++) {
		size_list[n]->setCheckable(false);
	}

	// Deinterlace
	deinterlaceGroup = new MyActionGroup(this);
	deinterlaceNoneAct = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_none", MediaSettings::NoDeinterlace);
	deinterlaceL5Act = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_l5", MediaSettings::L5);
	deinterlaceYadif0Act = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_yadif0", MediaSettings::Yadif);
	deinterlaceYadif1Act = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_yadif1", MediaSettings::Yadif_1);
	deinterlaceLBAct = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_lb", MediaSettings::LB);
	deinterlaceKernAct = new MyActionGroupItem(this, deinterlaceGroup, "deinterlace_kern", MediaSettings::Kerndeint);
	connect( deinterlaceGroup, SIGNAL(activated(int)),
             core, SLOT(changeDeinterlace(int)) );

	// Audio channels
	channelsGroup = new MyActionGroup(this);
	/* channelsDefaultAct = new MyActionGroupItem(this, channelsGroup, "channels_default", MediaSettings::ChDefault); */
	channelsStereoAct = new MyActionGroupItem(this, channelsGroup, "channels_stereo", MediaSettings::ChStereo);
	channelsSurroundAct = new MyActionGroupItem(this, channelsGroup, "channels_surround", MediaSettings::ChSurround);
	channelsFull51Act = new MyActionGroupItem(this, channelsGroup, "channels_ful51", MediaSettings::ChFull51);
	connect( channelsGroup, SIGNAL(activated(int)),
             core, SLOT(setAudioChannels(int)) );

	// Stereo mode
	stereoGroup = new MyActionGroup(this);
	stereoAct = new MyActionGroupItem(this, stereoGroup, "stereo", MediaSettings::Stereo);
	leftChannelAct = new MyActionGroupItem(this, stereoGroup, "left_channel", MediaSettings::Left);
	rightChannelAct = new MyActionGroupItem(this, stereoGroup, "right_channel", MediaSettings::Right);
	connect( stereoGroup, SIGNAL(activated(int)),
             core, SLOT(setStereoMode(int)) );

	// Video aspect
	aspectGroup = new MyActionGroup(this);
	aspectDetectAct = new MyActionGroupItem(this, aspectGroup, "aspect_detect", MediaSettings::AspectAuto);
	aspect11Act = new MyActionGroupItem(this, aspectGroup, "aspect_1:1", MediaSettings::Aspect11 );
	aspect32Act = new MyActionGroupItem(this, aspectGroup, "aspect_3:2", MediaSettings::Aspect32);
	aspect43Act = new MyActionGroupItem(this, aspectGroup, "aspect_4:3", MediaSettings::Aspect43);
	aspect54Act = new MyActionGroupItem(this, aspectGroup, "aspect_5:4", MediaSettings::Aspect54 );
	aspect149Act = new MyActionGroupItem(this, aspectGroup, "aspect_14:9", MediaSettings::Aspect149 );
	aspect1410Act = new MyActionGroupItem(this, aspectGroup, "aspect_14:10", MediaSettings::Aspect1410 );
	aspect169Act = new MyActionGroupItem(this, aspectGroup, "aspect_16:9", MediaSettings::Aspect169 );
	aspect1610Act = new MyActionGroupItem(this, aspectGroup, "aspect_16:10", MediaSettings::Aspect1610 );
	aspect235Act = new MyActionGroupItem(this, aspectGroup, "aspect_2.35:1", MediaSettings::Aspect235 );
	{
		QAction * sep = new QAction(aspectGroup);
		sep->setSeparator(true);
	}
	aspectNoneAct = new MyActionGroupItem(this, aspectGroup, "aspect_none", MediaSettings::AspectNone);

	connect( aspectGroup, SIGNAL(activated(int)),
             core, SLOT(changeAspectRatio(int)) );

	// Rotate
	rotateGroup = new MyActionGroup(this);
	rotateNoneAct = new MyActionGroupItem(this, rotateGroup, "rotate_none", MediaSettings::NoRotate);
	rotateClockwiseFlipAct = new MyActionGroupItem(this, rotateGroup, "rotate_clockwise_flip", MediaSettings::Clockwise_flip);
	rotateClockwiseAct = new MyActionGroupItem(this, rotateGroup, "rotate_clockwise", MediaSettings::Clockwise);
	rotateCounterclockwiseAct = new MyActionGroupItem(this, rotateGroup, "rotate_counterclockwise", MediaSettings::Counterclockwise);
	rotateCounterclockwiseFlipAct = new MyActionGroupItem(this, rotateGroup, "rotate_counterclockwise_flip", MediaSettings::Counterclockwise_flip);
	connect( rotateGroup, SIGNAL(activated(int)),
             core, SLOT(changeRotate(int)) );

	// On Top
	onTopActionGroup = new MyActionGroup(this);
	onTopAlwaysAct = new MyActionGroupItem( this,onTopActionGroup,"on_top_always",Preferences::AlwaysOnTop);
	onTopNeverAct = new MyActionGroupItem( this,onTopActionGroup,"on_top_never",Preferences::NeverOnTop);
	onTopWhilePlayingAct = new MyActionGroupItem( this,onTopActionGroup,"on_top_playing",Preferences::WhilePlayingOnTop);
	connect( onTopActionGroup , SIGNAL(activated(int)),
             this, SLOT(changeStayOnTop(int)) );

	toggleStayOnTopAct = new MyAction( this, "toggle_stay_on_top");
	connect( toggleStayOnTopAct, SIGNAL(triggered()), this, SLOT(toggleStayOnTop()) );


#if USE_ADAPTER
	screenGroup = new MyActionGroup(this);
	screenDefaultAct = new MyActionGroupItem(this, screenGroup, "screen_default", -1);
#ifdef Q_OS_WIN
	DeviceList display_devices = DeviceInfo::displayDevices();
	if (!display_devices.isEmpty()) {
		for (int n = 0; n < display_devices.count(); n++) {
			int id = display_devices[n].ID().toInt();
			QString desc = display_devices[n].desc();
			MyAction * screen_item = new MyActionGroupItem(this, screenGroup, QString("screen_%1").arg(n).toAscii().constData(), id);
			screen_item->change( "&"+QString::number(n) + " - " + desc);
		}
	}
	else
#endif // Q_OS_WIN
	for (int n = 1; n <= 4; n++) {
		MyAction * screen_item = new MyActionGroupItem(this, screenGroup, QString("screen_%1").arg(n).toAscii().constData(), n);
		screen_item->change( "&"+QString::number(n) );
	}

	connect( screenGroup, SIGNAL(activated(int)),
             core, SLOT(changeAdapter(int)) );
#endif

#if PROGRAM_SWITCH
	// Program track
	programTrackGroup = new MyActionGroup(this);
	connect( programTrackGroup, SIGNAL(activated(int)), 
	         core, SLOT(changeProgram(int)) );
#endif

	// Video track
	videoTrackGroup = new MyActionGroup(this);
	connect( videoTrackGroup, SIGNAL(activated(int)), 
	         core, SLOT(changeVideo(int)) );

	// Audio track
	audioTrackGroup = new MyActionGroup(this);
	connect( audioTrackGroup, SIGNAL(activated(int)), 
	         core, SLOT(changeAudio(int)) );

	// Subtitle track
	subtitleTrackGroup = new MyActionGroup(this);
    connect( subtitleTrackGroup, SIGNAL(activated(int)), 
	         core, SLOT(changeSubtitle(int)) );

	ccGroup = new MyActionGroup(this);
	ccNoneAct = new MyActionGroupItem(this, ccGroup, "cc_none", 0);
	ccChannel1Act = new MyActionGroupItem(this, ccGroup, "cc_ch_1", 1);
	ccChannel2Act = new MyActionGroupItem(this, ccGroup, "cc_ch_2", 2);
	ccChannel3Act = new MyActionGroupItem(this, ccGroup, "cc_ch_3", 3);
	ccChannel4Act = new MyActionGroupItem(this, ccGroup, "cc_ch_4", 4);
	connect( ccGroup, SIGNAL(activated(int)),
             core, SLOT(changeClosedCaptionChannel(int)) );

	// Titles
	titleGroup = new MyActionGroup(this);
	connect( titleGroup, SIGNAL(activated(int)),
			 core, SLOT(changeTitle(int)) );

	// Angles
	angleGroup = new MyActionGroup(this);
	connect( angleGroup, SIGNAL(activated(int)),
			 core, SLOT(changeAngle(int)) );

	// Chapters
	chapterGroup = new MyActionGroup(this);
	connect( chapterGroup, SIGNAL(activated(int)),
			 core, SLOT(changeChapter(int)) );

#if DVDNAV_SUPPORT
	dvdnavUpAct = new MyAction(Qt::SHIFT | Qt::Key_Up, this, "dvdnav_up");
	connect( dvdnavUpAct, SIGNAL(triggered()), core, SLOT(dvdnavUp()) );

	dvdnavDownAct = new MyAction(Qt::SHIFT | Qt::Key_Down, this, "dvdnav_down");
	connect( dvdnavDownAct, SIGNAL(triggered()), core, SLOT(dvdnavDown()) );

	dvdnavLeftAct = new MyAction(Qt::SHIFT | Qt::Key_Left, this, "dvdnav_left");
	connect( dvdnavLeftAct, SIGNAL(triggered()), core, SLOT(dvdnavLeft()) );

	dvdnavRightAct = new MyAction(Qt::SHIFT | Qt::Key_Right, this, "dvdnav_right");
	connect( dvdnavRightAct, SIGNAL(triggered()), core, SLOT(dvdnavRight()) );

	dvdnavMenuAct = new MyAction(Qt::SHIFT | Qt::Key_Return, this, "dvdnav_menu");
	connect( dvdnavMenuAct, SIGNAL(triggered()), core, SLOT(dvdnavMenu()) );

	dvdnavSelectAct = new MyAction(Qt::Key_Return, this, "dvdnav_select");
	connect( dvdnavSelectAct, SIGNAL(triggered()), core, SLOT(dvdnavSelect()) );

	dvdnavPrevAct = new MyAction(Qt::SHIFT | Qt::Key_Escape, this, "dvdnav_prev");
	connect( dvdnavPrevAct, SIGNAL(triggered()), core, SLOT(dvdnavPrev()) );

	dvdnavMouseAct = new MyAction( this, "dvdnav_mouse");
	connect( dvdnavMouseAct, SIGNAL(triggered()), core, SLOT(dvdnavMouse()) );
#endif

}

#if AUTODISABLE_ACTIONS
void BaseGui::setActionsEnabled(bool b) {
	// Menu Play
	playAct->setEnabled(b);
	playOrPauseAct->setEnabled(b);
	pauseAct->setEnabled(b);
	pauseAndStepAct->setEnabled(b);
	stopAct->setEnabled(b);
	frameStepAct->setEnabled(b);
	rewind1Act->setEnabled(b);
	rewind2Act->setEnabled(b);
	rewind3Act->setEnabled(b);
	forward1Act->setEnabled(b);
	forward2Act->setEnabled(b);
	forward3Act->setEnabled(b);
	//repeatAct->setEnabled(b);
	gotoAct->setEnabled(b);

	// Menu Speed
	normalSpeedAct->setEnabled(b);
	halveSpeedAct->setEnabled(b);
	doubleSpeedAct->setEnabled(b);
	decSpeed10Act->setEnabled(b);
	incSpeed10Act->setEnabled(b);
	decSpeed4Act->setEnabled(b);
	incSpeed4Act->setEnabled(b);
	decSpeed1Act->setEnabled(b);
	incSpeed1Act->setEnabled(b);

	// Menu Video
	videoEqualizerAct->setEnabled(b);
	screenshotAct->setEnabled(b);
	screenshotsAct->setEnabled(b);
	flipAct->setEnabled(b);
	mirrorAct->setEnabled(b);
	postProcessingAct->setEnabled(b);
	phaseAct->setEnabled(b);
	deblockAct->setEnabled(b);
	deringAct->setEnabled(b);
	gradfunAct->setEnabled(b);
	addNoiseAct->setEnabled(b);
	addLetterboxAct->setEnabled(b);
	upscaleAct->setEnabled(b);

	// Menu Audio
	audioEqualizerAct->setEnabled(b);
	muteAct->setEnabled(b);
	decVolumeAct->setEnabled(b);
	incVolumeAct->setEnabled(b);
	decAudioDelayAct->setEnabled(b);
	incAudioDelayAct->setEnabled(b);
	audioDelayAct->setEnabled(b);
	extrastereoAct->setEnabled(b);
	karaokeAct->setEnabled(b);
	volnormAct->setEnabled(b);
	loadAudioAct->setEnabled(b);
	//unloadAudioAct->setEnabled(b);

	// Menu Subtitles
	loadSubsAct->setEnabled(b);
	//unloadSubsAct->setEnabled(b);
	decSubDelayAct->setEnabled(b);
	incSubDelayAct->setEnabled(b);
	subDelayAct->setEnabled(b);
	decSubPosAct->setEnabled(b);
	incSubPosAct->setEnabled(b);
	incSubStepAct->setEnabled(b);
	decSubStepAct->setEnabled(b);
	incSubScaleAct->setEnabled(b);
	decSubScaleAct->setEnabled(b);

	// Actions not in menus
#if !USE_MULTIPLE_SHORTCUTS
	decVolume2Act->setEnabled(b);
	incVolume2Act->setEnabled(b);
#endif
	decContrastAct->setEnabled(b);
	incContrastAct->setEnabled(b);
	decBrightnessAct->setEnabled(b);
	incBrightnessAct->setEnabled(b);
	decHueAct->setEnabled(b);
	incHueAct->setEnabled(b);
	decSaturationAct->setEnabled(b);
	incSaturationAct->setEnabled(b);
	decGammaAct->setEnabled(b);
	incGammaAct->setEnabled(b);
	nextVideoAct->setEnabled(b);
	nextAudioAct->setEnabled(b);
	nextSubtitleAct->setEnabled(b);
	nextChapterAct->setEnabled(b);
	prevChapterAct->setEnabled(b);
	doubleSizeAct->setEnabled(b);

	// Moving and zoom
	moveUpAct->setEnabled(b);
	moveDownAct->setEnabled(b);
	moveLeftAct->setEnabled(b);
	moveRightAct->setEnabled(b);
	incZoomAct->setEnabled(b);
	decZoomAct->setEnabled(b);
	resetZoomAct->setEnabled(b);
	autoZoomAct->setEnabled(b);
	autoZoom169Act->setEnabled(b);
	autoZoom235Act->setEnabled(b);

#if DVDNAV_SUPPORT
	dvdnavUpAct->setEnabled(b);
	dvdnavDownAct->setEnabled(b);
	dvdnavLeftAct->setEnabled(b);
	dvdnavRightAct->setEnabled(b);
	dvdnavMenuAct->setEnabled(b);
	dvdnavSelectAct->setEnabled(b);
	dvdnavPrevAct->setEnabled(b);
	dvdnavMouseAct->setEnabled(b);
#endif

	// Groups
	denoiseGroup->setActionsEnabled(b);
	unsharpGroup->setActionsEnabled(b);
	sizeGroup->setActionsEnabled(b);
	deinterlaceGroup->setActionsEnabled(b);
	aspectGroup->setActionsEnabled(b);
	rotateGroup->setActionsEnabled(b);
#if USE_ADAPTER
	screenGroup->setActionsEnabled(b);
#endif
	channelsGroup->setActionsEnabled(b);
	stereoGroup->setActionsEnabled(b);
}

void BaseGui::enableActionsOnPlaying() {
	qDebug("BaseGui::enableActionsOnPlaying");

	setActionsEnabled(true);

	playAct->setEnabled(false);

	// Screenshot option
	bool screenshots_enabled = ( (pref->use_screenshot) && 
                                 (!pref->screenshot_directory.isEmpty()) &&
                                 (QFileInfo(pref->screenshot_directory).isDir()) );

	screenshotAct->setEnabled( screenshots_enabled );
	screenshotsAct->setEnabled( screenshots_enabled );

	// Disable the compact action if not using video window
	compactAct->setEnabled( panel->isVisible() );

	// Enable or disable the audio equalizer
	audioEqualizerAct->setEnabled(pref->use_audio_equalizer);

	// Disable audio actions if there's not audio track
	if ((core->mdat.audios.numItems()==0) && (core->mset.external_audio.isEmpty())) {
		audioEqualizerAct->setEnabled(false);
		muteAct->setEnabled(false);
		decVolumeAct->setEnabled(false);
		incVolumeAct->setEnabled(false);
		decAudioDelayAct->setEnabled(false);
		incAudioDelayAct->setEnabled(false);
		audioDelayAct->setEnabled(false);
		extrastereoAct->setEnabled(false);
		karaokeAct->setEnabled(false);
		volnormAct->setEnabled(false);
		channelsGroup->setActionsEnabled(false);
		stereoGroup->setActionsEnabled(false);
	}

	// Disable video actions if it's an audio file
	if (core->mdat.novideo) {
		videoEqualizerAct->setEnabled(false);
		screenshotAct->setEnabled(false);
		screenshotsAct->setEnabled(false);
		flipAct->setEnabled(false);
		mirrorAct->setEnabled(false);
		postProcessingAct->setEnabled(false);
		phaseAct->setEnabled(false);
		deblockAct->setEnabled(false);
		deringAct->setEnabled(false);
		gradfunAct->setEnabled(false);
		addNoiseAct->setEnabled(false);
		addLetterboxAct->setEnabled(false);
		upscaleAct->setEnabled(false);
		doubleSizeAct->setEnabled(false);

		// Moving and zoom
		moveUpAct->setEnabled(false);
		moveDownAct->setEnabled(false);
		moveLeftAct->setEnabled(false);
		moveRightAct->setEnabled(false);
		incZoomAct->setEnabled(false);
		decZoomAct->setEnabled(false);
		resetZoomAct->setEnabled(false);
		autoZoomAct->setEnabled(false);
		autoZoom169Act->setEnabled(false);
		autoZoom235Act->setEnabled(false);

		denoiseGroup->setActionsEnabled(false);
		unsharpGroup->setActionsEnabled(false);
		sizeGroup->setActionsEnabled(false);
		deinterlaceGroup->setActionsEnabled(false);
		aspectGroup->setActionsEnabled(false);
		rotateGroup->setActionsEnabled(false);
#if USE_ADAPTER
		screenGroup->setActionsEnabled(false);
#endif
	}

#if USE_ADAPTER
	screenGroup->setActionsEnabled(pref->vo.startsWith(OVERLAY_VO));
#endif

#ifndef Q_OS_WIN
	// Disable video filters if using vdpau
	if ((pref->vdpau.disable_video_filters) && (pref->vo.startsWith("vdpau"))) {
		screenshotAct->setEnabled(false);
		screenshotsAct->setEnabled(false);
		flipAct->setEnabled(false);
		mirrorAct->setEnabled(false);
		postProcessingAct->setEnabled(false);
		phaseAct->setEnabled(false);
		deblockAct->setEnabled(false);
		deringAct->setEnabled(false);
		gradfunAct->setEnabled(false);
		addNoiseAct->setEnabled(false);
		addLetterboxAct->setEnabled(false);
		upscaleAct->setEnabled(false);

		deinterlaceGroup->setActionsEnabled(false);
		rotateGroup->setActionsEnabled(false);
		denoiseGroup->setActionsEnabled(false);
		unsharpGroup->setActionsEnabled(false);

		displayMessage( tr("Video filters are disabled when using vdpau") );
	}
#endif

#if DVDNAV_SUPPORT
	if (!core->mdat.filename.startsWith("dvdnav:")) {
		dvdnavUpAct->setEnabled(false);
		dvdnavDownAct->setEnabled(false);
		dvdnavLeftAct->setEnabled(false);
		dvdnavRightAct->setEnabled(false);
		dvdnavMenuAct->setEnabled(false);
		dvdnavSelectAct->setEnabled(false);
		dvdnavPrevAct->setEnabled(false);
		dvdnavMouseAct->setEnabled(false);
	}
#endif
}

void BaseGui::disableActionsOnStop() {
	qDebug("BaseGui::disableActionsOnStop");

	setActionsEnabled(false);

	playAct->setEnabled(true);
	playOrPauseAct->setEnabled(true);
	stopAct->setEnabled(true);
}

void BaseGui::togglePlayAction(Core::State state) {
	qDebug("BaseGui::togglePlayAction");
	if (state == Core::Playing)
		playAct->setEnabled(false);
	else
		playAct->setEnabled(true);
}
#endif // AUTODISABLE_ACTIONS

void BaseGui::retranslateStrings() {
	setWindowIcon( Images::icon("logo", 64) );

	// ACTIONS

	// Menu File
	openFileAct->change( Images::icon("open"), tr("&File...") );
	openDirectoryAct->change( Images::icon("openfolder"), tr("D&irectory...") );
	openPlaylistAct->change( Images::icon("open_playlist"), tr("&Playlist...") );
	openVCDAct->change( Images::icon("vcd"), tr("V&CD") );
	openAudioCDAct->change( Images::icon("cdda"), tr("&Audio CD") );
	openDVDAct->change( Images::icon("dvd"), tr("&DVD from drive") );
	openDVDFolderAct->change( Images::icon("dvd_hd"), tr("D&VD from folder...") );
	openURLAct->change( Images::icon("url"), tr("&URL...") );
	exitAct->change( Images::icon("close"), tr("C&lose") );

	// Favorites
	/*
	favorites->editAct()->setText( tr("&Edit...") );
	favorites->addCurrentAct()->setText( tr("&Add current media") );
	*/

	// TV & Radio submenus
	/*
	tvlist->editAct()->setText( tr("&Edit...") );
	radiolist->editAct()->setText( tr("&Edit...") );
	tvlist->addCurrentAct()->setText( tr("&Add current media") );
	radiolist->addCurrentAct()->setText( tr("&Add current media") );
	tvlist->jumpAct()->setText( tr("&Jump...") );
	radiolist->jumpAct()->setText( tr("&Jump...") );
	tvlist->nextAct()->setText( tr("Next TV channel") );
	tvlist->previousAct()->setText( tr("Previous TV channel") );
	radiolist->nextAct()->setText( tr("Next radio channel") );
	radiolist->previousAct()->setText( tr("Previous radio channel") );
	*/

	// Menu Play
	playAct->change( tr("P&lay") );
	if (qApp->isLeftToRight()) 
		playAct->setIcon( Images::icon("play") );
	else
		playAct->setIcon( Images::flippedIcon("play") );

	pauseAct->change( Images::icon("pause"), tr("&Pause"));
	stopAct->change( Images::icon("stop"), tr("&Stop") );
	frameStepAct->change( Images::icon("frame_step"), tr("&Frame step") );

	playOrPauseAct->change( tr("Play / Pause") );
	if (qApp->isLeftToRight()) 
		playOrPauseAct->setIcon( Images::icon("play_pause") );
	else
		playOrPauseAct->setIcon( Images::flippedIcon("play_pause") );

	pauseAndStepAct->change( Images::icon("pause"), tr("Pause / Frame step") );

	setJumpTexts(); // Texts for rewind*Act and forward*Act

	// Submenu A-B
	setAMarkerAct->change( Images::icon("a_marker"), tr("Set &A marker") );
	setBMarkerAct->change( Images::icon("b_marker"), tr("Set &B marker") );
	clearABMarkersAct->change( Images::icon("clear_markers"), tr("&Clear A-B markers") );
	repeatAct->change( Images::icon("repeat"), tr("&Repeat") );

	gotoAct->change( Images::icon("jumpto"), tr("&Jump to...") );

	// Submenu speed
	normalSpeedAct->change( tr("&Normal speed") );
	halveSpeedAct->change( tr("&Halve speed") );
	doubleSpeedAct->change( tr("&Double speed") );
	decSpeed10Act->change( tr("Speed &-10%") );
	incSpeed10Act->change( tr("Speed &+10%") );
	decSpeed4Act->change( tr("Speed -&4%") );
	incSpeed4Act->change( tr("&Speed +4%") );
	decSpeed1Act->change( tr("Speed -&1%") );
	incSpeed1Act->change( tr("S&peed +1%") );

	// Menu Video
	fullscreenAct->change( Images::icon("fullscreen"), tr("&Fullscreen") );
	compactAct->change( Images::icon("compact"), tr("&Compact mode") );
	videoEqualizerAct->change( Images::icon("equalizer"), tr("&Equalizer") );
	screenshotAct->change( Images::icon("screenshot"), tr("&Screenshot") );
	screenshotsAct->change( Images::icon("screenshots"), tr("Start/stop takin&g screenshots") );
#ifdef VIDEOPREVIEW
	videoPreviewAct->change( Images::icon("video_preview"), tr("Pre&view...") );
#endif
	flipAct->change( Images::icon("flip"), tr("Fli&p image") );
	mirrorAct->change( Images::icon("mirror"), tr("Mirr&or image") );

	decZoomAct->change( tr("Zoom &-") );
	incZoomAct->change( tr("Zoom &+") );
	resetZoomAct->change( tr("&Reset") );
	autoZoomAct->change( tr("&Auto zoom") );
	autoZoom169Act->change( tr("Zoom for &16:9") );
	autoZoom235Act->change( tr("Zoom for &2.35:1") );
	moveLeftAct->change( tr("Move &left") );
	moveRightAct->change( tr("Move &right") );
	moveUpAct->change( tr("Move &up") );
	moveDownAct->change( tr("Move &down") );

#if USE_MPLAYER_PANSCAN
	decPanscanAct->change( "Panscan -" );
	incPanscanAct->change( "Panscan +" );
#endif

	// Submenu Filters
	postProcessingAct->change( tr("&Postprocessing") );
	phaseAct->change( tr("&Autodetect phase") );
	deblockAct->change( tr("&Deblock") );
	deringAct->change( tr("De&ring") );
	gradfunAct->change( tr("Debanding (&gradfun)") );
	addNoiseAct->change( tr("Add n&oise") );
	addLetterboxAct->change( Images::icon("letterbox"), tr("Add &black borders") );
	upscaleAct->change( Images::icon("upscaling"), tr("Soft&ware scaling") );

	// Menu Audio
	audioEqualizerAct->change( Images::icon("audio_equalizer"), tr("E&qualizer") );
	QIcon icset( Images::icon("volume") );
	icset.addPixmap( Images::icon("mute"), QIcon::Normal, QIcon::On  );
	muteAct->change( icset, tr("&Mute") );
	decVolumeAct->change( Images::icon("audio_down"), tr("Volume &-") );
	incVolumeAct->change( Images::icon("audio_up"), tr("Volume &+") );
	decAudioDelayAct->change( Images::icon("delay_down"), tr("&Delay -") );
	incAudioDelayAct->change( Images::icon("delay_up"), tr("D&elay +") );
	audioDelayAct->change( Images::icon("audio_delay"), tr("Set dela&y...") );
	loadAudioAct->change( Images::icon("open"), tr("&Load external file...") );
	unloadAudioAct->change( Images::icon("unload"), tr("U&nload") );

	// Submenu Filters
	extrastereoAct->change( tr("&Extrastereo") );
	karaokeAct->change( tr("&Karaoke") );
	volnormAct->change( tr("Volume &normalization") );

	// Menu Subtitles
	loadSubsAct->change( Images::icon("open"), tr("&Load...") );
	unloadSubsAct->change( Images::icon("unload"), tr("U&nload") );
	decSubDelayAct->change( Images::icon("delay_down"), tr("Delay &-") );
	incSubDelayAct->change( Images::icon("delay_up"), tr("Delay &+") );
	subDelayAct->change( Images::icon("sub_delay"), tr("Se&t delay...") );
	decSubPosAct->change( Images::icon("sub_up"), tr("&Up") );
	incSubPosAct->change( Images::icon("sub_down"), tr("&Down") );
	decSubScaleAct->change( Images::icon("dec_sub_scale"), tr("S&ize -") );
	incSubScaleAct->change( Images::icon("inc_sub_scale"), tr("Si&ze +") );
	decSubStepAct->change( Images::icon("dec_sub_step"), 
                           tr("&Previous line in subtitles") );
	incSubStepAct->change( Images::icon("inc_sub_step"), 
                           tr("N&ext line in subtitles") );
	useAssAct->change( Images::icon("use_ass_lib"), tr("Use SSA/&ASS library") );
	useForcedSubsOnlyAct->change( Images::icon("forced_subs"), tr("&Forced subtitles only") );

	subVisibilityAct->change( Images::icon("sub_visibility"), tr("Subtitle &visibility") );

#ifdef FIND_SUBTITLES
	showFindSubtitlesDialogAct->change( Images::icon("download_subs"), tr("Find subtitles on &OpenSubtitles.org...") );
	openUploadSubtitlesPageAct->change( Images::icon("upload_subs"), tr("Upload su&btitles to OpenSubtitles.org...") );
#endif

	ccNoneAct->change( tr("&Off", "closed captions menu") );
	ccChannel1Act->change( "&1" );
	ccChannel2Act->change( "&2" );
	ccChannel3Act->change( "&3" );
	ccChannel4Act->change( "&4" );

	// Menu Options
	showPlaylistAct->change( Images::icon("playlist"), tr("&Playlist") );
	showPropertiesAct->change( Images::icon("info"), tr("View &info and properties...") );
	showPreferencesAct->change( Images::icon("prefs"), tr("P&references") );
	showTubeBrowserAct->change( Images::icon("tubebrowser"), tr("&YouTube browser") );

	// Submenu Logs
#ifdef LOG_MPLAYER
	showLogMplayerAct->change( "MPlayer" );
#endif
#ifdef LOG_SMPLAYER
	showLogSmplayerAct->change( "SMPlayer" );
#endif

	// Menu Help
	showFAQAct->change( Images::icon("faq"), tr("&FAQ") );
	showCLOptionsAct->change( Images::icon("cl_help"), tr("&Command line options") );
	showCheckUpdatesAct->change( Images::icon("check_updates"), tr("Check for &updates") );
	donateAct->change( Images::icon("donate"), tr("&Donate") );
	aboutQtAct->change( QPixmap(":/icons-png/qt.png"), tr("About &Qt") );
	aboutThisAct->change( Images::icon("logo_small"), tr("About &SMPlayer") );

	// Playlist
	playNextAct->change( tr("&Next") );
	playPrevAct->change( tr("Pre&vious") );

	if (qApp->isLeftToRight()) {
		playNextAct->setIcon( Images::icon("next") );
		playPrevAct->setIcon( Images::icon("previous") );
	} else {
		playNextAct->setIcon( Images::flippedIcon("next") );
		playPrevAct->setIcon( Images::flippedIcon("previous") );
	}


	// Actions not in menus or buttons
	// Volume 2
#if !USE_MULTIPLE_SHORTCUTS
	decVolume2Act->change( tr("Dec volume (2)") );
	incVolume2Act->change( tr("Inc volume (2)") );
#endif
	// Exit fullscreen
	exitFullscreenAct->change( tr("Exit fullscreen") );

	nextOSDAct->change( tr("OSD - Next level") );
	decContrastAct->change( tr("Dec contrast") );
	incContrastAct->change( tr("Inc contrast") );
	decBrightnessAct->change( tr("Dec brightness") );
	incBrightnessAct->change( tr("Inc brightness") );
	decHueAct->change( tr("Dec hue") );
	incHueAct->change( tr("Inc hue") );
	decSaturationAct->change( tr("Dec saturation") );
	incSaturationAct->change( tr("Inc saturation") );
	decGammaAct->change( tr("Dec gamma") );
	incGammaAct->change( tr("Inc gamma") );
	nextVideoAct->change( tr("Next video") );
	nextAudioAct->change( tr("Next audio") );
	nextSubtitleAct->change( tr("Next subtitle") );
	nextChapterAct->change( tr("Next chapter") );
	prevChapterAct->change( tr("Previous chapter") );
	doubleSizeAct->change( tr("&Toggle double size") );
	resetVideoEqualizerAct->change( tr("Reset video equalizer") );
	resetAudioEqualizerAct->change( tr("Reset audio equalizer") );
	showContextMenuAct->change( tr("Show context menu") );
	nextAspectAct->change( Images::icon("next_aspect"), tr("Next aspect ratio") );
	nextWheelFunctionAct->change( Images::icon("next_wheel_function"), tr("Next wheel function") );

	showFilenameAct->change( tr("Show filename on OSD") );
	toggleDeinterlaceAct->change( tr("Toggle deinterlacing") );


	// Action groups
	osdNoneAct->change( tr("Subtitles onl&y") );
	osdSeekAct->change( tr("Volume + &Seek") );
	osdTimerAct->change( tr("Volume + Seek + &Timer") );
	osdTotalAct->change( tr("Volume + Seek + Timer + T&otal time") );


	// MENUS
	openMenu->menuAction()->setText( tr("&Open") );
	playMenu->menuAction()->setText( tr("&Play") );
	videoMenu->menuAction()->setText( tr("&Video") );
	audioMenu->menuAction()->setText( tr("&Audio") );
	subtitlesMenu->menuAction()->setText( tr("&Subtitles") );
	browseMenu->menuAction()->setText( tr("&Browse") );
	optionsMenu->menuAction()->setText( tr("Op&tions") );
	helpMenu->menuAction()->setText( tr("&Help") );

	/*
	openMenuAct->setIcon( Images::icon("open_menu") );
	playMenuAct->setIcon( Images::icon("play_menu") );
	videoMenuAct->setIcon( Images::icon("video_menu") );
	audioMenuAct->setIcon( Images::icon("audio_menu") );
	subtitlesMenuAct->setIcon( Images::icon("subtitles_menu") );
	browseMenuAct->setIcon( Images::icon("browse_menu") );
	optionsMenuAct->setIcon( Images::icon("options_menu") );
	helpMenuAct->setIcon( Images::icon("help_menu") );
	*/

	// Menu Open
	recentfiles_menu->menuAction()->setText( tr("&Recent files") );
	recentfiles_menu->menuAction()->setIcon( Images::icon("recents") );
	clearRecentsAct->change( Images::icon("delete"), tr("&Clear") );

	disc_menu->menuAction()->setText( tr("&Disc") );
	disc_menu->menuAction()->setIcon( Images::icon("open_disc") );

	/* favorites->menuAction()->setText( tr("&Favorites") ); */
	favorites->menuAction()->setText( tr("F&avorites") );
	favorites->menuAction()->setIcon( Images::icon("open_favorites") ); 

	tvlist->menuAction()->setText( tr("&TV") );
	tvlist->menuAction()->setIcon( Images::icon("open_tv") );

	radiolist->menuAction()->setText( tr("Radi&o") );
	radiolist->menuAction()->setIcon( Images::icon("open_radio") );

	// Menu Play
	speed_menu->menuAction()->setText( tr("Sp&eed") );
	speed_menu->menuAction()->setIcon( Images::icon("speed") );

	ab_menu->menuAction()->setText( tr("&A-B section") );
	ab_menu->menuAction()->setIcon( Images::icon("ab_menu") );

	// Menu Video
	videotrack_menu->menuAction()->setText( tr("&Track", "video") );
	videotrack_menu->menuAction()->setIcon( Images::icon("video_track") );

	videosize_menu->menuAction()->setText( tr("Si&ze") );
	videosize_menu->menuAction()->setIcon( Images::icon("video_size") );

	/*
	panscan_menu->menuAction()->setText( tr("&Pan && scan") );
	panscan_menu->menuAction()->setIcon( Images::icon("panscan") );
	*/
	zoom_menu->menuAction()->setText( tr("Zoo&m") );
	zoom_menu->menuAction()->setIcon( Images::icon("zoom") );

	aspect_menu->menuAction()->setText( tr("&Aspect ratio") );
	aspect_menu->menuAction()->setIcon( Images::icon("aspect") );

	deinterlace_menu->menuAction()->setText( tr("&Deinterlace") );
	deinterlace_menu->menuAction()->setIcon( Images::icon("deinterlace") );

	videofilter_menu->menuAction()->setText( tr("F&ilters") );
	videofilter_menu->menuAction()->setIcon( Images::icon("video_filters") );

	rotate_menu->menuAction()->setText( tr("&Rotate") );
	rotate_menu->menuAction()->setIcon( Images::icon("rotate") );

	ontop_menu->menuAction()->setText( tr("S&tay on top") );
	ontop_menu->menuAction()->setIcon( Images::icon("ontop") );

#if USE_ADAPTER
	screen_menu->menuAction()->setText( tr("Scree&n") );
	screen_menu->menuAction()->setIcon( Images::icon("screen") );
#endif

	denoise_menu->menuAction()->setText( tr("De&noise") );
	denoise_menu->menuAction()->setIcon( Images::icon("denoise") );

	unsharp_menu->menuAction()->setText( tr("Blur/S&harp") );
	unsharp_menu->menuAction()->setIcon( Images::icon("unsharp") );

	aspectDetectAct->change( tr("&Auto") );
	aspect11Act->change( "1&:1" );
	aspect32Act->change( "&3:2" );
	aspect43Act->change( "&4:3" );
	aspect54Act->change( "&5:4" );
	aspect149Act->change( "&14:9" );
	aspect1410Act->change( "1&4:10" );
	aspect169Act->change( "16:&9" );
	aspect1610Act->change( "1&6:10" );
	aspect235Act->change( "&2.35:1" );
	aspectNoneAct->change( tr("&Disabled") );

	deinterlaceNoneAct->change( tr("&None") );
	deinterlaceL5Act->change( tr("&Lowpass5") );
	deinterlaceYadif0Act->change( tr("&Yadif (normal)") );
	deinterlaceYadif1Act->change( tr("Y&adif (double framerate)") );
	deinterlaceLBAct->change( tr("Linear &Blend") );
	deinterlaceKernAct->change( tr("&Kerndeint") );

	denoiseNoneAct->change( tr("&Off", "denoise menu") );
	denoiseNormalAct->change( tr("&Normal","denoise menu") );
	denoiseSoftAct->change( tr("&Soft", "denoise menu") );

	unsharpNoneAct->change( tr("&None", "unsharp menu") );
	blurAct->change( tr("&Blur", "unsharp menu") );
	sharpenAct->change( tr("&Sharpen", "unsharp menu") );

	rotateNoneAct->change( tr("&Off") );
	rotateClockwiseFlipAct->change( tr("&Rotate by 90 degrees clockwise and flip") );
	rotateClockwiseAct->change( tr("Rotate by 90 degrees &clockwise") );
	rotateCounterclockwiseAct->change( tr("Rotate by 90 degrees counterclock&wise") );
	rotateCounterclockwiseFlipAct->change( tr("Rotate by 90 degrees counterclockwise and &flip") );

	onTopAlwaysAct->change( tr("&Always") );
	onTopNeverAct->change( tr("&Never") );
	onTopWhilePlayingAct->change( tr("While &playing") );
	toggleStayOnTopAct->change( tr("Toggle stay on top") );

#if USE_ADAPTER
	screenDefaultAct->change( tr("&Default") );
#endif

	// Menu Audio
	audiotrack_menu->menuAction()->setText( tr("&Track", "audio") );
	audiotrack_menu->menuAction()->setIcon( Images::icon("audio_track") );

	audiofilter_menu->menuAction()->setText( tr("&Filters") );
	audiofilter_menu->menuAction()->setIcon( Images::icon("audio_filters") );

	audiochannels_menu->menuAction()->setText( tr("&Channels") );
	audiochannels_menu->menuAction()->setIcon( Images::icon("audio_channels") );

	stereomode_menu->menuAction()->setText( tr("&Stereo mode") );
	stereomode_menu->menuAction()->setIcon( Images::icon("stereo_mode") );

	/* channelsDefaultAct->change( tr("&Default") ); */
	channelsStereoAct->change( tr("&Stereo") );
	channelsSurroundAct->change( tr("&4.0 Surround") );
	channelsFull51Act->change( tr("&5.1 Surround") );

	stereoAct->change( tr("&Stereo") );
	leftChannelAct->change( tr("&Left channel") );
	rightChannelAct->change( tr("&Right channel") );

	// Menu Subtitle
	subtitlestrack_menu->menuAction()->setText( tr("&Select") );
	subtitlestrack_menu->menuAction()->setIcon( Images::icon("sub") );

	closed_captions_menu->menuAction()->setText( tr("&Closed captions") );
	closed_captions_menu->menuAction()->setIcon( Images::icon("closed_caption") );

	// Menu Browse 
	titles_menu->menuAction()->setText( tr("&Title") );
	titles_menu->menuAction()->setIcon( Images::icon("title") );

	chapters_menu->menuAction()->setText( tr("&Chapter") );
	chapters_menu->menuAction()->setIcon( Images::icon("chapter") );

	angles_menu->menuAction()->setText( tr("&Angle") );
	angles_menu->menuAction()->setIcon( Images::icon("angle") );

#if PROGRAM_SWITCH
	programtrack_menu->menuAction()->setText( tr("P&rogram", "program") );
	programtrack_menu->menuAction()->setIcon( Images::icon("program_track") );
#endif


#if DVDNAV_SUPPORT
	dvdnavUpAct->change(Images::icon("dvdnav_up"), tr("DVD menu, move up"));
	dvdnavDownAct->change(Images::icon("dvdnav_down"), tr("DVD menu, move down"));
	dvdnavLeftAct->change(Images::icon("dvdnav_left"), tr("DVD menu, move left"));
	dvdnavRightAct->change(Images::icon("dvdnav_right"), tr("DVD menu, move right"));
	dvdnavMenuAct->change(Images::icon("dvdnav_menu"), tr("DVD &menu"));
	dvdnavSelectAct->change(Images::icon("dvdnav_select"), tr("DVD menu, select option"));
	dvdnavPrevAct->change(Images::icon("dvdnav_prev"), tr("DVD &previous menu"));
	dvdnavMouseAct->change(Images::icon("dvdnav_mouse"), tr("DVD menu, mouse click"));
#endif

	// Menu Options
	osd_menu->menuAction()->setText( tr("&OSD") );
	osd_menu->menuAction()->setIcon( Images::icon("osd") );

#if defined(LOG_MPLAYER) || defined(LOG_SMPLAYER)
	logs_menu->menuAction()->setText( tr("&View logs") );
	logs_menu->menuAction()->setIcon( Images::icon("logs") );
#endif

	// To be sure that the "<empty>" string is translated
	initializeMenus();

	// Other things
#ifdef LOG_MPLAYER
	mplayer_log_window->setWindowTitle( tr("SMPlayer - MPlayer log") );
#endif
#ifdef LOG_SMPLAYER
	smplayer_log_window->setWindowTitle( tr("SMPlayer - SMPlayer log") );
#endif

	updateRecents();
	updateWidgets();

	// Update actions view in preferences
	// It has to be done, here. The actions are translated after the
	// preferences dialog.
	if (pref_dialog) pref_dialog->mod_input()->actions_editor->updateView();
}

void BaseGui::setJumpTexts() {
	rewind1Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking1)) );
	rewind2Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking2)) );
	rewind3Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking3)) );

	forward1Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking1)) );
	forward2Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking2)) );
	forward3Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking3)) );

	if (qApp->isLeftToRight()) {
		rewind1Act->setIcon( Images::icon("rewind10s") );
		rewind2Act->setIcon( Images::icon("rewind1m") );
		rewind3Act->setIcon( Images::icon("rewind10m") );

		forward1Act->setIcon( Images::icon("forward10s") );
		forward2Act->setIcon( Images::icon("forward1m") );
		forward3Act->setIcon( Images::icon("forward10m") );
	} else {
		rewind1Act->setIcon( Images::flippedIcon("rewind10s") );
		rewind2Act->setIcon( Images::flippedIcon("rewind1m") );
		rewind3Act->setIcon( Images::flippedIcon("rewind10m") );

		forward1Act->setIcon( Images::flippedIcon("forward10s") );
		forward2Act->setIcon( Images::flippedIcon("forward1m") );
		forward3Act->setIcon( Images::flippedIcon("forward10m") );
	}
}

void BaseGui::setWindowCaption(const QString & title) {
	setWindowTitle(title);
}

void BaseGui::createCore() {
	core = new Core( mplayerwindow, this );

	connect( core, SIGNAL(menusNeedInitialize()),
             this, SLOT(initializeMenus()) );
	connect( core, SIGNAL(widgetsNeedUpdate()),
             this, SLOT(updateWidgets()) );
	connect( core, SIGNAL(videoEqualizerNeedsUpdate()),
             this, SLOT(updateVideoEqualizer()) );

	connect( core, SIGNAL(audioEqualizerNeedsUpdate()),
             this, SLOT(updateAudioEqualizer()) );

	connect( core, SIGNAL(showFrame(int)),
             this, SIGNAL(frameChanged(int)) );

	connect( core, SIGNAL(ABMarkersChanged(int,int)),
             this, SIGNAL(ABMarkersChanged(int,int)) );

	connect( core, SIGNAL(showTime(double)),
             this, SLOT(gotCurrentTime(double)) );

	connect( core, SIGNAL(needResize(int, int)),
             this, SLOT(resizeWindow(int,int)) );
	connect( core, SIGNAL(showMessage(QString)),
             this, SLOT(displayMessage(QString)) );
	connect( core, SIGNAL(stateChanged(Core::State)),
             this, SLOT(displayState(Core::State)) );
	connect( core, SIGNAL(stateChanged(Core::State)),
             this, SLOT(checkStayOnTop(Core::State)), Qt::QueuedConnection );

	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(enterFullscreenOnPlay()) );
	connect( core, SIGNAL(mediaStoppedByUser()),
             this, SLOT(exitFullscreenOnStop()) );

	connect( core, SIGNAL(mediaStoppedByUser()),
             mplayerwindow, SLOT(showLogo()) );

	connect( core, SIGNAL(mediaLoaded()),
             this, SLOT(enableActionsOnPlaying()) );
#if NOTIFY_AUDIO_CHANGES
	connect( core, SIGNAL(audioTracksChanged()),
             this, SLOT(enableActionsOnPlaying()) );
#endif
	connect( core, SIGNAL(mediaFinished()),
             this, SLOT(disableActionsOnStop()) );
	connect( core, SIGNAL(mediaStoppedByUser()),
             this, SLOT(disableActionsOnStop()) );

	connect( core, SIGNAL(stateChanged(Core::State)),
             this, SLOT(togglePlayAction(Core::State)) );

	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(newMediaLoaded()), Qt::QueuedConnection );
	connect( core, SIGNAL(mediaInfoChanged()),
             this, SLOT(updateMediaInfo()) );

	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(checkPendingActionsToRun()), Qt::QueuedConnection );
#if REPORT_OLD_MPLAYER
	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(checkMplayerVersion()), Qt::QueuedConnection );
#endif
	connect( core, SIGNAL(failedToParseMplayerVersion(QString)),
             this, SLOT(askForMplayerVersion(QString)) );

	connect( core, SIGNAL(mplayerFailed(QProcess::ProcessError)),
             this, SLOT(showErrorFromMplayer(QProcess::ProcessError)) );

	connect( core, SIGNAL(mplayerFinishedWithError(int)),
             this, SLOT(showExitCodeFromMplayer(int)) );

	// Hide mplayer window
#if ALLOW_TO_HIDE_VIDEO_WINDOW_ON_AUDIO_FILES
	if (pref->hide_video_window_on_audio_files) {
		connect( core, SIGNAL(noVideo()), this, SLOT(hidePanel()) );
	} else {
		connect( core, SIGNAL(noVideo()), mplayerwindow, SLOT(showLogo()) );
	}
#else
	connect( core, SIGNAL(noVideo()), this, SLOT(hidePanel()) );
#endif

	// Log mplayer output
#ifdef LOG_MPLAYER
	connect( core, SIGNAL(aboutToStartPlaying()),
             this, SLOT(clearMplayerLog()) );
	connect( core, SIGNAL(logLineAvailable(QString)),
             this, SLOT(recordMplayerLog(QString)) );

	connect( core, SIGNAL(mediaLoaded()), 
             this, SLOT(autosaveMplayerLog()) );
#endif
}

void BaseGui::createMplayerWindow() {
    mplayerwindow = new MplayerWindow( panel );
	mplayerwindow->setObjectName("mplayerwindow");
#if USE_COLORKEY
	mplayerwindow->setColorKey( pref->color_key );
#endif
	mplayerwindow->allowVideoMovement( pref->allow_video_movement );

#if LOGO_ANIMATION
	mplayerwindow->setAnimatedLogo( pref->animated_logo);
#endif

	QHBoxLayout * layout = new QHBoxLayout;
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addWidget(mplayerwindow);
	panel->setLayout(layout);

	// mplayerwindow
	/*
    connect( mplayerwindow, SIGNAL(rightButtonReleased(QPoint)),
	         this, SLOT(showPopupMenu(QPoint)) );
	*/

	// mplayerwindow mouse events
	connect( mplayerwindow, SIGNAL(doubleClicked()),
             this, SLOT(doubleClickFunction()) );
	connect( mplayerwindow, SIGNAL(leftClicked()),
             this, SLOT(leftClickFunction()) );
	connect( mplayerwindow, SIGNAL(rightClicked()),
             this, SLOT(rightClickFunction()) );
	connect( mplayerwindow, SIGNAL(middleClicked()),
             this, SLOT(middleClickFunction()) );
	connect( mplayerwindow, SIGNAL(xbutton1Clicked()),
             this, SLOT(xbutton1ClickFunction()) );
	connect( mplayerwindow, SIGNAL(xbutton2Clicked()),
             this, SLOT(xbutton2ClickFunction()) );
	connect( mplayerwindow, SIGNAL(mouseMoved(QPoint)),
             this, SLOT(checkMousePos(QPoint)) );
}

void BaseGui::createVideoEqualizer() {
	// Equalizer
	video_equalizer = new VideoEqualizer(this);

	connect( video_equalizer->contrast, SIGNAL(valueChanged(int)), 
             core, SLOT(setContrast(int)) );
	connect( video_equalizer->brightness, SIGNAL(valueChanged(int)), 
             core, SLOT(setBrightness(int)) );
	connect( video_equalizer->hue, SIGNAL(valueChanged(int)), 
             core, SLOT(setHue(int)) );
	connect( video_equalizer->saturation, SIGNAL(valueChanged(int)), 
             core, SLOT(setSaturation(int)) );
	connect( video_equalizer->gamma, SIGNAL(valueChanged(int)), 
             core, SLOT(setGamma(int)) );
	connect( video_equalizer, SIGNAL(visibilityChanged()),
             this, SLOT(updateWidgets()) );
}

void BaseGui::createAudioEqualizer() {
	// Audio Equalizer
	audio_equalizer = new AudioEqualizer(this);

	connect( audio_equalizer->eq[0], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq0(int)) );
	connect( audio_equalizer->eq[1], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq1(int)) );
	connect( audio_equalizer->eq[2], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq2(int)) );
	connect( audio_equalizer->eq[3], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq3(int)) );
	connect( audio_equalizer->eq[4], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq4(int)) );
	connect( audio_equalizer->eq[5], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq5(int)) );
	connect( audio_equalizer->eq[6], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq6(int)) );
	connect( audio_equalizer->eq[7], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq7(int)) );
	connect( audio_equalizer->eq[8], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq8(int)) );
	connect( audio_equalizer->eq[9], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq9(int)) );

	connect( audio_equalizer, SIGNAL(applyClicked(AudioEqualizerList)), 
             core, SLOT(setAudioAudioEqualizerRestart(AudioEqualizerList)) );

	connect( audio_equalizer, SIGNAL(visibilityChanged()),
             this, SLOT(updateWidgets()) );
}

void BaseGui::createPlaylist() {
#if DOCK_PLAYLIST
	playlist = new Playlist(core, this, 0);
#else
	//playlist = new Playlist(core, this, "playlist");
	playlist = new Playlist(core, 0);
#endif

	/*
	connect( playlist, SIGNAL(playlistEnded()),
             this, SLOT(exitFullscreenOnStop()) );
	*/
	connect( playlist, SIGNAL(playlistEnded()),
             this, SLOT(playlistHasFinished()) );

	connect( playlist, SIGNAL(playlistEnded()),
             mplayerwindow, SLOT(showLogo()) );

	/*
	connect( playlist, SIGNAL(visibilityChanged()),
             this, SLOT(playlistVisibilityChanged()) );
	*/

}

void BaseGui::createPanel() {
	panel = new QWidget( this );
	panel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	panel->setMinimumSize( QSize(1,1) );
	panel->setFocusPolicy( Qt::StrongFocus );

	// panel
	panel->setAutoFillBackground(TRUE);
	ColorUtils::setBackgroundColor( panel, QColor(0,0,0) );
}

void BaseGui::createPreferencesDialog() {
	pref_dialog = new PreferencesDialog(this);
	pref_dialog->setModal(false);
	/* pref_dialog->mod_input()->setActionsList( actions_list ); */
	connect( pref_dialog, SIGNAL(applied()),
             this, SLOT(applyNewPreferences()) );
}

void BaseGui::createFilePropertiesDialog() {
	file_dialog = new FilePropertiesDialog(this);
	file_dialog->setModal(false);
	connect( file_dialog, SIGNAL(applied()),
             this, SLOT(applyFileProperties()) );
}


void BaseGui::createMenus() {
	// MENUS
	openMenu = menuBar()->addMenu("Open");
	playMenu = menuBar()->addMenu("Play");
	videoMenu = menuBar()->addMenu("Video");
	audioMenu = menuBar()->addMenu("Audio");
	subtitlesMenu = menuBar()->addMenu("Subtitles");
	/* menuBar()->addMenu(favorites); */
	browseMenu = menuBar()->addMenu("Browse");
	optionsMenu = menuBar()->addMenu("Options");
	helpMenu = menuBar()->addMenu("Help");

	// OPEN MENU
	openMenu->addAction(openFileAct);

	recentfiles_menu = new QMenu(this);
	recentfiles_menu->addAction( clearRecentsAct );
	recentfiles_menu->addSeparator();

	openMenu->addMenu( recentfiles_menu );
	openMenu->addMenu(favorites);
	openMenu->addAction(openDirectoryAct);
	openMenu->addAction(openPlaylistAct);

	// Disc submenu
	disc_menu = new QMenu(this);
	disc_menu->menuAction()->setObjectName("disc_menu");
	disc_menu->addAction(openDVDAct);
	disc_menu->addAction(openDVDFolderAct);
	disc_menu->addAction(openVCDAct);
	disc_menu->addAction(openAudioCDAct);

	openMenu->addMenu(disc_menu);

	openMenu->addAction(openURLAct);
/* #ifndef Q_OS_WIN */
	openMenu->addMenu(tvlist);
	openMenu->addMenu(radiolist);
/* #endif */
	openMenu->addSeparator();
	openMenu->addAction(exitAct);
	
	// PLAY MENU
	playMenu->addAction(playAct);
	playMenu->addAction(pauseAct);
	/* playMenu->addAction(playOrPauseAct); */
	playMenu->addAction(stopAct);
	playMenu->addAction(frameStepAct);
	playMenu->addSeparator();
	playMenu->addAction(rewind1Act);
	playMenu->addAction(forward1Act);
	playMenu->addAction(rewind2Act);
	playMenu->addAction(forward2Act);
	playMenu->addAction(rewind3Act);
	playMenu->addAction(forward3Act);
	playMenu->addSeparator();

	// Speed submenu
	speed_menu = new QMenu(this);
	speed_menu->menuAction()->setObjectName("speed_menu");
	speed_menu->addAction(normalSpeedAct);
	speed_menu->addSeparator();
	speed_menu->addAction(halveSpeedAct);
	speed_menu->addAction(doubleSpeedAct);
	speed_menu->addSeparator();
	speed_menu->addAction(decSpeed10Act);
	speed_menu->addAction(incSpeed10Act);
	speed_menu->addSeparator();
	speed_menu->addAction(decSpeed4Act);
	speed_menu->addAction(incSpeed4Act);
	speed_menu->addSeparator();
	speed_menu->addAction(decSpeed1Act);
	speed_menu->addAction(incSpeed1Act);

	playMenu->addMenu(speed_menu);

	// A-B submenu
	ab_menu = new QMenu(this);
	ab_menu->menuAction()->setObjectName("ab_menu");
	ab_menu->addAction(setAMarkerAct);
	ab_menu->addAction(setBMarkerAct);
	ab_menu->addAction(clearABMarkersAct);
	ab_menu->addSeparator();
	ab_menu->addAction(repeatAct);

	playMenu->addSeparator();
	playMenu->addMenu(ab_menu);

	playMenu->addSeparator();
	playMenu->addAction(gotoAct);
	playMenu->addSeparator();
	playMenu->addAction(playPrevAct);
	playMenu->addAction(playNextAct);
	
	// VIDEO MENU
	videotrack_menu = new QMenu(this);
	videotrack_menu->menuAction()->setObjectName("videotrack_menu");

	videoMenu->addMenu(videotrack_menu);

	videoMenu->addAction(fullscreenAct);
	videoMenu->addAction(compactAct);

#if USE_ADAPTER
	// Screen submenu
	screen_menu = new QMenu(this);
	screen_menu->menuAction()->setObjectName("screen_menu");
	screen_menu->addActions( screenGroup->actions() );
	videoMenu->addMenu(screen_menu);
#endif

	// Size submenu
	videosize_menu = new QMenu(this);
	videosize_menu->menuAction()->setObjectName("videosize_menu");
	videosize_menu->addActions( sizeGroup->actions() );
	videosize_menu->addSeparator();
	videosize_menu->addAction(doubleSizeAct);
	videoMenu->addMenu(videosize_menu);

	// Zoom submenu
	zoom_menu = new QMenu(this);
	zoom_menu->menuAction()->setObjectName("zoom_menu");
	zoom_menu->addAction(resetZoomAct);
	zoom_menu->addSeparator();
	zoom_menu->addAction(autoZoomAct);
	zoom_menu->addAction(autoZoom169Act);
	zoom_menu->addAction(autoZoom235Act);
	zoom_menu->addSeparator();
	zoom_menu->addAction(decZoomAct);
	zoom_menu->addAction(incZoomAct);
	zoom_menu->addSeparator();
	zoom_menu->addAction(moveLeftAct);
	zoom_menu->addAction(moveRightAct);
	zoom_menu->addAction(moveUpAct);
	zoom_menu->addAction(moveDownAct);

	videoMenu->addMenu(zoom_menu);

	// Aspect submenu
	aspect_menu = new QMenu(this);
	aspect_menu->menuAction()->setObjectName("aspect_menu");
	aspect_menu->addActions( aspectGroup->actions() );

	videoMenu->addMenu(aspect_menu);

	// Deinterlace submenu
	deinterlace_menu = new QMenu(this);
	deinterlace_menu->menuAction()->setObjectName("deinterlace_menu");
	deinterlace_menu->addActions( deinterlaceGroup->actions() );

	videoMenu->addMenu(deinterlace_menu);

	// Video filter submenu
	videofilter_menu = new QMenu(this);
	videofilter_menu->menuAction()->setObjectName("videofilter_menu");
	videofilter_menu->addAction(postProcessingAct);
	videofilter_menu->addAction(deblockAct);
	videofilter_menu->addAction(deringAct);
	videofilter_menu->addAction(gradfunAct);
	videofilter_menu->addAction(addNoiseAct);
	videofilter_menu->addAction(addLetterboxAct);
	videofilter_menu->addAction(upscaleAct);
	videofilter_menu->addAction(phaseAct);

	// Denoise submenu
	denoise_menu = new QMenu(this);
	denoise_menu->menuAction()->setObjectName("denoise_menu");
	denoise_menu->addActions(denoiseGroup->actions());
	videofilter_menu->addMenu(denoise_menu);

	// Unsharp submenu
	unsharp_menu = new QMenu(this);
	unsharp_menu->menuAction()->setObjectName("unsharp_menu");
	unsharp_menu->addActions(unsharpGroup->actions());
	videofilter_menu->addMenu(unsharp_menu);
	/*
	videofilter_menu->addSeparator();
	videofilter_menu->addActions(denoiseGroup->actions());
	videofilter_menu->addSeparator();
	videofilter_menu->addActions(unsharpGroup->actions());
	*/
	videoMenu->addMenu(videofilter_menu);

	// Rotate submenu
	rotate_menu = new QMenu(this);
	rotate_menu->menuAction()->setObjectName("rotate_menu");
	rotate_menu->addActions(rotateGroup->actions());

	videoMenu->addMenu(rotate_menu);

	videoMenu->addAction(flipAct);
	videoMenu->addAction(mirrorAct);
	videoMenu->addSeparator();
	videoMenu->addAction(videoEqualizerAct);
	videoMenu->addAction(screenshotAct);
	videoMenu->addAction(screenshotsAct);

	// Ontop submenu
	ontop_menu = new QMenu(this);
	ontop_menu->menuAction()->setObjectName("ontop_menu");
	ontop_menu->addActions(onTopActionGroup->actions());

	videoMenu->addMenu(ontop_menu);

#ifdef VIDEOPREVIEW
	videoMenu->addSeparator();
	videoMenu->addAction(videoPreviewAct);
#endif


    // AUDIO MENU

	// Audio track submenu
	audiotrack_menu = new QMenu(this);
	audiotrack_menu->menuAction()->setObjectName("audiotrack_menu");

	audioMenu->addMenu(audiotrack_menu);

	audioMenu->addAction(loadAudioAct);
	audioMenu->addAction(unloadAudioAct);

	// Filter submenu
	audiofilter_menu = new QMenu(this);
	audiofilter_menu->menuAction()->setObjectName("audiofilter_menu");
	audiofilter_menu->addAction(extrastereoAct);
	audiofilter_menu->addAction(karaokeAct);
	audiofilter_menu->addAction(volnormAct);

	audioMenu->addMenu(audiofilter_menu);

	// Audio channels submenu
	audiochannels_menu = new QMenu(this);
	audiochannels_menu->menuAction()->setObjectName("audiochannels_menu");
	audiochannels_menu->addActions( channelsGroup->actions() );

	audioMenu->addMenu(audiochannels_menu);

	// Stereo mode submenu
	stereomode_menu = new QMenu(this);
	stereomode_menu->menuAction()->setObjectName("stereomode_menu");
	stereomode_menu->addActions( stereoGroup->actions() );

	audioMenu->addMenu(stereomode_menu);
	audioMenu->addAction(audioEqualizerAct);
	audioMenu->addSeparator();
	audioMenu->addAction(muteAct);
	audioMenu->addSeparator();
	audioMenu->addAction(decVolumeAct);
	audioMenu->addAction(incVolumeAct);
	audioMenu->addSeparator();
	audioMenu->addAction(decAudioDelayAct);
	audioMenu->addAction(incAudioDelayAct);
	audioMenu->addSeparator();
	audioMenu->addAction(audioDelayAct);


	// SUBTITLES MENU
	// Track submenu
	subtitlestrack_menu = new QMenu(this);
	subtitlestrack_menu->menuAction()->setObjectName("subtitlestrack_menu");

	subtitlesMenu->addMenu(subtitlestrack_menu);

	subtitlesMenu->addAction(loadSubsAct);
	subtitlesMenu->addAction(unloadSubsAct);
	subtitlesMenu->addSeparator();

	closed_captions_menu = new QMenu(this);
	closed_captions_menu->menuAction()->setObjectName("closed_captions_menu");
	closed_captions_menu->addAction( ccNoneAct);
	closed_captions_menu->addAction( ccChannel1Act);
	closed_captions_menu->addAction( ccChannel2Act);
	closed_captions_menu->addAction( ccChannel3Act);
	closed_captions_menu->addAction( ccChannel4Act);
	subtitlesMenu->addMenu(closed_captions_menu);
	subtitlesMenu->addSeparator();

	subtitlesMenu->addAction(decSubDelayAct);
	subtitlesMenu->addAction(incSubDelayAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(subDelayAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubPosAct);
	subtitlesMenu->addAction(incSubPosAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubScaleAct);
	subtitlesMenu->addAction(incSubScaleAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubStepAct);
	subtitlesMenu->addAction(incSubStepAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(useForcedSubsOnlyAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(subVisibilityAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(useAssAct);
#ifdef FIND_SUBTITLES
	subtitlesMenu->addSeparator(); //turbos
	subtitlesMenu->addAction(showFindSubtitlesDialogAct);
	subtitlesMenu->addAction(openUploadSubtitlesPageAct); //turbos
#endif

	// BROWSE MENU
	// Titles submenu
	titles_menu = new QMenu(this);
	titles_menu->menuAction()->setObjectName("titles_menu");

	browseMenu->addMenu(titles_menu);

	// Chapters submenu
	chapters_menu = new QMenu(this);
	chapters_menu->menuAction()->setObjectName("chapters_menu");

	browseMenu->addMenu(chapters_menu);

	// Angles submenu
	angles_menu = new QMenu(this);
	angles_menu->menuAction()->setObjectName("angles_menu");

	browseMenu->addMenu(angles_menu);

#if DVDNAV_SUPPORT
	browseMenu->addSeparator();
	browseMenu->addAction(dvdnavMenuAct);
	browseMenu->addAction(dvdnavPrevAct);
#endif

#if PROGRAM_SWITCH
	programtrack_menu = new QMenu(this);
	programtrack_menu->menuAction()->setObjectName("programtrack_menu");

	browseMenu->addSeparator();
	browseMenu->addMenu(programtrack_menu);
#endif


	// OPTIONS MENU
	optionsMenu->addAction(showPropertiesAct);
	optionsMenu->addAction(showPlaylistAct);
	// Check if the smplayer youtube browser is installed
	{
		QString tube_exec = Paths::appPath() + "/smtube";
		#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
		tube_exec += ".exe";
		#endif
		if (QFile::exists(tube_exec)) {
			optionsMenu->addAction(showTubeBrowserAct);
			qDebug("BaseGui::createMenus: %s does exist", tube_exec.toUtf8().constData());
		} else {
			qDebug("BaseGui::createMenus: %s does not exist", tube_exec.toUtf8().constData());
		}
	}

	// OSD submenu
	osd_menu = new QMenu(this);
	osd_menu->menuAction()->setObjectName("osd_menu");
	osd_menu->addActions(osdGroup->actions());

	optionsMenu->addMenu(osd_menu);

	// Logs submenu
#if defined(LOG_MPLAYER) || defined(LOG_SMPLAYER)
	logs_menu = new QMenu(this);
	#ifdef LOG_MPLAYER
	logs_menu->addAction(showLogMplayerAct);
	#endif
	#ifdef LOG_SMPLAYER
	logs_menu->addAction(showLogSmplayerAct);
	#endif
	optionsMenu->addMenu(logs_menu);
#endif

	optionsMenu->addAction(showPreferencesAct);

	/*
	Favorites * fav = new Favorites(Paths::configPath() + "/test.fav", this);
	connect(fav, SIGNAL(activated(QString)), this, SLOT(open(QString)));
	optionsMenu->addMenu( fav->menu() )->setText("Favorites");
	*/

	// HELP MENU
	helpMenu->addAction(showFAQAct);
	helpMenu->addAction(showCLOptionsAct);
	helpMenu->addAction(showCheckUpdatesAct);
	helpMenu->addAction(donateAct);
	helpMenu->addSeparator();
	helpMenu->addAction(aboutQtAct);
	helpMenu->addAction(aboutThisAct);

	// POPUP MENU
	if (!popup)
		popup = new QMenu(this);
	else
		popup->clear();

	popup->addMenu( openMenu );
	popup->addMenu( playMenu );
	popup->addMenu( videoMenu );
	popup->addMenu( audioMenu );
	popup->addMenu( subtitlesMenu );
	popup->addMenu( favorites );
	popup->addMenu( browseMenu );
	popup->addMenu( optionsMenu );

	// let's show something, even a <empty> entry
	initializeMenus();
}

/*
void BaseGui::closeEvent( QCloseEvent * e )  {
	qDebug("BaseGui::closeEvent");

	qDebug("mplayer_log_window: %d x %d", mplayer_log_window->width(), mplayer_log_window->height() );
	qDebug("smplayer_log_window: %d x %d", smplayer_log_window->width(), smplayer_log_window->height() );

	mplayer_log_window->close();
	smplayer_log_window->close();
	playlist->close();
	equalizer->close();

	core->stop();
	e->accept();
}
*/


void BaseGui::closeWindow() {
	qDebug("BaseGui::closeWindow");

	if (core->state() != Core::Stopped) {
		core->stop();
	}

	//qApp->quit();
	emit quitSolicited();
}

void BaseGui::showPlaylist() {
	showPlaylist( !playlist->isVisible() );
}

void BaseGui::showPlaylist(bool b) {
	if ( !b ) {
		playlist->hide();
	} else {
		exitFullscreenIfNeeded();
		playlist->show();
	}
	//updateWidgets();
}

void BaseGui::showVideoEqualizer() {
	showVideoEqualizer( !video_equalizer->isVisible() );
}

void BaseGui::showVideoEqualizer(bool b) {
	if (!b) {
		video_equalizer->hide();
	} else {
		// Exit fullscreen, otherwise dialog is not visible
		exitFullscreenIfNeeded();
		video_equalizer->show();
	}
	updateWidgets();
}

void BaseGui::showAudioEqualizer() {
	showAudioEqualizer( !audio_equalizer->isVisible() );
}

void BaseGui::showAudioEqualizer(bool b) {
	if (!b) {
		audio_equalizer->hide();
	} else {
		// Exit fullscreen, otherwise dialog is not visible
		exitFullscreenIfNeeded();
		audio_equalizer->show();
	}
	updateWidgets();
}

void BaseGui::showPreferencesDialog() {
	qDebug("BaseGui::showPreferencesDialog");

	exitFullscreenIfNeeded();
	
	if (!pref_dialog) {
		createPreferencesDialog();
	}

	pref_dialog->setData(pref);

	pref_dialog->mod_input()->actions_editor->clear();
	pref_dialog->mod_input()->actions_editor->addActions(this);
#if !DOCK_PLAYLIST
	pref_dialog->mod_input()->actions_editor->addActions(playlist);
#endif

	// Set playlist preferences
	PrefPlaylist * pl = pref_dialog->mod_playlist();
	pl->setDirectoryRecursion(playlist->directoryRecursion());
	pl->setAutoGetInfo(playlist->autoGetInfo());
	pl->setSavePlaylistOnExit(playlist->savePlaylistOnExit());
	pl->setPlayFilesFromStart(playlist->playFilesFromStart());

	pref_dialog->show();
}

// The user has pressed OK in preferences dialog
void BaseGui::applyNewPreferences() {
	qDebug("BaseGui::applyNewPreferences");

	bool need_update_language = false;

	pref_dialog->getData(pref);

	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString( pref->default_font );
		qApp->setFont(f);
	}

#ifndef NO_USE_INI_FILES
	PrefGeneral *_general = pref_dialog->mod_general();
	if (_general->fileSettingsMethodChanged()) {
		core->changeFileSettingsMethod(pref->file_settings_method);
	}
#endif

	PrefInterface *_interface = pref_dialog->mod_interface();
	if (_interface->recentsChanged()) {
		updateRecents();
	}
	if (_interface->languageChanged()) need_update_language = true;

	if (_interface->iconsetChanged()) { 
		need_update_language = true;
		// Stylesheet
#if ALLOW_CHANGE_STYLESHEET
		changeStyleSheet(pref->iconset);
#endif
	}

#if ALLOW_TO_HIDE_VIDEO_WINDOW_ON_AUDIO_FILES
	if (pref->hide_video_window_on_audio_files) {
		connect( core, SIGNAL(noVideo()), this, SLOT(hidePanel()) );
		disconnect( core, SIGNAL(noVideo()), mplayerwindow, SLOT(hideLogo()) );
	} else {
		disconnect( core, SIGNAL(noVideo()), this, SLOT(hidePanel()) );
		connect( core, SIGNAL(noVideo()), mplayerwindow, SLOT(showLogo()) );
		if (!panel->isVisible()) {
			resize( width(), height() + 200);
			panel->show();
		}
	}
#endif

	PrefAdvanced *advanced = pref_dialog->mod_advanced();
#if REPAINT_BACKGROUND_OPTION
	if (advanced->repaintVideoBackgroundChanged()) {
		mplayerwindow->videoLayer()->setRepaintBackground(pref->repaint_video_background);
	}
#endif
#if USE_COLORKEY
	if (advanced->colorkeyChanged()) {
		mplayerwindow->setColorKey( pref->color_key );
	}
#endif
	if (advanced->monitorAspectChanged()) {
		mplayerwindow->setMonitorAspect( pref->monitor_aspect_double() );
	}

	// Update playlist preferences
	PrefPlaylist * pl = pref_dialog->mod_playlist();
	playlist->setDirectoryRecursion(pl->directoryRecursion());
	playlist->setAutoGetInfo(pl->autoGetInfo());
	playlist->setSavePlaylistOnExit(pl->savePlaylistOnExit());
	playlist->setPlayFilesFromStart(pl->playFilesFromStart());


	if (need_update_language) {
		translator->load(pref->language);
	}

	setJumpTexts(); // Update texts in menus
	updateWidgets(); // Update the screenshot action

#if STYLE_SWITCHING
	if (_interface->styleChanged()) {
		qDebug( "selected style: '%s'", pref->style.toUtf8().data() );
		if ( !pref->style.isEmpty()) {
			qApp->setStyle( pref->style );
		} else {
			qDebug("setting default style: '%s'", default_style.toUtf8().data() );
			qApp->setStyle( default_style );
		}
	}
#endif

    // Restart the video if needed
	if (pref_dialog->requiresRestart())
		core->restart();

	// Update actions
	pref_dialog->mod_input()->actions_editor->applyChanges();
	saveActions();

#ifndef NO_USE_INI_FILES
	pref->save();
#endif


	if (_interface->guiChanged()) {
#ifdef GUI_CHANGE_ON_RUNTIME
		core->stop();
		emit guiChanged(pref->gui);
#else
		QMessageBox::information(this, tr("Information"),
			tr("You need to restart SMPlayer to use the new GUI.") );
#endif
	}
}


void BaseGui::showFilePropertiesDialog() {
	qDebug("BaseGui::showFilePropertiesDialog");

	exitFullscreenIfNeeded();

	if (!file_dialog) {
		createFilePropertiesDialog();
	}

	setDataToFileProperties();

	file_dialog->show();
}

void BaseGui::setDataToFileProperties() {
	// Save a copy of the original values
	if (core->mset.original_demuxer.isEmpty()) 
		core->mset.original_demuxer = core->mdat.demuxer;

	if (core->mset.original_video_codec.isEmpty()) 
		core->mset.original_video_codec = core->mdat.video_codec;

	if (core->mset.original_audio_codec.isEmpty()) 
		core->mset.original_audio_codec = core->mdat.audio_codec;

	QString demuxer = core->mset.forced_demuxer;
	if (demuxer.isEmpty()) demuxer = core->mdat.demuxer;

	QString ac = core->mset.forced_audio_codec;
	if (ac.isEmpty()) ac = core->mdat.audio_codec;

	QString vc = core->mset.forced_video_codec;
	if (vc.isEmpty()) vc = core->mdat.video_codec;

	file_dialog->setDemuxer(demuxer, core->mset.original_demuxer);
	file_dialog->setAudioCodec(ac, core->mset.original_audio_codec);
	file_dialog->setVideoCodec(vc, core->mset.original_video_codec);

	file_dialog->setMplayerAdditionalArguments( core->mset.mplayer_additional_options );
	file_dialog->setMplayerAdditionalVideoFilters( core->mset.mplayer_additional_video_filters );
	file_dialog->setMplayerAdditionalAudioFilters( core->mset.mplayer_additional_audio_filters );

	file_dialog->setMediaData( core->mdat );
}

void BaseGui::applyFileProperties() {
	qDebug("BaseGui::applyFileProperties");

	bool need_restart = false;
	bool demuxer_changed = false;

#undef TEST_AND_SET
#define TEST_AND_SET( Pref, Dialog ) \
	if ( Pref != Dialog ) { Pref = Dialog; need_restart = TRUE; }

	QString prev_demuxer = core->mset.forced_demuxer;

	QString demuxer = file_dialog->demuxer();
	if (demuxer == core->mset.original_demuxer) demuxer="";
	TEST_AND_SET(core->mset.forced_demuxer, demuxer);

	if (prev_demuxer != core->mset.forced_demuxer) {
		// Demuxer changed
		demuxer_changed = true;
		core->mset.current_audio_id = MediaSettings::NoneSelected;
		core->mset.current_sub_id = MediaSettings::NoneSelected;
	}

	QString ac = file_dialog->audioCodec();
	if (ac == core->mset.original_audio_codec) ac="";
	TEST_AND_SET(core->mset.forced_audio_codec, ac);

	QString vc = file_dialog->videoCodec();
	if (vc == core->mset.original_video_codec) vc="";
	TEST_AND_SET(core->mset.forced_video_codec, vc);

	TEST_AND_SET(core->mset.mplayer_additional_options, file_dialog->mplayerAdditionalArguments());
	TEST_AND_SET(core->mset.mplayer_additional_video_filters, file_dialog->mplayerAdditionalVideoFilters());
	TEST_AND_SET(core->mset.mplayer_additional_audio_filters, file_dialog->mplayerAdditionalAudioFilters());

	// Restart the video to apply
	if (need_restart) {
		if (demuxer_changed) {
			core->reload();
		} else {
			core->restart();
		}
	}
}


void BaseGui::updateMediaInfo() {
    qDebug("BaseGui::updateMediaInfo");

	if (file_dialog) {
		if (file_dialog->isVisible()) setDataToFileProperties();
	}

	setWindowCaption( core->mdat.displayName(pref->show_tag_in_window_title) + " - SMPlayer" );

	emit videoInfoChanged(core->mdat.video_width, core->mdat.video_height, core->mdat.video_fps.toDouble());
}

void BaseGui::newMediaLoaded() {
    qDebug("BaseGui::newMediaLoaded");

	pref->history_recents->addItem( core->mdat.filename );
	updateRecents();

	// If a VCD, Audio CD or DVD, add items to playlist
	bool is_disc = ( (core->mdat.type == TYPE_VCD) || (core->mdat.type == TYPE_DVD) || (core->mdat.type == TYPE_AUDIO_CD) );
#if DVDNAV_SUPPORT
	// Don't add the list of titles if using dvdnav
	if ((core->mdat.type == TYPE_DVD) && (core->mdat.filename.startsWith("dvdnav:"))) is_disc = false;
#endif
	if (pref->auto_add_to_playlist && is_disc)
	{
		int first_title = 1;
		if (core->mdat.type == TYPE_VCD) first_title = pref->vcd_initial_title;

		QString type = "dvd"; // FIXME: support dvdnav
		if (core->mdat.type == TYPE_VCD) type="vcd";
		else
		if (core->mdat.type == TYPE_AUDIO_CD) type="cdda";

		if (core->mset.current_title_id == first_title) {
			playlist->clear();
			QStringList l;
			QString s;
			QString folder;
			if (core->mdat.type == TYPE_DVD) {
				DiscData disc_data = DiscName::split(core->mdat.filename);
				folder = disc_data.device;
			}
			for (int n=0; n < core->mdat.titles.numItems(); n++) {
				s = type + "://" + QString::number(core->mdat.titles.itemAt(n).ID());
				if ( !folder.isEmpty() ) {
					s += "/" + folder; // FIXME: dvd names are not created as they should
				}
				l.append(s);
			}
			playlist->addFiles(l);
			//playlist->setModified(false); // Not a real playlist
		}
	} /*else {
		playlist->clear();
		playlist->addCurrentFile();
	}*/

	if ((core->mdat.type == TYPE_FILE) && (pref->auto_add_to_playlist) && (pref->add_to_playlist_consecutive_files)) {
		// Look for consecutive files
		QStringList files_to_add = Helper::searchForConsecutiveFiles(core->mdat.filename);
		if (!files_to_add.empty()) playlist->addFiles(files_to_add);
	}
}

#ifdef LOG_MPLAYER
void BaseGui::clearMplayerLog() {
	mplayer_log.clear();
	if (mplayer_log_window->isVisible()) mplayer_log_window->clear();
}

void BaseGui::recordMplayerLog(QString line) {
	if (pref->log_mplayer) {
		if ( (line.indexOf("A:")==-1) && (line.indexOf("V:")==-1) ) {
			line.append("\n");
			mplayer_log.append(line);
			if (mplayer_log_window->isVisible()) mplayer_log_window->appendText(line);
		}
	}
}

/*! 
	Save the mplayer log to a file, so it can be used by external
	applications.
*/
void BaseGui::autosaveMplayerLog() {
	qDebug("BaseGui::autosaveMplayerLog");

	if (pref->autosave_mplayer_log) {
		if (!pref->mplayer_log_saveto.isEmpty()) {
			QFile file( pref->mplayer_log_saveto );
			if ( file.open( QIODevice::WriteOnly ) ) {
				QTextStream strm( &file );
				strm << mplayer_log;
				file.close();
			}
		}
	}
}

void BaseGui::showMplayerLog() {
    qDebug("BaseGui::showMplayerLog");

	exitFullscreenIfNeeded();

    mplayer_log_window->setText( mplayer_log );
	mplayer_log_window->show();
}
#endif

#ifdef LOG_SMPLAYER
void BaseGui::recordSmplayerLog(QString line) {
	if (pref->log_smplayer) {
		line.append("\n");
		smplayer_log.append(line);
		if (smplayer_log_window->isVisible()) smplayer_log_window->appendText(line);
	}
}

void BaseGui::showLog() {
	qDebug("BaseGui::showLog");

	exitFullscreenIfNeeded();

	smplayer_log_window->setText( smplayer_log );
	smplayer_log_window->show();
}
#endif


void BaseGui::initializeMenus() {
	qDebug("BaseGui::initializeMenus");

#define EMPTY 1

	int n;

	// Subtitles
	subtitleTrackGroup->clear(true);
	QAction * subNoneAct = subtitleTrackGroup->addAction( tr("&None") );
	subNoneAct->setData(MediaSettings::SubNone);
	subNoneAct->setCheckable(true);
	for (n=0; n < core->mdat.subs.numItems(); n++) {
		QAction *a = new QAction(subtitleTrackGroup);
		a->setCheckable(true);
		a->setText(core->mdat.subs.itemAt(n).displayName());
		a->setData(n);
	}
	subtitlestrack_menu->addActions( subtitleTrackGroup->actions() );

	// Audio
	audioTrackGroup->clear(true);
	if (core->mdat.audios.numItems()==0) {
		QAction * a = audioTrackGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	} else {
		for (n=0; n < core->mdat.audios.numItems(); n++) {
			QAction *a = new QAction(audioTrackGroup);
			a->setCheckable(true);
			a->setText(core->mdat.audios.itemAt(n).displayName());
			a->setData(core->mdat.audios.itemAt(n).ID());
		}
	}
	audiotrack_menu->addActions( audioTrackGroup->actions() );

#if PROGRAM_SWITCH
	// Program
	programTrackGroup->clear(true);
	if (core->mdat.programs.numItems()==0) {
		QAction * a = programTrackGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	} else {
		for (n=0; n < core->mdat.programs.numItems(); n++) {
			QAction *a = new QAction(programTrackGroup);
			a->setCheckable(true);
			a->setText(core->mdat.programs.itemAt(n).displayName());
			a->setData(core->mdat.programs.itemAt(n).ID());
		}
	}
	programtrack_menu->addActions( programTrackGroup->actions() );
#endif

	// Video
	videoTrackGroup->clear(true);
	if (core->mdat.videos.numItems()==0) {
		QAction * a = videoTrackGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	} else {
		for (n=0; n < core->mdat.videos.numItems(); n++) {
			QAction *a = new QAction(videoTrackGroup);
			a->setCheckable(true);
			a->setText(core->mdat.videos.itemAt(n).displayName());
			a->setData(core->mdat.videos.itemAt(n).ID());
		}
	}
	videotrack_menu->addActions( videoTrackGroup->actions() );

	// Titles
	titleGroup->clear(true);
	if (core->mdat.titles.numItems()==0) {
		QAction * a = titleGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	} else {
		for (n=0; n < core->mdat.titles.numItems(); n++) {
			QAction *a = new QAction(titleGroup);
			a->setCheckable(true);
			a->setText(core->mdat.titles.itemAt(n).displayName());
			a->setData(core->mdat.titles.itemAt(n).ID());
		}
	}
	titles_menu->addActions( titleGroup->actions() );

	// Chapters
	chapterGroup->clear(true);
	if (core->mdat.chapters.numItems() > 0) {
		for (n=0; n < core->mdat.chapters.numItems(); n++) {
			QAction *a = new QAction(chapterGroup);
			//a->setCheckable(true);
			a->setText(core->mdat.chapters.itemAt(n).name());
			a->setData(core->mdat.chapters.itemAt(n).ID());
		}
	} 
	else
	if (core->mdat.n_chapters > 0) {
		for (n=0; n < core->mdat.n_chapters; n++) {
			QAction *a = new QAction(chapterGroup);
			//a->setCheckable(true);
			a->setText( QString::number(n+1) );
			a->setData( n + Core::firstChapter() );
		}
	} 
	else {
		QAction * a = chapterGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	}
	chapters_menu->addActions( chapterGroup->actions() );

	// Angles
	angleGroup->clear(true);
	int n_angles = 0;
	if (core->mset.current_angle_id > 0) {
		int i = core->mdat.titles.find(core->mset.current_angle_id);
		if (i > -1) n_angles = core->mdat.titles.itemAt(i).angles();
	}
	if (n_angles > 0) {
		for (n=1; n <= n_angles; n++) {
			QAction *a = new QAction(angleGroup);
			a->setCheckable(true);
			a->setText( QString::number(n) );
			a->setData( n );
		}
	} else {
		QAction * a = angleGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	}
	angles_menu->addActions( angleGroup->actions() );
}

void BaseGui::updateRecents() {
	qDebug("BaseGui::updateRecents");

	// Not clear the first 2 items
	while (recentfiles_menu->actions().count() > 2) {
		QAction * a = recentfiles_menu->actions()[2];
		recentfiles_menu->removeAction( a );
		a->deleteLater();
	}

	int current_items = 0;

	if (pref->history_recents->count() > 0) {
		for (int n=0; n < pref->history_recents->count(); n++) {
			QString i = QString::number( n+1 );
			QString fullname = pref->history_recents->item(n);
			QString filename = fullname;
			QFileInfo fi(fullname);
			//if (fi.exists()) filename = fi.fileName(); // Can be slow

			// Let's see if it looks like a file (no dvd://1 or something)
			if (fullname.indexOf(QRegExp("^.*://.*")) == -1) filename = fi.fileName();

			if (filename.size() > 85) {
				filename = filename.left(80) + "...";
			}

			QAction * a = recentfiles_menu->addAction( QString("%1. " + filename ).arg( i.insert( i.size()-1, '&' ), 3, ' ' ));
			a->setStatusTip(fullname);
			a->setData(n);
			connect(a, SIGNAL(triggered()), this, SLOT(openRecent()));
			current_items++;
		}
	} else {
		QAction * a = recentfiles_menu->addAction( tr("<empty>") );
		a->setEnabled(false);
	}

	recentfiles_menu->menuAction()->setVisible( current_items > 0 );
}

void BaseGui::clearRecentsList() {
	// Delete items in menu
	pref->history_recents->clear();
	updateRecents();
}

void BaseGui::updateWidgets() {
	qDebug("BaseGui::updateWidgets");

	// Subtitles menu
	subtitleTrackGroup->setChecked( core->mset.current_sub_id );

	// Disable the unload subs action if there's no external subtitles
	unloadSubsAct->setEnabled( !core->mset.external_subtitles.isEmpty() );

	// Closed caption menu
	ccGroup->setChecked( core->mset.closed_caption_channel );
	
	// Audio menu
	audioTrackGroup->setChecked( core->mset.current_audio_id );
	channelsGroup->setChecked( core->mset.audio_use_channels );
	stereoGroup->setChecked( core->mset.stereo_mode );
	// Disable the unload audio file action if there's no external audio file
	unloadAudioAct->setEnabled( !core->mset.external_audio.isEmpty() );

#if PROGRAM_SWITCH
	// Program menu
	programTrackGroup->setChecked( core->mset.current_program_id );
#endif

	// Video menu
	videoTrackGroup->setChecked( core->mset.current_video_id );

	// Aspect ratio
	aspectGroup->setChecked( core->mset.aspect_ratio_id );

	// Rotate
	rotateGroup->setChecked( core->mset.rotate );

#if USE_ADAPTER
	screenGroup->setChecked( pref->adapter );
#endif

	// OSD
	osdGroup->setChecked( pref->osd );

	// Titles
	titleGroup->setChecked( core->mset.current_title_id );

	// Chapters
	chapterGroup->setChecked( core->mset.current_chapter_id );

	// Angles
	angleGroup->setChecked( core->mset.current_angle_id );

	// Deinterlace menu
	deinterlaceGroup->setChecked( core->mset.current_deinterlacer );

	// Video size menu
	sizeGroup->setChecked( pref->size_factor );

	// Auto phase
	phaseAct->setChecked( core->mset.phase_filter );

	// Deblock
	deblockAct->setChecked( core->mset.deblock_filter );

	// Dering
	deringAct->setChecked( core->mset.dering_filter );

	// Gradfun
	gradfunAct->setChecked( core->mset.gradfun_filter );

	// Add noise
	addNoiseAct->setChecked( core->mset.noise_filter );

	// Letterbox
	addLetterboxAct->setChecked( core->mset.add_letterbox );

	// Upscaling
	upscaleAct->setChecked( core->mset.upscaling_filter );


	// Postprocessing
	postProcessingAct->setChecked( core->mset.postprocessing_filter );

	// Denoise submenu
	denoiseGroup->setChecked( core->mset.current_denoiser );

	// Unsharp submenu
	unsharpGroup->setChecked( core->mset.current_unsharp );

	/*
	// Fullscreen button
	fullscreenbutton->setOn(pref->fullscreen); 

	// Mute button
	mutebutton->setOn(core->mset.mute);
	if (core->mset.mute) 
		mutebutton->setPixmap( Images::icon("mute_small") );
	else
		mutebutton->setPixmap( Images::icon("volume_small") );

	// Volume slider
	volumeslider->setValue( core->mset.volume );
	*/

	// Mute menu option
	muteAct->setChecked( (pref->global_volume ? pref->mute : core->mset.mute) );

	// Karaoke menu option
	karaokeAct->setChecked( core->mset.karaoke_filter );

	// Extrastereo menu option
	extrastereoAct->setChecked( core->mset.extrastereo_filter );

	// Volnorm menu option
	volnormAct->setChecked( core->mset.volnorm_filter );

	// Repeat menu option
	repeatAct->setChecked( core->mset.loop );

	// Fullscreen action
	fullscreenAct->setChecked( pref->fullscreen );

	// Time slider
	if (core->state()==Core::Stopped) {
		//FIXME
		//timeslider->setValue( (int) core->mset.current_sec );
	}

	// Video equalizer
	videoEqualizerAct->setChecked( video_equalizer->isVisible() );

	// Audio equalizer
	audioEqualizerAct->setChecked( audio_equalizer->isVisible() );

	// Playlist
#if !DOCK_PLAYLIST
	//showPlaylistAct->setChecked( playlist->isVisible() );
#endif

#if DOCK_PLAYLIST
	showPlaylistAct->setChecked( playlist->isVisible() );
#endif

	// Compact mode
	compactAct->setChecked( pref->compact_mode );

	// Stay on top
	onTopActionGroup->setChecked( (int) pref->stay_on_top );

	// Flip
	flipAct->setChecked( core->mset.flip );

	// Mirror
	mirrorAct->setChecked( core->mset.mirror );

	// Use ass lib
	useAssAct->setChecked( pref->use_ass_subtitles );

	// Forced subs
	useForcedSubsOnlyAct->setChecked( pref->use_forced_subs_only );

	// Subtitle visibility
	subVisibilityAct->setChecked(pref->sub_visibility);

	// Enable or disable subtitle options
	bool e = ((core->mset.current_sub_id != MediaSettings::SubNone) &&
              (core->mset.current_sub_id != MediaSettings::NoneSelected));

	if (core->mset.closed_caption_channel !=0 ) e = true; // Enable if using closed captions

	decSubDelayAct->setEnabled(e);
	incSubDelayAct->setEnabled(e);
	subDelayAct->setEnabled(e);
	decSubPosAct->setEnabled(e);
	incSubPosAct->setEnabled(e);
	decSubScaleAct->setEnabled(e);
	incSubScaleAct->setEnabled(e);
	decSubStepAct->setEnabled(e);
	incSubStepAct->setEnabled(e);
}

void BaseGui::updateVideoEqualizer() {
	// Equalizer
	video_equalizer->contrast->setValue( core->mset.contrast );
	video_equalizer->brightness->setValue( core->mset.brightness );
	video_equalizer->hue->setValue( core->mset.hue );
	video_equalizer->saturation->setValue( core->mset.saturation );
	video_equalizer->gamma->setValue( core->mset.gamma );
}

void BaseGui::updateAudioEqualizer() {
	// Audio Equalizer
	for (int n = 0; n < 10; n++) {
		audio_equalizer->eq[n]->setValue( core->mset.audio_equalizer[n].toInt() );
	}
}

/*
void BaseGui::playlistVisibilityChanged() {
#if !DOCK_PLAYLIST
	bool visible = playlist->isVisible();

	showPlaylistAct->setChecked( visible );
#endif
}
*/

/*
void BaseGui::openRecent(int item) {
	qDebug("BaseGui::openRecent: %d", item);
	if ((item > -1) && (item < RECENTS_CLEAR)) { // 1000 = Clear item
		open( recents->item(item) );
	}
}
*/

void BaseGui::openRecent() {
	QAction *a = qobject_cast<QAction *> (sender());
	if (a) {
		int item = a->data().toInt();
		qDebug("BaseGui::openRecent: %d", item);
		QString file = pref->history_recents->item(item);

		if (pref->auto_add_to_playlist) {
			if (playlist->maybeSave()) {
				playlist->clear();
				playlist->addFile(file, Playlist::NoGetInfo);

				open( file );
			}
		} else {
			open( file );
		}

	}
}

void BaseGui::open(QString file) {
	qDebug("BaseGui::open: '%s'", file.toUtf8().data());

	// If file is a playlist, open that playlist
	QString extension = QFileInfo(file).suffix().toLower();
	if ( ((extension=="m3u") || (extension=="m3u8")) && (QFile::exists(file)) ) {
		playlist->load_m3u(file);
	} 
	else
	if (extension=="pls") {
		playlist->load_pls(file);
	}
	else 
	if (QFileInfo(file).isDir()) {
		openDirectory(file);
	} 
	else {
		// Let the core to open it, autodetecting the file type
		//if (playlist->maybeSave()) {
		//	playlist->clear();
		//	playlist->addFile(file);

			core->open(file);
		//}
	}

	if (QFile::exists(file)) pref->latest_dir = QFileInfo(file).absolutePath();
}

void BaseGui::openFiles(QStringList files) {
	qDebug("BaseGui::openFiles");
	if (files.empty()) return;

	if (files.count()==1) {
		if (pref->auto_add_to_playlist) {
			if (playlist->maybeSave()) {
				playlist->clear();
				playlist->addFile(files[0], Playlist::NoGetInfo);

				open(files[0]);
			}
		} else {
			open(files[0]);
		}
	} else {
		if (playlist->maybeSave()) {
			playlist->clear();
			playlist->addFiles(files);
			open(files[0]);
		}
	}
}

void BaseGui::openFavorite(QString file) {
	qDebug("BaseGui::openFavorite");

	openFiles(QStringList() << file);
}

void BaseGui::openURL() {
	qDebug("BaseGui::openURL");

	exitFullscreenIfNeeded();

	/*
    bool ok;
    QString s = QInputDialog::getText(this, 
            tr("SMPlayer - Enter URL"), tr("URL:"), QLineEdit::Normal,
            pref->last_url, &ok );

    if ( ok && !s.isEmpty() ) {

		//playlist->clear();
		//playlistdock->hide();

		openURL(s);
    } else {
        // user entered nothing or pressed Cancel
    }
	*/

	InputURL d(this);

	for (int n=0; n < pref->history_urls->count(); n++) {
		d.setURL( pref->history_urls->url(n) );
	}

	if (d.exec() == QDialog::Accepted ) {
		QString url = d.url();
		if (!url.isEmpty()) {
			pref->history_urls->addUrl(url);
			openURL(url);
		}
	}
}

void BaseGui::openURL(QString url) {
	if (!url.isEmpty()) {
		//pref->history_urls->addUrl(url);

		if (pref->auto_add_to_playlist) {
			if (playlist->maybeSave()) {
				core->openStream(url);

				playlist->clear();
				playlist->addFile(url, Playlist::NoGetInfo);
			}
		} else {
			core->openStream(url);
		}
	}
}


void BaseGui::openFile() {
	qDebug("BaseGui::fileOpen");

	exitFullscreenIfNeeded();

	Extensions e;
    QString s = MyFileDialog::getOpenFileName(
                       this, tr("Choose a file"), pref->latest_dir, 
                       tr("Multimedia") + e.allPlayable().forFilter()+";;" +
                       tr("Video") + e.video().forFilter()+";;" +
                       tr("Audio") + e.audio().forFilter()+";;" +
                       tr("Playlists") + e.playlist().forFilter()+";;" +
                       tr("All files") +" (*.*)" );

    if ( !s.isEmpty() ) {
		openFile(s);
	}
}

void BaseGui::openFile(QString file) {
	qDebug("BaseGui::openFile: '%s'", file.toUtf8().data());

   if ( !file.isEmpty() ) {

		//playlist->clear();
		//playlistdock->hide();

		// If file is a playlist, open that playlist
		QString extension = QFileInfo(file).suffix().toLower();
		if ( (extension=="m3u") || (extension=="m3u8") ) {
			playlist->load_m3u(file);
		} 
		else
		if (extension=="pls") {
			playlist->load_pls(file);
		}
		else
		if (extension=="iso") {
			if (playlist->maybeSave()) {
				core->open(file);
			}
		}
		else {
			if (pref->auto_add_to_playlist) {
				if (playlist->maybeSave()) {
					core->openFile(file);

					playlist->clear();
					playlist->addFile(file, Playlist::NoGetInfo);
				}
			} else {
				core->openFile(file);
			}
		}
		if (QFile::exists(file)) pref->latest_dir = QFileInfo(file).absolutePath();
	}
}

void BaseGui::configureDiscDevices() {
	QMessageBox::information( this, tr("SMPlayer - Information"),
			tr("The CDROM / DVD drives are not configured yet.\n"
			   "The configuration dialog will be shown now, "
               "so you can do it."), QMessageBox::Ok);
	
	showPreferencesDialog();
	pref_dialog->showSection( PreferencesDialog::Drives );
}

void BaseGui::openVCD() {
	qDebug("BaseGui::openVCD");

	if ( (pref->dvd_device.isEmpty()) || 
         (pref->cdrom_device.isEmpty()) )
	{
		configureDiscDevices();
	} else {
		if (playlist->maybeSave()) {
			core->openVCD( pref->vcd_initial_title );
		}
	}
}

void BaseGui::openAudioCD() {
	qDebug("BaseGui::openAudioCD");

	if ( (pref->dvd_device.isEmpty()) || 
         (pref->cdrom_device.isEmpty()) )
	{
		configureDiscDevices();
	} else {
		if (playlist->maybeSave()) {
			core->openAudioCD();
		}
	}
}

void BaseGui::openDVD() {
	qDebug("BaseGui::openDVD");

	if ( (pref->dvd_device.isEmpty()) || 
         (pref->cdrom_device.isEmpty()) )
	{
		configureDiscDevices();
	} else {
		if (playlist->maybeSave()) {
#if DVDNAV_SUPPORT
			core->openDVD( DiscName::joinDVD(pref->use_dvdnav ? 0: 1, pref->dvd_device, pref->use_dvdnav) );
#else
			core->openDVD( DiscName::joinDVD(1, pref->dvd_device, false) );
#endif
		}
	}
}

void BaseGui::openDVDFromFolder() {
	qDebug("BaseGui::openDVDFromFolder");

	if (playlist->maybeSave()) {
		InputDVDDirectory *d = new InputDVDDirectory(this);
		d->setFolder( pref->last_dvd_directory );

		if (d->exec() == QDialog::Accepted) {
			qDebug("BaseGui::openDVDFromFolder: accepted");
			openDVDFromFolder( d->folder() );
		}

		delete d;
	}
}

void BaseGui::openDVDFromFolder(QString directory) {
	pref->last_dvd_directory = directory;
#if DVDNAV_SUPPORT
	core->openDVD( DiscName::joinDVD(pref->use_dvdnav ? 0: 1, directory, pref->use_dvdnav) );
#else
	core->openDVD( DiscName::joinDVD(1, directory, false) );
#endif
}

void BaseGui::openDirectory() {
	qDebug("BaseGui::openDirectory");

	QString s = MyFileDialog::getExistingDirectory(
                    this, tr("Choose a directory"),
                    pref->latest_dir );

	if (!s.isEmpty()) {
		openDirectory(s);
	}
}

void BaseGui::openDirectory(QString directory) {
	qDebug("BaseGui::openDirectory: '%s'", directory.toUtf8().data());

	if (Helper::directoryContainsDVD(directory)) {
		core->open(directory);
	} 
	else {
		QFileInfo fi(directory);
		if ( (fi.exists()) && (fi.isDir()) ) {
			playlist->clear();
			//playlist->addDirectory(directory);
			playlist->addDirectory( fi.absoluteFilePath() );
			playlist->startPlay();
		} else {
			qDebug("BaseGui::openDirectory: directory is not valid");
		}
	}
}

void BaseGui::loadSub() {
	qDebug("BaseGui::loadSub");

	exitFullscreenIfNeeded();

	Extensions e;
    QString s = MyFileDialog::getOpenFileName(
        this, tr("Choose a file"), 
	    pref->latest_dir, 
        tr("Subtitles") + e.subtitles().forFilter()+ ";;" +
        tr("All files") +" (*.*)" );

	if (!s.isEmpty()) core->loadSub(s);
}

void BaseGui::setInitialSubtitle(const QString & subtitle_file) {
	qDebug("BaseGui::setInitialSubtitle: '%s'", subtitle_file.toUtf8().constData());

	core->setInitialSubtitle(subtitle_file);
}

void BaseGui::loadAudioFile() {
	qDebug("BaseGui::loadAudioFile");

	exitFullscreenIfNeeded();

	Extensions e;
	QString s = MyFileDialog::getOpenFileName(
        this, tr("Choose a file"), 
	    pref->latest_dir, 
        tr("Audio") + e.audio().forFilter()+";;" +
        tr("All files") +" (*.*)" );

	if (!s.isEmpty()) core->loadAudioFile(s);
}

void BaseGui::helpFAQ() {
	QUrl url = QUrl::fromLocalFile(Paths::doc("faq.html", pref->language));
	qDebug("BaseGui::helpFAQ: file to open %s", url.toString().toUtf8().data());
	QDesktopServices::openUrl( url );
}

void BaseGui::helpCLOptions() {
	if (clhelp_window == 0) {
		clhelp_window = new LogWindow(this);
	}
	clhelp_window->setWindowTitle( tr("SMPlayer command line options") );
	clhelp_window->setHtml(CLHelp::help(true));
	clhelp_window->show();
}

void BaseGui::helpCheckUpdates() {
	QString url = "http://smplayer.sourceforge.net/latest.php";
	if (!pref->language.isEmpty()) url += QString("?tr_lang=%1").arg(pref->language);
	QDesktopServices::openUrl( QUrl(url) );
}

void BaseGui::helpDonate() {
	QMessageBox d(QMessageBox::NoIcon, tr("Donate"), 
		tr("If you like SMPlayer, a really good way to support it is by sending a donation, even the smallest one is highly appreciated.") + "<br>" +
        tr("You can send your donation using %1.").arg("<a href=\"https://sourceforge.net/donate/index.php?group_id=185512\">"+tr("this form")),
		QMessageBox::Ok, this);
	d.setIconPixmap( Images::icon("logo", 64) );
	d.exec();
}

void BaseGui::helpAbout() {
	About d(this);
	d.exec();
}

void BaseGui::helpAboutQt() {
	QMessageBox::aboutQt(this, tr("About Qt") );
}

void BaseGui::showGotoDialog() {
	TimeDialog d(this);
	d.setLabel(tr("&Jump to:"));
	d.setWindowTitle(tr("SMPlayer - Seek"));
	d.setMaximumTime( (int) core->mdat.duration);
	d.setTime( (int) core->mset.current_sec);
	if (d.exec() == QDialog::Accepted) {
		core->goToSec( d.time() );
	}
}

void BaseGui::showAudioDelayDialog() {
	bool ok;
	int delay = QInputDialog::getInteger(this, tr("SMPlayer - Audio delay"),
                                         tr("Audio delay (in milliseconds):"), core->mset.audio_delay, 
                                         -3600000, 3600000, 1, &ok);
	if (ok) {
		core->setAudioDelay(delay);
	}
}

void BaseGui::showSubDelayDialog() {
	bool ok;
	int delay = QInputDialog::getInteger(this, tr("SMPlayer - Subtitle delay"),
                                         tr("Subtitle delay (in milliseconds):"), core->mset.sub_delay, 
                                         -3600000, 3600000, 1, &ok);
	if (ok) {
		core->setSubDelay(delay);
	}
}

void BaseGui::exitFullscreen() {
	if (pref->fullscreen) {
		toggleFullscreen(false);
	}
}

void BaseGui::toggleFullscreen() {
	qDebug("BaseGui::toggleFullscreen");

	toggleFullscreen(!pref->fullscreen);
}

void BaseGui::toggleFullscreen(bool b) {
	qDebug("BaseGui::toggleFullscreen: %d", b);

	if (b==pref->fullscreen) {
		// Nothing to do
		qDebug("BaseGui::toggleFullscreen: nothing to do, returning");
		return;
	}

	pref->fullscreen = b;

	// If using mplayer window
	if (pref->use_mplayer_window) {
		core->tellmp("vo_fullscreen " + QString::number(b) );
		updateWidgets();
		return;
	}

	if (!panel->isVisible()) return; // mplayer window is not used.

	if (pref->fullscreen) {
		compactAct->setEnabled(false);

		if (pref->restore_pos_after_fullscreen) {
			win_pos = pos();
			win_size = size();
		}

		was_maximized = isMaximized();
		qDebug("BaseGui::toggleFullscreen: was_maximized: %d", was_maximized);

		aboutToEnterFullscreen();

		#ifdef Q_OS_WIN
		// Hack to avoid the windows taskbar to be visible on Windows XP
		if (QSysInfo::WindowsVersion < QSysInfo::WV_VISTA) {
			if (!pref->pause_when_hidden) hide();
		}
		#endif

		showFullScreen();

	} else {
		showNormal();

		if (was_maximized) showMaximized(); // It has to be called after showNormal()

		aboutToExitFullscreen();

		if (pref->restore_pos_after_fullscreen) {
			move( win_pos );
			resize( win_size );
		}

		compactAct->setEnabled(true);
	}

	updateWidgets();

	if ((pref->add_blackborders_on_fullscreen) && 
        (!core->mset.add_letterbox)) 
	{
		core->restart();
	}

	setFocus(); // Fixes bug #2493415
}


void BaseGui::aboutToEnterFullscreen() {
	if (!pref->compact_mode) {
		menuBar()->hide();
		statusBar()->hide();
	}
}

void BaseGui::aboutToExitFullscreen() {
	if (!pref->compact_mode) {
		menuBar()->show();
		statusBar()->show();
	}
}


void BaseGui::leftClickFunction() {
	qDebug("BaseGui::leftClickFunction");

	if (!pref->mouse_left_click_function.isEmpty()) {
		processFunction(pref->mouse_left_click_function);
	}
}

void BaseGui::rightClickFunction() {
	qDebug("BaseGui::rightClickFunction");

	if (!pref->mouse_right_click_function.isEmpty()) {
		processFunction(pref->mouse_right_click_function);
	}
}

void BaseGui::doubleClickFunction() {
	qDebug("BaseGui::doubleClickFunction");

	if (!pref->mouse_double_click_function.isEmpty()) {
		processFunction(pref->mouse_double_click_function);
	}
}

void BaseGui::middleClickFunction() {
	qDebug("BaseGui::middleClickFunction");

	if (!pref->mouse_middle_click_function.isEmpty()) {
		processFunction(pref->mouse_middle_click_function);
	}
}

void BaseGui::xbutton1ClickFunction() {
	qDebug("BaseGui::xbutton1ClickFunction");

	if (!pref->mouse_xbutton1_click_function.isEmpty()) {
		processFunction(pref->mouse_xbutton1_click_function);
	}
}

void BaseGui::xbutton2ClickFunction() {
	qDebug("BaseGui::xbutton2ClickFunction");

	if (!pref->mouse_xbutton2_click_function.isEmpty()) {
		processFunction(pref->mouse_xbutton2_click_function);
	}
}

void BaseGui::processFunction(QString function) {
	qDebug("BaseGui::processFunction: '%s'", function.toUtf8().data());

	//parse args for checkable actions
	QRegExp func_rx("(.*) (true|false)");
	bool value = false;
	bool checkableFunction = false;

	if(func_rx.indexIn(function) > -1){
		function = func_rx.cap(1);
		value = (func_rx.cap(2) == "true");
		checkableFunction = true;
	} //end if

	QAction * action = ActionsEditor::findAction(this, function);
	if (!action) action = ActionsEditor::findAction(playlist, function);

	if (action) {
		qDebug("BaseGui::processFunction: action found");

		if (!action->isEnabled()) {
			qDebug("BaseGui::processFunction: action is disabled, doing nothing");
			return;
		}

		if (action->isCheckable()){
			if(checkableFunction)
				action->setChecked(value);
			else
				//action->toggle();
				action->trigger();
		}else{
			action->trigger();
		}
	}
}

void BaseGui::runActions(QString actions) {
	qDebug("BaseGui::runActions");

	actions = actions.simplified(); // Remove white space

	QAction * action;
	QStringList actionsList = actions.split(" ");

	for (int n = 0; n < actionsList.count(); n++) {
		QString actionStr = actionsList[n];
		QString par = ""; //the parameter which the action takes

		//set par if the next word is a boolean value
		if ( (n+1) < actionsList.count() ) {
			if ( (actionsList[n+1].toLower() == "true") || (actionsList[n+1].toLower() == "false") ) {
				par = actionsList[n+1].toLower();
				n++;
			} //end if
		} //end if

		action = ActionsEditor::findAction(this, actionStr);
		if (!action) action = ActionsEditor::findAction(playlist, actionStr);

		if (action) {
			qDebug("BaseGui::runActions: running action: '%s' (par: '%s')",
				   actionStr.toUtf8().data(), par.toUtf8().data() );

			if (action->isCheckable()) {
				if (par.isEmpty()) {
					//action->toggle();
					action->trigger();
				} else {
					action->setChecked( (par == "true") );
				} //end if
			} else {
				action->trigger();
			} //end if
		} else {
			qWarning("BaseGui::runActions: action: '%s' not found",actionStr.toUtf8().data());
		} //end if
	} //end for
}

void BaseGui::checkPendingActionsToRun() {
	qDebug("BaseGui::checkPendingActionsToRun");

	QString actions;
	if (!pending_actions_to_run.isEmpty()) {
		actions = pending_actions_to_run;
		pending_actions_to_run.clear();
		if (!pref->actions_to_run.isEmpty()) {
			actions = pref->actions_to_run +" "+ actions;
		}
	} else {
		actions = pref->actions_to_run;
	}

	if (!actions.isEmpty()) {
		qDebug("BaseGui::checkPendingActionsToRun: actions: '%s'", actions.toUtf8().constData());
		runActions(actions);
	}
}

#if REPORT_OLD_MPLAYER
void BaseGui::checkMplayerVersion() {
	qDebug("BaseGui::checkMplayerVersion");

	// Qt 4.3.5 is crazy, I can't popup a messagebox here, it calls 
	// this function once and again when the messagebox is shown

	if ( (pref->mplayer_detected_version > 0) && (!MplayerVersion::isMplayerAtLeast(25158)) ) {
		QTimer::singleShot(1000, this, SLOT(displayWarningAboutOldMplayer()));
	}
}

void BaseGui::displayWarningAboutOldMplayer() {
	qDebug("BaseGui::displayWarningAboutOldMplayer");

	if (!pref->reported_mplayer_is_old) {
		QMessageBox::warning(this, tr("Warning - Using old MPlayer"),
			tr("The version of MPlayer (%1) installed on your system "
               "is obsolete. SMPlayer can't work well with it: some "
               "options won't work, subtitle selection may fail...")
               .arg(MplayerVersion::toString(pref->mplayer_detected_version)) +
            "<br><br>" + 
            tr("Please, update your MPlayer.") +
            "<br><br>" + 
            tr("(This warning won't be displayed anymore)") );

		pref->reported_mplayer_is_old = true;
	}
	//else
	//statusBar()->showMessage( tr("Using an old MPlayer, please update it"), 10000 );
}
#endif

void BaseGui::dragEnterEvent( QDragEnterEvent *e ) {
	qDebug("BaseGui::dragEnterEvent");

	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}



void BaseGui::dropEvent( QDropEvent *e ) {
	qDebug("BaseGui::dropEvent");

	QStringList files;

	if (e->mimeData()->hasUrls()) {
		QList <QUrl> l = e->mimeData()->urls();
		QString s;
		for (int n=0; n < l.count(); n++) {
			if (l[n].isValid()) {
				qDebug("BaseGui::dropEvent: scheme: '%s'", l[n].scheme().toUtf8().data());
				if (l[n].scheme() == "file") 
					s = l[n].toLocalFile();
				else
					s = l[n].toString();
				/*
				qDebug(" * '%s'", l[n].toString().toUtf8().data());
				qDebug(" * '%s'", l[n].toLocalFile().toUtf8().data());
				*/
				qDebug("BaseGui::dropEvent: file: '%s'", s.toUtf8().data());
				files.append(s);
			}
		}
	}


	qDebug( "BaseGui::dropEvent: count: %d", files.count());
	if (files.count() > 0) {
		if (files.count() == 1) {
			QFileInfo fi( files[0] );

			Extensions e;
			QRegExp ext_sub(e.subtitles().forRegExp());
			ext_sub.setCaseSensitivity(Qt::CaseInsensitive);
			if (ext_sub.indexIn(fi.suffix()) > -1) {
				qDebug( "BaseGui::dropEvent: loading sub: '%s'", files[0].toUtf8().data());
				core->loadSub( files[0] );
			}
			else
			if (fi.isDir()) {
				openDirectory( files[0] );
			} else {
				//openFile( files[0] );
				if (pref->auto_add_to_playlist) {
					if (playlist->maybeSave()) {
						playlist->clear();
						playlist->addFile(files[0], Playlist::NoGetInfo);

						open( files[0] );
					}
				} else {
					open( files[0] );
				}
			}
		} else {
			// More than one file
			qDebug("BaseGui::dropEvent: adding files to playlist");
			playlist->clear();
			playlist->addFiles(files);
			//openFile( files[0] );
			playlist->startPlay();
		}
	}
}

void BaseGui::showPopupMenu() {
	showPopupMenu(QCursor::pos());
}

void BaseGui::showPopupMenu( QPoint p ) {
	//qDebug("BaseGui::showPopupMenu: %d, %d", p.x(), p.y());
	popup->move( p );
	popup->show();
}

/*
void BaseGui::mouseReleaseEvent( QMouseEvent * e ) {
	qDebug("BaseGui::mouseReleaseEvent");

	if (e->button() == Qt::LeftButton) {
		e->accept();
		emit leftClicked();
	}
	else
	if (e->button() == Qt::MidButton) {
		e->accept();
		emit middleClicked();
	}
	//
	//else
	//if (e->button() == Qt::RightButton) {
	//	showPopupMenu( e->globalPos() );
    //}
	//
	else 
		e->ignore();
}

void BaseGui::mouseDoubleClickEvent( QMouseEvent * e ) {
	e->accept();
	emit doubleClicked();
}
*/

void BaseGui::wheelEvent( QWheelEvent * e ) {
	qDebug("BaseGui::wheelEvent: delta: %d", e->delta());
	e->accept();

	if (e->orientation() == Qt::Vertical) {
	    if (e->delta() >= 0)
	        emit wheelUp();
	    else
	        emit wheelDown();
	} else {
		qDebug("BaseGui::wheelEvent: horizontal event received, doing nothing");
	}
}


// Called when a video has started to play
void BaseGui::enterFullscreenOnPlay() {
	qDebug("BaseGui::enterFullscreenOnPlay: arg_start_in_fullscreen: %d, pref->start_in_fullscreen: %d", arg_start_in_fullscreen, pref->start_in_fullscreen);

	if (arg_start_in_fullscreen != 0) {
		if ( (arg_start_in_fullscreen == 1) || (pref->start_in_fullscreen) ) {
			if (!pref->fullscreen) toggleFullscreen(TRUE);
		}
	}
}

// Called when the playlist has stopped
void BaseGui::exitFullscreenOnStop() {
    if (pref->fullscreen) {
		toggleFullscreen(FALSE);
	}
}

void BaseGui::playlistHasFinished() {
	qDebug("BaseGui::playlistHasFinished");
	exitFullscreenOnStop();

	qDebug("BaseGui::playlistHasFinished: arg_close_on_finish: %d, pref->close_on_finish: %d", arg_close_on_finish, pref->close_on_finish);

	if (arg_close_on_finish != 0) {
		if ((arg_close_on_finish == 1) || (pref->close_on_finish)) exitAct->trigger();
	}
}

void BaseGui::displayState(Core::State state) {
	qDebug("BaseGui::displayState: %s", core->stateToString().toUtf8().data());
	switch (state) {
		case Core::Playing:	statusBar()->showMessage( tr("Playing %1").arg(core->mdat.filename), 2000); break;
		case Core::Paused:	statusBar()->showMessage( tr("Pause") ); break;
		case Core::Stopped:	statusBar()->showMessage( tr("Stop") , 2000); break;
	}
	if (state == Core::Stopped) setWindowCaption( "SMPlayer" );

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	/* Disable screensaver by event */
	just_stopped = false;
	
	if (state == Core::Stopped) {
		just_stopped = true;
		int time = 1000 * 60; // 1 minute
		QTimer::singleShot( time, this, SLOT(clear_just_stopped()) );
	}
#endif
}

void BaseGui::displayMessage(QString message) {
	statusBar()->showMessage(message, 2000);
}

void BaseGui::gotCurrentTime(double sec) {
	//qDebug( "DefaultGui::displayTime: %f", sec);

	static int last_second = 0;

	if (floor(sec)==last_second) return; // Update only once per second
	last_second = (int) floor(sec);

	QString time = Helper::formatTime( (int) sec ) + " / " +
                           Helper::formatTime( (int) core->mdat.duration );

	//qDebug( " duration: %f, current_sec: %f", core->mdat.duration, core->mset.current_sec);

	emit timeChanged( time );
}

void BaseGui::resizeWindow(int w, int h) {
	qDebug("BaseGui::resizeWindow: %d, %d", w, h);

	// If fullscreen, don't resize!
	if (pref->fullscreen) return;

	if ( (pref->resize_method==Preferences::Never) && (panel->isVisible()) ) {
		return;
	}

	if (!panel->isVisible()) {
		panel->show();

		// Enable compact mode
		//compactAct->setEnabled(true);
	}

	if (pref->size_factor != 100) {
		w = w * pref->size_factor / 100;
		h = h * pref->size_factor / 100;
	}

	qDebug("BaseGui::resizeWindow: size to scale: %d, %d", w, h);

	QSize video_size(w,h);

	if (video_size == panel->size()) {
		qDebug("BaseGui::resizeWindow: the panel size is already the required size. Doing nothing.");
		return;
	}

	int diff_width = this->width() - panel->width();
	int diff_height = this->height() - panel->height();

	int new_width = w + diff_width;
	int new_height = h + diff_height;

#if USE_MINIMUMSIZE
	int minimum_width = minimumSizeHint().width();
	if (pref->gui_minimum_width != 0) minimum_width = pref->gui_minimum_width;
	if (new_width < minimum_width) {
		qDebug("BaseGui::resizeWindow: width is too small, setting width to %d", minimum_width);
		new_width = minimum_width;
	}
#endif

	resize(new_width, new_height);

	qDebug("BaseGui::resizeWindow: done: window size: %d, %d", this->width(), this->height());
	qDebug("BaseGui::resizeWindow: done: panel->size: %d, %d", 
           panel->size().width(),  
           panel->size().height() );
	qDebug("BaseGui::resizeWindow: done: mplayerwindow->size: %d, %d", 
           mplayerwindow->size().width(),  
           mplayerwindow->size().height() );
}

void BaseGui::hidePanel() {
	qDebug("BaseGui::hidePanel");

	if (panel->isVisible()) {
		// Exit from fullscreen mode 
	    if (pref->fullscreen) { toggleFullscreen(false); update(); }

		// Exit from compact mode first
		if (pref->compact_mode) toggleCompactMode(false);

		//resizeWindow( size().width(), 0 );
		int width = size().width();
		if (width > pref->default_size.width()) width = pref->default_size.width();
		resize( width, size().height() - panel->size().height() );
		panel->hide();

		// Disable compact mode
		//compactAct->setEnabled(false);
	}
}

void BaseGui::displayGotoTime(int t) {
#ifdef SEEKBAR_RESOLUTION
	int jump_time = (int)core->mdat.duration * t / SEEKBAR_RESOLUTION;
#else
	int jump_time = (int)core->mdat.duration * t / 100;
#endif
	QString s = tr("Jump to %1").arg( Helper::formatTime(jump_time) );
	statusBar()->showMessage( s, 1000 );

	if (pref->fullscreen) {
		core->displayTextOnOSD( s );
	}
}

void BaseGui::goToPosOnDragging(int t) {
	if (pref->update_while_seeking) {
#if ENABLE_DELAYED_DRAGGING
		#ifdef SEEKBAR_RESOLUTION
		core->goToPosition(t);
		#else
		core->goToPos(t);
		#endif
#else
		if ( ( t % 4 ) == 0 ) {
			qDebug("BaseGui::goToPosOnDragging: %d", t);
			#ifdef SEEKBAR_RESOLUTION
			core->goToPosition(t);
			#else
			core->goToPos(t);
			#endif
		}
#endif
	}
}

void BaseGui::toggleCompactMode() {
	toggleCompactMode( !pref->compact_mode );
}

void BaseGui::toggleCompactMode(bool b) {
	qDebug("BaseGui::toggleCompactMode: %d", b);

	if (b) 
		aboutToEnterCompactMode();
	else
		aboutToExitCompactMode();

	pref->compact_mode = b;
	updateWidgets();
}

void BaseGui::aboutToEnterCompactMode() {
	menuBar()->hide();
	statusBar()->hide();
}

void BaseGui::aboutToExitCompactMode() {
	menuBar()->show();
	statusBar()->show();
}

void BaseGui::setStayOnTop(bool b) {
	qDebug("BaseGui::setStayOnTop: %d", b);

	if ( (b && (windowFlags() & Qt::WindowStaysOnTopHint)) ||
         (!b && (!(windowFlags() & Qt::WindowStaysOnTopHint))) )
	{
		// identical do nothing
		qDebug("BaseGui::setStayOnTop: nothing to do");
		return;
	}

	ignore_show_hide_events = true;

	bool visible = isVisible();

	QPoint old_pos = pos();

	if (b) {
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	}
	else {
		setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	}

	move(old_pos);

	if (visible) {
		show();
	}

	ignore_show_hide_events = false;
}

void BaseGui::changeStayOnTop(int stay_on_top) {
	switch (stay_on_top) {
		case Preferences::AlwaysOnTop : setStayOnTop(true); break;
		case Preferences::NeverOnTop  : setStayOnTop(false); break;
		case Preferences::WhilePlayingOnTop : setStayOnTop((core->state() == Core::Playing)); break;
	}

	pref->stay_on_top = (Preferences::OnTop) stay_on_top;
	updateWidgets();
}

void BaseGui::checkStayOnTop(Core::State state) {
	qDebug("BaseGui::checkStayOnTop");
    if ((!pref->fullscreen) && (pref->stay_on_top == Preferences::WhilePlayingOnTop)) {
		setStayOnTop((state == Core::Playing));
	}
}

void BaseGui::toggleStayOnTop() {
	if (pref->stay_on_top == Preferences::AlwaysOnTop) 
		changeStayOnTop(Preferences::NeverOnTop);
	else
	if (pref->stay_on_top == Preferences::NeverOnTop) 
		changeStayOnTop(Preferences::AlwaysOnTop);
}

// Called when a new window (equalizer, preferences..) is opened.
void BaseGui::exitFullscreenIfNeeded() {
	/*
	if (pref->fullscreen) {
		toggleFullscreen(FALSE);
	}
	*/
}

void BaseGui::checkMousePos(QPoint p) {
	//qDebug("BaseGui::checkMousePos: %d, %d", p.x(), p.y());

	bool compact = (pref->floating_display_in_compact_mode && pref->compact_mode);

	if (!pref->fullscreen && !compact) return;

	#define MARGIN 70

	int margin = MARGIN + pref->floating_control_margin;

	if (p.y() > mplayerwindow->height() - margin) {
		//qDebug("BaseGui::checkMousePos: %d, %d", p.x(), p.y());
		if (!near_bottom) {
			emit cursorNearBottom(p);
			near_bottom = true;
		}
	} else {
		if (near_bottom) {
			emit cursorFarEdges();
			near_bottom = false;
		}
	}

	if (p.y() < margin) {
		//qDebug("BaseGui::checkMousePos: %d, %d", p.x(), p.y());
		if (!near_top) {
			emit cursorNearTop(p);
			near_top = true;
		}
	} else {
		if (near_top) {
			emit cursorFarEdges();
			near_top = false;
		}
	}
}

#if ALLOW_CHANGE_STYLESHEET
void BaseGui::loadQss(QString filename) {
	QFile file( filename );
	file.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(file.readAll());

	qApp->setStyleSheet(styleSheet);
}

void BaseGui::changeStyleSheet(QString style) {
	if (style.isEmpty())  {
		qApp->setStyleSheet("");
	} 
	else {
		QString qss_file = Paths::configPath() + "/themes/" + pref->iconset +"/style.qss";
		//qDebug("BaseGui::changeStyleSheet: '%s'", qss_file.toUtf8().data());
		if (!QFile::exists(qss_file)) {
			qss_file = Paths::themesPath() +"/"+ pref->iconset +"/style.qss";
		}
		if (QFile::exists(qss_file)) {
			qDebug("BaseGui::changeStyleSheet: '%s'", qss_file.toUtf8().data());
			loadQss(qss_file);
		} else {
			qApp->setStyleSheet("");
		}
	}
}
#endif

void BaseGui::loadActions() {
	qDebug("BaseGui::loadActions");
	ActionsEditor::loadFromConfig(this, settings);
#if !DOCK_PLAYLIST
	ActionsEditor::loadFromConfig(playlist, settings);
#endif

	actions_list = ActionsEditor::actionsNames(this);
#if !DOCK_PLAYLIST
	actions_list += ActionsEditor::actionsNames(playlist);
#endif
}

void BaseGui::saveActions() {
	qDebug("BaseGui::saveActions");

	ActionsEditor::saveToConfig(this, settings);
#if !DOCK_PLAYLIST
	ActionsEditor::saveToConfig(playlist, settings);
#endif
}


void BaseGui::showEvent( QShowEvent * ) {
	qDebug("BaseGui::showEvent");

	if (ignore_show_hide_events) return;

	//qDebug("BaseGui::showEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if ((pref->pause_when_hidden) && (core->state() == Core::Paused)) {
		qDebug("BaseGui::showEvent: unpausing");
		core->pause(); // Unpauses
	}
}

void BaseGui::hideEvent( QHideEvent * ) {
	qDebug("BaseGui::hideEvent");

	if (ignore_show_hide_events) return;

	//qDebug("BaseGui::hideEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if ((pref->pause_when_hidden) && (core->state() == Core::Playing)) {
		qDebug("BaseGui::hideEvent: pausing");
		core->pause();
	}
}

void BaseGui::askForMplayerVersion(QString line) {
	qDebug("BaseGui::askForMplayerVersion: %s", line.toUtf8().data());

	if (pref->mplayer_user_supplied_version <= 0) {
		InputMplayerVersion d(this);
		d.setVersion( pref->mplayer_user_supplied_version );
		d.setVersionFromOutput(line);
		if (d.exec() == QDialog::Accepted) {
			pref->mplayer_user_supplied_version = d.version();
			qDebug("BaseGui::askForMplayerVersion: user supplied version: %d", pref->mplayer_user_supplied_version);
		}
	} else {
		qDebug("BaseGui::askForMplayerVersion: already have a version supplied by user, so no asking");
	}
}

void BaseGui::showExitCodeFromMplayer(int exit_code) {
	qDebug("BaseGui::showExitCodeFromMplayer: %d", exit_code);

	if (!pref->report_mplayer_crashes) {
		qDebug("BaseGui::showExitCodeFromMplayer: not displaying error dialog");
		return;
	}

	if (exit_code != 255 ) {
		ErrorDialog d(this);
		d.setText(tr("MPlayer has finished unexpectedly.") + " " + 
	              tr("Exit code: %1").arg(exit_code));
#ifdef LOG_MPLAYER
		d.setLog( mplayer_log );
#endif
		d.exec();
	} 
}

void BaseGui::showErrorFromMplayer(QProcess::ProcessError e) {
	qDebug("BaseGui::showErrorFromMplayer");

	if (!pref->report_mplayer_crashes) {
		qDebug("showErrorFromMplayer: not displaying error dialog");
		return;
	}

	if ((e == QProcess::FailedToStart) || (e == QProcess::Crashed)) {
		ErrorDialog d(this);
		if (e == QProcess::FailedToStart) {
			d.setText(tr("MPlayer failed to start.") + " " + 
                         tr("Please check the MPlayer path in preferences."));
		} else {
			d.setText(tr("MPlayer has crashed.") + " " + 
                      tr("See the log for more info."));
		}
#ifdef LOG_MPLAYER
		d.setLog( mplayer_log );
#endif
		d.exec();
	}
}


#ifdef FIND_SUBTITLES
void BaseGui::showFindSubtitlesDialog() {
	qDebug("BaseGui::showFindSubtitlesDialog");

	if (!find_subs_dialog) {
		find_subs_dialog = new FindSubtitlesWindow(this, Qt::Window | Qt::WindowMinMaxButtonsHint);
		find_subs_dialog->setSettings(Global::settings);
		find_subs_dialog->setWindowIcon(windowIcon());
#if DOWNLOAD_SUBS
		connect(find_subs_dialog, SIGNAL(subtitleDownloaded(const QString &)),
                core, SLOT(loadSub(const QString &)));
#endif
	}

	find_subs_dialog->show();
	find_subs_dialog->setMovie(core->mdat.filename);
}

void BaseGui::openUploadSubtitlesPage() {	
	//QDesktopServices::openUrl( QUrl("http://ds6.ovh.org/hashsubtitles/upload.php") );
	//QDesktopServices::openUrl( QUrl("http://www.opensubtitles.com/upload") );
	QDesktopServices::openUrl( QUrl("http://www.opensubtitles.org/uploadjava") );
}
#endif

#ifdef VIDEOPREVIEW
void BaseGui::showVideoPreviewDialog() {
	qDebug("BaseGui::showVideoPreviewDialog");

	if (video_preview == 0) {
		video_preview = new VideoPreview( pref->mplayer_bin, this );
		video_preview->setSettings(Global::settings);
	}

	if (!core->mdat.filename.isEmpty()) {
		video_preview->setVideoFile(core->mdat.filename);

		// DVD
		if (core->mdat.type==TYPE_DVD) {
			QString file = core->mdat.filename;
			DiscData disc_data = DiscName::split(file);
			QString dvd_folder = disc_data.device;
			if (dvd_folder.isEmpty()) dvd_folder = pref->dvd_device;
			int dvd_title = disc_data.title;
			file = disc_data.protocol + "://" + QString::number(dvd_title);

			video_preview->setVideoFile(file);
			video_preview->setDVDDevice(dvd_folder);
		} else {
			video_preview->setDVDDevice("");
		}
	}

	video_preview->setMplayerPath(pref->mplayer_bin);

	if ( (video_preview->showConfigDialog(this)) && (video_preview->createThumbnails()) ) {
		video_preview->show();
		video_preview->adjustWindowSize();
	}
}
#endif

void BaseGui::showTubeBrowser() {
	qDebug("BaseGui::showTubeBrowser");
	QString exec = Paths::appPath() + "/smtube";
	qDebug("BaseGui::showTubeBrowser: '%s'", exec.toUtf8().constData());
	if (!QProcess::startDetached(exec, QStringList())) {
		QMessageBox::warning(this, tr("An error happened - SMPlayer"), tr("The YouTube Browser couldn't be launched"));
	}
}

// Language change stuff
void BaseGui::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QMainWindow::changeEvent(e);
	}
}

#ifdef Q_OS_WIN
/* Disable screensaver by event */
bool BaseGui::winEvent ( MSG * m, long * result ) {
	//qDebug("BaseGui::winEvent");
	if (m->message==WM_SYSCOMMAND) {
		if ((m->wParam & 0xFFF0)==SC_SCREENSAVE || (m->wParam & 0xFFF0)==SC_MONITORPOWER) {
			qDebug("BaseGui::winEvent: received SC_SCREENSAVE or SC_MONITORPOWER");
			qDebug("BaseGui::winEvent: avoid_screensaver: %d", pref->avoid_screensaver);
			qDebug("BaseGui::winEvent: playing: %d", core->state()==Core::Playing);
			qDebug("BaseGui::winEvent: video: %d", !core->mdat.novideo);
			
			if ((pref->avoid_screensaver) && (core->state()==Core::Playing) && (!core->mdat.novideo)) {
				qDebug("BaseGui::winEvent: not allowing screensaver");
				(*result) = 0;
				return true;
			} else {
				if ((pref->avoid_screensaver) && (just_stopped)) {
					qDebug("BaseGui::winEvent: file just stopped, so not allowing screensaver for a while");
					(*result) = 0;
					return true;
				} else {
					qDebug("BaseGui::winEvent: allowing screensaver");
					return false;
				}
			}
		}
	}
	return false;
}
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
void BaseGui::clear_just_stopped() {
	qDebug("BaseGui::clear_just_stopped");
	just_stopped = false;
}
#endif

#include "moc_basegui.cpp"

TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

QT += network xml
#QT +=  opengl 

RESOURCES = icons.qrc

INCLUDEPATH += corelib findsubtitles
DEPENDPATH += corelib findsubtitles

#DEFINES += USE_QXT

HEADERS	+= guiconfig.h \
    corelib/config.h \
	corelib/constants.h \
	svn_revision.h \
	version.h \
	corelib/global.h \
	corelib/helper.h \
	translator.h \
	corelib/subtracks.h \
	corelib/trackdata.h \
	corelib/tracks.h \
	extensions.h \
	corelib/desktopinfo.h \
	corelib/myprocess.h \
	corelib/mplayerversion.h \
	corelib/mplayerprocess.h \
    corelib/infoprovider.h \
	corelib/mplayerwindow.h \
	corelib/mediadata.h \
	corelib/audioequalizerlist.h \
	corelib/mediasettings.h \
	corelib/preferences.h \
	images.h \
	inforeader.h \
	recents.h \
	corelib/core.h \
	logwindow.h \
	infofile.h \
	seekwidget.h \
	mytablewidget.h \
	shortcutgetter.h \
	actionseditor.h \
    filechooser.h \
	preferencesdialog.h \
	mycombobox.h \
	tristatecombo.h \
    languages.h \
	prefwidget.h \
	prefgeneral.h \
	prefdrives.h \
	prefinterface.h \
	prefperformance.h \
	prefinput.h \
	prefsubtitles.h \
	prefadvanced.h \
	filepropertiesdialog.h \
	playlist.h \
	playlistpreferences.h \
	playlistdock.h \
	verticaltext.h \
	eqslider.h \
	videoequalizer.h \
	audioequalizer.h \
    myslider.h \
	timeslider.h \
	inputdvddirectory.h \
	inputurl.h \
	myaction.h \
	myactiongroup.h \
	myserver.h \
	myclient.h \
	filedialog.h \
	inputmplayerversion.h \
	about.h \
    errordialog.h \
    timedialog.h \
    findsubtitles/simplehttp.h \
    findsubtitles/osparser.h \
    findsubtitles/findsubtitleswindow.h \
	basegui.h \
	baseguiplus.h \
	floatingwidget.h \
	widgetactions.h \
	toolbareditor.h \
	defaultgui.h \
	minigui.h \
	smplayer.h \
	clhelp.h


SOURCES	+= version.cpp \
	corelib/global.cpp \
	corelib/helper.cpp \
	translator.cpp \
	corelib/subtracks.cpp \
	corelib/trackdata.cpp \
	corelib/tracks.cpp \
	extensions.cpp \
	corelib/desktopinfo.cpp \
	corelib/myprocess.cpp \
	corelib/mplayerversion.cpp \
	corelib/mplayerprocess.cpp \
    corelib/infoprovider.cpp \
	corelib/mplayerwindow.cpp \
	corelib/mediadata.cpp \
	corelib/mediasettings.cpp \
	corelib/preferences.cpp \
	images.cpp \
	inforeader.cpp \
	recents.cpp \
	corelib/core.cpp \
	logwindow.cpp \
	infofile.cpp \
	seekwidget.cpp \
	mytablewidget.cpp \
	shortcutgetter.cpp \
	actionseditor.cpp \
    filechooser.cpp \
	preferencesdialog.cpp \
	mycombobox.cpp \
	tristatecombo.cpp \
    languages.cpp \
	prefwidget.cpp \
	prefgeneral.cpp \
	prefdrives.cpp \
	prefinterface.cpp \
	prefperformance.cpp \
	prefinput.cpp \
	prefsubtitles.cpp \
	prefadvanced.cpp \
	filepropertiesdialog.cpp \
	playlist.cpp \
	playlistpreferences.cpp \
	playlistdock.cpp \
	verticaltext.cpp \
	eqslider.cpp \
	videoequalizer.cpp \
	audioequalizer.cpp \
    myslider.cpp \
	timeslider.cpp \
	inputdvddirectory.cpp \
	inputurl.cpp \
	myaction.cpp \
	myactiongroup.cpp \
	myserver.cpp \
	myclient.cpp \
	filedialog.cpp \
	inputmplayerversion.cpp \
	about.cpp \
    errordialog.cpp \
    timedialog.cpp \
    findsubtitles/simplehttp.cpp \
    findsubtitles/osparser.cpp \
    findsubtitles/findsubtitleswindow.cpp \
	basegui.cpp \
	baseguiplus.cpp \
	floatingwidget.cpp \
	widgetactions.cpp \
	toolbareditor.cpp \
	defaultgui.cpp \
	minigui.cpp \
	clhelp.cpp \
	smplayer.cpp \
	main.cpp

#libqxt
contains(DEFINES, USE_QXT) {
	CONFIG  += qxt
	QXT     += core
}

FORMS = inputdvddirectory.ui logwindowbase.ui filepropertiesdialog.ui \
        eqslider.ui seekwidget.ui inputurl.ui \
        preferencesdialog.ui prefgeneral.ui prefdrives.ui prefinterface.ui \
        prefperformance.ui prefinput.ui prefsubtitles.ui prefadvanced.ui \
        about.ui inputmplayerversion.ui errordialog.ui timedialog.ui \
        playlistpreferences.ui filechooser.ui \
        findsubtitles/findsubtitleswindow.ui

TRANSLATIONS = translations/smplayer_es.ts translations/smplayer_de.ts \
               translations/smplayer_sk.ts translations/smplayer_it.ts \
               translations/smplayer_fr.ts translations/smplayer_zh_CN.ts \
               translations/smplayer_ru_RU.ts translations/smplayer_hu.ts \
               translations/smplayer_en_US.ts translations/smplayer_pl.ts \
               translations/smplayer_ja.ts translations/smplayer_nl.ts \
               translations/smplayer_uk_UA.ts translations/smplayer_pt_BR.ts \
               translations/smplayer_ka.ts translations/smplayer_cs.ts \
               translations/smplayer_bg.ts translations/smplayer_tr.ts \
               translations/smplayer_sv.ts translations/smplayer_sr.ts \
               translations/smplayer_zh_TW.ts translations/smplayer_ro_RO.ts \
               translations/smplayer_pt_PT.ts translations/smplayer_el_GR.ts \
               translations/smplayer_fi.ts translations/smplayer_ko.ts \
               translations/smplayer_mk.ts translations/smplayer_eu.ts \
               translations/smplayer_ca.ts translations/smplayer_sl_SI.ts \
               translations/smplayer_ar_SY.ts translations/smplayer_ku.ts

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj

  DEFINES += DATA_PATH=$(DATA_PATH)
  DEFINES += DOC_PATH=$(DOC_PATH)
  DEFINES += TRANSLATION_PATH=$(TRANSLATION_PATH)
  DEFINES += CONF_PATH=$(CONF_PATH)
  DEFINES += THEMES_PATH=$(THEMES_PATH)
  DEFINES += SHORTCUTS_PATH=$(SHORTCUTS_PATH)
  #DEFINES += NO_DEBUG_ON_CONSOLE

  #DEFINES += KDE_SUPPORT
  #INCLUDEPATH += /opt/kde3/include/
  #LIBS += -lkio -L/opt/kde3/lib/

  #contains( DEFINES, KDE_SUPPORT) {
  #	HEADERS += mysystemtrayicon.h
  #	SOURCES += mysystemtrayicon.cpp
  #}

  #HEADERS += 	prefassociations.h winfileassoc.h
  #SOURCES += 	prefassociations.cpp winfileassoc.cpp
  #FORMS += prefassociations.ui
}

win32 {
	HEADERS += corelib/screensaver.h
	SOURCES += corelib/screensaver.cpp

	!contains( DEFINES, PORTABLE_APP ) {
		DEFINES += USE_ASSOCIATIONS
	}
	
	contains( DEFINES, USE_ASSOCIATIONS ) {
		HEADERS += 	prefassociations.h winfileassoc.h
		SOURCES += 	prefassociations.cpp winfileassoc.cpp
		FORMS += prefassociations.ui
	}

	contains(TEMPLATE,vcapp) {
		LIBS += ole32.lib user32.lib
	} else {
		LIBS += libole32
	}
	
	RC_FILE = smplayer.rc
	DEFINES += NO_DEBUG_ON_CONSOLE
#	debug {
#		CONFIG += console
#	}
}


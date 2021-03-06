TEMPLATE = app
LANGUAGE = C++

CONFIG += qt warn_on release

QT += network xml

INCLUDEPATH += ..
DEPENDPATH += ..

HEADERS += simplehttp.h \
           osparser.h \
           ../filehash.h \
           ../lineedit_with_icon.h \
           ../filechooser.h \
           ../languages.h \
           fixsubs.h \
           findsubtitlesconfigdialog.h \
           findsubtitleswindow.h

SOURCES += simplehttp.cpp \
           osparser.cpp \
           ../filehash.cpp \
           ../lineedit_with_icon.cpp \
           ../filechooser.cpp \
           ../languages.cpp \
           fixsubs.cpp \
           findsubtitlesconfigdialog.cpp \
           findsubtitleswindow.cpp \
           main.cpp

FORMS += findsubtitleswindow.ui findsubtitlesconfigdialog.ui

DEFINES += NO_SMPLAYER_SUPPORT DOWNLOAD_SUBS

contains( DEFINES, DOWNLOAD_SUBS ) {
	INCLUDEPATH += filedownloader quazip
	DEPENDPATH += filedownloader quazip

	HEADERS += filedownloader.h subchooserdialog.h
	SOURCES += filedownloader.cpp subchooserdialog.cpp

	FORMS += subchooserdialog.ui

	HEADERS += crypt.h \
	           ioapi.h \
	           quazip.h \
	           quazipfile.h \
	           quazipfileinfo.h \
	           quazipnewinfo.h \
	           unzip.h \
	           zip.h

	SOURCES += ioapi.c \
	           quazip.cpp \
	           quazipfile.cpp \
	           quazipnewinfo.cpp \
	           unzip.c \
	           zip.c

	LIBS += -lz
	
	win32 {
		INCLUDEPATH += c:\development\zlib-1.2.3
		LIBS += -Lc:\development\zlib-1.2.3
	}
}

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

win32 {
  CONFIG += console
}


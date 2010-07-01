TEMPLATE = app
HEADERS = mainwindow.h infinite_scroll.h bible_text_source.h search_dialog.h \
		search_results.h select_dialog.h settings_dialog.h install_dialog.h \
		select_translation.h
SOURCES = main.cpp mainwindow.cpp infinite_scroll.cpp bible_text_source.cpp \
		search_dialog.cpp search_results.cpp select_dialog.cpp \
		settings_dialog.cpp install_dialog.cpp text_source.cpp pdb.cpp \
		pdb_text_source.cpp select_translation.cpp
QT += maemo5
LIBS += -lsword 
INCLUDEPATH += /usr/include/sword

unix {
	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}
	isEmpty(BINPREFIX) {
		BINPREFIX = /usr/local
	}
	BINDIR = $$BINPREFIX/katana
	DATADIR = $$PREFIX/share

	#DEFINES += DATADIR=\"$$DATADIR\" PKGDATADIR=\"$$PKGDATADIR\"

	INSTALLS += target desktop icon48

	target.path = $$BINDIR

	desktop.path = $$DATADIR/applications/hildon
	desktop.files += $${TARGET}.desktop

	icon48.path = $$DATADIR/icons/hicolor/48x48/hildon
	icon48.files += $${TARGET}.png
}

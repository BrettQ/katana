
#include <iostream>
#include <QtGui>

#include "install_dialog.h"
#include "mainwindow.h"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("KatanaSoft");
	QCoreApplication::setApplicationName("Katana");
	if (!installTranslationIfNecessary())
		return 1;
	MainWindow window;
	window.show();
	return app.exec();
}

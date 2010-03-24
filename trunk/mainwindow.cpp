#include "mainwindow.h"

#include "bible_text_source.h"
#include "infinite_scroll.h"
#include "search_dialog.h"
#include "search_results.h"
#include "select_dialog.h"
#include "settings_dialog.h"

#include <iostream>
#include <mce/mode-names.h>
#include <mce/dbus-names.h>
#include <QProgressDialog>
#include <QScrollArea>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtGui>
#include <QX11Info>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

MainWindow::MainWindow() : QMainWindow(NULL)
{
	mShowShortTitle = false;

	setAttribute(Qt::WA_Maemo5StackedWindow);
	createMenu();

	QSettings settings;
	QString translation = settings.value("initial/translation",
										"KJV").toString();
	QString book = settings.value("initial/book", "Genesis").toString();
	int chapter = settings.value("initial/chapter", 0).toInt();
	int verse = settings.value("initial/verse", 0).toInt();
	mBible = getBibleInfo(translation);

	QFrame* frame = new QFrame;
	mLayout = new QHBoxLayout;
    mpViewer = createViewer(book, chapter, verse);
	mLayout->addWidget(mpViewer);
	mSearchResults = new SearchResultsFrame();
	mLayout->addWidget(mSearchResults);
	mLayout->setContentsMargins(0, 0, 0, 0);
	mLayout->setSpacing(0);
	frame->setLayout(mLayout);
	setCentralWidget(frame);

	connect(mSearchResults, SIGNAL(resultSelected(const QString&)), this,
			SLOT(goToVerse(const QString&)));

	QDBusConnection::systemBus().connect(QString(),
										MCE_SIGNAL_PATH,
										MCE_SIGNAL_IF,
										MCE_DEVICE_ORIENTATION_SIG,
										this,
										SLOT(orientationChanged(QString)));

	// Tell maemo-status-volume to grab/ungrab increase/decrease keys
	unsigned long val = 1;
	Atom atom = XInternAtom( QX11Info::display(), "_HILDON_ZOOM_KEY_ATOM", 0);
	XChangeProperty(QX11Info::display(),
					winId(),
					atom,
					XA_INTEGER,
					32,
					PropModeReplace,
					(unsigned char *)&val,
					1);
	setWindowTitle("Katana");
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	QSettings settings;
	settings.setValue("initial/translation", mBible->getBibleName());
	settings.setValue("initial/book", mpViewer->getSourceName());
	settings.setValue("initial/chapter", mpViewer->getCurrentSection());
	settings.setValue("initial/verse", mpViewer->getCurrentParagraph());
	settings.sync();
	event->accept();
}

void MainWindow::createMenu()
{
	menuBar()->addAction("Go to Verse", this, SLOT(onSelectVerse()));
	menuBar()->addAction("Select Translation", this, SLOT(selectTranslation()));
	menuBar()->addAction("Search", this, SLOT(onSearch()));
	menuBar()->addAction("Settings", this, SLOT(onSettings()));
}

void MainWindow::replaceViewer(InfiniteScrollViewer* viewer)
{
	QLayoutItem* item = mLayout->takeAt(0);
	mLayout->insertWidget(0, viewer);
	mpViewer = viewer;
	viewer->show();
	item->widget()->hide();
	delete item->widget();
	delete item;
	viewer->setFocus(Qt::TabFocusReason);
}

void MainWindow::setLandscape()
{
	setAttribute(Qt::WA_Maemo5LandscapeOrientation, true);
}

void MainWindow::setPortrait()
{
	setAttribute(Qt::WA_Maemo5PortraitOrientation, true);
}

bool MainWindow::event(QEvent* ev)
{
	switch (ev->type())
	{
		case QEvent::WindowActivate:
			QDBusConnection::systemBus().call(
				QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH,
											   MCE_REQUEST_IF,
											   MCE_ACCELEROMETER_ENABLE_REQ));
		break;
		case QEvent::WindowDeactivate:
			QDBusConnection::systemBus().call(
				QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH,
											   MCE_REQUEST_IF,
											   MCE_ACCELEROMETER_DISABLE_REQ));
			break;
		default:
			break;
	}

	return QWidget::event(ev);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_F7)
		mpViewer->scrollPage(false);
	if (event->key() == Qt::Key_F8)
		mpViewer->scrollPage(true);
	if (event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z)
	{
		if (!onDialogKey(event->key(), event->text()))
			selectVerse(event->text());
	}
}

void MainWindow::onSelectVerse()
{
	selectVerse("");
}

void MainWindow::selectVerse(QString startingFilter)
{
	SelectResult result;
	if (::selectVerse(this, mBible, startingFilter, result))
	{
		switch (result.getType())
		{
		case SelectResult::Type_SearchText:
			search(result.search_GetText(), "");
			break;
		case SelectResult::Type_SearchDialog:
			startSearch(result.search_GetText());
			break;
		case SelectResult::Type_SelectedVerse:
			replaceViewer(createViewer(result.verse_GetBook(),
									result.verse_GetChapter(), 0));
			mSearchResults->hideResults();
			break;
		}
	}
}

void MainWindow::search(QString text, QString scope)
{
	QProgressDialog progress("Searching...", QString(), 0, 100, this);
	mCurrentSearchText = text;
	QList<Key> results = mBible->search(text, scope, &progress);
	mSearchResults->handleResults(results);
}

void MainWindow::startSearch(QString text)
{
	SearchDialog dlg(this, text);
	if (dlg.exec() == QDialog::Accepted)
		search(dlg.getSearchText(), dlg.getSearchScope());
}

InfiniteScrollViewer* MainWindow::createViewer(QString book, int chapter,
											int verse)
{
	TextSource* bibleSource = getBibleTextSource(mBible, book);
	QString highlight;
	if (mSearchResults && mSearchResults->isShowingResults())
		highlight = mCurrentSearchText;
	return new InfiniteScrollViewer(this, bibleSource,
									shouldUseNewLineForVerses(),
									getTextFontSize(),
									chapter, verse,
									highlight, mShowShortTitle);
}

void MainWindow::selectTranslation()
{
	QString translation;
	if (::selectTranslation(this, translation))
	{
		QString bookName = mpViewer->getSourceName();
		int chapter = mpViewer->getCurrentSection();
		int verse = mpViewer->getCurrentParagraph();

		mBible = getBibleInfo(translation);
		replaceViewer(createViewer(bookName, chapter, verse));
		mSearchResults->hideResults();
	}
}

void MainWindow::onSearch()
{
	SearchDialog dlg(this, "");
	if (dlg.exec() == QDialog::Accepted)
		search(dlg.getSearchText(), dlg.getSearchScope());
}

void MainWindow::onSettings()
{
	SettingsDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted)
	{
		QString bookName = mpViewer->getSourceName();
		int chapter = mpViewer->getCurrentSection();
		int verse = mpViewer->getCurrentParagraph();

		replaceViewer(createViewer(bookName, chapter, verse));
		mSearchResults->hideResults();
	}
		
}

void MainWindow::goToVerse(QString verse)
{
	Key key = mBible->getKeyForString(verse);

	replaceViewer(createViewer(key.mBook, key.mChapter, key.mVerse));
}
void MainWindow::orientationChanged(const QString& newOrientation)
{
	bool bPortrait = newOrientation == QLatin1String(MCE_ORIENTATION_PORTRAIT);
	mShowShortTitle = bPortrait;
	mpViewer->setShowShortTitle(mShowShortTitle);
	if (bPortrait)
		setPortrait();
	else
		setLandscape();
}



#include "mainwindow.h"

#include "bible_text_source.h"
#include "infinite_scroll.h"
#include "search_dialog.h"
#include "search_results.h"

#include <iostream>
#include <mce/mode-names.h>
#include <mce/dbus-names.h>
#include <QProgressDialog>
#include <QScrollArea>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtGui>
#include <Qt/qmaemo5kineticscroller.h>

MainWindow::MainWindow() : QMainWindow()
{
	mSelectVerseAction = NULL;
	mSelectTransAction = NULL;
	mSearchAction = NULL;

	setWindowTitle("Katana");

	QSettings settings;
	QString translation = settings.value("initial/translation",
										"KJV").toString();
	QString book = settings.value("initial/book", "Genesis").toString();
	int chapter = settings.value("initial/chapter", 0).toInt();
	int verse = settings.value("initial/verse", 0).toInt();
	mBible = getBibleInfo(translation);
	TextSource* bibleSource = getBibleTextSource(mBible, book);

	QFrame* frame = new QFrame;
	mLayout = new QHBoxLayout;
	mpViewer = new InfiniteScrollViewer(this, bibleSource, chapter,
										verse, false, "");
	mLayout->addWidget(mpViewer);
	mSearchResults = new SearchResultsFrame();
	mLayout->addWidget(mSearchResults);
	mLayout->setContentsMargins(0, 0, 0, 0);
	mLayout->setSpacing(0);
	frame->setLayout(mLayout);
	setCentralWidget(frame);

	connect(mSearchResults, SIGNAL(resultSelected(const QString&)), this,
			SLOT(goToVerse(const QString&)));

	createActions();
	createMenu();
	QDBusConnection::systemBus().connect(QString(),
										MCE_SIGNAL_PATH,
										MCE_SIGNAL_IF,
										MCE_DEVICE_ORIENTATION_SIG,
										this,
										SLOT(orientationChanged(QString)));
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

void MainWindow::createActions()
{
	mSelectVerseAction = new QAction("Go to Verse", this);
	connect(mSelectVerseAction, SIGNAL(triggered()), this, SLOT(selectVerse()));

	mSelectTransAction = new QAction("Select Translation", this);
	connect(mSelectTransAction, SIGNAL(triggered()), this, SLOT(selectTranslation()));

	mSearchAction = new QAction("Search", this);
	connect(mSearchAction, SIGNAL(triggered()), this, SLOT(search()));
}

void MainWindow::createMenu()
{
	menuBar()->addAction(mSelectVerseAction);
	menuBar()->addAction(mSelectTransAction);
	menuBar()->addAction(mSearchAction);
}

void MainWindow::replaceViewer(InfiniteScrollViewer* viewer)
{
	QLayoutItem* item = mLayout->takeAt(0);
	mLayout->insertWidget(0, viewer);
	mpViewer = viewer;
	delete item;
}

void MainWindow::setLandscape()
{
	setAttribute(Qt::WA_Maemo5ForceLandscapeOrientation, true);
}

void MainWindow::setPortrait()
{
	setAttribute(Qt::WA_Maemo5ForcePortraitOrientation, true);
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

void MainWindow::selectVerse()
{
	QString bookName;
	int chapter = 0;
	if (::selectVerse(this, mBible, bookName, chapter))
	{
		TextSource* bibleSource = getBibleTextSource(mBible, bookName);
		InfiniteScrollViewer* viewer = \
			new InfiniteScrollViewer(this, bibleSource,
									chapter, 0, true, "");

		replaceViewer(viewer);
		mSearchResults->hideResults();
	}
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
		TextSource* bibleSource = getBibleTextSource(mBible, bookName);
		InfiniteScrollViewer* viewer = \
			new InfiniteScrollViewer(this, bibleSource, chapter,
									verse, false, "");

		replaceViewer(viewer);
		mSearchResults->hideResults();
	}
}

void MainWindow::search()
{
	SearchDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted)
	{
		QProgressDialog progress("Searching...", QString(), 0, 100, this);
		QString searchText = dlg.getSearchText();
		mCurrentSearchText = searchText;
		QList<Key> results = mBible->search(searchText, dlg.getSearchScope(),
											&progress);
		mSearchResults->handleResults(results);
	}
}

void MainWindow::goToVerse(QString verse)
{
	Key key = mBible->getKeyForString(verse);

	TextSource* bibleSource = getBibleTextSource(mBible, key.mBook);
	InfiniteScrollViewer* viewer = \
		new InfiniteScrollViewer(this, bibleSource, key.mChapter,
								key.mVerse, true, mCurrentSearchText);

	replaceViewer(viewer);
}
void MainWindow::orientationChanged(const QString& newOrientation)
{
	bool bPortrait = newOrientation == QLatin1String(MCE_ORIENTATION_PORTRAIT);
	mpViewer->setShouldShowPosition(!bPortrait);
	if (bPortrait)
		setPortrait();
	else
		setLandscape();
}

bool SelectDialog::select(QWidget* parent, QList<QStringList> choices,
							QString choicesDescrip, QString& selectedChoice)
{
	selectedChoice = "";

	SelectDialog dlg(parent, choices, choicesDescrip);
	if (dlg.exec() != QDialog::Accepted)
		return false;
	selectedChoice = dlg.mSelectedChoice;
	return true;
}

SelectDialog::SelectDialog(QWidget* parent, QList<QStringList> choices,
						QString choicesDescrip) : QDialog(parent)
{
	mChoices = choices;

	setMinimumHeight(800);
	setModal(true);
	setWindowTitle("Select " + choicesDescrip);

	mSignalMapper = new QSignalMapper(this);
	connect(mSignalMapper, SIGNAL(mapped(const QString&)), this,
			SLOT(selectChoice(const QString&)));

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	for (int i = 0; i < mChoices.count(); i++)
		appendChoices(mChoices[i], mainLayout);

	QFrame* frame = new QFrame();
	frame->setLayout(mainLayout);
	frame->setStyleSheet("QPushButton{"
						"padding: 12px; "
						"margin: 0px; "
						"min-width: 65px;"
						"}");

	QScrollArea* scroll = new QScrollArea;
	scroll->setWidget(frame);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
									QSizePolicy::Preferred));
	mScroller = new QMaemo5KineticScroller(scroll);
	mLayout.addWidget(scroll);
	setContentsMargins(0, 0, 0, 0);
	setLayout(&mLayout);
}

void SelectDialog::appendChoices(QStringList choices,
								QVBoxLayout* parentLayout)
{
	int max_width = parentWidget()->width() - 100/* margin for scrolling*/;
	QHBoxLayout* currentRow = NULL;
	int currentWidth = 0;
	for (int i = 0; i < choices.count(); i++)
	{
		QPushButton* button = new QPushButton(choices[i]);
		// TODO: This is ugly. The sizeHint() for the button
		// is off. Research why.
		int buttonWidth = button->sizeHint().width() - 10;
		if (!currentRow || currentWidth + buttonWidth > max_width)
		{
			if (currentRow)
				currentRow->insertStretch(max_width - currentWidth);
			currentRow = new QHBoxLayout();
			currentRow->setSpacing(0);
			parentLayout->addLayout(currentRow);
			currentWidth = 0;
		}
		currentRow->addWidget(button);
		currentWidth += buttonWidth;

		mSignalMapper->setMapping(button, choices[i]);
		connect(button, SIGNAL(clicked()), mSignalMapper, SLOT(map()));
	}
	currentRow->insertStretch(100);
}

void SelectDialog::selectChoice(const QString& choice)
{
	mSelectedChoice = choice;
	accept();
}

bool selectVerse(QWidget* parent, BibleInfo* bible,
				QString& bookName, int& chapter)
{
	QList<QStringList> choices;
	choices.push_back(bible->getOTBookNames());
	choices.push_back(bible->getNTBookNames());

	QString selectedChoice;
	if (!SelectDialog::select(parent, choices, "Book", selectedChoice))
		return false;
	QString selectedBook = selectedChoice;
	int selectedBookNum = bible->getBookNum(selectedBook);

	choices.clear();
	QStringList subChoices;
	for (int i = 0; i < bible->getNumChapters(selectedBookNum); i++)
		subChoices.push_back(QString("%1").arg(i + 1));
	choices.push_back(subChoices);
	if (!SelectDialog::select(parent, choices, "Chapter", selectedChoice))
		return false;
	int selectedChapter = selectedChoice.toInt() - 1;

	bookName = selectedBook;
	chapter = selectedChapter;
	return true;
}

bool selectTranslation(QWidget* parent, QString& translation)
{
	QList<QStringList> choices;
	choices.push_back(getAvailableTranslations());

	QString selectedChoice;
	if (!SelectDialog::select(parent, choices, "Translation", selectedChoice))
		return false;

	translation = selectedChoice;
	return true;
}


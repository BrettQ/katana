
#include "mainwindow.h"

#include "bible_text_source.h"
#include "infinite_scroll.h"
#include "search_dialog.h"

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
	mLayout->setContentsMargins(0, 0, 0, 0);
	mLayout->setSpacing(0);
	frame->setLayout(mLayout);
	setCentralWidget(frame);

	mSearchResultsMapper = new QSignalMapper(this);
	connect(mSearchResultsMapper, SIGNAL(mapped(const QString&)), this,
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

void MainWindow::createSearchResultsPane(QList<Key> results)
{
	while (mLayout->count() > 1)
	{
		QLayoutItem* item = mLayout->itemAt(1);
		mLayout->removeItem(item);
		for (int i = 0; i < item->layout()->count(); i++)
			item->layout()->itemAt(i)->widget()->hide();
		delete item;
	}

	QVBoxLayout* buttonsLayout = new QVBoxLayout;
	buttonsLayout->setSpacing(0);
	buttonsLayout->setContentsMargins(0, 0, 0, 0);
	for (int i = 0; i < results.count(); i++)
	{
		Key key = results[i];
		QString result = QString("%1 %2:%3").arg(key.mBook)
											.arg(key.mChapter+1)
											.arg(key.mVerse+1);
		QPushButton* button = new QPushButton(result);
		QFont font = button->font();
		font.setPointSize(12);
		button->setFont(font);
		mSearchResultsMapper->setMapping(button, result);
		connect(button, SIGNAL(clicked()), mSearchResultsMapper, SLOT(map()));
		buttonsLayout->addWidget(button);
	}
	QScrollArea* scroll = new QScrollArea;
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
									QSizePolicy::Preferred));

	QFrame* frame = new QFrame();
	frame->setStyleSheet("QPushButton{"
						"padding: 3px 10px 3px 10px;"
						"}");
	frame->setLayout(buttonsLayout);
	scroll->setWidget(frame);

	QVBoxLayout* searchLayout = new QVBoxLayout;
	QLabel* label = new QLabel("<center>Search<br/>Results:</center>");
	// TODO: this is a hack to work around an apparent bug
	// in QT. When switching to 4.6, we should check to see
	// if we can remove this, since it doesn't respect themes.
	label->setStyleSheet("background: #000");
	searchLayout->addWidget(label);
	searchLayout->addWidget(scroll);

	mLayout->addLayout(searchLayout);
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
		createSearchResultsPane(results);
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
	mainLayout->setContentsMargins(0, 0, 0, 0);
	for (int i = 0; i < mChoices.count(); i++)
		appendChoices(mChoices[i], mainLayout);

	QFrame* frame = new QFrame();
	frame->setLayout(mainLayout);
	frame->setStyleSheet("QPushButton{"
						"padding: 10px; "
						"min-width: 50px;"
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
	// TODO: the 730 width here should at some point be calculated
	// so that we can support portrait mode nicely. We should be
	// able to get the parent window's width and subtract off padding
	// and scrollbars.
	QHBoxLayout* currentRow = NULL;
	int currentWidth = 0;
	for (int i = 0; i < choices.count(); i++)
	{
		QPushButton* button = new QPushButton(choices[i]);
		if (!currentRow || currentWidth + button->sizeHint().width() > 725)
		{
			if (currentRow)
				currentRow->insertStretch(725 - currentWidth);
			currentRow = new QHBoxLayout();
			parentLayout->addLayout(currentRow);
			currentWidth = 0;
		}
		currentRow->addWidget(button);
		currentWidth += button->sizeHint().width() - 13;

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



#include "infinite_scroll.h"

#include <QApplication>
#include <QEvent>
#include <QGestureEvent>
#include <QScrollBar>
#include <QString>
#include <QSwipeGesture>
#include <QTextBlock>
#include <QTextCodec>
#include <QTimer>

#include <iostream>

/* TODO:
 * Reap far-offscreen sections. Possibly once there's a full section of buffer?
 */

QString defaultCss = "\
a\
{\
	text-decoration: none;\
	font-size: %2pt;\
	color: #888;\
}\
span\
{\
	font-family: sans-serif;\
	font-size: %1pt;\
}\
.SectionTitle\
{\
	font-size: %3pt;\
	font-weight: bold;\
}\
span.scripRef\
{\
	visibility: hidden;\
	color: #BBB;\
	font-size: %4pt;\
}\
";

void SearchResultsHighlighter::highlightBlock(const QString& text)
{
	QTextCharFormat format;
	format.setForeground(Qt::blue);

	int index = text.indexOf(mText, 0, Qt::CaseInsensitive);
	while (index >= 0)
	{
		setFormat(index, mText.length(), format);

		index = text.indexOf(mText, index + mText.length(), Qt::CaseInsensitive);
	}
}
InfiniteScrollViewer::InfiniteScrollViewer(QWidget* mainWindow,
										TextSource* textSource,
										bool newLineForParagraphs,
										int fontSize,
										int startingSection,
										int startingParagraph,
										QString searchText,
										bool shortTitle) : QTextBrowser(mainWindow)
{
	mTextSource = textSource;
	mNewLineForParagraphs = newLineForParagraphs;
	mDocument = new QTextDocument();
	mMainWindow = mainWindow;

	if (searchText.length() > 0)
		mHighlighter = new SearchResultsHighlighter(mDocument, searchText);

	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onScroll()));
	setReadOnly(true);
	setTextInteractionFlags(Qt::NoTextInteraction);
	setDocument(mDocument);
	setAttribute(Qt::WA_OpaquePaintEvent);

	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf8"));
	grabGesture(Qt::SwipeGesture);

	mShowShortTitle = shortTitle;

	mFirstSection = 0;
	mFirstParagraph = 0;
	mLastSection = 0;
	mLastParagraph = 0;

	int verseNumSize = (double)fontSize * 0.875;
	int sectionSize = (double)fontSize * 1.75;
	int scripRefSize = (double)fontSize * 0.7;
	QString css = defaultCss.arg(fontSize).arg(verseNumSize)
					.arg(sectionSize).arg(scripRefSize);
	mDocument->setDefaultStyleSheet(css);

	mCurrentSection = startingSection;
	mCurrentParagraph = startingParagraph;
	mHasDoneInitialFill = false;
}

InfiniteScrollViewer::~InfiniteScrollViewer()
{
	delete mDocument;
	delete mTextSource;
}

QString InfiniteScrollViewer::getSourceName()
{
	return mTextSource->getSourceName();
}

int InfiniteScrollViewer::getCurrentSection()
{
	return mCurrentSection;
}

int InfiniteScrollViewer::getCurrentParagraph()
{
	return mCurrentParagraph;
}

void InfiniteScrollViewer::setShowShortTitle(bool bShow)
{
	mShowShortTitle = bShow;
	updateTitle();
}

void InfiniteScrollViewer::scrollPage(bool bUp)
{
	int iShift = height() - 20;
	if (bUp)
		iShift *= -1;
	int iTarget = verticalScrollBar()->value() + iShift;
	verticalScrollBar()->setValue(iTarget);
}

void InfiniteScrollViewer::fillInitial(int section, int startingParagraph)
{
	QTextCursor cursor(mDocument);
	insertSectionStart(cursor, section);
	int lastParagraph = startingParagraph + 10;
	if (lastParagraph >= mTextSource->getNumParagraphs(section))
		lastParagraph = mTextSource->getNumParagraphs(section) - 1;
	for (int i = 0; i <= lastParagraph; i++)
		insertParagraph(cursor, section, i);

	mFirstSection = section;
	mFirstParagraph = 0;
	mLastSection = section;
	mLastParagraph = lastParagraph;
	mHasDoneInitialFill = true;

	fillBottomText();

	rebuildAnchorPositions();
}

void InfiniteScrollViewer::fillTopTextIfNecessary()
{
	// We have to add a section at a time, since that's the only
	// way to create a document reflow. (Adding a section at a
	// time doesn't do this because we add it in its own block.)
	if (getTopPadding() < 500 && mFirstSection > 0)
	{
		int startingHeight = verticalScrollBar()->maximum();
		int startingScroll = verticalScrollBar()->value();

		int section = mFirstSection - 1;
		QTextCursor cursor(mDocument);
		cursor.movePosition(QTextCursor::Start);
		cursor.beginEditBlock();
		insertSectionStart(cursor, section);
		for (int i = 0; i < mTextSource->getNumParagraphs(section); i++)
		{
			insertParagraph(cursor, section, i);
		}
		cursor.endEditBlock();

		int scrollDelta = verticalScrollBar()->maximum() - startingHeight;
		verticalScrollBar()->setValue(startingScroll + scrollDelta);

		mFirstSection = section;
		mFirstParagraph = 0;

		rebuildAnchorPositions();
	}
}

void InfiniteScrollViewer::fillBottomTextIfNecessary()
{
	int startingScroll = verticalScrollBar()->value();

	while (getBottomPadding() < height() + 100 && !filledToEnd())
		fillBottomText();

	verticalScrollBar()->setValue(startingScroll);
	rebuildAnchorPositions();
}

void InfiniteScrollViewer::fillBottomText()
{
	int addedVerses = 0;

	QTextCursor cursor(mDocument);
	cursor.movePosition(QTextCursor::End);
	cursor.beginEditBlock();
	while (addedVerses < 5 && !filledToEnd())
	{
		int section = mLastSection;
		int paragraph = mLastParagraph + 1;
		if (paragraph >= mTextSource->getNumParagraphs(section))
		{
			section = mLastSection + 1;
			paragraph = 0;
			insertSectionStart(cursor, section);
		}

		insertParagraph(cursor, section, paragraph);
		mLastSection = section;
		mLastParagraph = paragraph;
		addedVerses++;
	}

	cursor.endEditBlock();
}

bool InfiniteScrollViewer::filledToEnd()
{
	return mLastSection >= mTextSource->getNumSections() - 1 &&
		mLastParagraph >= mTextSource->getNumParagraphs(mLastSection) - 1;
}

void InfiniteScrollViewer::initialScroll()
{
	if (!mHasDoneInitialFill)
		return;
	fillBottomTextIfNecessary();
	scrollTo(mCurrentSection, mCurrentParagraph);
}

void InfiniteScrollViewer::insertParagraph(QTextCursor& cursor, int section,
										int paragraph)
{
	QString text = mTextSource->getText(section, paragraph);
	QString spacing;
	if (!mNewLineForParagraphs)
		spacing = "&nbsp;&nbsp;&nbsp;";
	QString html = QString("<span>%1<a name=\"%2_%3\">%4</a> %5</span>").arg(
							spacing).arg(section).arg(paragraph
							).arg(paragraph + 1).arg(text);
	if (mNewLineForParagraphs)
		startBlock(cursor);
	cursor.insertHtml(html);
}

void InfiniteScrollViewer::insertSectionStart(QTextCursor& cursor, int section)
{
	startBlock(cursor);
	setAlignment(Qt::AlignCenter);
	QString html = QString("<span class=\"SectionTitle\">%1</span>").arg(section + 1);
	cursor.insertHtml(html);
	startBlock(cursor);
	setAlignment(Qt::AlignLeft);
}

void InfiniteScrollViewer::startBlock(QTextCursor& cursor)
{
	QTextBlockFormat blockFormat;
	QTextCharFormat charFormat;
	cursor.insertBlock(blockFormat, charFormat);
}

void InfiniteScrollViewer::scrollTo(int section, int paragraph)
{
	QString name = QString("%1_%2").arg(section).arg(paragraph);
	scrollToAnchor(name);
}

void InfiniteScrollViewer::updatePosition()
{
	QTextCursor cursor = cursorForPosition(QPoint(0, 0));

	for (int i = 0; i < mAnchorNames.count(); i++)
	{
		if (mAnchorPositions[i] >= cursor.position())
		{
			QStringList ids = mAnchorNames[i].split('_');
			mCurrentSection = ids[0].toInt();
			mCurrentParagraph = ids[1].toInt();
			return;
		}
	}
	mCurrentSection = 0;
	mCurrentParagraph = 0;
}

void InfiniteScrollViewer::updateTitle()
{
	QString descrip = mTextSource->getSourceDescrip(mShowShortTitle);
	int section = getCurrentSection();
	int paragraph = getCurrentParagraph();
	QString title;
	if (!mShowShortTitle)
		title += "Katana - ";

	title += QString("%1 %2:%3").arg(descrip).arg(section+1).arg(paragraph+1);
	mMainWindow->setWindowTitle(title);
}

void InfiniteScrollViewer::rebuildAnchorPositions()
{
	mAnchorNames.clear();
	mAnchorPositions.clear();

	for (QTextBlock block = mDocument->begin(); block.isValid(); block = block.next())
	{
		for (QTextBlock::Iterator iter = block.begin(); !iter.atEnd(); iter++)
		{
			QTextFragment fragment = iter.fragment();
			QTextCharFormat format = fragment.charFormat();
			if (format.isAnchor())
			{
				mAnchorNames.push_back(format.anchorName());
				mAnchorPositions.push_back(fragment.position());
			}
		}
	}
}

int InfiniteScrollViewer::getTopPadding()
{
	return verticalScrollBar()->value();
}

int InfiniteScrollViewer::getBottomPadding()
{
	return verticalScrollBar()->maximum() - 
			verticalScrollBar()->value();
}

void InfiniteScrollViewer::showEvent(QShowEvent* event)
{
	QTextBrowser::showEvent(event);
	repaint();
	fillInitial(mCurrentSection, mCurrentParagraph);

	updateTitle();

	// We have to wait until all text is rendered, so that
	// we can scroll to the right place.
	QTimer::singleShot(100, this, SLOT(initialScroll()));
}
void InfiniteScrollViewer::resizeEvent(QResizeEvent* event)
{
	QTextBrowser::resizeEvent(event);
	initialScroll();
}

bool InfiniteScrollViewer::event(QEvent* event)
{
	if (event->type() == QEvent::Gesture)
	{
		std::cout << event << "\n";
		QGesture* gesture = static_cast<QGestureEvent*>(event)->gesture(Qt::SwipeGesture);
		if (gesture)
		{
			QSwipeGesture* swipe = static_cast<QSwipeGesture*>(gesture);
			if (swipe->state() == Qt::GestureFinished &&
				swipe->horizontalDirection() == QSwipeGesture::Left)
			{
				std::cout << "swipe: " << swipe->horizontalDirection() << "\n";
				std::cout << swipe->verticalDirection() << "\n";
				std::cout << swipe->swipeAngle() << "\n";
			}
		}
		return true;
	}
	return QTextBrowser::event(event);
}

void InfiniteScrollViewer::onScroll()
{
	QTimer::singleShot(1, this, SLOT(onScrollTimer()));
}

void InfiniteScrollViewer::onScrollTimer()
{
	updatePosition();
	updateTitle();
	fillBottomTextIfNecessary();
	fillTopTextIfNecessary();
}


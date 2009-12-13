
#include "infinite_scroll.h"

#include <QScrollBar>
#include <QString>
#include <QTextBlock>
#include <QTimer>
#include <Qt/qmaemo5kineticscroller.h>

#include <iostream>

/* TODO:
 * Reap far-offscreen sections. Possibly once there's a full section of buffer?
 */

QString defaultCss = "\
a\
{\
	text-decoration: none;\
	font-size: 14pt;\
	color: #888;\
}\
span\
{\
	font-family: sans-serif;\
	font-size: 16pt;\
}\
.SectionTitle\
{\
	font-size: 28pt;\
	font-weight: bold;\
}\
";

void SearchResultsHighlighter::highlightBlock(const QString& text)
{
	// TODO: For some bizarre reason, setting the color here
	// to red actually results in blue text, and vice versa.
	// (We're actually wanting blue here.)
	QTextCharFormat format;
	format.setForeground(Qt::red);

	int index = text.indexOf(mText);
	while (index >= 0)
	{
		setFormat(index, mText.length(), format);

		index = text.indexOf(mText, index + mText.length());
	}
}
InfiniteScrollViewer::InfiniteScrollViewer(QWidget* mainWindow,
										TextSource* textSource,
										int startingSection,
										int startingParagraph,
										bool highlightStart,
										QString searchText)
{
	mTextSource = textSource;
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
	mScroller = new QMaemo5KineticScroller(this);
	mScroller->setMaximumVelocity(2000);
	mScroller->setDecelerationFactor(0.75);

	m_bShowPosition = true;

	mHighlightStart = highlightStart;
	mFirstSection = 0;
	mFirstParagraph = 0;
	mLastSection = 0;
	mLastParagraph = 0;

	mDocument->setDefaultStyleSheet(defaultCss);

	mCurrentSection = startingSection;
	mCurrentParagraph = startingParagraph;
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

QString InfiniteScrollViewer::getSourceDescrip()
{
	return mTextSource->getSourceDescrip();
}

int InfiniteScrollViewer::getCurrentSection()
{
	return mCurrentSection;
}

int InfiniteScrollViewer::getCurrentParagraph()
{
	return mCurrentParagraph;
}

void InfiniteScrollViewer::setShouldShowPosition(bool bShow)
{
	m_bShowPosition = bShow;
	updateTitle();
}

void InfiniteScrollViewer::fillInitial(int section, int startingParagraph)
{
	QTextCursor cursor(mDocument);
	insertSectionStart(cursor, section);
	int lastParagraph = startingParagraph + 10;
	if (lastParagraph < mTextSource->getNumParagraphs(section))
		lastParagraph = mTextSource->getNumParagraphs(section);
	for (int i = 0; i < lastParagraph; i++)
		insertParagraph(cursor, section, i);

	mFirstSection = section;
	mFirstParagraph = 0;
	mLastSection = section;
	mLastParagraph = lastParagraph;

	rebuildAnchorPositions();
}

void InfiniteScrollViewer::fillTopText()
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

void InfiniteScrollViewer::fillBottomText()
{
	if (getBottomPadding() < 500)
	{
		int addedVerses = 0;
		int startingScroll = verticalScrollBar()->value();

		QTextCursor cursor(mDocument);
		cursor.movePosition(QTextCursor::End);
		cursor.beginEditBlock();
		while (addedVerses < 5 &&
			(mLastSection < mTextSource->getNumSections() - 1 ||
			mLastParagraph < mTextSource->getNumParagraphs(mLastSection) - 1))
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
		verticalScrollBar()->setValue(startingScroll);
		rebuildAnchorPositions();
	}
}

void InfiniteScrollViewer::initialScroll()
{
	scrollTo(mCurrentSection, mCurrentParagraph);

	if (mHighlightStart)
	{
		QString name = QString("%1_%2").arg(mCurrentSection).
									arg(mCurrentParagraph);
		for (int i = 0; i < mAnchorNames.count(); i++)
		{
			if (mAnchorNames[i] == name)
			{
				mHighlightStart = false;
				QTextCursor cursor(mDocument);
				cursor.beginEditBlock();
				cursor.movePosition(QTextCursor::Right,
									QTextCursor::MoveAnchor,
									mAnchorPositions[i]);
				cursor.movePosition(QTextCursor::WordRight,
									QTextCursor::KeepAnchor);
				QTextCharFormat format = cursor.charFormat();
				format.setForeground(Qt::red);
				cursor.setCharFormat(format);
				cursor.endEditBlock();
			}
		}
	}
}

void InfiniteScrollViewer::insertParagraph(QTextCursor& cursor, int section, int paragraph)
{
	QString text = mTextSource->getText(section, paragraph);
	QString html = QString("<span>&nbsp;&nbsp;<a name=\"%1_%2\">%3</a> %4</span>").arg(
							section).arg(paragraph).arg(paragraph + 1).arg(text);
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
	QString descrip = getSourceDescrip();
	int section = getCurrentSection();
	int paragraph = getCurrentParagraph();
	if (m_bShowPosition)
	{
		mMainWindow->setWindowTitle(QString("Katana - %1 %2:%3").arg(descrip).
											arg(section+1).arg(paragraph+1));
	}
	else
		mMainWindow->setWindowTitle("Katana");
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
	QTextEdit::showEvent(event);
	repaint();
	fillInitial(mCurrentSection, mCurrentParagraph);

	updateTitle();
	fillBottomText();

	// We have to wait until all text is rendered, so that
	// we can scroll to the right place.
	QTimer::singleShot(100, this, SLOT(initialScroll()));
}
void InfiniteScrollViewer::resizeEvent(QResizeEvent* event)
{
	QTextEdit::resizeEvent(event);
	initialScroll();
}

void InfiniteScrollViewer::onScroll()
{
	QTimer::singleShot(1, this, SLOT(onScrollTimer()));
}

void InfiniteScrollViewer::onScrollTimer()
{
	updatePosition();
	updateTitle();
	fillBottomText();
	fillTopText();
}


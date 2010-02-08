#include "search_results.h"

#include "bible_text_source.h"

#include <iostream>

#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalMapper>
#include <QVBoxLayout>

SearchResultsFrame::SearchResultsFrame()
{
	mLayout = new QVBoxLayout;
	mLayout->setSpacing(2);
	mLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(mLayout);
	mSearchResultsMapper = NULL;
}

SearchResultsFrame::~SearchResultsFrame()
{
}

void SearchResultsFrame::handleResults(QList<Key> results)
{
	hideResults();
	mLayout->setContentsMargins(3, 3, 0, 3);

	mSearchResultsMapper = new QSignalMapper();
	connect(mSearchResultsMapper, SIGNAL(mapped(const QString&)), this,
			SLOT(onSelect(const QString&)));

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
	mScroll = new QScrollArea;
	mScroll->setFrameShape(QFrame::NoFrame);
	mScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mScroll->setSizePolicy(QSizePolicy(QSizePolicy::Maximum,
									QSizePolicy::Preferred));

	QFrame* frame = new QFrame();
	frame->setStyleSheet("QPushButton{"
						"padding: 3px 10px 3px 10px;"
						"}");
	frame->setLayout(buttonsLayout);
	mScroll->setWidget(frame);
	mLayout->addWidget(mScroll);
}

void SearchResultsFrame::hideResults()
{
	mLayout->setContentsMargins(0, 0, 0, 0);
	while (mLayout->count() > 0)
	{
		QLayoutItem* item = mLayout->itemAt(0);
		mLayout->removeItem(item);
		delete item;
	}
	delete mSearchResultsMapper;
	mSearchResultsMapper = NULL;
}

void SearchResultsFrame::onSelect(const QString& result)
{
	emit resultSelected(result);
}


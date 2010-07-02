#include "search_results.h"

#include "text_source.h"

#include <iostream>

#include <QFontMetrics>
#include <QListWidget>
#include <QListWidget>
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
	mResultsList = NULL;
}

SearchResultsFrame::~SearchResultsFrame()
{
}

void SearchResultsFrame::handleResults(QList<Key> results)
{
	hideResults();
	mLayout->setContentsMargins(3, 3, 0, 3);

	QPushButton* hideButton =
		new QPushButton(QIcon::fromTheme("general_close"), "");
	connect(hideButton, SIGNAL(clicked()), this, SLOT(onHideClicked()));
	mLayout->addWidget(hideButton);
	mResultsList = new QListWidget;
	QStringList items;
	for (int i = 0; i < results.count(); i++)
		items.append(results[i].toString());
	mResultsList->addItems(items);
	setStyleSheet("QListWidget{"
				"font-size: 14pt;"
				"}");
	int widest = 0;
	for (int i = 0; i < results.count(); i++)
	{
		QListWidgetItem* item = mResultsList->item(i);
		item->setSizeHint(QSize(40, 40));

		// Calculate row width
		QFont font = item->font();
		font.setPointSize(14);
		QFontMetrics metrics(font);
		int width = metrics.width(item->text());
		if (width > widest)
			widest = width;
	}
	mLayout->addWidget(mResultsList);
	// TODO: 20 is a hack for scrollbar
	mResultsList->setMaximumWidth(widest + 20);
	mResultsList->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

	connect(mResultsList, SIGNAL(itemActivated(QListWidgetItem*)),
			this, SLOT(onSelect(QListWidgetItem*)));
}

bool SearchResultsFrame::isShowingResults()
{
	return mResultsList != NULL;
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

	if (mResultsList != NULL)
	{
		while (mResultsList->count() > 0)
			delete mResultsList->takeItem(0);
		delete mResultsList;
		mResultsList = NULL;
	}
}

void SearchResultsFrame::onHideClicked()
{
	hideResults();
}

void SearchResultsFrame::onSelect(QListWidgetItem* item)
{
	emit resultSelected(item->text());
}


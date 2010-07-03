#include "search_dialog.h"

#include <QComboBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include "text_source.h"

SearchDialog::SearchDialog(QWidget* parent, QString startingText,
						TextSource* currentSource) : QDialog(parent)
{
	setWindowTitle("Search");

	mScopeOptions.insert("Entire Bible", "");
	mScopeOptions.insert("Old Testament", "Genesis 1:1 - Malachi 4:6;");
	mScopeOptions.insert("New Testament", "Matthew 1:1 - Revelation 22:21;");
	mScopeOptions.insert("Gospels", "Matthew 1:1 - John 21:25;");

	// Add option for current book
	QString book = currentSource->getSuperSectionName();
	Key start(book, 0, 0);
	int numChapters = currentSource->getNumSections();
	Key end(book, numChapters, currentSource->getNumParagraphs(numChapters)-1);
	mScopeOptions.insert("Book of " + book,
						start.toString() + " - " + end.toString());

	// Construct layout
	QVBoxLayout* layout = new QVBoxLayout;

	mSearchEdit = new QLineEdit();
	mSearchEdit->setText(startingText);
	layout->addWidget(mSearchEdit);

	QHBoxLayout* buttonLayout = new QHBoxLayout;

	// Scope
	mScopeCombo = new QComboBox();
	mScopeCombo->addItems(mScopeOptions.keys());
	mScopeCombo->setCurrentIndex(mScopeOptions.keys().indexOf("Entire Bible"));
	buttonLayout->addWidget(mScopeCombo);

	buttonLayout->insertStretch(100);
	QPushButton* searchButton = new QPushButton("Search");
	buttonLayout->addWidget(searchButton);
	layout->addLayout(buttonLayout);
	setLayout(layout);

	connect(searchButton, SIGNAL(clicked()), this, SLOT(accept()));
}

void SearchDialog::accept()
{
	QString scopeName = mScopeCombo->itemText(mScopeCombo->currentIndex());
	mSearchScope = mScopeOptions.value(scopeName);
	mSearchText = mSearchEdit->text();
	QDialog::accept();
}


#include "search_dialog.h"

#include <QComboBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

SearchDialog::SearchDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle("Search");

	// TODO: these should probably be loaded 
	// from a configuration file at some point.
	mScopeOptions.insert("Entire Bible", "");
	mScopeOptions.insert("Old Testament", "Genesis 1:1 - Malachi 4:6;");
	mScopeOptions.insert("New Testament", "Matthew 1:1 - Revelation 22:21;");
	mScopeOptions.insert("Gospels", "Matthew 1:1 - John 21:25;");

	QVBoxLayout* layout = new QVBoxLayout;

	mSearchEdit = new QLineEdit();
	layout->addWidget(mSearchEdit);

	QHBoxLayout* buttonLayout = new QHBoxLayout;

	// Scope
	mScopeCombo = new QComboBox();
	mScopeCombo->addItems(mScopeOptions.keys());
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


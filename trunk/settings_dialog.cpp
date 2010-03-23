#include "settings_dialog.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle("Settings");

	QHBoxLayout* layout = new QHBoxLayout;

	QVBoxLayout* settingsLayout = new QVBoxLayout;
	mNewLineCheck = new QCheckBox("Display verses on separate lines");
	mNewLineCheck->setChecked(useNewLineForVerses());
	mNewLineCheck->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
											QSizePolicy::Preferred));
	settingsLayout->addWidget(mNewLineCheck);

	layout->addLayout(settingsLayout);

	mSaveButton = new QPushButton("Save");
	layout->addWidget(mSaveButton);

	setLayout(layout);

	connect(mSaveButton, SIGNAL(clicked()), this, SLOT(accept()));
}

void SettingsDialog::accept()
{
	bool useNewLine = mNewLineCheck->isChecked();
	QSettings settings;
	settings.setValue("settings/verseNewLine", useNewLine);
	settings.sync();
	QDialog::accept();
}

bool useNewLineForVerses()
{
	QSettings settings;
	return settings.value("settings/verseNewLine", false).toBool();
}

#include "settings_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

QString getTextFontSizeStr();

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle("Settings");

	QHBoxLayout* layout = new QHBoxLayout;

	QVBoxLayout* settingsLayout = new QVBoxLayout;
	mNewLineCheck = new QCheckBox("Display verses on separate lines");
	mNewLineCheck->setChecked(shouldUseNewLineForVerses());
	mNewLineCheck->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
											QSizePolicy::MinimumExpanding));
	settingsLayout->addWidget(mNewLineCheck);
	QHBoxLayout* fontLayout = new QHBoxLayout;
	mFontSizeCombo = new QComboBox;
	mFontSizeCombo->addItem("Small");
	mFontSizeCombo->addItem("Medium");
	mFontSizeCombo->addItem("Large");
	selectItem(mFontSizeCombo, getTextFontSizeStr());
	mFontSizeCombo->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
											QSizePolicy::MinimumExpanding));
	fontLayout->addWidget(new QLabel("Font Size: "));
	fontLayout->addWidget(mFontSizeCombo);
	settingsLayout->addLayout(fontLayout);

	layout->addLayout(settingsLayout);

	QVBoxLayout* saveLayout = new QVBoxLayout;
	saveLayout->insertStretch(1);

	mSaveButton = new QPushButton("Save");
	saveLayout->addWidget(mSaveButton);
	layout->addLayout(saveLayout);

	setLayout(layout);

	connect(mSaveButton, SIGNAL(clicked()), this, SLOT(accept()));
}
void SettingsDialog::selectItem(QComboBox* combo, QString text)
{
	for (int i = 0; i < combo->count(); i++)
	{
		if (combo->itemText(i).toLower() == text.toLower())
		{
			combo->setCurrentIndex(i);
			return;
		}
	}
}

void SettingsDialog::accept()
{
	bool shouldUseNewLine = mNewLineCheck->isChecked();
	QString fontSize = mFontSizeCombo->currentText();
	QSettings settings;
	settings.setValue("settings/verseNewLine", shouldUseNewLine);
	settings.setValue("settings/textSize", fontSize.toLower());
	settings.sync();
	QDialog::accept();
}

bool shouldUseNewLineForVerses()
{
	QSettings settings;
	return settings.value("settings/verseNewLine", false).toBool();
}

QString getTextFontSizeStr()
{
	QSettings settings;
	return settings.value("settings/textSize", "small").toString();
}

int getTextFontSize()
{
	QString size = getTextFontSizeStr();
	if (size == "small")
		return 16;
	else if (size == "medium")
		return 20;
	else if (size == "large")
		return 24;
	return 16;
}

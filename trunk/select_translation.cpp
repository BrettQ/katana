#include "select_translation.h"

#include <QEventLoop>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

#include "bible_text_source.h"
#include "pdb_text_source.h"
#include "install_dialog.h"

SelectTranslationWidget::SelectTranslationWidget(QWidget* parent) :
	QWidget(parent)
{
	mEventLoop = NULL;
	setAttribute(Qt::WA_Maemo5StackedWindow);
	setWindowFlags(windowFlags() | Qt::Window);

	QStringList names, descs;
	getAvailableTranslations(names, descs);
	getAvailablePDBTranslations(names, descs);
	for (int i = 0; i < names.size(); i++)
		mTranslations.insert(descs[i], names[i]);

	mList = new QListWidget(this);
	mList->addItems(mTranslations.keys());
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget(mList);
	setLayout(layout);

	QHBoxLayout* buttonsLayout = new QHBoxLayout;
	QPushButton* installButton = new QPushButton("Install");
	installButton->setObjectName("Install");
	buttonsLayout->addWidget(installButton);
	QPushButton* removeButton = new QPushButton("Remove");
	removeButton->setObjectName("Remove");
	buttonsLayout->addWidget(removeButton);
	layout->addLayout(buttonsLayout);
	connect(mList, SIGNAL(itemActivated(QListWidgetItem*)),
			this, SLOT(onSelect(QListWidgetItem*)));
	connect(installButton, SIGNAL(clicked()), this, SLOT(onInstallClicked()));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(onDeleteClicked()));
	setWindowTitle("Select Translation");
	setStyleSheet("QPushButton#Install,QPushButton#Remove{"
				"padding: 12px; "
				"}");
}

bool SelectTranslationWidget::display(QString& translation)
{
	show();

	mEventLoop = new QEventLoop;
	mEventLoop->exec(QEventLoop::AllEvents);
	if (mSelected != "")
	{
		translation = mSelected;
		return true;
	}
	return false;
}

void SelectTranslationWidget::onSelect(QListWidgetItem* item)
{
	mSelected = mTranslations[item->text()];
	mEventLoop->exit();
}

void SelectTranslationWidget::onInstallClicked()
{
	InstallTranslationsDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted)
	{
		mSelected = dlg.getNewTranslation();
		mEventLoop->exit();
	}
}

void SelectTranslationWidget::onDeleteClicked()
{
	DeleteTranslationsDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted)
		mEventLoop->exit();
}

bool selectTranslation(QWidget* parent, QString& translation)
{
	SelectTranslationWidget widget(parent);
	return widget.display(translation);
}

#include "install_dialog.h"

#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMaemo5InformationBox>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QString>
#include <QtAlgorithms>
#include <QTimer>
#include <QVBoxLayout>

#include <sword/ftptrans.h>
#include <sword/installmgr.h>
#include <sword/swmgr.h>
#include <sword/swmodule.h>

using namespace sword;

const QString sourceName_c = "CrossWire Bible Society";

const QString swordPath_c = "/home/user/.sword";
const QString installPath_c = swordPath_c + "/InstallMgr";

bool lessThan(const SWModule* r, const SWModule* l)
{
	return stricmp(r->Description(), l->Description()) < 0;
}

class DlgStatusReporter : public StatusReporter
{
public:
	DlgStatusReporter(QWidget* parent)
	{
		mDialog = NULL;
		mParent = parent;
	}
	~DlgStatusReporter()
	{
		if (mDialog)
			delete mDialog;
	}
	void startProcess(QString title)
	{
		if (mDialog)
			delete mDialog;

		mDialog = new QProgressDialog(title, QString(), 0, 100, mParent);
		mDialog->setAutoClose(false);
		mDialog->setWindowTitle(title);
		mDialog->show();
	}
	void endProcess()
	{
		delete mDialog;
		mDialog = NULL;
	}

protected:
	// Sword interfaces
	virtual void preStatus(long totalBytes, long completedBytes,
							const char *message)
	{
		mDialog->setMinimum(0);
		mDialog->setMaximum(totalBytes);
		mDialog->setValue(completedBytes);
		mDialog->setLabelText(message);
	}

	virtual void statusUpdate(double dlTotal, double dlNow)
	{
		mDialog->setMaximum(dlTotal);
		mDialog->setValue(dlNow);
	}

protected:
	QProgressDialog* mDialog;
	QWidget* mParent;
};

InstallTranslationsDialog::InstallTranslationsDialog(QWidget* pParent) :
	QDialog(pParent)
{
	setWindowTitle("Select one or more translations to install");
	setMinimumHeight(800);

	{
		// Add the default remote source if it doesn't already exist.
		InstallMgr installMgr(installPath_c.toAscii().data());
		if (installMgr.sources.find("CrossWire Bible Society") ==
				installMgr.sources.end())
		{
			InstallSource installSource("FTP");
			installSource.caption = "CrossWire Bible Society";
			installSource.source = "ftp.crosswire.org";
			installSource.directory = "/pub/sword/raw";
			QString confPath = installPath_c + "/InstallMgr.conf";
			SWConfig config(confPath.toAscii().data());
			config["General"]["PassiveFTP"] = "true";
			config["Sources"]["FTPSource"] = installSource.getConfEnt();
			config.Save();
		}
	}
	mStatusReporter = new DlgStatusReporter(this);
	mInstallMgr = new InstallMgr(installPath_c.toAscii().data(),
								mStatusReporter);
	mInstallSource = mInstallMgr->sources.find(
						sourceName_c.toAscii().data())->second;

	mMainMgr = new SWMgr;

	mLanguageCombo = new QComboBox;
	mLanguageCombo->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
											QSizePolicy::Minimum));
	QHBoxLayout* languageLayout = new QHBoxLayout;
	languageLayout->addWidget(new QLabel("Language: "));
	languageLayout->addWidget(mLanguageCombo);

	QVBoxLayout* selectionLayout = new QVBoxLayout;
	selectionLayout->addLayout(languageLayout);
	mTransListWidget = new QListWidget;
	mTransListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
	selectionLayout->addWidget(mTransListWidget);

	QHBoxLayout* mainLayout = new QHBoxLayout;
	mainLayout->addLayout(selectionLayout);
	
	QPushButton* installButton = new QPushButton("Install");
	QVBoxLayout* installLayout = new QVBoxLayout;
	installLayout->insertStretch(1);
	installLayout->addWidget(installButton);
	mainLayout->addLayout(installLayout);
	setLayout(mainLayout);

	connect(mLanguageCombo, SIGNAL(currentIndexChanged(const QString&)),
			this, SLOT(onLanguageChange(const QString&)));
	connect(installButton, SIGNAL(clicked()), this, SLOT(accept()));
}

void InstallTranslationsDialog::accept()
{
	QString language = mTranslations.keys()[mLanguageCombo->currentIndex()];
	QList<SWModule*> translationsToInstall;
	for (int i = 0; i < mTransListWidget->selectedItems().count(); i++)
	{
		int index = mTransListWidget->row(
						mTransListWidget->selectedItems()[i]);
		translationsToInstall.append(mTranslations[language][index]);
	}
	if (!translationsToInstall.size())
		return;

	for (int i = 0; i < translationsToInstall.size(); i++)
	{
		if (!installModule(translationsToInstall[i]))
		{
			QMessageBox::critical(this, "Error",
								"Unable to install translations. " 
								"Check your internet connection.");
			return;
		}
	}
	if (translationsToInstall.size() == 1)
		mNewTranslation = translationsToInstall[0]->Name();

	QMaemo5InformationBox::information(NULL,
									"Translations Installed Successfully",
                                    QMaemo5InformationBox::DefaultTimeout);
	QDialog::accept();
}

void InstallTranslationsDialog::showEvent(QShowEvent*)
{
	QTimer::singleShot(100, this, SLOT(postShow()));
}

QString InstallTranslationsDialog::getNewTranslation()
{
	return mNewTranslation;
}

void InstallTranslationsDialog::postShow()
{
	QString warning = "Katana is about to connect over the internet to "
					"download translations.\n"
					"If you live in a persecuted country and do not wish to"
					" risk detection, you should *not* use this feature!\n\n"
					"Do you want to continue?";
	if (QMessageBox::warning(this, "Warning!", warning,
			QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes)
	{
		reject();
		return;
	}
	mInstallMgr->setUserDisclaimerConfirmed(true);
	mStatusReporter->startProcess("Refreshing remote source");
	if (mInstallMgr->refreshRemoteSource(mInstallSource))
	{
		QMessageBox::critical(this, "Unable to refresh",
							"Unable to refresh remote "
							"source. Please check your internet connection.");
		mStatusReporter->endProcess();
		reject();
		return;
	}
	mStatusReporter->endProcess();

	SWMgr* mgr = mInstallSource->getMgr();
    std::map<SWModule*, int> mods =
			InstallMgr::getModuleStatus(*mMainMgr, *mgr);
	for (std::map<SWModule*, int>::iterator it = mods.begin();
		it != mods.end();
		it++)
	{
		if (it->second & InstallMgr::MODSTAT_NEW && 
			strcmp(it->first->Type(), "Biblical Texts") == 0)
		{
			QLocale locale(it->first->Lang());
			QString lang = QLocale::languageToString(locale.language());
			if (lang == "C")
				lang = "Other";
			if (!mTranslations.contains(lang))
				mTranslations[lang] = QList<sword::SWModule*>();
			mTranslations[lang].append(it->first);
		}
	}

	for (int i = 0; i < mTranslations.keys().size(); i++)
	{
		QString key = mTranslations.keys()[i];
		qSort(mTranslations[key].begin(), mTranslations[key].end(), lessThan);
		mLanguageCombo->addItem(mTranslations.keys()[i]);
	}

	int englishIndex = mLanguageCombo->findText("English");
	if (englishIndex != -1)
		mLanguageCombo->setCurrentIndex(englishIndex);
}

void InstallTranslationsDialog::onLanguageChange(const QString& text)
{
	mTransListWidget->clear();

	QList<SWModule*> translations = mTranslations[text];
	for (int i = 0; i < translations.size(); i++)
		mTransListWidget->addItem(translations[i]->Description());
}

bool InstallTranslationsDialog::installModule(sword::SWModule* module)
{
	mStatusReporter->startProcess(QString("Installing ") +
								module->Description());
	int result = mInstallMgr->installModule(mMainMgr, 0,
											module->Name(), mInstallSource);
	mStatusReporter->endProcess();
	return !result;
}

DeleteTranslationsDialog::DeleteTranslationsDialog(QWidget* parent) :
	QDialog(parent)
{
	setWindowTitle("Remove Translations");
	setMinimumHeight(800);

	mInstallMgr = new InstallMgr(installPath_c.toAscii().data());
	mInstallSource = mInstallMgr->sources.find(
						sourceName_c.toAscii().data())->second;

	mMainMgr = new SWMgr;
	for (ModMap::iterator it = mMainMgr->Modules.begin();
		it != mMainMgr->Modules.end();
		it++)
	{
		if (strcmp(it->second->Type(), "Biblical Texts") == 0)
			mTranslations.append(it->second);
	}
	qSort(mTranslations.begin(), mTranslations.end(), lessThan);

	// Layout
	QHBoxLayout* mainLayout = new QHBoxLayout;
	mTransListWidget = new QListWidget;
	mTransListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
	for (int i = 0; i < mTranslations.size(); i++)
		mTransListWidget->addItem(mTranslations[i]->Description());
	mainLayout->addWidget(mTransListWidget);
	QPushButton* deleteButton = new QPushButton("Delete");
	QVBoxLayout* deleteLayout = new QVBoxLayout;
	deleteLayout->insertStretch(1);
	deleteLayout->addWidget(deleteButton);
	mainLayout->addLayout(deleteLayout);
	setLayout(mainLayout);

	connect(deleteButton, SIGNAL(clicked()), this, SLOT(accept()));
}

void DeleteTranslationsDialog::accept()
{
	if (mTransListWidget->selectedItems().count() == mTranslations.size())
	{
		QMessageBox::critical(this, "Error",
							"You can't remove all the translations!");
		return;
	}
	QString question = "Are you sure that you want to delete the"
						"selected translations?";
	if (QMessageBox::question(this, "Delete Translations?", question,
						QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes)
		return;

	for (int i = 0; i < mTransListWidget->selectedItems().count(); i++)
	{
		int index = mTransListWidget->row(
						mTransListWidget->selectedItems()[i]);
		mInstallMgr->removeModule(mMainMgr, mTranslations[index]->Name());
	}

	QMaemo5InformationBox::information(NULL,
									"Translations Deleted Successfully",
                                    QMaemo5InformationBox::DefaultTimeout);
	QDialog::accept();
}


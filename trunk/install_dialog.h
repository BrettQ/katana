#include <QDialog>
#include <QMap>

namespace sword
{
	class InstallMgr;
	class InstallSource;
	class SWMgr;
	class SWModule;
}
class QComboBox;
class QListWidget;
class DlgStatusReporter;
class InstallTranslationsDialog : public QDialog
{
	Q_OBJECT
public:
	InstallTranslationsDialog(QWidget* parent);

	QString getNewTranslation();

protected slots:
	void swordClicked();
	void pdbClicked();

protected:
	QString mNewTranslation;

	QPushButton* mSwordButton;
	QPushButton* mPDBButton;
};

class InstallSwordTranslationsDlg : public QDialog
{
	Q_OBJECT
public:
	InstallSwordTranslationsDlg(QWidget* parent);

	QString getNewTranslation();

public slots:
	virtual void accept();
	virtual void showEvent(QShowEvent* event);
	void postShow();
	void onLanguageChange(const QString& text);

protected:
	bool installModule(sword::SWModule* module);

private:
	sword::InstallMgr* mInstallMgr;
	sword::InstallSource* mInstallSource;
	sword::SWMgr* mMainMgr;
	QMap<QString, QList<sword::SWModule*> > mTranslations;

	DlgStatusReporter* mStatusReporter;

	QString mNewTranslation;

	// Widgets
	QComboBox* mLanguageCombo;
	QListWidget* mTransListWidget;
};

class DeleteTranslationsDialog : public QDialog
{
	Q_OBJECT
public:
	DeleteTranslationsDialog(QWidget* parent);

public slots:
	virtual void accept();

protected:
	sword::InstallMgr* mInstallMgr;
	sword::InstallSource* mInstallSource;
	sword::SWMgr* mMainMgr;
	QList<sword::SWModule*> mTranslations;

	// Widgets
	QListWidget* mTransListWidget;
};

bool installTranslationIfNecessary();

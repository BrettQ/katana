#include <QDialog>
#include <QMap>

namespace sword
{
	class InstallMgr;
	class InstallSource;
	class SWMgr;
	class SWModule;
}
class QListWidget;
class DlgStatusReporter;
class InstallTranslationsDialog : public QDialog
{
	Q_OBJECT
public:
	InstallTranslationsDialog(QWidget* parent);

	QString getNewTranslation();

public slots:
	virtual void accept();
	virtual void showEvent(QShowEvent* event);
	virtual void postShow();

protected:
	bool installModule(sword::SWModule* module);

private:
	sword::InstallMgr* mInstallMgr;
	sword::InstallSource* mInstallSource;
	sword::SWMgr* mMainMgr;
	QList<sword::SWModule*> mTranslations;

	DlgStatusReporter* mStatusReporter;

	QString mNewTranslation;

	// Widgets
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

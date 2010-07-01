#include <QWidget>
#include <QMap>

class QEventLoop;
class QListWidget;
class QListWidgetItem;

class SelectTranslationWidget : public QWidget
{
	Q_OBJECT
public:
	SelectTranslationWidget(QWidget* parent);

	bool display(QString& translation);

protected slots:
	void onSelect(QListWidgetItem* item);
	virtual void onInstallClicked();
	virtual void onDeleteClicked();

protected:
	QListWidget* mList;
	QMap<QString, QString> mTranslations;
	QEventLoop* mEventLoop;
	QString mSelected;
};

bool selectTranslation(QWidget* parent, QString& translation);


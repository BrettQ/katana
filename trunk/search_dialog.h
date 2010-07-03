#include <QDialog>
#include <QMap>

class QLineEdit;
class QComboBox;
class TextSource;
class SearchDialog : public QDialog
{
	Q_OBJECT
public:
	SearchDialog(QWidget* parent, QString startingText,
				TextSource* currentSource);

	QString getSearchText() const { return mSearchText; }
	QString getSearchScope() const { return mSearchScope; }

public slots:
	virtual void accept();

private:
	QString mSearchText;
	QString mSearchScope;

	QLineEdit* mSearchEdit;
	QComboBox* mScopeCombo;
	QMap<QString, QString> mScopeOptions;
};



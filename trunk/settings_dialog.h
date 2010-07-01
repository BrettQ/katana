#include <QDialog>
#include <QMap>

class QCheckBox;
class QComboBox;
class QPushButton;
class SettingsDialog : public QDialog
{
	Q_OBJECT
public:
	SettingsDialog(QWidget* parent);

	// Returns true if a single new translation was installed.
	bool getNewTranslation(QString& translationName);

protected:
	void selectItem(QComboBox* combo, QString text);

public slots:
	virtual void accept();

private:
	QCheckBox* mNewLineCheck;
	QComboBox* mFontSizeCombo;
	QPushButton* mInstallButton;
	QPushButton* mDeleteButton;
	QPushButton* mSaveButton;

	QString mNewTranslation;
};

bool shouldUseNewLineForVerses();

int getTextFontSize();


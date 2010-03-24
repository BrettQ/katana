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

protected:
	void selectItem(QComboBox* combo, QString text);

public slots:
	virtual void accept();

private:
	QCheckBox* mNewLineCheck;
	QComboBox* mFontSizeCombo;
	QPushButton* mSaveButton;
};

bool shouldUseNewLineForVerses();

int getTextFontSize();


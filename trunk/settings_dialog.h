#include <QDialog>
#include <QMap>

class QCheckBox;
class QPushButton;
class SettingsDialog : public QDialog
{
	Q_OBJECT
public:
	SettingsDialog(QWidget* pParent);

public slots:
	virtual void accept();

private:
	QCheckBox* mNewLineCheck;
	QPushButton* mSaveButton;
};

bool useNewLineForVerses();


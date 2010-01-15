#include "precompiled.h"

class SelectLayout;
class SelectDialog : public QDialog
{
	Q_OBJECT

public:
	// choicesDescrip should be a single noun (e.g. "Verse")
	static bool select(QWidget* parent, QList<QStringList> choices,
					QString choicesDescrip, QString startingFilter,
					QString& selectedChoice);
	// selectNoFilter does not allow filtering choices
	static bool selectNoFilter(QWidget* parent, QList<QStringList> choices,
					QString choicesDescrip, QString& selectedChoice);

protected:
	SelectDialog(QWidget* parent, QList<QStringList> choices,
				QString choicesDescrip, QString* startingFilter);

protected:
	void setChoices();
	void appendChoices(QStringList choices, SelectLayout* parentLayout);
	void filterChoices();

	virtual void keyPressEvent(QKeyEvent* event);

private slots:
	void selectChoice(const QString& choice);

protected:
	QList<QStringList> mChoices;
	QList<QPushButton*> mButtons;

	QString mSelectedChoice;
	QString mFilterText;
	bool mShouldFilter;

    SelectLayout* mLayout;
	QFrame* mFrame;
	QSignalMapper* mSignalMapper;
	QMaemo5KineticScroller* mScroller;
};

class BibleInfo;
bool selectVerse(QWidget* parent, BibleInfo* bible,
				QString startingFilter,
				QString& bookName, int& chapter);
bool selectTranslation(QWidget* parent, QString& translation);

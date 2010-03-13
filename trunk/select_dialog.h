#include <QtGui>

class SelectLayout;
class Selector : public QObject
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

	bool onKey(int key, QString text);
	void close();

protected:
	Selector(QWidget* parent, QList<QStringList> choices,
				QString choicesDescrip, QString* startingFilter);
	virtual ~Selector();
	bool display();

protected:
	void setChoices();
	void appendChoices(QStringList choices, SelectLayout* parentLayout);
	void filterChoices();

private slots:
	void selectChoice(const QString& choice);

protected:
	QList<QStringList> mChoices;
	QList<QPushButton*> mButtons;

	QScrollArea* scroll;
	QFrame* mFrame;
	QString mSelectedChoice;
	QString mFilterText;
	bool mShouldFilter;

	QEventLoop* mEventLoop;
	SelectLayout* mLayout;
	QSignalMapper* mSignalMapper;
};

class SelectFrame : public QScrollArea
{
	Q_OBJECT

public:
	SelectFrame(QWidget* parent, Selector* selector);

protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void closeEvent(QCloseEvent* event);

private:
	Selector* mSelector;
};

class BibleInfo;
bool selectVerse(QWidget* parent, BibleInfo* bible,
				QString startingFilter,
				QString& bookName, int& chapter);
bool selectTranslation(QWidget* parent, QString& translation);

// Returns true if a select dialog is open to handle key.
bool onDialogKey(int key, QString text);

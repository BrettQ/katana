#include <QtGui>

class SelectLayout;
class Selector : public QObject
{
	Q_OBJECT

public:
	// choicesDescrip should be a single noun (e.g. "Verse")
	static bool select(QWidget* parent, QList<QStringList> choices,
					QString choicesDescrip, QString startingFilter,
					QString& searchText,
					bool& searchImmediately,
					QString& selectedChoice);
	// selectNoFilter does not allow filtering choices
	static bool selectNoFilter(QWidget* parent, QList<QStringList> choices,
					QString choicesDescrip, QString& selectedChoice);

	bool onKey(int key, QString text);
	void close();
	void searchEditClicked();

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
	void searchClicked();


protected:
	QList<QStringList> mChoices;
	QList<QPushButton*> mButtons;

	QScrollArea* scroll;
	QFrame* mFrame;
	QString mSelectedChoice;
	QString mSearchText;
	bool mSearchImmediately;
	QString mFilterText;
	bool mShouldFilter;
	QLineEdit* mSearchEdit;
	QPushButton* mSearchButton;

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

class SearchEdit : public QLineEdit
{
	Q_OBJECT
public:
	SearchEdit(QWidget* parent, Selector* selector);

protected slots:
	void onClick();

protected:
	virtual void focusInEvent(QFocusEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);

	Selector* mSelector;
	bool mFocused;
};

class SelectResult
{
public:
	enum Type
	{
		Type_SearchText,
		Type_SearchDialog,
		Type_SelectedVerse
	};
	static SelectResult search(QString text, bool searchImmediately);

	static SelectResult verse(QString bookName, int chapter);

	Type getType() const;

	QString search_GetText() const;

	QString verse_GetBook() const;
	int verse_GetChapter() const;

protected:
	Type mType;
	QString mSearchText;
	QString mBookName;
	int mChapter;
};
class BibleInfo;
bool selectVerse(QWidget* parent, BibleInfo* bible,
				QString startingFilter,
				SelectResult& result);
bool selectTranslation(QWidget* parent, QString& translation);

// Returns true if a select dialog is open to handle key.
bool onDialogKey(int key, QString text);

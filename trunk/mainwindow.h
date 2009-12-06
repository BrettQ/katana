#include <QDialog>
#include <QHBoxLayout>
#include <QMainWindow>

#include "bible_text_source.h"

class BibleInfo;
class InfiniteScrollViewer;
class Key;
class QSignalMapper;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow();

protected:
	virtual void closeEvent(QCloseEvent* event);

	void createActions();
	void createMenu();

	void createSearchResultsPane(QList<Key> results);

	void replaceViewer(InfiniteScrollViewer* viewer);

protected slots:
	void selectVerse();
	void selectTranslation();
	void search();
	void goToVerse(QString verse);

private:
	BibleInfo* mBible;
	InfiniteScrollViewer* mpViewer;

	QHBoxLayout* mLayout;
	QAction* mSelectVerseAction;
	QAction* mSelectTransAction;

	// Searching
	QString mCurrentSearchText;
	QAction* mSearchAction;
	QSignalMapper* mSearchResultsMapper;
};

class QMaemo5KineticScroller;
class SelectDialog : public QDialog
{
	Q_OBJECT

public:
	// choicesDescip should be a single noun (e.g. "Verse")
	static bool select(QWidget* parent, QList<QStringList> choices,
						QString choicesDescrip, QString& selectedChoice);

protected:
	SelectDialog(QWidget* parent, QList<QStringList> choices,
				QString choicesDescrip);

protected:
	void appendChoices(QStringList choices, QVBoxLayout* parentLayout);

private slots:
	void selectChoice(const QString& choice);

protected:
	QList<QStringList> mChoices;

	QString mSelectedChoice;

	QHBoxLayout mLayout;
	QSignalMapper* mSignalMapper;
	QList<QWidget*> mButtons;
	QMaemo5KineticScroller* mScroller;
};

bool selectVerse(QWidget* parent, BibleInfo* bible,
				QString& bookName, int& chapter, int& verse);
bool selectTranslation(QWidget* parent, QString& translation);


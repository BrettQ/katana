#include <QDialog>
#include <QHBoxLayout>
#include <QMainWindow>

#include "bible_text_source.h"

class BibleInfo;
class InfiniteScrollViewer;
class Key;
class QSignalMapper;
class SearchResultsFrame;
class QFrame;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow();

protected:
	virtual void closeEvent(QCloseEvent* event);

	void createMenu();

	void replaceViewer(InfiniteScrollViewer* viewer);

	virtual void keyPressEvent(QKeyEvent* event);
	void selectVerse(QString startingFilter);
	void search(QString text, QString scope);
	void startSearch(QString text);

	InfiniteScrollViewer* createViewer(QString book, int chapter, int verse);
	InfiniteScrollViewer* createViewerWithHighlight(QString book, int chapter,
												int verse, QString highlight);

protected slots:
	void onSelectVerse();
	void selectTranslation();
	void onSearch();
	void onSettings();
	void goToVerse(QString verse);

private:
	BibleInfo* mBible;
	InfiniteScrollViewer* mpViewer;

	bool mShowShortTitle;
	QHBoxLayout* mLayout;
	SearchResultsFrame* mSearchResults;

	// Searching
	QString mCurrentSearchText;
};



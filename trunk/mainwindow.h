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

	void setLandscape();
	void setPortrait();

	bool event(QEvent* ev);

	virtual void keyPressEvent(QKeyEvent* event);
	void selectVerse(QString startingFilter);
	void search(QString text, QString scope);
	void startSearch(QString text);

protected slots:
	void onSelectVerse();
	void selectTranslation();
	void onSearch();
	void goToVerse(QString verse);
	void orientationChanged(const QString& newOrientation);

private:
	BibleInfo* mBible;
	InfiniteScrollViewer* mpViewer;

	bool mShowShortTitle;
	QHBoxLayout* mLayout;
	SearchResultsFrame* mSearchResults;

	// Searching
	QString mCurrentSearchText;
};



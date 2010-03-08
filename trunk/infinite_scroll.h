
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextBrowser>

// A pure-interface class used by InfiniteScrollViewer.
class TextSource
{
public:
	virtual QString getSourceName()=0;
	virtual QString getSourceDescrip(bool bShort)=0;
	virtual int getNumSections()=0;
	virtual int getNumParagraphs(int section)=0;
	virtual QString getText(int section, int paragraph)=0;
};

class SearchResultsHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT
public:
	SearchResultsHighlighter(QTextDocument* document, QString text) : \
		QSyntaxHighlighter(document)
	{
		mText = text;
	}

protected:
	virtual void highlightBlock(const QString& text);

private:
	QString mText;
};

class QTextDocument;
class QTextCursor;
class QSyntaxHighlighter;
class InfiniteScrollViewer : public QTextBrowser
{
	Q_OBJECT
public:
	InfiniteScrollViewer(QWidget* mainWindow, TextSource* textSource,
						int startingSection, int startingParagraph,
						QString searchText, bool shortTitle);
	~InfiniteScrollViewer();

	QString getSourceName();
	int getCurrentSection();
	int getCurrentParagraph();

	void setShowShortTitle(bool bShow);

	void scrollPage(bool bUp);

protected:
	void fillInitial(int startingSection, int startingParagraph);
	void fillTopTextIfNecessary();
	void fillBottomTextIfNecessary();
	void fillBottomText();
	bool filledToEnd();
	void insertParagraph(QTextCursor& cursor, int section, int paragraph);
	void insertSectionStart(QTextCursor& cursor, int section);

	void startBlock(QTextCursor& cursor);

	void scrollTo(int section, int paragraph);

	void updatePosition();
	void updateTitle();

	void rebuildAnchorPositions();

protected slots:
	void initialScroll();
	void onScroll();
	void onScrollTimer();

protected:
	virtual void showEvent(QShowEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual bool event(QEvent* ev);

private:
	int getTopPadding();
	int getBottomPadding();

private:
	TextSource* mTextSource;
	QTextDocument* mDocument;
	QWidget* mMainWindow;
	QSyntaxHighlighter* mHighlighter;

	bool mShowShortTitle;

	int mFirstSection;
	int mFirstParagraph;
	int mLastSection;
	int mLastParagraph;

	int mCurrentSection;
	int mCurrentParagraph;

	QList<QString> mAnchorNames;
	QList<int> mAnchorPositions;
};


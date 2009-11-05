
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextEdit>

// A pure-interface class used by InfiniteScrollViewer.
class TextSource
{
public:
	virtual QString getSourceName()=0;
	virtual QString getSourceDescrip()=0;
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
class InfiniteScrollViewer : public QTextEdit
{
	Q_OBJECT
public:
	InfiniteScrollViewer(QWidget* mainWindow, TextSource* textSource,
						int starting_section, int starting_paragraph,
						bool highlightStart, QString searchText);
	~InfiniteScrollViewer();

	QString getSourceName();
	QString getSourceDescrip();
	int getCurrentSection();
	int getCurrentParagraph();

protected:
	void fillInitial(int starting_section, int starting_paragraph);
	void fillTopText();
	void fillBottomText();
	void insertParagraph(QTextCursor& cursor, int section, int paragraph);
	void insertSectionStart(QTextCursor& cursor, int section);

	void startBlock(QTextCursor& cursor);

	void scrollTo(int section, int paragraph);

	void updatePosition();
	void updateTitle();

	void rebuildAnchorPositions();

protected slots:
	void initialScroll();

protected:
	virtual void showEvent(QShowEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

private:
	int getTopPadding();
	int getBottomPadding();

private:
	TextSource* mTextSource;
	QTextDocument* mDocument;
	QWidget* mMainWindow;
	QSyntaxHighlighter* mHighlighter;

	int mHighlightStart;
	int mFirstSection;
	int mFirstParagraph;
	int mLastSection;
	int mLastParagraph;

	int mCurrentSection;
	int mCurrentParagraph;

	QList<QString> mAnchorNames;
	QList<int> mAnchorPositions;
};


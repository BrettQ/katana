#include <QString>
#include <QMap>

class QProgressDialog;

class Key
{
public:
	Key(QString book, int chapter, int verse)
	{
		mBook = book;
		mChapter = chapter;
		mVerse = verse;
	}

	static Key fromString(QString string);
	QString toString();

	QString mBook;
	int mChapter;
	int mVerse;
};

// A pure-interface class used by InfiniteScrollViewer.
class TextSource
{
public:
	QList<QStringList> getSuperSections();

	void setSuperSection(QString superSection);
	int getNumSections();
	int getNumParagraphs(int section);
	QString getText(int section, int paragraph);
	bool isUnicode();

	QString getSourceName();
	QString getSourceDescrip(bool useShort);
	QString getSuperSectionName();

	// Searching
	bool search(QString text, QString scope,
				QProgressDialog* progress, QList<Key>& results);

	int getNumChapters(QString bookName);

protected:
	virtual QList<QStringList> derived_getBooks()=0;

	virtual int derived_getBookNum(QString book)=0;
	virtual QString derived_getBookName(int bookNum)=0;
	virtual int derived_getNumChapters(int book)=0;
	virtual int derived_getNumVerses(int book, int chapter)=0;
	virtual QString derived_getText(int book, int chapter, int verse)=0;
	virtual bool derived_isUnicode()=0;

	virtual QString derived_getSourceName()=0;
	virtual QString derived_getSourceDescrip(bool useShort)=0;

	// Scope always has zero or two elements,
	// a start and end verse (inclusive.)
	virtual bool derived_search(QString text, QList<Key> scope,
						QProgressDialog* progress, QList<Key>& results)=0;

protected:
	int mBook;
	int mNumChapters;
	QMap<int, int> mChapterVerses;
};


#include <QString>
#include <QList>

class PDBFile;
class VersionHeader;
class WordRetriever;
class BookInfo;

class BibleFile
{
public:
	BibleFile();
	~BibleFile();

	bool open(QString path, QString& error);

	int getNumBooks();
	int getNumChapters(int bookNum);
	int getNumVerses(int bookNum, int chapter);

	QString getBookShortName(int bookNum);
	QString getBookLongName(int bookNum);

	QString getVerse(int bookNum, int chapter, int verse);

protected:
	PDBFile* mFile;
	VersionHeader* mHeader;
	void* mWordIndexInfo;
	WordRetriever* mWordRetriever;
	QList<BookInfo*> mBooks;
};

void test();


#include <QString>
#include <QList>

class PDBFile;
class TranslationHeader;
class WordRetriever;
class BookInfo;

class BibleFile
{
public:
	BibleFile();
	~BibleFile();

	bool open(QString path, QString& error);

	QString getDescription();

	int getNumBooks();
	int getNumChapters(int bookNum);
	int getNumVerses(int bookNum, int chapter);
	int getTotalVerses(int startBook=-1, int endBook=-1);

	QString getBookShortName(int bookNum);
	QString getBookLongName(int bookNum);

	QString getVerse(int bookNum, int chapter, int verse);

protected:
	PDBFile* mFile;
	TranslationHeader* mHeader;
	void* mWordIndexInfo;
	WordRetriever* mWordRetriever;
	QList<BookInfo*> mBooks;
};


#include "pdb.h"

#include <iostream>

#include <QFile>
#include <QDataStream>
#include <QRegExp>
#include <QStringList>

#include <assert.h>

/* The PalmBible+ format:
This format uses the standard PDB file and records:
http://en.wikipedia.org/wiki/PDB_%28Palm_OS%29

The first record in the database is the TranslationHeader.
It defines basic attributes of the file including the location of 
the word index. After the header, the record contains BookHeader
records for each book.

The BookHeader points to a book index record. This index consists of
different chapter/verse indices for efficient seeking. (See BookInfo::load()
for more details.)

The word index is a list of all words, grouped by length, that appear
in the translation. Actual verse content consists of contiguous numbers
(unsigned shorts) that point to words in the index. Some "words" in the
word index are really two words joined together; index of text, the word
in the word index consists of four bytes, a pair of (unsigned short)
word indices.

Besides actual verse text, the verse content contains book and chapter titles.
To delineate these sections, each section is preceded by a control word
(control words are greater than 0xFFFB.) We ignore the chapter and book
titles, and only look at verse text (whose control word is 0xFFFC.)

The verse text contains non-Biblical content, surrounded by braces. We ignore
this content by regex'ing it out.

T

*/
#pragma pack(push, 1)

struct PDBHeader
{
	char mName[32];
	unsigned short mAttributes;
	unsigned short mVersion;
	unsigned long mCreateTime;
	unsigned long mModTime;
	unsigned long mBackupTime;
	unsigned long mModNum;
	unsigned long mAppInfo;
	unsigned long mSortInfo;
	unsigned long mType;
	unsigned long mCreator;
	unsigned long mUniqueIDSeed;
	unsigned long mNextRecordList;
	unsigned short mNumRecords;
};

struct RecHeader
{
	unsigned long mOffset;
	unsigned char mAttributes;
	unsigned char mUniqueID[3];
};

struct TranslationHeader
{
	char mName[16];
	char mDesc[128];
	char mSeparator;
	char mAttributes;
	unsigned short mWordIndex;
	unsigned short mTotalWordRecords;
	unsigned short mTotalBooks;
};

struct BookHeader
{
	unsigned short mBookNum;
	unsigned short mBookIndex;
	unsigned short mTotalBookRecords;
	char mShortName[8];
	char mLongName[32];
};

struct WordIndexHeader
{
	unsigned short mWordLength;
	unsigned short mTotalWords;
	char mIsCompressed;
	char mUnused;
};

#pragma pack(pop)

inline void fixEndian(unsigned short& x)
{
    x = (x>>8) | 
        (x<<8);
}

inline void fixEndian(unsigned long& x)
{
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}


class PDBFile
{
public:
	PDBFile();
	~PDBFile();

	bool load(QString path, QString& error);

	QString getName();
	int getNumRecords();

	void* getRecord(int recordNum, int& recSize);

protected:
	QFile* mFile;
	QString mName;
	int mNumRecords;
};

PDBFile::PDBFile()
{
	mNumRecords = 0;
}

PDBFile::~PDBFile()
{
	delete mFile;
	mFile = NULL;
}

bool PDBFile::load(QString path, QString& error)
{
	mFile = new QFile(path);
	if (!mFile->open(QIODevice::ReadOnly))
	{
		error = "Unable to open file.";
		return false;
	}
	PDBHeader header;
	mFile->read((char*)&header, sizeof(header));
	fixEndian(header.mNumRecords);

	mName = header.mName;
	mNumRecords = header.mNumRecords;
	return true;
}

QString PDBFile::getName()
{
	return mName;
}

int PDBFile::getNumRecords()
{
	return mNumRecords;
}

int getRecOffset(QFile* file, int recordNum)
{
	int headerStart = sizeof(PDBHeader) + sizeof(RecHeader) * recordNum;
	file->seek(headerStart);
	RecHeader header;
	file->read((char*)&header, sizeof(header));
	fixEndian(header.mOffset);
	return header.mOffset;
}
void* PDBFile::getRecord(int recordNum, int& recSize)
{
	int recOffset = getRecOffset(mFile, recordNum);
	int nextRecOffset = -1;
	if (recordNum < mNumRecords-1)
		nextRecOffset = getRecOffset(mFile, recordNum+1);
	else
		nextRecOffset = mFile->size();

	recSize = nextRecOffset - recOffset;
	char* pRecord = (char*)malloc(recSize);
	mFile->seek(recOffset);
	mFile->read(pRecord, recSize);
	return pRecord;
}

class WordRetriever
{
public:
	WordRetriever(PDBFile* file)
	{
		mWordIndex = NULL;
		mWordData = NULL;
		mFile = file;
	}
	~WordRetriever()
	{
		free(mWordIndex);
		free(mWordData);
	}
	bool init(int wordIndexRecNum, int totalWordRecords, QString& /*error*/)
	{
		int indexLength = -1;
		mWordIndex = (char*)mFile->getRecord(wordIndexRecNum,
											indexLength);
		unsigned short totalIndices = *(unsigned short*)mWordIndex;
		fixEndian(totalIndices);
		char* nextRecord = mWordIndex + sizeof(short);

		for (int i = 0; i < totalIndices; i++)
		{
			WordIndexHeader* header = (WordIndexHeader*)nextRecord;
			fixEndian(header->mWordLength);
			fixEndian(header->mTotalWords);
			mWordIndices.append(header);
			nextRecord += sizeof(WordIndexHeader);
		}

		// Load word index
		char* wordIndexRecords[totalWordRecords];
		int wordIndexRecordLen[totalWordRecords];
		int totalWordLength = 0;
		for (int i = 0; i < totalWordRecords; i++)
		{
			wordIndexRecords[i] = (char*)mFile->getRecord(wordIndexRecNum+1+i,
														wordIndexRecordLen[i]);
			totalWordLength += wordIndexRecordLen[i];
		}
		mWordData = (char*)malloc(totalWordLength);
		int curOffset = 0;
		for (int i = 0; i < totalWordRecords; i++)
		{
			memcpy(mWordData + curOffset, wordIndexRecords[i],
					wordIndexRecordLen[i]);
			free(wordIndexRecords[i]);
			curOffset += wordIndexRecordLen[i];
		}
		return true;
	}

	QString getWord(int wordNum)
	{
		WordIndexPos pos;
		assert(getWordIndexInfo(wordNum, pos));

		int wordStartOffset = pos.mIndexStart + \
							pos.mRelPos * pos.mIndex->mWordLength;
		return QString::fromAscii(mWordData + wordStartOffset,
								pos.mIndex->mWordLength);
	}
	bool isCompressed(int wordNum, int& length)
	{
		length = 0;
		WordIndexPos pos;
		if (!getWordIndexInfo(wordNum, pos))
		{
			length = 0;
			return mWordIndices.last()->mIsCompressed;
		}
		length = pos.mIndex->mWordLength;
		return pos.mIndex->mIsCompressed;
	}
	int getCompressedWordNum(int wordNum, int offset)
	{
		WordIndexPos pos;
		assert(getWordIndexInfo(wordNum, pos));

		int wordStartOffset = pos.mIndexStart + \
							pos.mRelPos * pos.mIndex->mWordLength + \
							(offset * 2);
		int result = (char)*(mWordData + wordStartOffset) * 256;
		result += (char)*(mWordData + wordStartOffset + 1);
		return result;
	}
protected:
	class WordIndexPos
	{
	public:
		const WordIndexHeader* mIndex;
		int mRelPos;
		int mIndexStart;
	};
	bool getWordIndexInfo(int wordNum, WordIndexPos& pos)
	{
		wordNum--;
		int indexStart = 0;
		for (int i = 0; i < mWordIndices.size(); i++)
		{
			if (wordNum >= mWordIndices[i]->mTotalWords)
			{
				wordNum -= mWordIndices[i]->mTotalWords;
				indexStart += mWordIndices[i]->mTotalWords *
					mWordIndices[i]->mWordLength;
			}
			else
			{
				pos.mIndex = mWordIndices[i];
				pos.mRelPos = wordNum;
				pos.mIndexStart = indexStart;
				return true;
			}
		}
		return false;
	}

	char* mWordIndex;
	char* mWordData;
	QList<WordIndexHeader*> mWordIndices;
	PDBFile* mFile;
};

class BookInfo
{
public:
	BookInfo()
	{
	}

	bool load(BookHeader* bookHeader, char* bookData,
			int dataLength, QString& error)
	{
		error = "";
		mHeader = (BookHeader*)bookHeader;
		mTotalChapters = *(unsigned short*)bookData;
		fixEndian(mTotalChapters);

		int headerLength = sizeof(short) + // mTotalChapters
						(sizeof(short) + sizeof(long)) * mTotalChapters;
		if (dataLength < headerLength)
		{
			error = "Invalid book header.";
			return false;
		}
		dataLength -= headerLength;

		bookData += sizeof(short);
		for (int i = 0; i < mTotalChapters; i++)
		{
			unsigned short numVerses = *(unsigned short*)bookData;
			fixEndian(numVerses);
			mChapterNumVerses.append(numVerses);
			bookData += sizeof(short);
		}
		for (int i = 0; i < mTotalChapters; i++)
		{
			unsigned long verseOffset = *(unsigned long*)bookData;
			fixEndian(verseOffset);
			mChapterCharOffsets.append(verseOffset);
			bookData += sizeof(long);
		}
		int totalVerses = (int)mChapterNumVerses.last();
		if (dataLength < totalVerses * (int)sizeof(short))
		{
			error = "Invalid book header, verses section.";
			return false;
		}
		for (int i = 0; i < totalVerses; i++)
		{
			unsigned short verseOffset = *(unsigned short*)bookData;
			fixEndian(verseOffset);
			mVerseOffsetsWithinChapter.append(verseOffset);
			bookData += sizeof(short);
		}
		return true;
	}

	QString getShortName()
	{
		QString name = QString::fromAscii(mHeader->mShortName,
										strlen(mHeader->mShortName));
		return name.trimmed();
	}

	QString getLongName()
	{
		QString name = QString::fromAscii(mHeader->mLongName,
										strlen(mHeader->mLongName));
		return name.trimmed();
	}

	int getNumChapters()
	{
		return mTotalChapters;
	}

	int getNumVerses(int chapter)
	{
		int prevStart = 0;
		if (chapter > 0)
			prevStart = mChapterNumVerses[chapter-1];
		return mChapterNumVerses[chapter] - prevStart;
	}

	int getTotalVerses()
	{
		return mVerseOffsetsWithinChapter.size();
	}

	int getDataStart()
	{
		return mHeader->mBookIndex+1;
	}

	// chapter and verse are zero-based
	int getVerseStartOffset(int chapter, int verse, int& length)
	{
		int verseStart = getRelVerseStart(chapter, verse);
		length = getRelVerseStart(chapter, verse+1) - verseStart;

		int chapterStart = mChapterCharOffsets[chapter];
		return chapterStart + verseStart;
	}

protected:
	int getRelVerseStart(int chapter, int verse)
	{
		if (verse == 0)
			return 0;

		int absVerseNum = verse - 1;
		if (chapter > 0)
			absVerseNum += mChapterNumVerses[chapter-1];
		return mVerseOffsetsWithinChapter[absVerseNum];
	}
	BookHeader* mHeader;
	unsigned short mTotalChapters;
	QList<unsigned short> mChapterNumVerses;
	QList<unsigned long> mChapterCharOffsets;
	QList<unsigned short> mVerseOffsetsWithinChapter;
};


BibleFile::BibleFile()
{
	mHeader = NULL;
	mFile = new PDBFile;
	mWordRetriever = new WordRetriever(mFile);
}

BibleFile::~BibleFile()
{
	delete mHeader;
}

bool BibleFile::open(QString path, QString& error)
{
	error = "";

	if (!mFile->load(path, error))
		return false;

	int recSize = -1;
	mHeader = (TranslationHeader*)mFile->getRecord(0, recSize);
	if ((unsigned)recSize < sizeof(TranslationHeader))
	{
		error = "Invalid header record.";
		return false;
	}
	fixEndian(mHeader->mWordIndex);
	fixEndian(mHeader->mTotalWordRecords);
	fixEndian(mHeader->mTotalBooks);

	if ((unsigned)recSize < sizeof(TranslationHeader) + 
		sizeof(BookInfo) * mHeader->mTotalBooks)
	{
		error = "Invalid header record, book section.";
		return false;
	}
	if (!(mHeader->mAttributes & 0x02))
	{
		error = "PDB file is byte-shifted. Katana does not "
				"support this file. Please contact katana@joshnisly.com "
				"with this file so that it can be supported.";
		return false;
	}
	// Load book information
	char* nextRecord = (char*)mHeader + sizeof(TranslationHeader);
	for (int i = 0; i < mHeader->mTotalBooks; i++)
	{
		BookHeader* bookHeader = (BookHeader*)nextRecord;
		fixEndian(bookHeader->mBookNum);
		fixEndian(bookHeader->mBookIndex);
		fixEndian(bookHeader->mTotalBookRecords);
		
		int bookDataLength = -1;
		char* bookData = (char*)mFile->getRecord(bookHeader->mBookIndex,
										bookDataLength);
		BookInfo* book = new BookInfo;
		if (!book->load(bookHeader, bookData, bookDataLength, error))
		{
			delete book;
			return false;
		}
		mBooks.append(book);

		nextRecord += sizeof(BookHeader);
	}

	if (!mWordRetriever->init(mHeader->mWordIndex,
							mHeader->mTotalWordRecords, error))
	{
		return false;
	}

	return true;
}

QString BibleFile::getDescription()
{
	QString desc = QString::fromAscii(mHeader->mDesc,
									strlen(mHeader->mDesc));
	// Limit length to ~40 chars
	return desc.trimmed();
}

int BibleFile::getNumBooks()
{
	return mHeader->mTotalBooks;
}
int BibleFile::getNumChapters(int bookNum)
{
	return mBooks[bookNum]->getNumChapters();
}

int BibleFile::getNumVerses(int bookNum, int chapter)
{
	return mBooks[bookNum]->getNumVerses(chapter);
}

int BibleFile::getTotalVerses(int startBook, int endBook)
{
	if (startBook == -1)
		startBook = 0;
	if (endBook == -1)
		endBook = mBooks.size()-1;
	int total = 0;
	for (int i = startBook; i <= endBook; i++)
		total += mBooks[i]->getTotalVerses();
	return total;
}

QString BibleFile::getBookShortName(int bookNum)
{
	return mBooks[bookNum]->getShortName();
}

QString BibleFile::getBookLongName(int bookNum)
{
	return mBooks[bookNum]->getLongName();
}

class WordIterator
{
	const static int recordSize = 4096;

public:
	WordIterator(PDBFile* file, int startRecordNum, int startOffset)
	{
		mFile = file;
		mCurRecord = NULL;
		mCurRecordNum = -1;
		startRecord = startRecordNum;
		mCurOffset = startOffset * 2;
	}

	int getNextWordNum()
	{
		// Make sure that we've got a valid record
		int recordNum = startRecord + mCurOffset / recordSize;
		if (recordNum != mCurRecordNum)
		{
			if (mCurRecord)
				free(mCurRecord);
			int recordLength = -1;
			mCurRecord = (char*)mFile->getRecord(recordNum, recordLength);
			//assert(recordLength == recordSize);
			mCurRecordNum = recordNum;
		}

		int recOffset = mCurOffset - (recordNum - startRecord) * recordSize;
		int wordNum = mCurRecord[recOffset++] * 256;
		wordNum += mCurRecord[recOffset];
		mCurOffset += 2;
		return wordNum;
	}
protected:
	PDBFile* mFile;
	int startRecord;
	int mCurOffset;

	char* mCurRecord;
	int mCurRecordNum;
};

void processWord(int word, WordRetriever* wordRetriever,
				bool& ignoreWords, QStringList& words)
{
	if (word > 0xFFF0)
		ignoreWords = (word != 0xFFFC);
	else if (!ignoreWords && word != 0)
	{
		QString wordResult = wordRetriever->getWord(word);
		if (words.size() > 0 &&
			(wordResult.length() == 1 && wordResult[0].isPunct()))
		{
			words.last() = words.last() + wordResult;
		}
		else
			words.append(wordResult);
	}
}

QString BibleFile::getVerse(int bookNum, int chapter, int verse)
{
	BookInfo* book = mBooks[bookNum];
	int verseLength = 0;
	int offset = book->getVerseStartOffset(chapter, verse, verseLength);
	WordIterator iter(mFile, book->getDataStart(), offset);

	bool ignoreWords = false;
	QStringList words;
	for (int i = 0; i < verseLength; i++)
	{
		unsigned long wordNum = iter.getNextWordNum();

		int repeatLength = 0;
		if (mWordRetriever->isCompressed(wordNum, repeatLength))
		{
			for (int repeatNum = 0; repeatNum < repeatLength / 2; repeatNum++)
			{
				int realWordNum = \
					mWordRetriever->getCompressedWordNum(wordNum, repeatNum);
				processWord(realWordNum, mWordRetriever, ignoreWords, words);
			}
		}
		else
			processWord(wordNum, mWordRetriever, ignoreWords, words);

	}
	QString result = words.join(" ");
	QRegExp braceRegExp("\\{.*\\}");
	result = result.replace(braceRegExp, "");
	return result;
}


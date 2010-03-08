#pragma once

#include <QList>
#include <QString>

class Key
{
public:
	Key(QString book, int chapter, int verse)
	{
		mBook = book;
		mChapter = chapter;
		mVerse = verse;
	}

	QString mBook;
	int mChapter;
	int mVerse;
};

namespace sword
{
	struct SWModule;
}
class QProgressDialog;
class BibleInfo
{
public:
	// This class takes ownership of module.
	BibleInfo(sword::SWModule* module);
	~BibleInfo();

	QString getBibleName();

	QStringList getOTBookNames();
	QStringList getNTBookNames();

	int getBookNum(QString bookName);
	QString getShortBookName(int bookNum);
	int getNumChapters(int book);
	int getNumVerses(int book, int chapter);
	QString getVerseText(int book, int chapter, int verse);

	Key getKeyForString(QString verseDesc);

	// Searching
	QList<Key> search(QString text, QString scope, QProgressDialog* progress);

protected:
	sword::SWModule* mModule;
};

QStringList getAvailableTranslations();

class TextSource;
BibleInfo* getBibleInfo(QString translation);
TextSource* getBibleTextSource(BibleInfo* bible, QString book);


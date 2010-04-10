
#include "bible_text_source.h"
#include "text_source.h"

#include <iostream>

#include <QMap>
#include <QProgressDialog>

#include <sword/listkey.h>
#include <sword/swmgr.h>
#include <sword/swmodule.h>
#include <sword/versekey.h>

using namespace sword;

BibleInfo::BibleInfo(SWMgr* mgr, SWModule* module)
{
	mModule = module;
	mMgr = mgr;
}

BibleInfo::~BibleInfo()
{
	delete mMgr;
}

QString BibleInfo::getBibleName()
{
	return mModule->Name();
}

QStringList BibleInfo::getOTBookNames()
{
	QStringList bookNames;

	VerseKey* key = (VerseKey*)mModule->CreateKey();
	for ((*key) = TOP; !key->Error(); key->Book(key->Book() + 1))
	{
		if (key->Testament() == 1)
			bookNames.push_back(QString::fromUtf8(key->getBookName()));
	}

	delete key;
	return bookNames;
}

QStringList BibleInfo::getNTBookNames()
{
	QStringList bookNames;

	VerseKey* key = (VerseKey*)mModule->CreateKey();
	for ((*key) = TOP; !key->Error(); key->Book(key->Book() + 1))
	{
		if (key->Testament() == 2)
			bookNames.push_back(QString::fromUtf8(key->getBookName()));
	}

	delete key;
	return bookNames;
}

int BibleInfo::getBookNum(QString bookName)
{
	VerseKey* key = (VerseKey*)mModule->CreateKey();
	for ((*key) = TOP; !key->Error(); key->Book(key->Book() + 1))
	{
		if (QString::fromUtf8(key->getBookName()) == bookName)
		{
			int book = key->Book();
			if (key->Testament() > 1)
				book += key->BMAX[0];
			delete key;
			return book;
		}
	}
	delete key;
	return -1;
}

QString BibleInfo::getShortBookName(int bookNum)
{
	VerseKey* key = (VerseKey*)mModule->CreateKey();
	(*key) = TOP;
	key->Book(bookNum);
	return key->getBookAbbrev();
}

int BibleInfo::getNumChapters(int book)
{
	VerseKey* key = (VerseKey*)mModule->CreateKey();
	(*key) = TOP;
	key->Book(book);
	(*key) = MAXCHAPTER;
	int result = key->Chapter();
	delete key;
	return result;
}

int BibleInfo::getNumVerses(int book, int chapter)
{
	VerseKey* key = (VerseKey*)mModule->CreateKey();
	(*key) = TOP;
	key->Book(book);
	key->Chapter(chapter + 1);
	(*key) = MAXVERSE;
	int result = key->Verse();
	delete key;
	return result;
}

QString BibleInfo::getVerseText(int book, int chapter, int verse)
{
	VerseKey* key = (VerseKey*)mModule->CreateKey();
	(*key) = TOP;
	key->Book(book);
	key->Chapter(chapter + 1);
	key->Verse(verse + 1);
	QString text = mModule->RenderText(key);
	delete key;
	return text;
}

void searchCallback(char percent, void* data)
{
	((QProgressDialog*)data)->setValue(percent);
}

QList<Key> BibleInfo::search(QString text, QString scopeString,
							QProgressDialog* progress)
{
	ListKey scope;
	ListKey* scopePtr = NULL;

	if (scopeString != "")
	{
		scope = VerseKey().ParseVerseList(scopeString.toAscii().data(),
												"", true);
		scopePtr = &scope;
	}

	ListKey& results = mModule->search(text.toAscii().data(),
											0, -2, scopePtr, 0,
											searchCallback, progress);
	results.Persist(true);

	QList<Key> verses;
	for (int i = 0; i < results.Count(); i++)
	{
		VerseKey* key = (VerseKey*)results.getElement(i);
		verses.push_back(Key(key->getBookName(),
							key->Chapter()-1,
							key->Verse()-1));
	}

	return verses;
}

Key BibleInfo::getKeyForString(QString verseDesc)
{
	VerseKey* swordKey = (VerseKey*)mModule->CreateKey();
	(*swordKey) = verseDesc.toAscii().data();

	return Key(swordKey->getBookName(),
			swordKey->Chapter()-1,
			swordKey->Verse()-1);
}

class BibleTextSource : public TextSource
{
public:
	// The caller is responsible to delete bible.
	BibleTextSource(BibleInfo* bible, QString bookName)
	{
		mBible = bible;
		mBookName = bookName;
		mBookNum = bible->getBookNum(bookName);
		mShortBookName = bible->getShortBookName(mBookNum);
	}

	virtual QString getSourceName()
	{
		return mBookName;
	}
	virtual QString getSourceDescrip(bool shortTitle)
	{
		if (shortTitle)
			return mShortBookName;
		else
			return mBible->getBibleName() + " - " + mBookName;
	}
	virtual int getNumSections()
	{
		return mBible->getNumChapters(mBookNum);
	}
	virtual int getNumParagraphs(int section)
	{
		if (mCachedNumVerses.contains(section))
			return mCachedNumVerses[section];
		
		int verses = mBible->getNumVerses(mBookNum, section);
		mCachedNumVerses[section] = verses;
		return verses;
	}
	virtual QString getText(int section, int paragraph)
	{
		return mBible->getVerseText(mBookNum, section, paragraph);
	}

private:
	BibleInfo* mBible;
	QString mBookName;
	QString mShortBookName;
	int mBookNum;
	QMap<int, int> mCachedNumVerses;
};

QStringList getAvailableTranslations()
{
	SWMgr library;
	QStringList translations;
	for (ModMap::iterator iter = library.Modules.begin();
		iter != library.Modules.end();
		iter++)
	{
		SWModule* module = (*iter).second;
		if (strcmp(module->Type(), "Biblical Texts") == 0)
			translations.push_back(module->Name());
	}
	return translations;
}

BibleInfo* getBibleInfo(QString translation)
{
	// We allocate a new SWMgr for each translation because SWMgr
	// can't handle available translations updating out from under it.
	// This allows us to always use an up-to-date SWMgr.
	SWMgr* mgr = new SWMgr;
	SWModule* module = mgr->getModule(translation.toAscii().data());
	if (!module)
	{
		QStringList translations = getAvailableTranslations();
		if (!translations.count())
		{
			std::cout << "No texts available!\n";
			return NULL;
		}
		module = mgr->getModule(translations[0].toAscii().data());
	}
	return new BibleInfo(mgr, module);
}

TextSource* getBibleTextSource(BibleInfo* bible, QString book)
{
	return new BibleTextSource(bible, book);
}


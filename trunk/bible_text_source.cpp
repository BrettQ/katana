
#include "bible_text_source.h"
#include "infinite_scroll.h" // TODO: this should be broken out into text_source.h

#include <iostream>

#include <QProgressDialog>

#include <sword/listkey.h>
#include <sword/swmgr.h>
#include <sword/swmodule.h>
#include <sword/versekey.h>

sword::SWMgr library;

BibleInfo::BibleInfo(sword::SWModule* module)
{
	mModule = module;
}

BibleInfo::~BibleInfo()
{
	delete mModule;
}

QString BibleInfo::getBibleName()
{
	return mModule->Name();
}

QStringList BibleInfo::getOTBookNames()
{
	QStringList bookNames;

	sword::VerseKey* key = (sword::VerseKey*)mModule->CreateKey();
	for ((*key) = sword::TOP; !key->Error(); key->Book(key->Book() + 1))
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

	sword::VerseKey* key = (sword::VerseKey*)mModule->CreateKey();
	for ((*key) = sword::TOP; !key->Error(); key->Book(key->Book() + 1))
	{
		if (key->Testament() == 2)
			bookNames.push_back(QString::fromUtf8(key->getBookName()));
	}

	delete key;
	return bookNames;
}

int BibleInfo::getBookNum(QString bookName)
{
	sword::VerseKey* key = (sword::VerseKey*)mModule->CreateKey();
	for ((*key) = sword::TOP; !key->Error(); key->Book(key->Book() + 1))
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

int BibleInfo::getNumChapters(int book)
{
	sword::VerseKey* key = (sword::VerseKey*)mModule->CreateKey();
	(*key) = sword::TOP;
	key->Book(book);
	(*key) = sword::MAXCHAPTER;
	int result = key->Chapter();
	delete key;
	return result;
}

int BibleInfo::getNumVerses(int book, int chapter)
{
	sword::VerseKey* key = (sword::VerseKey*)mModule->CreateKey();
	(*key) = sword::TOP;
	key->Book(book);
	key->Chapter(chapter + 1);
	(*key) = sword::MAXVERSE;
	int result = key->Verse();
	delete key;
	return result;
}

QString BibleInfo::getVerseText(int book, int chapter, int verse)
{
	sword::VerseKey* key = (sword::VerseKey*)mModule->CreateKey();
	(*key) = sword::TOP;
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
	sword::ListKey scope;
	sword::ListKey* scopePtr = NULL;

	if (scopeString != "")
	{
		scope = sword::VerseKey().ParseVerseList(scopeString.toAscii().data(),
												"", true);
		scopePtr = &scope;
	}

	sword::ListKey& results = mModule->search(text.toAscii().data(),
											0, -2, scopePtr, 0,
											searchCallback, progress);
	results.Persist(true);

	QList<Key> verses;
	for (int i = 0; i < results.Count(); i++)
	{
		sword::VerseKey* key = (sword::VerseKey*)results.getElement(i);
		verses.push_back(Key(key->getBookName(),
							key->Chapter()-1,
							key->Verse()-1));
	}

	return verses;
}

Key BibleInfo::getKeyForString(QString verseDesc)
{
	sword::VerseKey* swordKey = (sword::VerseKey*)mModule->CreateKey();
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
	}

	virtual QString getSourceName()
	{
		return mBookName;
	}
	virtual QString getSourceDescrip()
	{
		return mBible->getBibleName() + " - " + mBookName;
	}
	virtual int getNumSections()
	{
		return mBible->getNumChapters(mBookNum);
	}
	virtual int getNumParagraphs(int section)
	{
		return mBible->getNumVerses(mBookNum, section);
	}
	virtual QString getText(int section, int paragraph)
	{
		return mBible->getVerseText(mBookNum, section, paragraph);
	}

private:
	BibleInfo* mBible;
	QString mBookName;
	int mBookNum;
};

QStringList getAvailableTranslations()
{
	QStringList translations;
	for (sword::ModMap::iterator iter = library.Modules.begin();
		iter != library.Modules.end();
		iter++)
	{
		sword::SWModule* module = (*iter).second;
		if (strcmp(module->Type(), "Biblical Texts") == 0)
			translations.push_back(module->Name());
	}
	return translations;
}

BibleInfo* getBibleInfo(QString translation)
{
	sword::SWModule* module = library.getModule(translation.toAscii().data());
	if (!module)
		return NULL;

	return new BibleInfo(module);
}

TextSource* getBibleTextSource(BibleInfo* bible, QString book)
{
	return new BibleTextSource(bible, book);
}


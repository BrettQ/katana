
#include "bible_text_source.h"
#include "text_source.h"

#include <iostream>

#include <QApplication>
#include <QMap>
#include <QProgressDialog>

#include <sword/listkey.h>
#include <sword/swmgr.h>
#include <sword/swmodule.h>
#include <sword/versekey.h>

using namespace sword;

// Small helper class to pass data to/from search callback
class Search
{
public:
	Search()
	{
		mCancelled = false;
	}

	SWModule* mModule;
	QProgressDialog* mDlg;
	bool mCancelled;
};

void searchCallback(char percent, void* data)
{
	Search* search = (Search*)data;
	QApplication::processEvents();
	search->mDlg->setValue(percent);
	if (search->mDlg->wasCanceled())
	{
		search->mCancelled = true;
		search->mModule->terminateSearch = true;
	}
}

class BibleTextSource : public TextSource
{
public:
	// This class takes ownership of manager and module.
	BibleTextSource(sword::SWMgr* mgr, sword::SWModule* module)
	{
		mModule = module;
		mMgr = mgr;
	}
	~BibleTextSource()
	{
		delete mMgr;
	}

protected:
	virtual QList<QStringList> derived_getBooks()
	{
		QList<QStringList> allBooks;
		for (int i = 1; i <= 2; i++)
		{
			QStringList books;

			VerseKey* key = (VerseKey*)mModule->CreateKey();
			for ((*key) = TOP; !key->Error(); key->Book(key->Book() + 1))
			{
				if (key->Testament() == i)
					books.push_back(QString::fromUtf8(key->getBookName()));
			}
			delete key;
			allBooks.append(books);
		}
		return allBooks;
	}

	virtual int derived_getBookNum(QString bookName)
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
	virtual QString derived_getBookName(int bookNum)
	{
		VerseKey* key = (VerseKey*)mModule->CreateKey();
		(*key) = TOP;
		key->Book(bookNum);
		return key->getBookName();
	}
	virtual int derived_getNumChapters(int book)
	{
		VerseKey* key = (VerseKey*)mModule->CreateKey();
		(*key) = TOP;
		key->Book(book);
		(*key) = MAXCHAPTER;
		int result = key->Chapter();
		delete key;
		return result;
	}
	virtual int derived_getNumVerses(int book, int chapter)
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
	virtual QString derived_getText(int book, int chapter, int verse)
	{
		VerseKey* key = (VerseKey*)mModule->CreateKey();
		(*key) = TOP;
		key->Book(book);
		key->Chapter(chapter + 1);
		key->Verse(verse + 1);
		QString text = mModule->RenderText(key);
		text = text.replace("<scripRef", "<span class=\"scripRef\"");
		text = text.replace("</scripRef>", "</span>");
		delete key;
		return text;
	}
	virtual bool derived_isUnicode()
	{
		return mModule->isUnicode();
	}

	virtual QString derived_getSourceName()
	{
		return mModule->Name();
	}
	virtual QString derived_getSourceDescrip(bool shortTitle)
	{
		if (shortTitle)
			return getShortBookName(mBook);
		else
			return getSourceName() + " - " + \
				derived_getBookName(mBook);
	}
	virtual bool derived_search(QString text, QString scopeString,
						QProgressDialog* progress, QList<Key>& results)
	{
		ListKey scope;
		ListKey* scopePtr = NULL;

		if (scopeString != "")
		{
			scope = VerseKey().ParseVerseList(scopeString.toAscii().data(),
											"", true);
			scopePtr = &scope;
		}

		Search search;
		search.mModule = mModule;
		search.mDlg = progress;
		ListKey& search_results = mModule->search(text.toAscii().data(),
												0, -2, scopePtr, 0,
												searchCallback, &search);
		if (search.mCancelled)
			return false;

		search_results.Persist(true);

		for (int i = 0; i < search_results.Count(); i++)
		{
			VerseKey* key = (VerseKey*)search_results.getElement(i);
			results.push_back(Key(key->getBookName(),
								key->Chapter()-1,
								key->Verse()-1));
		}

		return true;
	}

private:
	QString getShortBookName(int bookNum)
	{
		VerseKey* key = (VerseKey*)mModule->CreateKey();
		(*key) = TOP;
		key->Book(bookNum);
		return key->getBookAbbrev();
	}
	SWMgr* mMgr;
	SWModule* mModule;
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

TextSource* getBibleTextSource(QString translation, QString book)
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
	BibleTextSource* textSource = new BibleTextSource(mgr, module);
	textSource->setSuperSection(book);
	return textSource;
}


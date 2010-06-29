#include <QtGui>

#include "pdb.h"
#include "text_source.h"

QString browseForPDB(QWidget* parent)
{
	QFileDialog dialog;
	dialog.setFileMode(QFileDialog::ExistingFiles);
	if (dialog.exec())
		return dialog.selectedFiles()[0];
	return "";
}

QStringList loadRegisteredPDBs()
{
	QSettings settings;
	QString allPaths = settings.value("pdb/paths", "").toString();
	return allPaths.split('|', QString::SkipEmptyParts);
}

QString pdbPathToName(QString path)
{
	QFileInfo info(path);
	return info.fileName();
}

QStringList getAllPDBNames()
{
	QStringList names;
	QStringList paths = loadRegisteredPDBs();
	for (int i = 0; i < paths.size(); i++)
		names.append(pdbPathToName(paths[i]));
	return names;
}

bool isPDBTranslation(QString translation)
{
	QStringList allPDBs = getAllPDBNames();
	return allPDBs.indexOf(translation) != -1;
}

void saveRegisteredPDBs(const QStringList& paths)
{
	QString allPaths = paths.join("|");
	QSettings settings;
	settings.setValue("pdb/paths", allPaths);
	settings.sync();
}

bool registerPDB(QString path, QString& error)
{
	QStringList paths = loadRegisteredPDBs();
	for (int i = 0; i < paths.size(); i++)
	{
		if (pdbPathToName(paths[i]) == pdbPathToName(path))
		{
			error = "A file with the same name already exists.";
			return false;
		}
	}

	paths.append(path);
	saveRegisteredPDBs(paths);
	return true;
}

class PDBTextSource : public TextSource
{
public:
	PDBTextSource()
	{
		mPDB = NULL;
	}
	~PDBTextSource()
	{
		delete mPDB;
	}
	bool load(QString path, QString& error)
	{
		mPDB = new BibleFile;
		mFileName = QFileInfo(path).fileName();
		return mPDB->open(path, error);
	}
	virtual QList<QStringList> derived_getBooks()
	{
		QStringList books;
		for (int i = 0; i < mPDB->getNumBooks(); i++)
			books.append(mPDB->getBookLongName(i));
		QList<QStringList> allBooks;
		allBooks.append(books);
		return allBooks;
	}

	virtual QString derived_getBookName(int bookNum)
	{
		return mPDB->getBookLongName(bookNum);
	}
	virtual int derived_getBookNum(QString book)
	{
		for (int i = 0; i < mPDB->getNumBooks(); i++)
		{
			if (mPDB->getBookLongName(i) == book)
				return i;
		}
		return 0;
	}
	virtual int derived_getNumChapters(int book)
	{
		return mPDB->getNumChapters(book);
	}
	virtual int derived_getNumVerses(int book, int chapter)
	{
		return mPDB->getNumVerses(book, chapter);
	}
	virtual QString derived_getText(int book, int chapter, int verse)
	{
		return mPDB->getVerse(book, chapter, verse);
	}
	virtual bool derived_isUnicode()
	{
		return false;
	}

	virtual QString derived_getSourceName()
	{
		return mFileName;
	}
	virtual QString derived_getSourceDescrip(bool shortTitle)
	{
		if (shortTitle)
			return mPDB->getBookShortName(mBook);
		else
			return getSourceName() + " - " + \
				mPDB->getBookLongName(mBook);
		return mFileName;
	}

	virtual Key derived_getKeyForString(QString verseDesc)
	{
		return Key("", 0, 0);
	}
	virtual bool derived_search(QString text, QString scope,
						QProgressDialog* progress, QList<Key>& results)
	{
		return false;
	}
private:
	BibleFile* mPDB;
	QString mFileName;
};

TextSource* getPDBTextSource(QString name, QString bookName)
{
	PDBTextSource* source = new PDBTextSource;
	QString error;

	QStringList paths = loadRegisteredPDBs();
	QString path;
	for (int i = 0; i < paths.size(); i++)
	{
		if (pdbPathToName(paths[i]) == name)
			path = paths[i];
	}
	if (path == "" && paths.size() > 0)
		path = paths[0];

	if (!source->load(path, error))
	{
		printf("unable to load! %s\n", error.toAscii().data());
		return NULL;
	}
	source->setSuperSection(bookName);
	return source;
}


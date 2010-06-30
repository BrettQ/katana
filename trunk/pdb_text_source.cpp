#include <QtGui>

#include "pdb.h"
#include "text_source.h"

QStringList loadAllPDBs()
{
	QDir dir("/home/user/MyDocs");
	QStringList pdbs = dir.entryList(QDir::Files, QDir::Name);
	QStringList paths;
	for (int i = 0; i < pdbs.size(); i++)
	{
		if (pdbs[i].toLower().endsWith("pdb"))
			paths.append("/home/user/MyDocs/" + pdbs[i]);
	}
	return paths;
}

QString pdbPathToName(QString path)
{
	QFileInfo info(path);
	return info.fileName();
}

QStringList getAllPDBNames()
{
	QStringList names;
	QStringList paths = loadAllPDBs();
	for (int i = 0; i < paths.size(); i++)
	{
		QFileInfo file(paths[i]);
		if (file.exists() || true)
			names.append(file.fileName());
	}
	return names;
}

bool isPDBTranslation(QString translation)
{
	QStringList allPDBs = getAllPDBNames();
	return allPDBs.indexOf(translation) != -1;
}

bool registerPDB(QString path, QString& error)
{
	QStringList paths = loadAllPDBs();
	for (int i = 0; i < paths.size(); i++)
	{
		if (pdbPathToName(paths[i]) == pdbPathToName(path))
		{
			error = "A file with the same name already exists.";
			return false;
		}
	}

	paths.append(path);
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

	virtual bool derived_search(QString text, QString /*scope*/,
						QProgressDialog* progress, QList<Key>& results)
	{
		int totalSearchedVerses = 0;
		int totalVerses = mPDB->getTotalVerses();
		QRegExp search(text, Qt::CaseInsensitive, QRegExp::FixedString);
		for (int bookNum = 0; bookNum < mPDB->getNumBooks(); bookNum++)
		{
			QString bookName = mPDB->getBookLongName(bookNum);
			for (int chapter = 0;
				chapter < mPDB->getNumChapters(bookNum);
				chapter++)
			{
				for (int verse = 0;
					verse < mPDB->getNumVerses(bookNum, chapter);
					verse++)
				{
					QString text = mPDB->getVerse(bookNum, chapter, verse);
					if (text.indexOf(search) != -1)
						results.append(Key(bookName, chapter, verse));

					totalSearchedVerses++;
					if (totalSearchedVerses % 100 == 0)
					{
						int percent = totalSearchedVerses * 100 /
									totalVerses;
						QApplication::processEvents();
						progress->setValue(percent);
						if (progress->wasCanceled())
							return false;
					}
				}
			}
		}
		return true;
	}
private:
	BibleFile* mPDB;
	QString mFileName;
};

TextSource* getPDBTextSource(QString name, QString bookName)
{
	PDBTextSource* source = new PDBTextSource;
	QString error;

	QStringList paths = loadAllPDBs();
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
		QMessageBox::critical(NULL, "Error",
							"Unable to load PDB file: " + error);
		return NULL;
	}
	source->setSuperSection(bookName);
	return source;
}


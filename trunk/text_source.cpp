#include "text_source.h"

#include <QStringList>

QString Key::toString()
{
	return QString("%1 %2:%3").arg(mBook).arg(mChapter+1).arg(mVerse+1);
}

Key Key::fromString(QString string)
{
	Key key("", 0, 0);
	QStringList parts = string.split(" ");
	if (parts.size() < 2)
		return key;
	QString book = parts[0];
	QStringList refParts = parts.last().split(":");
	if (refParts.size() != 2)
		return key;

	parts.removeLast();
	key.mBook = parts.join(" ");
	key.mChapter = refParts[0].toInt() - 1;
	key.mVerse = refParts[1].toInt() - 1;
	return key;
}

QList<QStringList> TextSource::getSuperSections()
{
	return derived_getBooks();
}

void TextSource::setSuperSection(QString superSection)
{
	mBook = derived_getBookNum(superSection);
	mNumChapters = derived_getNumChapters(mBook);
}

int TextSource::getNumSections()
{
	return mNumChapters;
}

int TextSource::getNumParagraphs(int section)
{
	if (!mChapterVerses.contains(section))
		mChapterVerses[section] = derived_getNumVerses(mBook, section);
	return mChapterVerses[section];
}

QString TextSource::getText(int section, int paragraph)
{
	return derived_getText(mBook, section, paragraph);
}

bool TextSource::isUnicode()
{
	return derived_isUnicode();
}

QString TextSource::getSourceName()
{
	return derived_getSourceName();
}

QString TextSource::getSourceDescrip(bool useShort)
{
	return derived_getSourceDescrip(useShort);
}

QString TextSource::getSuperSectionName()
{
	return derived_getBookName(mBook);
}

int TextSource::getNumChapters(QString bookName)
{
	return derived_getNumChapters(derived_getBookNum(bookName));
}

bool TextSource::search(QString text, QString scopeString,
			QProgressDialog* progress, QList<Key>& results)
{
	QList<Key> scope;
	if (scopeString != "")
	{
		// For now, don't handle multiple ranges.
		scopeString = scopeString.remove(";");
		QStringList keys = scopeString.split("-");
		if (keys.size() != 2)
			return false;
		for (int i = 0; i < keys.size(); i++)
			scope.append(Key::fromString(keys[i].trimmed()));
	}
	return derived_search(text, scope, progress, results);
}


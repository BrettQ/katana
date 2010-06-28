#include "text_source.h"

#include <QStringList>

QList<QStringList> TextSource::getSuperSections()
{
	return derived_getBooks();
}

void TextSource::setSuperSection(QString superSection)
{
	mBook = derived_getBookNum(superSection);
}

int TextSource::getNumSections()
{
	return derived_getNumChapters(mBook);
}

int TextSource::getNumParagraphs(int section)
{
	return derived_getNumVerses(mBook, section);
}

QString TextSource::getText(int section, int paragraph)
{
	return derived_getText(mBook, section, paragraph);
}

bool TextSource::isUnicode()
{
	return derived_isUnicode();
}

Key TextSource::getKeyForString(QString verseDesc)
{
	return derived_getKeyForString(verseDesc);
}

bool TextSource::search(QString text, QString scope,
			QProgressDialog* progress, QList<Key>& results)
{
	return derived_search(text, scope, progress, results);
}

QString TextSource::getSourceName()
{
	return derived_getSourceName();
}

QString TextSource::getSourceDescrip(bool useShort)
{
	return derived_getSourceDescrip(useShort);
}

int TextSource::getNumChapters(QString bookName)
{
	return derived_getNumChapters(derived_getBookNum(bookName));
}

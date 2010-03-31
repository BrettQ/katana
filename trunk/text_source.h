#include <QString>

// A pure-interface class used by InfiniteScrollViewer.
class TextSource
{
public:
	virtual QString getSourceName()=0;
	virtual QString getSourceDescrip(bool bShort)=0;
	virtual int getNumSections()=0;
	virtual int getNumParagraphs(int section)=0;
	virtual QString getText(int section, int paragraph)=0;
};



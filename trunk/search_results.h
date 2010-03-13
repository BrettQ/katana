#include <QFrame>

class Key;
class QVBoxLayout;
class QPushButton;
class QScrollArea;
class QSignalMapper;
class SearchResultsFrame : public QFrame
{
	Q_OBJECT

public:
	SearchResultsFrame();
	~SearchResultsFrame();

	void handleResults(QList<Key> results);
	void hideResults();

signals:
	void resultSelected(const QString& result);

protected slots:
	void onSelect(const QString& result);

protected:
	QVBoxLayout* mLayout;
	QSignalMapper* mSearchResultsMapper;
	QScrollArea* mScroll;
};
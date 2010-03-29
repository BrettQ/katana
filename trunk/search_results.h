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
	bool isShowingResults();
	void hideResults();

signals:
	void resultSelected(const QString& result);

protected slots:
	void onSelect(const QString& result);
	void onHideClicked();

protected:
	QVBoxLayout* mLayout;
	QSignalMapper* mSearchResultsMapper;
	QScrollArea* mScroll;
};

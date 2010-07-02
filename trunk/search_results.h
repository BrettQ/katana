#include <QFrame>

class Key;
class QVBoxLayout;
class QPushButton;
class QListWidget;
class QListWidgetItem;
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
	void onHideClicked();
	void onSelect(QListWidgetItem* item);

protected:
	QVBoxLayout* mLayout;
	QListWidget* mResultsList;
};

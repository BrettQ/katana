#include "select_dialog.h"

#include "bible_text_source.h"

#include <QLayoutItem>
#include <QStringList>

#include <assert.h>
#include <iostream>

Selector* fgActiveDialog = NULL;

class SelectLayout : public QLayout
{
public:
	SelectLayout(QWidget* parent);
	~SelectLayout();

	void addItem(QLayoutItem* item);
	void addBreak();
	int horizontalSpacing() const;
	int verticalSpacing() const;
	Qt::Orientations expandingDirections() const;
	bool hasHeightForWidth() const;
	int heightForWidth(int) const;
	int count() const;
	QLayoutItem* itemAt(int index) const;
	QSize minimumSize() const;
	void setGeometry(const QRect& rect);
	QSize sizeHint() const;
	QLayoutItem* takeAt(int index);

private:
	int doLayout(const QRect& rect, bool testOnly) const;
	int smartSpacing(QStyle::PixelMetric pm) const;

	QList<QLayoutItem*> mItemList;
	QList<bool> mBreakBeforeList;
};

bool Selector::select(QWidget* parent, QList<QStringList> choices,
						QString choicesDescrip, QString startingFilter,
						QString& selectedChoice)
{
	selectedChoice = "";

	Selector select(parent, choices, choicesDescrip, &startingFilter);
	
	if (!select.display())
		return false;
	selectedChoice = select.mSelectedChoice;
	return true;
}

bool Selector::selectNoFilter(QWidget* parent, QList<QStringList> choices,
						QString choicesDescrip, QString& selectedChoice)
{
	selectedChoice = "";

	Selector select(parent, choices, choicesDescrip, NULL);
	
	if (!select.display())
		return false;
	selectedChoice = select.mSelectedChoice;
	return true;
}

Selector::Selector(QWidget* parent, QList<QStringList> choices,
				QString choicesDescrip, QString* startingFilter)
{
	fgActiveDialog = this;
	mChoices = choices;
	if (startingFilter)
	{
		mFilterText = *startingFilter;
		mShouldFilter = true;
	}
	else
		mShouldFilter = false;


	mFrame = new QFrame();
	mSignalMapper = new QSignalMapper();
	connect(mSignalMapper, SIGNAL(mapped(const QString&)), this,
			SLOT(selectChoice(const QString&)));

	scroll = new SelectFrame(parent, this);
	scroll->setAttribute(Qt::WA_Maemo5StackedWindow);
	scroll->setWindowFlags(mFrame->windowFlags() | Qt::Window);
	scroll->setWindowTitle("Select " + choicesDescrip);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
									QSizePolicy::Preferred));

	mFrame->setStyleSheet("QPushButton{"
						"padding: 12px; "
						"margin: 0px; "
						"min-width: 65px;"
						"}");
	setChoices();
	scroll->setWidget(mFrame);
	scroll->show();

	filterChoices();
}

Selector::~Selector()
{
	fgActiveDialog = NULL;
}

bool Selector::display()
{
    mEventLoop = new QEventLoop;
    mEventLoop->exec(QEventLoop::AllEvents);
	return mSelectedChoice != "";
}

void Selector::setChoices()
{
	mLayout = new SelectLayout(mFrame);
	mLayout->setSpacing(0);

	for (int i = 0; i < mChoices.count(); i++)
		appendChoices(mChoices[i], mLayout);
	mFrame->setLayout(mLayout);
}

void Selector::appendChoices(QStringList choices,
								SelectLayout* parentLayout)
{
	for (int i = 0; i < choices.count(); i++)
	{
		QPushButton* button = new QPushButton(choices[i]);
		mButtons.append(button);
		mSignalMapper->setMapping(button, choices[i]);
		connect(button, SIGNAL(clicked()), mSignalMapper, SLOT(map()));
		parentLayout->addWidget(button);
	}
	parentLayout->addBreak();
}
void Selector::filterChoices()
{
	if (!mShouldFilter)
		return;
	int matched = 0;
	QString lastMatch;
	for (int i = 0; i < mButtons.size(); i++)
	{
		if (mFilterText.length() == 0 ||
			mButtons[i]->text().contains(mFilterText, Qt::CaseInsensitive))
		{
			matched++;
			mButtons[i]->show();
			lastMatch = mButtons[i]->text();
		}
		else
			mButtons[i]->hide();
	}
	if (matched == 1)
		selectChoice(lastMatch);
}

bool Selector::onKey(int key, QString text)
{
	if (key >= Qt::Key_A && key <= Qt::Key_Z)
		mFilterText.append(text);
	else if (key == Qt::Key_Backspace)
		mFilterText = mFilterText.left(mFilterText.length() - 1);
	else
		return false;

	filterChoices();
	return true;
}

void Selector::close()
{
	mEventLoop->exit();
}

void Selector::selectChoice(const QString& choice)
{
	mSelectedChoice = choice;
	scroll->close();
}

SelectFrame::SelectFrame(QWidget* parent, Selector* selector) : QScrollArea(parent)
{
	mSelector = selector;
}

void SelectFrame::keyPressEvent(QKeyEvent* event)
{
	if (!mSelector->onKey(event->key(), event->text()))
		QScrollArea::keyPressEvent(event);
}

void SelectFrame::closeEvent(QCloseEvent*)
{
	mSelector->close();
}

SelectLayout::SelectLayout(QWidget* parent) : QLayout(parent)
{
	setContentsMargins(0, 0, 0, 0);
}

SelectLayout::~SelectLayout()
{
	while (true)
	{
		QLayoutItem* item = takeAt(0);
		if (!item)
			break;
		delete item;
	}
}

void SelectLayout::addItem(QLayoutItem* item)
{
	mItemList.append(item);
	mBreakBeforeList.append(false);
}

void SelectLayout::addBreak()
{
	mBreakBeforeList[mBreakBeforeList.count()-1] = true;
}

int SelectLayout::horizontalSpacing() const
{
	return 0;
}
int SelectLayout::verticalSpacing() const
{
	return 0;
}
Qt::Orientations SelectLayout::expandingDirections() const
{
	return Qt::Orientations(Qt::Horizontal|Qt::Vertical);
}
bool SelectLayout::hasHeightForWidth() const
{
	return true;
}
int SelectLayout::heightForWidth(int width) const
{
	return doLayout(QRect(0, 0, width, 0), true);
}
int SelectLayout::count() const
{
	return mItemList.size();
}
QLayoutItem* SelectLayout::itemAt(int index) const
{
	return mItemList.value(index);
}
QSize SelectLayout::minimumSize() const
{
	QSize size;
	for (int i = 0; i < mItemList.size(); i++)
		size = size.expandedTo(mItemList[i]->sizeHint());

	return size;
}
void SelectLayout::setGeometry(const QRect& rect)
{
	QLayout::setGeometry(rect);
	doLayout(rect, false);
}
QSize SelectLayout::sizeHint() const
{
	QSize size = QApplication::desktop()->size();
	size.setWidth(size.width() - 25);
	return size;
}
QLayoutItem* SelectLayout::takeAt(int index)
{
	if (index >= 0 && index < mItemList.count())
		return mItemList.takeAt(index);
	else
		return NULL;
}

int SelectLayout::doLayout(const QRect& rect, bool testOnly) const
{
	int x = rect.x();
	int y = rect.y();

	assert(mItemList.size() == mBreakBeforeList.size());
	int lineHeight = 0;
	for (int i = 0; i < mItemList.size(); i++)
	{
		if (i && mBreakBeforeList.value(i-1))
		{
			x = rect.x();
			y += lineHeight;
			lineHeight = 0;
		}
		QLayoutItem* item = mItemList[i];
		int nextX = x + item->sizeHint().width();
		if (nextX > rect.right() && lineHeight > 0)
		{
			x = rect.x();
			y = y + lineHeight;
			nextX = x + item->sizeHint().width();
			lineHeight = 0;
		}
		if (!testOnly)
			item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

		x = nextX;
		lineHeight = qMax(lineHeight, item->sizeHint().height());
	}

	return y + lineHeight - rect.y();
}
int SelectLayout::smartSpacing(QStyle::PixelMetric pm) const
{
	QObject* parent = this->parent();
	if (!parent)
		return -1;
	else if (parent->isWidgetType())
	{
		QWidget* widget = static_cast<QWidget*>(parent);
		widget->style()->pixelMetric(pm, 0, widget);
	}
	else
		return static_cast<QLayout*>(parent)->spacing();

	return 0;
}


bool selectVerse(QWidget* parent, BibleInfo* bible,
				QString startingFilter,
				QString& bookName, int& chapter)
{
	QList<QStringList> choices;
	choices.push_back(bible->getOTBookNames());
	choices.push_back(bible->getNTBookNames());

	QString selectedChoice;
	if (!Selector::select(parent, choices, "Book",
						startingFilter, selectedChoice))
	{
		return false;
	}
	QString selectedBook = selectedChoice;
	int selectedBookNum = bible->getBookNum(selectedBook);

	choices.clear();
	QStringList subChoices;
	for (int i = 0; i < bible->getNumChapters(selectedBookNum); i++)
		subChoices.push_back(QString("%1").arg(i + 1));
	choices.push_back(subChoices);
	if (!Selector::selectNoFilter(parent, choices,
									"Chapter", selectedChoice))
	{
		return false;
	}
	int selectedChapter = selectedChoice.toInt() - 1;

	bookName = selectedBook;
	chapter = selectedChapter;
	return true;
};

bool selectTranslation(QWidget* parent, QString& translation)
{
	QList<QStringList> choices;
	choices.push_back(getAvailableTranslations());

	QString selectedChoice;
	if (!Selector::select(parent, choices, "Translation", "", selectedChoice))
		return false;

	translation = selectedChoice;
	return true;
}

bool onDialogKey(int key, QString text)
{
	if (fgActiveDialog)
		fgActiveDialog->onKey(key, text);
	return fgActiveDialog != NULL;
}


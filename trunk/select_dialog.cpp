#include "select_dialog.h"

#include "text_source.h"
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
					QString& searchText,
					bool& searchImmediately,
					QString& selectedChoice)
{
	searchText = "";
	selectedChoice = "";

	Selector select(parent, choices, choicesDescrip, &startingFilter);
	
	if (!select.display())
		return false;
	if (select.mSearchText != "")
	{
		searchText = select.mSearchText;
		searchImmediately = select.mSearchImmediately;
	}
	else
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
	mSearchImmediately = false;
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
						"}"
						"QPushButton#Search{"
						"margin: 6px;"
						"padding: 13px;"
						"width: 150px;"
						"}"
						"QLabel{"
						"margin: 20px;"
						"}"
						"QLineEdit{"
						"width: 400px;"
						"}");
	setChoices();
	if (mShouldFilter)
	{
		mSearchEdit = new SearchEdit(mFrame, this);
		mSearchEdit->setText(mFilterText);
		mLayout->addBreak();
		mLayout->addWidget(mSearchEdit);
		mSearchButton = new QPushButton("Search");
		mSearchButton->setObjectName("Search");
		connect(mSearchButton, SIGNAL(clicked()), this, SLOT(searchClicked()));
		mLayout->addWidget(mSearchButton);
	}
	else
		mSearchEdit = NULL;
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
	return mSelectedChoice != "" || mSearchText != "";
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
	QRegExp re("\\b" + mFilterText + ".*", Qt::CaseInsensitive);
	for (int i = 0; i < mButtons.size(); i++)
	{
		if (mFilterText.length() == 0 || re.indexIn(mButtons[i]->text()) != -1)
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
	if (mSearchEdit)
	{
		mSearchEdit->setText(mFilterText);
		mSearchEdit->setCursorPosition(mFilterText.length());
	}
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

void Selector::searchClicked()
{
	mSearchText = mSearchEdit->text();
	mSearchImmediately = true;
	scroll->close();
}

void Selector::searchEditClicked()
{
	mSearchText = mSearchEdit->text();
	mSearchImmediately = false;
	scroll->close();
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
	assert(mItemList.size() == mBreakBeforeList.size());
	int y = rect.y();

	int curItem = 0;
	while (curItem < mItemList.size())
	{
		int x = rect.x();
		QList<QLayoutItem*> rowItems;
		QList<QRect> itemPositions;
		int rowHeight = 0;

		// Make a first pass to collect all buttons that will fit on this row.
		for (; curItem < mItemList.size(); curItem++)
		{
			QLayoutItem* item = mItemList[curItem];
			int nextX = x + item->sizeHint().width();
			if (nextX > rect.right() && rowHeight > 0)
				break;

			itemPositions.push_back(QRect(QPoint(x, y), item->sizeHint()));
			rowItems.push_back(mItemList[curItem]);
			rowHeight = qMax(rowHeight, item->sizeHint().height());
			x = nextX;

			if (mBreakBeforeList.value(curItem))
			{
				curItem++;
				break;
			}
		}

		bool isEOL = curItem >= mItemList.size() - 1 ||
					mBreakBeforeList.value(curItem - 1);
		if (!testOnly)
		{
			// Now adjust the widths to make full use of the line width.
			int buttonsWidth = x - rect.x();
			int availWidth = rect.width();
			if (availWidth - buttonsWidth > rowItems.size() && !isEOL)
			{
				// Adjust all but the last width (it will be adjusted below.)
				double dScaleFactor = double(availWidth) /
									double(buttonsWidth);
				int adjX = rect.x();
				for (int i = 0; i < rowItems.size(); i++)
				{
					int adjWidth = double(itemPositions[i].width()) *
									dScaleFactor;
					itemPositions[i].setLeft(adjX);
					itemPositions[i].setWidth(adjWidth);

					adjX += adjWidth;
				}
			}
			int adjX = rect.x();
			for (int i = 0; i < rowItems.size(); i++)
			{
				if (i == rowItems.size() - 1 && !isEOL)
					itemPositions[i].setWidth(availWidth - adjX);
				rowItems[i]->setGeometry(itemPositions[i]);
				adjX += itemPositions[i].width();
			}
		}

		// Reset row variables
		y += rowHeight;

		if (curItem && mBreakBeforeList.value(curItem - 1))
			y += 20;
	}
	return y + rect.y();
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

SelectResult SelectResult::search(QString text, bool searchImmediately)
{
	SelectResult result;
	if (searchImmediately)
		result.mType = Type_SearchText;
	else
		result.mType = Type_SearchDialog;
	result.mSearchText = text;
	return result;
}

SearchEdit::SearchEdit(QWidget* parent, Selector* selector) : QLineEdit(parent)
{
	mSelector = selector;
	mFocused = false;
}

void SearchEdit::onClick()
{
	mSelector->searchEditClicked();
}

void SearchEdit::focusInEvent(QFocusEvent* event)
{
	if (event->reason() == Qt::MouseFocusReason)
		mFocused = true;
	else
		clearFocus();
}

void SearchEdit::mouseReleaseEvent(QMouseEvent* /*event*/)
{
	if (mFocused)
		QTimer::singleShot(10, this, SLOT(onClick()));
}

SelectFrame::SelectFrame(QWidget* parent, Selector* selector) :
	QScrollArea(parent)
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

SelectResult SelectResult::verse(QString bookName, int chapter)
{
	SelectResult result;
	result.mType = Type_SelectedVerse;
	result.mBookName = bookName;
	result.mChapter = chapter;
	return result;
}

SelectResult::Type SelectResult::getType() const
{
	return mType;
}

QString SelectResult::search_GetText() const
{
	assert(mType == Type_SearchText || mType == Type_SearchDialog);
	return mSearchText;
}

QString SelectResult::verse_GetBook() const
{
	assert(mType == Type_SelectedVerse);
	return mBookName;
}

int SelectResult::verse_GetChapter() const
{
	assert(mType == Type_SelectedVerse);
	return mChapter;
}

bool selectVerse(QWidget* parent, TextSource* bible,
				QString startingFilter,
				SelectResult& result)
{
	QList<QStringList> choices = bible->getSuperSections();

	QString searchText;
	bool searchImmediately = false;
	QString selectedChoice;
	if (!Selector::select(parent, choices, "Book",
						startingFilter, searchText,
						searchImmediately, selectedChoice))
	{
		return false;
	}
	if (searchText != "")
	{
		result = SelectResult::search(searchText, searchImmediately);
		return true;
	}

	QString selectedBook = selectedChoice;

	choices.clear();
	QStringList subChoices;
	for (int i = 0; i < bible->getNumChapters(selectedBook); i++)
		subChoices.push_back(QString("%1").arg(i + 1));
	choices.push_back(subChoices);
	if (!Selector::selectNoFilter(parent, choices,
								"Chapter", selectedChoice))
	{
		return false;
	}
	int selectedChapter = selectedChoice.toInt() - 1;

	result = SelectResult::verse(selectedBook, selectedChapter);
	return true;
};

bool selectTranslation(QWidget* parent, QString& translation)
{
	QList<QStringList> choices;
	choices.push_back(getAvailableTranslations());

	QString selectedChoice;
	if (!Selector::selectNoFilter(parent, choices, "Translation",
								selectedChoice))
	{
		return false;
	}

	translation = selectedChoice;
	return true;
}

bool onDialogKey(int key, QString text)
{
	if (fgActiveDialog)
		fgActiveDialog->onKey(key, text);
	return fgActiveDialog != NULL;
}


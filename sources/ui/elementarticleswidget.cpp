/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "elementarticleswidget.h"

#include "dockfit.h"
#include "elementinfopartwidget.h"
#include "elementinfowidget.h"

#include <diagram.h>
#include <materiallist/materiallist.h>
#include <materiallist/materialselectiondialog.h>
#include <qetgraphicsitem/element.h>
#include <qetinformation.h>
#include <qetmessagebox.h>
#include <undocommand/changeelementinformationcommand.h>

#include <QApplication>
#include <QDockWidget>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
	constexpr int kBlockRole = Qt::UserRole;
	constexpr int kSubtitleRole = Qt::UserRole + 1;

	/**
		@brief Draws each article list row with a bold title on line 1
		and a summary subtitle on line 2.
	*/
	class ArticleListDelegate : public QStyledItemDelegate
	{
		public:
			explicit ArticleListDelegate(QObject *parent = nullptr) :
				QStyledItemDelegate(parent)
			{}

			QSize sizeHint(const QStyleOptionViewItem &option,
						   const QModelIndex &index) const override
			{
				QStyleOptionViewItem opt(option);
				initStyleOption(&opt, index);

				QFont title_font(opt.font);
				title_font.setBold(true);
				const QFontMetrics title_fm(title_font);
				const QFontMetrics sub_fm(opt.font);
				const int height = title_fm.height() + sub_fm.height() + 14;
				return QSize(opt.rect.width(), qMax(42, height));
			}

			void paint(QPainter *painter,
					   const QStyleOptionViewItem &option,
					   const QModelIndex &index) const override
			{
				QStyleOptionViewItem opt(option);
				initStyleOption(&opt, index);

				const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
				const QString title = opt.text;
				const QString subtitle = index.data(kSubtitleRole).toString();
				const bool selected = opt.state & QStyle::State_Selected;
				if (selected) {
					// Keep active highlight contrast even while editing fields on the right pane.
					opt.state |= QStyle::State_Active;
					opt.palette.setCurrentColorGroup(QPalette::Active);
				}

				// Clear default text before letting the style paint selection/focus chrome.
				opt.text.clear();
				style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

				painter->save();
				const QRect content_rect = opt.rect.adjusted(8, 5, -8, -5);
				const QColor text_color = selected
					? opt.palette.color(QPalette::Active, QPalette::HighlightedText)
					: opt.palette.color(QPalette::Text);

				if (index.column() == 1) {
					QFont qty_font(opt.font);
					qty_font.setBold(true);
					painter->setFont(qty_font);
					painter->setPen(text_color);
					painter->drawText(content_rect, Qt::AlignRight | Qt::AlignVCenter, title);
					painter->restore();
					return;
				}

				QFont title_font(opt.font);
				title_font.setBold(true);
				const QFontMetrics title_fm(title_font);
				const int title_h = title_fm.height();

				const QRect title_rect(content_rect.left(), content_rect.top(),
									   content_rect.width(), title_h);
				painter->setFont(title_font);
				painter->setPen(text_color);
				painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter,
								  title_fm.elidedText(title, Qt::ElideRight, title_rect.width()));

				const QRect sub_rect(content_rect.left(), title_rect.bottom() + 2,
									 content_rect.width(), content_rect.bottom() - title_rect.bottom() - 2);
				QColor sub_color = text_color;
				if (!selected) {
					sub_color.setAlphaF(0.75);
				}
				painter->setFont(opt.font);
				painter->setPen(sub_color);
				const QFontMetrics sub_fm(opt.font);
				painter->drawText(sub_rect, Qt::AlignLeft | Qt::AlignVCenter,
								  sub_fm.elidedText(subtitle, Qt::ElideRight, sub_rect.width()));
				painter->restore();
			}

			QWidget *createEditor(QWidget *parent,
								  const QStyleOptionViewItem &option,
								  const QModelIndex &index) const override
			{
				if (index.column() != 1) {
					return nullptr;
				}
				return QStyledItemDelegate::createEditor(parent, option, index);
			}
	};

	QString articleTitleForBlock(int block)
	{
		return ElementArticlesWidget::tr("Article %1").arg(block + 1);
	}

	QStringList articleColumnsForBlock(int block)
	{
		QStringList columns = MaterialList::columnsForBlock(block);
		if (block == 0) {
			// Placement and context fields stay in the Information tab rather than Articles.
			columns.removeAll(QETInformation::ELMT_FUNCTION);
			columns.removeAll(QETInformation::ELMT_COMMENT);
			columns.removeAll(QETInformation::ELMT_NOTES);
			columns.removeAll(QETInformation::ELMT_PLANT);
			columns.removeAll(QETInformation::ELMT_LOCATION);
		}
		const int desc_idx = columns.indexOf(QETInformation::ELMT_DESCRIPTION);
		const int desig_idx = columns.indexOf(QETInformation::ELMT_DESIGNATION);
		if (desc_idx >= 0 && desig_idx > desc_idx) {
			columns.move(desig_idx, desc_idx);
		}
		return columns;
	}
}

/**
	@brief ElementArticlesWidget::ElementArticlesWidget
	@param elmt element whose articles are edited
	@param parent parent widget
*/
ElementArticlesWidget::ElementArticlesWidget(Element *elmt, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent)
{
	if (DockFit::enabled() || isHostedInDock()) {
		setMinimumWidth(0);
	}
	setElement(elmt);
}

/**
	@brief ElementArticlesWidget::~ElementArticlesWidget
*/
ElementArticlesWidget::~ElementArticlesWidget() = default;

/**
	@brief ElementArticlesWidget::isHostedInDock
	@return true when embedded in a QDockWidget (or with DockFit enabled)
*/
bool ElementArticlesWidget::isHostedInDock() const
{
	if (DockFit::enabled() || m_live_edit) {
		return true;
	}
	for (const QWidget *w = parentWidget(); w; w = w->parentWidget()) {
		if (qobject_cast<const QDockWidget *>(w)) {
			return true;
		}
	}
	return false;
}

/**
	@brief ElementArticlesWidget::sizeHint
	@return a spacious default size when shown inside a standalone properties dialog,
	or a compact size so the side dock can stay narrow.
*/
QSize ElementArticlesWidget::sizeHint() const
{
	if (isHostedInDock()) {
		return QSize(280, 380);
	}
	return AbstractElementPropertiesEditorWidget::sizeHint().expandedTo(QSize(860, 580));
}

/**
	@brief ElementArticlesWidget::minimumSizeHint
	@return minimal size constraint in the dock so the side panel can always shrink
*/
QSize ElementArticlesWidget::minimumSizeHint() const
{
	if (isHostedInDock()) {
		return QSize(0, 200);
	}
	return AbstractElementPropertiesEditorWidget::minimumSizeHint();
}

/**
	@brief ElementArticlesWidget::managedKeys
	@return all DiagramContext keys managed by the Articles tab across blocks 0..4.
*/
QStringList ElementArticlesWidget::managedKeys()
{
	static const QStringList keys = []() {
		QStringList result;
		for (int block = 0; block < kBlockCount; ++block) {
			for (const QString &column : articleColumnsForBlock(block)) {
				const QString key = MaterialList::elementInfoKey(column, block);
				if (!key.isEmpty() && !result.contains(key)) {
					result << key;
				}
			}
		}
		return result;
	}();
	return keys;
}

/**
	@brief ElementArticlesWidget::setElement
	@param element element to edit
*/
void ElementArticlesWidget::setElement(Element *element)
{
	if (m_element == element) {
		return;
	}

	if (m_element) {
		disconnect(m_element.data(), &Element::elementInfoChange,
				   this, &ElementArticlesWidget::elementInfoChange);
	}

	m_explicitly_added.fill(false);
	m_pending_extra_info.clear();
	m_element = element;
	updateUi();

	if (m_element) {
		connect(m_element.data(), &Element::elementInfoChange,
				this, &ElementArticlesWidget::elementInfoChange);
	}
}

/**
	@brief ElementArticlesWidget::apply
*/
void ElementArticlesWidget::apply()
{
	if (!m_element || !m_element->diagram()) {
		return;
	}
	if (QUndoCommand *undo = associatedUndo()) {
		m_element->diagram()->undoStack().push(undo);
		m_pending_extra_info.clear();
	}
}

/**
	@brief ElementArticlesWidget::reset
*/
void ElementArticlesWidget::reset()
{
	if (m_element) {
		m_explicitly_added.fill(false);
		m_pending_extra_info.clear();
		updateUi();
	}
}

/**
	@brief ElementArticlesWidget::associatedUndo
	@return undo command when article fields changed, nullptr otherwise
*/
QUndoCommand *ElementArticlesWidget::associatedUndo() const
{
	if (!m_element) {
		return nullptr;
	}
	const DiagramContext old_info = m_element->elementInformations();
	const DiagramContext new_info = currentInfo();
	if (old_info != new_info) {
		return new ChangeElementInformationCommand(m_element, old_info, new_info);
	}
	return nullptr;
}

/**
	@brief ElementArticlesWidget::setLiveEdit
	@param live_edit true to apply edits immediately
	@return true
*/
bool ElementArticlesWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) {
		return true;
	}
	m_live_edit = live_edit;

	if (m_live_edit) {
		enableLiveEdit();
	} else {
		disableLiveEdit();
	}
	return true;
}

/**
	@brief ElementArticlesWidget::enableLiveEdit
*/
void ElementArticlesWidget::enableLiveEdit()
{
	for (ElementInfoPartWidget *part : std::as_const(m_parts)) {
		connect(part, &ElementInfoPartWidget::textChanged,
				this, &ElementArticlesWidget::apply, Qt::UniqueConnection);
	}
}

/**
	@brief ElementArticlesWidget::disableLiveEdit
*/
void ElementArticlesWidget::disableLiveEdit()
{
	for (ElementInfoPartWidget *part : std::as_const(m_parts)) {
		disconnect(part, &ElementInfoPartWidget::textChanged,
				   this, &ElementArticlesWidget::apply);
	}
}

/**
	@brief ElementArticlesWidget::buildInterface
	Build the master-detail split view: left list of articles, right editable detail pages.
*/
void ElementArticlesWidget::buildInterface()
{
	const bool in_dock = isHostedInDock();
	if (in_dock) {
		setMinimumWidth(0);
	}

	auto *root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(6, 6, 6, 6);

	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->setChildrenCollapsible(in_dock);

	// Left column: article selector + Add / Remove buttons
	m_left_panel = new QWidget(m_splitter);
	m_left_panel->setMinimumWidth(in_dock ? 0 : 220);
	auto *left_layout = new QVBoxLayout(m_left_panel);
	left_layout->setContentsMargins(0, 0, 0, 0);
	left_layout->setSpacing(4);

	m_article_list = new QTreeWidget(m_left_panel);
	m_article_list->setObjectName(QStringLiteral("m_article_list"));
	m_article_list->setColumnCount(2);
	m_article_list->setHeaderLabels({tr("Article"), tr("Quantity")});
	m_article_list->setRootIsDecorated(false);
	m_article_list->setIndentation(0);
	m_article_list->setAllColumnsShowFocus(true);
	m_article_list->setUniformRowHeights(true);
	m_article_list->setItemDelegate(new ArticleListDelegate(m_article_list));
	m_article_list->setSelectionMode(QAbstractItemView::SingleSelection);
	m_article_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_article_list->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
	m_article_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	if (QHeaderView *hdr = m_article_list->header()) {
		hdr->setStretchLastSection(false);
		hdr->setSectionResizeMode(0, QHeaderView::Stretch);
		hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents);
		hdr->setMinimumSectionSize(in_dock ? 36 : 56);
	}
	if (in_dock) {
		m_article_list->setMinimumWidth(0);
	}
	left_layout->addWidget(m_article_list, 1);

	auto *btn_row = new QHBoxLayout();
	btn_row->setSpacing(4);
	m_add_btn = new QPushButton(in_dock ? QStringLiteral("+") : tr("Add article"), m_left_panel);
	m_add_btn->setObjectName(QStringLiteral("m_add_article_btn"));
	m_remove_btn = new QPushButton(in_dock ? QStringLiteral("−") : tr("Remove"), m_left_panel);
	m_remove_btn->setObjectName(QStringLiteral("m_remove_article_btn"));
	if (in_dock) {
		m_add_btn->setMinimumWidth(0);
		m_remove_btn->setMinimumWidth(0);
		m_add_btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
		m_remove_btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
	}
	btn_row->addWidget(m_add_btn, 1);
	btn_row->addWidget(m_remove_btn, in_dock ? 1 : 0);
	left_layout->addLayout(btn_row);

	// Right column: active article header + stacked 2-column field grids
	m_right_panel = new QWidget(m_splitter);
	m_right_panel->setMinimumWidth(in_dock ? 0 : 480);
	auto *right_layout = new QVBoxLayout(m_right_panel);
	right_layout->setContentsMargins(0, 0, 0, 0);
	right_layout->setSpacing(6);

	auto *header_row = new QHBoxLayout();
	m_page_header_label = new QLabel(m_right_panel);
	QFont header_font = m_page_header_label->font();
	header_font.setBold(true);
	m_page_header_label->setFont(header_font);
	if (in_dock) {
		m_page_header_label->setMinimumWidth(0);
	}
	header_row->addWidget(m_page_header_label, 1);

	m_pick_material_btn = new QPushButton(
		in_dock ? tr("List…") : tr("Choose from materials list…"),
		m_right_panel);
	m_pick_material_btn->setObjectName(QStringLiteral("m_pick_material_btn"));
	m_pick_material_btn->setToolTip(tr("Choose from materials list…"));
	if (in_dock) {
		m_pick_material_btn->setMinimumWidth(0);
		m_pick_material_btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	}
	header_row->addWidget(m_pick_material_btn);
	right_layout->addLayout(header_row);

	auto *separator = new QFrame(m_right_panel);
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
	right_layout->addWidget(separator);

	m_stack = new QStackedWidget(m_right_panel);
	m_stack->setObjectName(QStringLiteral("m_article_stack"));
	if (in_dock) {
		m_stack->setMinimumWidth(0);
	}
	for (int block = 0; block < kBlockCount; ++block) {
		m_block_pages[block] = buildBlockPage(block);
		m_stack->addWidget(m_block_pages[block]);
	}
	right_layout->addWidget(m_stack, 1);

	m_splitter->addWidget(m_left_panel);
	m_splitter->addWidget(m_right_panel);
	m_splitter->setStretchFactor(0, 0);
	m_splitter->setStretchFactor(1, 1);
	const QString state_key = in_dock
		? QStringLiteral("elementarticleswidget/dock_splitter_state")
		: QStringLiteral("elementarticleswidget/splitter_state");
	const QByteArray saved_state = QSettings().value(state_key).toByteArray();
	if (saved_state.isEmpty() || !m_splitter->restoreState(saved_state)) {
		if (in_dock) {
			m_splitter->setSizes({100, 180});
		} else {
			m_splitter->setSizes({260, 560});
		}
	}
	connect(m_splitter, &QSplitter::splitterMoved, this, [this, state_key]() {
		QSettings().setValue(state_key, m_splitter->saveState());
	});
	root_layout->addWidget(m_splitter);

	connect(m_article_list, &QTreeWidget::currentItemChanged, this,
			[this](QTreeWidgetItem *current, QTreeWidgetItem *) {
		currentArticleChanged(current ? m_article_list->indexOfTopLevelItem(current) : -1);
	});
	connect(m_article_list, &QTreeWidget::itemChanged,
			this, &ElementArticlesWidget::articleItemChanged);
	connect(m_add_btn, &QPushButton::clicked,
			this, &ElementArticlesWidget::addArticle);
	connect(m_remove_btn, &QPushButton::clicked,
			this, &ElementArticlesWidget::removeCurrentArticle);
	connect(m_pick_material_btn, &QPushButton::clicked, this, [this]() {
		const int block = currentBlock();
		if (block >= 0) {
			materialFromFile(block);
		}
	});
}

/**
	@brief ElementArticlesWidget::buildBlockPage
	Create a scrollable 2-column grid of ElementInfoPartWidgets for one article block.
	@param block 0 for main article, 1..4 for auxiliary articles
	@return page widget ready to be added to m_stack
*/
QWidget *ElementArticlesWidget::buildBlockPage(int block)
{
	auto *scroll = new QScrollArea(m_stack);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);

	auto *container = new QWidget(scroll);
	auto *grid = new QGridLayout(container);
	grid->setContentsMargins(2, 2, 2, 2);
	grid->setHorizontalSpacing(10);
	grid->setVerticalSpacing(4);

	const QStringList columns = articleColumnsForBlock(block);
	for (int i = 0; i < columns.size(); ++i) {
		const QString &column = columns.at(i);
		const QString key = MaterialList::elementInfoKey(column, block);
		const QString label = MaterialList::translatedColumn(column);

		auto *part = new ElementInfoPartWidget(key, label, container);
		if (column == QLatin1String("description") || column == QLatin1String("designation")) {
			part->setMaterialButtonVisible(true);
			connect(part, &ElementInfoPartWidget::materialButtonClicked, this,
					[this, block]() { materialFromFile(block); });
		}
		if (column == QLatin1String("manufacturer")
				|| column == QLatin1String("description")
				|| column == QLatin1String("designation")
				|| column == QLatin1String("quantity")
				|| column == QLatin1String("auxiliary")) {
			connect(part, &ElementInfoPartWidget::textChanged, this,
					[this, block]() { updateArticleListRow(block); });
		}

		const int row = i / 2;
		const int col = i % 2;
		grid->addWidget(part, row, col);
		m_block_parts[block] << part;
		m_parts << part;
	}

	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(1, 1);
	grid->setRowStretch((columns.size() + 1) / 2, 1);

	scroll->setWidget(container);
	return scroll;
}

/**
	@brief ElementArticlesWidget::updateUi
*/
void ElementArticlesWidget::updateUi()
{
	if (!m_ui_built) {
		buildInterface();
		m_ui_built = true;
	}
	if (!m_element) {
		return;
	}
	m_pending_extra_info.clear();

	const bool was_live = m_live_edit;
	if (was_live) {
		disableLiveEdit();
	}

	const int selected_before = currentBlock();
	const DiagramContext element_info = m_element->elementInformations();
	m_loaded_values.clear();

	for (ElementInfoPartWidget *part : std::as_const(m_parts)) {
		const QString val = element_info.value(part->key()).toString();
		m_loaded_values.insert(part->key(), val);
		part->setText(val);
	}

	ElementInfoWidget::populateSuggestions(m_element, m_parts);
	refreshArticleList(selected_before);

	if (was_live) {
		enableLiveEdit();
	}
}

/**
	@brief ElementArticlesWidget::currentInfo
	@return element informations with only the article keys updated from this widget
*/
DiagramContext ElementArticlesWidget::currentInfo() const
{
	DiagramContext info = m_element ? m_element->elementInformations() : DiagramContext();

	for (const ElementInfoPartWidget *part : std::as_const(m_parts)) {
		if (!part->hasAcceptableInput()) {
			continue;
		}
		const QString key = part->key();
		if (part->text().isEmpty()) {
			info.remove(key);
		} else {
			info.addValue(key, part->text());
		}
	}

	for (auto it = m_pending_extra_info.cbegin(); it != m_pending_extra_info.cend(); ++it) {
		if (it.value().isEmpty()) {
			info.remove(it.key());
		} else {
			info.addValue(it.key(), it.value());
		}
	}
	return info;
}

/**
	@brief ElementArticlesWidget::elementInfoChange
	Refresh the widget only when an external change modified article keys.
*/
void ElementArticlesWidget::elementInfoChange()
{
	if (!m_element) {
		return;
	}

	const DiagramContext elmt_info = m_element->elementInformations();
	bool external_article_change = false;
	for (const ElementInfoPartWidget *part : std::as_const(m_parts)) {
		const QString key = part->key();
		const QString elmt_val = elmt_info.value(key).toString();
		if (elmt_val != m_loaded_values.value(key) && elmt_val != part->text()) {
			external_article_change = true;
			break;
		}
	}

	if (external_article_change) {
		updateUi();
	}
}

/**
	@brief ElementArticlesWidget::blockHasData
	@return true if any field of @p block currently holds non-empty text
*/
bool ElementArticlesWidget::blockHasData(int block) const
{
	if (block < 0 || block >= kBlockCount) {
		return false;
	}
	for (const ElementInfoPartWidget *part : m_block_parts[block]) {
		if (!part->text().trimmed().isEmpty()) {
			return true;
		}
	}
	return false;
}

/**
	@brief ElementArticlesWidget::articleSubtitle
	Build the second-line summary shown beneath "Article N" in the left list.
*/
QString ElementArticlesWidget::articleSubtitle(int block) const
{
	if (block < 0 || block >= kBlockCount) {
		return QString();
	}
	auto valueFor = [this, block](const QString &column) -> QString {
		const QString key = MaterialList::elementInfoKey(column, block);
		if (ElementInfoPartWidget *part = partForKey(key)) {
			return part->text().trimmed();
		}
		return QString();
	};

	const QString designation = valueFor(QStringLiteral("designation"));
	const QString description = valueFor(QStringLiteral("description"));

	QStringList parts;
	if (!designation.isEmpty()) {
		parts << designation;
	}
	if (!description.isEmpty()) {
		parts << description;
	}
	if (!parts.isEmpty()) {
		return parts.join(QStringLiteral(" / "));
	}

	if (block > 0) {
		const QString auxiliary = valueFor(QStringLiteral("auxiliary"));
		if (!auxiliary.isEmpty()) {
			return auxiliary;
		}
		return tr("Empty article");
	}

	const QString manufacturer = valueFor(QStringLiteral("manufacturer"));
	return manufacturer.isEmpty() ? tr("Main article (empty)") : manufacturer;
}

/**
	@brief ElementArticlesWidget::articleQuantity
	@return the quantity text shown in column 1 of the article table
*/
QString ElementArticlesWidget::articleQuantity(int block) const
{
	if (block < 0 || block >= kBlockCount) {
		return QString();
	}
	const QString key = MaterialList::elementInfoKey(QStringLiteral("quantity"), block);
	if (ElementInfoPartWidget *part = partForKey(key)) {
		return part->text().trimmed();
	}
	return QString();
}

/**
	@brief ElementArticlesWidget::refreshArticleList
	Populate the left list with Article 1 plus any active or non-empty auxiliary articles.
	@param preferred_block block to keep selected if still present
*/
void ElementArticlesWidget::refreshArticleList(int preferred_block)
{
	if (!m_article_list) {
		return;
	}

	const QSignalBlocker blocker(m_article_list);
	m_article_list->clear();

	int row_to_select = 0;
	for (int block = 0; block < kBlockCount; ++block) {
		if (block == 0 || blockHasData(block) || m_explicitly_added[block]) {
			auto *item = new QTreeWidgetItem(m_article_list);
			item->setText(0, articleTitleForBlock(block));
			item->setData(0, kBlockRole, block);
			item->setData(0, kSubtitleRole, articleSubtitle(block));
			item->setText(1, articleQuantity(block));
			item->setFlags(item->flags() | Qt::ItemIsEditable);
			if (block == preferred_block) {
				row_to_select = m_article_list->topLevelItemCount() - 1;
			}
		}
	}

	if (m_article_list->topLevelItemCount() > 0) {
		m_article_list->setCurrentItem(m_article_list->topLevelItem(row_to_select));
		currentArticleChanged(row_to_select);
	}
	updateActionButtons();
}

/**
	@brief ElementArticlesWidget::updateArticleListRow
	Update the subtitle and quantity column for @p block when edited on the right.
*/
void ElementArticlesWidget::updateArticleListRow(int block)
{
	if (!m_article_list) {
		return;
	}
	const QSignalBlocker blocker(m_article_list);
	for (int row = 0; row < m_article_list->topLevelItemCount(); ++row) {
		QTreeWidgetItem *item = m_article_list->topLevelItem(row);
		if (item && item->data(0, kBlockRole).toInt() == block) {
			item->setData(0, kSubtitleRole, articleSubtitle(block));
			item->setText(1, articleQuantity(block));
			break;
		}
	}
	updateActionButtons();
}

/**
	@brief ElementArticlesWidget::articleItemChanged
	Sync inline quantity edits from column 1 back into the article's quantity field.
*/
void ElementArticlesWidget::articleItemChanged(QTreeWidgetItem *item, int column)
{
	if (!item || column != 1) {
		return;
	}
	const int block = item->data(0, kBlockRole).toInt();
	const QString key = MaterialList::elementInfoKey(QStringLiteral("quantity"), block);
	if (ElementInfoPartWidget *part = partForKey(key)) {
		const QString qty = item->text(1).trimmed();
		if (part->text() != qty) {
			part->setText(qty);
		}
	}
}

/**
	@brief ElementArticlesWidget::updateActionButtons
*/
void ElementArticlesWidget::updateActionButtons()
{
	if (!m_add_btn || !m_remove_btn || !m_article_list) {
		return;
	}

	const bool can_add = m_article_list->topLevelItemCount() < kBlockCount;
	m_add_btn->setEnabled(can_add);
	m_add_btn->setToolTip(can_add
		? tr("Add another article slot (up to %1)").arg(kBlockCount)
		: tr("All %1 native article slots are already shown").arg(kBlockCount));

	const int block = currentBlock();
	const bool can_remove = block > 0 || blockHasData(0);
	m_remove_btn->setEnabled(can_remove);
	m_remove_btn->setToolTip(block > 0
		? tr("Clear and remove the selected auxiliary article")
		: tr("Clear the fields of Article 1"));
}

/**
	@brief ElementArticlesWidget::currentBlock
	@return active block index 0..4, or -1 if none
*/
int ElementArticlesWidget::currentBlock() const
{
	if (!m_article_list) {
		return -1;
	}
	const QTreeWidgetItem *item = m_article_list->currentItem();
	return item ? item->data(0, kBlockRole).toInt() : -1;
}

/**
	@brief ElementArticlesWidget::currentArticleChanged
*/
void ElementArticlesWidget::currentArticleChanged(int row)
{
	if (!m_article_list || !m_stack || row < 0 || row >= m_article_list->topLevelItemCount()) {
		return;
	}
	const int block = m_article_list->topLevelItem(row)->data(0, kBlockRole).toInt();
	if (block >= 0 && block < kBlockCount) {
		m_stack->setCurrentIndex(block);
		m_page_header_label->setText(articleTitleForBlock(block));
	}
	updateActionButtons();
}

/**
	@brief ElementArticlesWidget::addArticle
	Show the next unused native article slot (2..5) and select it.
*/
void ElementArticlesWidget::addArticle()
{
	if (!m_article_list) {
		return;
	}
	for (int block = 1; block < kBlockCount; ++block) {
		if (!blockHasData(block) && !m_explicitly_added[block]) {
			m_explicitly_added[block] = true;
			refreshArticleList(block);
			if (!m_block_parts[block].isEmpty()) {
				m_block_parts[block].first()->setFocusTolineEdit();
			}
			return;
		}
	}
}

/**
	@brief ElementArticlesWidget::removeCurrentArticle
	Clear the active block and, for auxiliary articles (blocks 1..4), hide the row.
*/
void ElementArticlesWidget::removeCurrentArticle()
{
	const int block = currentBlock();
	if (block < 0 || block >= kBlockCount) {
		return;
	}

	const bool had_data = blockHasData(block);
	const bool live_edit = m_live_edit;
	if (live_edit) {
		disableLiveEdit();
	}

	for (ElementInfoPartWidget *part : m_block_parts[block]) {
		part->setText(QString());
	}
	if (block > 0) {
		m_explicitly_added[block] = false;
	}

	if (live_edit) {
		enableLiveEdit();
		if (had_data) {
			apply();
		}
	}

	refreshArticleList(block > 0 ? block - 1 : 0);
}

/**
	@brief ElementArticlesWidget::materialFromFile
	Open the materials catalogue and write the chosen record into @p block.
*/
void ElementArticlesWidget::materialFromFile(int block)
{
	QString path = MaterialList::configuredPath();
	if (path.isEmpty()) {
		path = MaterialList::defaultPath();
		MaterialList::setConfiguredPath(path);
	}

	if (MaterialList::isEmptyFile(path)) {
		const auto answer = QET::QetMessageBox::question(
			this,
			tr("No materials list"),
			tr("There is no materials list file at this location:\n"
			   "%1\n"
			   "\n"
			   "Create it?", "message asking to create the material file").arg(path),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::Yes);

		if (answer != QMessageBox::Yes) {
			return;
		}

		QString error;
		if (!MaterialList::createFile(path, &error)) {
			QET::QetMessageBox::critical(this,
										 tr("Cannot create"),
										 tr("Cannot create the file:\n%1\n%2")
											.arg(path, error));
			return;
		}
	}

	MaterialSelectionDialog dialog(path, this);
	if (dialog.exec() == QDialog::Accepted) {
		applyMaterialRecord(dialog.selectedRecord(), block);
	}
}

/**
	@brief ElementArticlesWidget::applyMaterialRecord
	Write a catalogue entry into @p block as a single atomic change.
*/
void ElementArticlesWidget::applyMaterialRecord(const MaterialRecord &record, int block)
{
	if (record.values.isEmpty()) {
		return;
	}

	const bool live_edit = m_live_edit;
	if (live_edit) {
		disableLiveEdit();
	}

	for (const QString &column : MaterialList::columnsForBlock(block)) {
		if (!record.values.contains(column)) {
			continue;
		}
		const QString value = record.value(column);
		if (value.isEmpty() && !MaterialList::isArticleBound(column)) {
			continue;
		}
		const QString key = MaterialList::elementInfoKey(column, block);
		if (key.isEmpty() || !QETInformation::elementInfoKeys().contains(key)) {
			continue;
		}
		if (ElementInfoPartWidget *part = partForKey(key)) {
			part->setText(value);
			m_pending_extra_info.remove(key);
		} else {
			m_pending_extra_info.insert(key, value);
		}
	}

	updateArticleListRow(block);

	if (live_edit) {
		enableLiveEdit();
		apply();
	}
}

/**
	@brief ElementArticlesWidget::partForKey
*/
ElementInfoPartWidget *ElementArticlesWidget::partForKey(const QString &key) const
{
	for (ElementInfoPartWidget *part : std::as_const(m_parts)) {
		if (part->key() == key) {
			return part;
		}
	}
	return nullptr;
}

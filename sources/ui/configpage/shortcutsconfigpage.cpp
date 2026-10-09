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
#include "shortcutsconfigpage.h"

#include "../../qeticons.h"
#include "../../shortcutmanager.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <algorithm>
#include <utility>

/**
	@brief normalizedForSearch
	Decompose \a text and strip every non-spacing combining mark, then fold the
	case. Both the query terms and the row's searchable text are run through
	this, so "general" matches "Général" and "Ctrl+S" matches "ctrl+s"
	regardless of the keyboard layout the query was typed on.
*/
namespace {
	enum Column { ActionColumn, MenuColumn, SequenceColumn, ResetColumn };
}

static QString normalizedForSearch(const QString &text)
{
	const QString decomposed = text.normalized(QString::NormalizationForm_D);
	QString stripped;
	stripped.reserve(decomposed.size());
	for (const QChar &c : decomposed) {
		if (c.category() != QChar::Mark_NonSpacing) {
			stripped.append(c);
		}
	}
	return stripped.toCaseFolded();
}

/**
	@brief ShortcutsConfigPage::ShortcutsConfigPage
	@param parent
*/
ShortcutsConfigPage::ShortcutsConfigPage(QWidget *parent) :
	ConfigPage(parent)
{
	auto *vlayout = new QVBoxLayout();

	QLabel *title_label = new QLabel(this->title());
	vlayout->addWidget(title_label);

	QFrame *horiz_line = new QFrame();
	horiz_line->setFrameShape(QFrame::HLine);
	vlayout->addWidget(horiz_line);

	m_filter_edit = new QLineEdit(this);
	m_filter_edit->setPlaceholderText(tr("Filter shortcuts…"));
	connect(m_filter_edit, &QLineEdit::textChanged, this, &ShortcutsConfigPage::filterRows);

	m_quick_filter = new QComboBox(this);
	m_quick_filter->setObjectName(QStringLiteral("quickFilterCombo"));
	m_quick_filter->addItem(tr("all"));
	m_quick_filter->addItem(tr("Awarded exclusively"));
	m_quick_filter->addItem(tr("Unallocated only"));
	m_quick_filter->addItem(tr("Conflicts only"));
	connect(m_quick_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &ShortcutsConfigPage::quickFilterChanged);

	m_category_filter = new QComboBox(this);
	m_category_filter->setObjectName(QStringLiteral("categoryFilterCombo"));
	m_category_filter->addItem(tr("All categories"));
	connect(m_category_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &ShortcutsConfigPage::quickFilterChanged);

	m_count_label = new QLabel(this);
	m_count_label->setObjectName(QStringLiteral("shortcutCountLabel"));

	auto *filter_layout = new QHBoxLayout();
	filter_layout->addWidget(m_filter_edit, 1);
	filter_layout->addWidget(m_category_filter);
	filter_layout->addWidget(m_quick_filter);
	filter_layout->addWidget(m_count_label);
	vlayout->addLayout(filter_layout);

		//Press a key combination to list what uses it, without having to
		//know how QElectroTech spells it ("Ctrl+Maj+S", "Ctrl+Shift+S"…)
	m_key_search = new QKeySequenceEdit(this);
	m_key_search->setObjectName(QStringLiteral("keySearchEdit"));
	m_key_search->setToolTip(tr("Press a key combination to see which command uses it"));
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	m_key_search->setMaximumSequenceLength(1);
#endif
	connect(m_key_search, &QKeySequenceEdit::keySequenceChanged, this, [this]() { applyFilter(); });

	auto *clear_key_button = new QToolButton(this);
	clear_key_button->setIcon(QET::Icons::EditClear);
	clear_key_button->setToolTip(tr("Clear the searched key"));
	clear_key_button->setAutoRaise(true);
	connect(clear_key_button, &QToolButton::clicked, m_key_search, &QKeySequenceEdit::clear);

	auto *key_layout = new QHBoxLayout();
	key_layout->addWidget(new QLabel(tr("Search by key:"), this));
	key_layout->addWidget(m_key_search);
	key_layout->addWidget(clear_key_button);
	key_layout->addStretch();
	vlayout->addLayout(key_layout);

	m_tree = new QTreeWidget(this);
	m_tree->setHeaderLabels({tr("Action"), tr("Menu"), tr("Shortcut"), QString()});
	m_tree->header()->setSectionResizeMode(ActionColumn, QHeaderView::Stretch);
	m_tree->header()->setSectionResizeMode(MenuColumn, QHeaderView::ResizeToContents);
	m_tree->header()->setSectionResizeMode(SequenceColumn, QHeaderView::ResizeToContents);
	m_tree->header()->setSectionResizeMode(ResetColumn, QHeaderView::ResizeToContents);
	m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_tree->setSelectionMode(QAbstractItemView::NoSelection);
	vlayout->addWidget(m_tree);

	auto *reset_all_button = new QPushButton(tr("Reset everything"), this);
	connect(reset_all_button, &QPushButton::clicked, this, &ShortcutsConfigPage::resetAllRows);

	auto *copy_button = new QPushButton(QET::Icons::EditCopy, tr("Copy list"), this);
	copy_button->setToolTip(tr("Copies the shortcuts shown, to paste into a spreadsheet or a document"));
	connect(copy_button, &QPushButton::clicked, this, &ShortcutsConfigPage::copyList);

	auto *bottom_layout = new QHBoxLayout();
	bottom_layout->addWidget(copy_button);
	bottom_layout->addStretch();
	bottom_layout->addWidget(reset_all_button);
	vlayout->addLayout(bottom_layout);

	setLayout(vlayout);

	populateTable();
	applyFilter();
}

ShortcutsConfigPage::~ShortcutsConfigPage()
{
}

/**
	@brief ShortcutsConfigPage::populateTable
	Fill the tree with one collapsible top-level node per category and one child
	per shortcut known to ShortcutManager, sorted by category then action name.
*/
void ShortcutsConfigPage::populateTable()
{
	QList<ShortcutManager::ShortcutInfo> shortcuts = ShortcutManager::instance().allShortcuts();
	std::sort(shortcuts.begin(), shortcuts.end(),
		  [](const ShortcutManager::ShortcutInfo &a, const ShortcutManager::ShortcutInfo &b) {
			if (a.category != b.category) {
				return a.category < b.category;
			}
			return a.description < b.description;
		  });

	m_tree->clear();
	m_rows.clear();
	while (m_category_filter->count() > 1) {
		m_category_filter->removeItem(1);
	}
	m_rows.reserve(shortcuts.size());

	QHash<QString, QTreeWidgetItem *> category_nodes;

	for (const ShortcutManager::ShortcutInfo &info : shortcuts) {
		QTreeWidgetItem *category_item = category_nodes.value(info.category, nullptr);
		if (!category_item) {
			category_item = new QTreeWidgetItem(m_tree);
			category_item->setText(ActionColumn, info.category);
			category_item->setFlags(category_item->flags() & ~Qt::ItemIsEditable);
			category_nodes.insert(info.category, category_item);
			m_category_filter->addItem(info.category);
		}

		auto *child = new QTreeWidgetItem(category_item);
		const QString menu_path = menuPath(info.action);
		child->setText(ActionColumn, info.description);
		child->setIcon(ActionColumn, info.icon);
		child->setText(MenuColumn, menu_path);
		child->setFlags(child->flags() & ~Qt::ItemIsEditable);

		auto *edit = new QKeySequenceEdit(info.current_sequence, m_tree);
		connect(edit, &QKeySequenceEdit::editingFinished, this, &ShortcutsConfigPage::checkConflicts);
		m_tree->setItemWidget(child, SequenceColumn, edit);

		auto *reset_button = new QToolButton(m_tree);
		reset_button->setIcon(QET::Icons::EditUndo);
		reset_button->setToolTip(tr("Reset this shortcut"));
		reset_button->setAutoRaise(true);
		const int row_index = m_rows.size();
		connect(reset_button, &QToolButton::clicked, this, [this, row_index]() { resetRow(row_index); });
		m_tree->setItemWidget(child, ResetColumn, reset_button);

		m_rows << Row{info.id, info.category, info.description, menu_path,
			      info.default_sequence, edit, child, false};
	}

	checkConflicts();
}

/**
	@brief ShortcutsConfigPage::filterRows
	Re-run the combined text + quick filter whenever the search box changes.
*/
void ShortcutsConfigPage::filterRows(const QString &filter_text)
{
	Q_UNUSED(filter_text)
	applyFilter();
}

/**
	@brief ShortcutsConfigPage::quickFilterChanged
	Re-run the combined text + quick filter whenever the quick filter changes.
*/
void ShortcutsConfigPage::quickFilterChanged(int index)
{
	Q_UNUSED(index)
	applyFilter();
}

/**
	@brief ShortcutsConfigPage::applyFilter
	Hide every row that doesn't match both the search box and the quick filter.
	The text query is split on whitespace and every term must match the category,
	action name or current sequence (accent- and case-insensitively). Matching
	category nodes are expanded so hits are not hidden inside collapsed groups,
	and the "N actions" label tracks how many actions remain visible.
*/
void ShortcutsConfigPage::applyFilter()
{
	const QString needle = m_filter_edit->text().trimmed();
	const QStringList terms = needle.isEmpty()
			? QStringList()
			: needle.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);

	const int quick_filter = m_quick_filter->currentIndex();
	const QString category = m_category_filter->currentIndex() > 0
			? m_category_filter->currentText() : QString();
	const QKeySequence key = m_key_search->keySequence();

	int visible_actions = 0;
	for (const Row &row : std::as_const(m_rows)) {
		// Text is matched by substring, but the key sequence is matched exactly:
		// a substring match would let "Ctrl+S" also hit "Ctrl+Shift+F" (the "S"
		// of "Shift"), which is precisely the kind of false positive that hides
		// the one binding the user is looking for.
		const QString text_haystack = normalizedForSearch(
				row.category + QLatin1Char(' ') + row.description
				+ QLatin1Char(' ') + row.menu_path);
		const QString sequence_text = normalizedForSearch(row.edit->keySequence().toString());

		bool matches = true;
		for (const QString &term : terms) {
			const QString t = normalizedForSearch(term);
			if (!text_haystack.contains(t) && sequence_text != t) {
				matches = false;
				break;
			}
		}

		if (matches && !category.isEmpty()) {
			matches = row.category == category;
		}
		if (matches && !key.isEmpty()) {
				//A pressed key also finds the sequences it starts, such
				//as a two-key shortcut whose first key it is
			const QKeySequence sequence = row.edit->keySequence();
			matches = !sequence.isEmpty() && key.matches(sequence) != QKeySequence::NoMatch;
		}

		if (matches) {
			switch (quick_filter) {
			case BoundOnly:
				matches = !row.edit->keySequence().isEmpty();
				break;
			case UnboundOnly:
				matches = row.edit->keySequence().isEmpty();
				break;
			case ConflictsOnly:
				matches = row.conflicted;
				break;
			default:
				break;
			}
		}

		row.item->setHidden(!matches);
		if (matches) {
			++visible_actions;
		}
	}

	const bool filtering = !needle.isEmpty() || quick_filter != ShowAll
			|| !category.isEmpty() || !key.isEmpty();
	for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
		QTreeWidgetItem *top = m_tree->topLevelItem(i);
		bool any_visible = false;
		for (int j = 0; j < top->childCount(); ++j) {
			if (!top->child(j)->isHidden()) {
				any_visible = true;
				break;
			}
		}
		top->setHidden(!any_visible);
		top->setExpanded(filtering && any_visible);
	}

	m_count_label->setText(tr("%n actions", nullptr, visible_actions));
}

// IDs use stable context prefixes; translated display categories are not scopes.
// Main-window actions are shared by the editors, and depth actions by the
// diagram and element editors. Panel-specific shortcuts retain their own scope.
static bool shortcutScopesOverlap(const QString &first_id, const QString &second_id)
{
	const QString first = first_id.section(QLatin1Char('.'), 0, 0);
	const QString second = second_id.section(QLatin1Char('.'), 0, 0);
	if (first == second || first == QLatin1String("mainwindow")
			|| second == QLatin1String("mainwindow"))
		return true;

	const auto has_depth = [](const QString &scope) {
		return scope == QLatin1String("diagrameditor")
				|| scope == QLatin1String("elementeditor");
	};
	return (first == QLatin1String("depth") && has_depth(second))
			|| (second == QLatin1String("depth") && has_depth(first));
}

/**
	@brief ShortcutsConfigPage::checkConflicts
	Highlight every row whose currently-edited sequence is shared, non-empty,
	with another row, and explain the conflict in the shortcut editor's tooltip.
*/
void ShortcutsConfigPage::checkConflicts()
{
	QHash<QString, QList<int>> sequence_to_rows;
	for (int row = 0; row < m_rows.size(); ++row) {
		const QString sequence_text = m_rows.at(row).edit->keySequence().toString();
		if (!sequence_text.isEmpty()) {
			sequence_to_rows[sequence_text] << row;
		}
	}

	for (int row = 0; row < m_rows.size(); ++row) {
		Row &current_row = m_rows[row];
		const QString sequence_text = current_row.edit->keySequence().toString();
		QList<int> conflicting_rows;
		for (int other : sequence_to_rows.value(sequence_text)) {
			if (other != row && shortcutScopesOverlap(current_row.id, m_rows.at(other).id))
				conflicting_rows << other;
		}
		const bool conflicted = !conflicting_rows.isEmpty();

		current_row.conflicted = conflicted;

		if (conflicted) {
			QStringList other_descriptions;
			for (int other_row : conflicting_rows) {
				if (other_row != row) {
					other_descriptions << m_rows.at(other_row).description;
				}
			}
			current_row.item->setBackground(ActionColumn, QColor(255, 205, 205));
			current_row.edit->setToolTip(
				tr("This shortcut is also used by : %1").arg(other_descriptions.join(QStringLiteral(", "))));
		} else {
			current_row.item->setBackground(ActionColumn, QBrush());
			current_row.edit->setToolTip(QString());
		}
	}

	// A sequence edit can change while a filter is active (search-by-key or the
	// conflicts-only quick filter); refresh the visible set so the list doesn't
	// show stale results.
	const bool filtering = !m_filter_edit->text().trimmed().isEmpty()
			|| m_quick_filter->currentIndex() != ShowAll
			|| m_category_filter->currentIndex() > 0
			|| !m_key_search->keySequence().isEmpty();
	if (filtering) {
		applyFilter();
	}
}

/**
	@brief ShortcutsConfigPage::resetRow
	Reset the shortcut editor at \a row_index to its default sequence.
*/
void ShortcutsConfigPage::resetRow(int row_index)
{
	if (row_index < 0 || row_index >= m_rows.size()) {
		return;
	}
	m_rows.at(row_index).edit->setKeySequence(m_rows.at(row_index).default_sequence);
	checkConflicts();
}

void ShortcutsConfigPage::resetAllRows()
{
	for (const Row &row : std::as_const(m_rows)) {
		row.edit->setKeySequence(row.default_sequence);
	}
	checkConflicts();
}

/**
	@brief ShortcutsConfigPage::applyConf
	Persist every row's shortcut edit through ShortcutManager, which also
	applies it immediately to every currently live QAction sharing that id.
*/
void ShortcutsConfigPage::applyConf()
{
	for (const Row &row : std::as_const(m_rows)) {
		ShortcutManager::instance().setSequence(row.id, row.edit->keySequence());
	}
}

/**
	@brief ShortcutsConfigPage::menuPath
	@param action
	@return where \a action is in the menu bar, such as "Projet › Scripts",
	or an empty string for a command that is in no menu of the menu bar
	(a toolbar or shortcut bar only command, or one with no live action)
*/
QString ShortcutsConfigPage::menuPath(const QAction *action)
{
	if (!action) {
		return QString();
	}
	for (QObject *object : action->associatedObjects()) {
		QStringList titles;
		for (auto *menu = qobject_cast<QMenu *>(object); menu; ) {
			titles.prepend(menu->title().remove(QLatin1Char('&')));
			QMenu *parent_menu = nullptr;
			bool in_menu_bar = false;
			for (QObject *owner : menu->menuAction()->associatedObjects()) {
				if (qobject_cast<QMenuBar *>(owner)) {
					in_menu_bar = true;
				} else if (!parent_menu) {
					parent_menu = qobject_cast<QMenu *>(owner);
				}
			}
			if (in_menu_bar) {
				return titles.join(QStringLiteral(" › "));
			}
			menu = parent_menu;
		}
	}
	return QString();
}

/**
	@brief ShortcutsConfigPage::listAsText
	@return the shortcuts shown, as the edits currently hold them, one per
	line with tab-separated columns and a header line: pasted into a
	spreadsheet it fills one cell per column.
*/
QString ShortcutsConfigPage::listAsText() const
{
	QStringList lines;
	lines << QStringList{tr("Category"), tr("Menu"), tr("Action"), tr("Shortcut")}
		 .join(QLatin1Char('\t'));
	for (const Row &row : std::as_const(m_rows)) {
		if (row.item->isHidden()) {
			continue;
		}
		lines << QStringList{row.category, row.menu_path, row.description,
				     row.edit->keySequence().toString(QKeySequence::NativeText)}
			 .join(QLatin1Char('\t'));
	}
	return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

/**
	@brief ShortcutsConfigPage::copyList
	Put listAsText() on the clipboard.
*/
void ShortcutsConfigPage::copyList()
{
	QApplication::clipboard()->setText(listAsText());
}

QString ShortcutsConfigPage::title() const
{
	return tr("Shortcuts", "configuration page title");
}

QIcon ShortcutsConfigPage::icon() const
{
	return QET::Icons::ConfigureShortcuts;
}

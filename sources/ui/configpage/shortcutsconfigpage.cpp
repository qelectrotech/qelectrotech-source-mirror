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

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
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
	m_filter_edit->setPlaceholderText(tr("Filtrer les raccourcis…"));
	connect(m_filter_edit, &QLineEdit::textChanged, this, &ShortcutsConfigPage::filterRows);

	m_quick_filter = new QComboBox(this);
	m_quick_filter->setObjectName(QStringLiteral("quickFilterCombo"));
	m_quick_filter->addItem(tr("Tous"));
	m_quick_filter->addItem(tr("Attribués uniquement"));
	m_quick_filter->addItem(tr("Non attribués uniquement"));
	m_quick_filter->addItem(tr("Conflits uniquement"));
	connect(m_quick_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &ShortcutsConfigPage::quickFilterChanged);

	m_count_label = new QLabel(this);
	m_count_label->setObjectName(QStringLiteral("shortcutCountLabel"));

	auto *filter_layout = new QHBoxLayout();
	filter_layout->addWidget(m_filter_edit, 1);
	filter_layout->addWidget(m_quick_filter);
	filter_layout->addWidget(m_count_label);
	vlayout->addLayout(filter_layout);

	m_tree = new QTreeWidget(this);
	m_tree->setHeaderLabels({tr("Action"), tr("Raccourci"), QString()});
	m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_tree->setSelectionMode(QAbstractItemView::NoSelection);
	vlayout->addWidget(m_tree);

	auto *reset_all_button = new QPushButton(tr("Tout réinitialiser"), this);
	connect(reset_all_button, &QPushButton::clicked, this, &ShortcutsConfigPage::resetAllRows);

	auto *bottom_layout = new QHBoxLayout();
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
	m_rows.reserve(shortcuts.size());

	QHash<QString, QTreeWidgetItem *> category_nodes;

	for (const ShortcutManager::ShortcutInfo &info : shortcuts) {
		QTreeWidgetItem *category_item = category_nodes.value(info.category, nullptr);
		if (!category_item) {
			category_item = new QTreeWidgetItem(m_tree);
			category_item->setText(0, info.category);
			category_item->setFlags(category_item->flags() & ~Qt::ItemIsEditable);
			category_nodes.insert(info.category, category_item);
		}

		auto *child = new QTreeWidgetItem(category_item);
		child->setText(0, info.description);
		child->setFlags(child->flags() & ~Qt::ItemIsEditable);

		auto *edit = new QKeySequenceEdit(info.current_sequence, m_tree);
		connect(edit, &QKeySequenceEdit::editingFinished, this, &ShortcutsConfigPage::checkConflicts);
		m_tree->setItemWidget(child, 1, edit);

		auto *reset_button = new QToolButton(m_tree);
		reset_button->setIcon(QET::Icons::EditUndo);
		reset_button->setToolTip(tr("Réinitialiser ce raccourci"));
		reset_button->setAutoRaise(true);
		const int row_index = m_rows.size();
		connect(reset_button, &QToolButton::clicked, this, [this, row_index]() { resetRow(row_index); });
		m_tree->setItemWidget(child, 2, reset_button);

		m_rows << Row{info.id, info.category, info.description, info.default_sequence, edit, child, false};
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

	int visible_actions = 0;
	for (const Row &row : std::as_const(m_rows)) {
		// Text is matched by substring, but the key sequence is matched exactly:
		// a substring match would let "Ctrl+S" also hit "Ctrl+Shift+F" (the "S"
		// of "Shift"), which is precisely the kind of false positive that hides
		// the one binding the user is looking for.
		const QString text_haystack = normalizedForSearch(
				row.category + QLatin1Char(' ') + row.description);
		const QString sequence_text = normalizedForSearch(row.edit->keySequence().toString());

		bool matches = true;
		for (const QString &term : terms) {
			const QString t = normalizedForSearch(term);
			if (!text_haystack.contains(t) && sequence_text != t) {
				matches = false;
				break;
			}
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

	const bool filtering = !needle.isEmpty() || quick_filter != ShowAll;
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

	m_count_label->setText(tr("%n action(s)", nullptr, visible_actions));
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
		const QList<int> &conflicting_rows = sequence_to_rows.value(sequence_text);
		const bool conflicted = !sequence_text.isEmpty() && conflicting_rows.size() > 1;

		current_row.conflicted = conflicted;

		if (conflicted) {
			QStringList other_descriptions;
			for (int other_row : conflicting_rows) {
				if (other_row != row) {
					other_descriptions << m_rows.at(other_row).description;
				}
			}
			current_row.item->setBackground(0, QColor(255, 205, 205));
			current_row.edit->setToolTip(
				tr("Ce raccourci est aussi utilisé par : %1").arg(other_descriptions.join(QStringLiteral(", "))));
		} else {
			current_row.item->setBackground(0, QBrush());
			current_row.edit->setToolTip(QString());
		}
	}

	// A sequence edit can change while a filter is active (search-by-key or the
	// conflicts-only quick filter); refresh the visible set so the list doesn't
	// show stale results.
	const bool filtering = !m_filter_edit->text().trimmed().isEmpty()
			|| m_quick_filter->currentIndex() != ShowAll;
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

QString ShortcutsConfigPage::title() const
{
	return tr("Raccourcis", "configuration page title");
}

QIcon ShortcutsConfigPage::icon() const
{
	return QET::Icons::ConfigureToolbars;
}

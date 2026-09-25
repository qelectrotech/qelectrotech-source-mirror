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
#include "jumptoelementdialog.h"

#include "../diagram.h"
#include "../diagramview.h"
#include "../qetproject.h"
#include "../qetgraphicsitem/element.h"

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QRegularExpression>
#include <QVBoxLayout>

/**
	@brief JumpToElementDialog::JumpToElementDialog
	@param diagram : the diagram whose elements can be jumped to
	@param parent
*/
JumpToElementDialog::JumpToElementDialog(Diagram *diagram, QWidget *parent) :
	QDialog(parent),
	m_diagram(diagram)
{
	setWindowTitle(tr("Atteindre un élément", "window title"));

	m_filter_edit = new QLineEdit(this);
	m_filter_edit->setPlaceholderText(tr("Nom, label ou information de l'élément, ou case (ex. B13 ou 3-B13)…"));
	m_filter_edit->installEventFilter(this);

	m_result_list = new QListWidget(this);
	m_result_list->setFocusPolicy(Qt::NoFocus);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(m_filter_edit);
	layout->addWidget(m_result_list);
	setLayout(layout);
	resize(420, 320);

	connect(m_filter_edit, &QLineEdit::textChanged, this, &JumpToElementDialog::updateFilteredList);
	connect(m_result_list, &QListWidget::itemActivated, this, &JumpToElementDialog::activateCurrentItem);

	buildCandidates();
	updateFilteredList(QString());
	m_filter_edit->setFocus();
}

JumpToElementDialog::~JumpToElementDialog()
{
}

/**
	@brief JumpToElementDialog::buildCandidates
	Collect every element on m_diagram into m_candidates, along with the
	searchable text used to filter it (its type name, its label, and its
	other element informations).
*/
void JumpToElementDialog::buildCandidates()
{
	m_candidates.clear();
	if (!m_diagram) {
		return;
	}

	for (auto item : m_diagram->items()) {
		Element *element = qgraphicsitem_cast<Element *>(item);
		if (!element) {
			continue;
		}

		const QString label = element->elementInformations().value(QStringLiteral("label")).toString();
		const QString name = element->name();

		Candidate candidate;
		candidate.element = element;
		candidate.label = label;
		candidate.display_text = label.isEmpty() ? name : (label + QStringLiteral(" — ") + name);

		QStringList search_parts;
		search_parts << label << name;
		const DiagramContext infos = element->elementInformations();
		for (const QString &key : infos.keys()) {
			search_parts << infos.value(key).toString();
		}
		candidate.search_text = search_parts.join(QLatin1Char(' ')).toLower();

		m_candidates << candidate;
	}
}

/**
	@brief JumpToElementDialog::updateFilteredList
	Refill m_result_list with every candidate whose search text contains
	@a filter_text (case-insensitive), and select the first match.
	@param filter_text
*/
void JumpToElementDialog::updateFilteredList(const QString &filter_text)
{
	m_result_list->clear();

	const QString needle = filter_text.trimmed().toLower();
	bool exact_label_match = false;
	for (int i = 0; i < m_candidates.size(); ++i) {
		const Candidate &candidate = m_candidates.at(i);
		if (!candidate.element) {
			continue;
		}
		if (!needle.isEmpty() && !candidate.search_text.contains(needle)) {
			continue;
		}
		auto *list_item = new QListWidgetItem(candidate.display_text, m_result_list);
		list_item->setData(Qt::UserRole, i);
		if (candidate.label.compare(needle, Qt::CaseInsensitive) == 0) {
			exact_label_match = true;
		}
	}

		//A cell of the border, on this folio (ex : B13) or on another one
		//(ex : 3-B13, folio 3, the way cross references write it), comes
		//first, unless an element is labelled exactly like it: Enter keeps
		//jumping to that element.
	if (QListWidgetItem *cell_item = cellItem(needle)) {
		m_result_list->insertItem(exact_label_match ? m_result_list->count() : 0,
					  cell_item);
	}

	if (m_result_list->count() > 0) {
		m_result_list->setCurrentRow(0);
	}
}

/**
	@brief JumpToElementDialog::activateCurrentItem
	Select the element corresponding to the currently highlighted result
	on the diagram, scroll it into view, and close this dialog.
*/
void JumpToElementDialog::activateCurrentItem()
{
	QListWidgetItem *current = m_result_list->currentItem();
	if (!current || !m_diagram) {
		reject();
		return;
	}

	const int index = current->data(Qt::UserRole).toInt();
	if (index == -1) {
		const QList<Diagram *> diagrams = m_diagram->project()
				? m_diagram->project()->diagrams()
				: QList<Diagram *>();
		const int folio = current->data(Qt::UserRole + 2).toInt();
		if (folio >= 0 && folio < diagrams.size()) {
			zoomToCell(diagrams.at(folio), current->data(Qt::UserRole + 1).toRectF());
		}
		accept();
		return;
	}
	if (index < 0 || index >= m_candidates.size()) {
		reject();
		return;
	}

	Element *element = m_candidates.at(index).element;
	if (!element) {
		reject();
		return;
	}

	m_diagram->clearSelection();
	element->setSelected(true);
	element->ensureVisible();
	accept();
}

/**
	@brief JumpToElementDialog::cellItem
	@param needle : the typed text, trimmed and lower case
	@return a new list item for the cell \a needle names, on m_diagram
	(ex : b13) or on the folio at a position of the project (ex : 3-b13,
	3b13, p3b13), or nullptr if \a needle names no cell of an existing
	folio. The item holds -1, the cell rect and the folio position.
*/
QListWidgetItem *JumpToElementDialog::cellItem(const QString &needle) const
{
	if (!m_diagram || !m_diagram->project()) {
		return nullptr;
	}
	const QList<Diagram *> diagrams = m_diagram->project()->diagrams();

	Diagram *diagram = m_diagram;
	QString cell = needle;
	QString text;
	QRectF cell_rect = diagram->border_and_titleblock.cellRect(cell);
	if (cell_rect.isNull()) {
			//A folio position then a cell, like %f-%l%c in cross references
		static const QRegularExpression folio_cell_re(
			QStringLiteral("^p?\\s*(\\d{1,4})\\s*[-/.:]?\\s*([a-z]+\\s*\\d{1,4})$"));
		const QRegularExpressionMatch match = folio_cell_re.match(needle);
		if (!match.hasMatch()) {
			return nullptr;
		}
		const int folio = match.captured(1).toInt();
		if (folio < 1 || folio > diagrams.size()) {
			return nullptr;
		}
		diagram = diagrams.at(folio - 1);
		cell = match.captured(2);
		cell_rect = diagram->border_and_titleblock.cellRect(cell);
		if (cell_rect.isNull()) {
			return nullptr;
		}
	}

	cell = cell.remove(QLatin1Char(' ')).toUpper();
	if (diagram == m_diagram) {
		text = tr("Case %1").arg(cell);
	} else {
		const QString title = diagram->title();
		text = title.isEmpty()
				? tr("Folio %1, case %2").arg(diagrams.indexOf(diagram) + 1).arg(cell)
				: tr("Folio %1 (%2), case %3").arg(diagrams.indexOf(diagram) + 1).arg(title, cell);
	}

	auto *item = new QListWidgetItem(text);
	item->setData(Qt::UserRole, -1);
	item->setData(Qt::UserRole + 1, cell_rect);
	item->setData(Qt::UserRole + 2, diagrams.indexOf(diagram));
	return item;
}

/**
	@brief JumpToElementDialog::zoomToCell
	Show \a diagram and zoom its view on \a cell_rect with one cell of
	context around it.
	@param diagram : the folio holding the cell
	@param cell_rect : the cell, in scene coordinate
*/
void JumpToElementDialog::zoomToCell(Diagram *diagram, const QRectF &cell_rect)
{
	const QRectF rect = cell_rect.adjusted(-cell_rect.width(), -cell_rect.height(),
					       cell_rect.width(), cell_rect.height());
	const bool other_folio = diagram != m_diagram;
	if (other_folio) {
		diagram->showMe();
	}
	for (QGraphicsView *view : diagram->views()) {
		if (auto *diagram_view = qobject_cast<DiagramView *>(view)) {
			if (other_folio) {
					//A folio shown for the first time is laid out at
					//its size once the tab switch has been handled
				QMetaObject::invokeMethod(diagram_view, [diagram_view, rect]() {
					diagram_view->zoomToRect(rect);
				}, Qt::QueuedConnection);
			} else {
				diagram_view->zoomToRect(rect);
			}
			return;
		}
	}
}

/**
	@brief JumpToElementDialog::eventFilter
	Redirect Up/Down/Enter/Escape typed in the filter field to the result
	list, so the user never has to leave the text field to navigate or
	confirm a choice.
*/
bool JumpToElementDialog::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == m_filter_edit && event->type() == QEvent::KeyPress) {
		auto *key_event = static_cast<QKeyEvent *>(event);
		switch (key_event->key()) {
			case Qt::Key_Down:
			case Qt::Key_Up: {
				const int row_count = m_result_list->count();
				if (row_count == 0) {
					return true;
				}
				int row = m_result_list->currentRow();
				row = key_event->key() == Qt::Key_Down
						? (row + 1) % row_count
						: (row - 1 + row_count) % row_count;
				m_result_list->setCurrentRow(row);
				return true;
			}
			case Qt::Key_Return:
			case Qt::Key_Enter:
				activateCurrentItem();
				return true;
			case Qt::Key_Escape:
				reject();
				return true;
			default:
				break;
		}
	}
	return QDialog::eventFilter(watched, event);
}

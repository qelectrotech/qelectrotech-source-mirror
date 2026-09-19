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
#include "../qetgraphicsitem/element.h"

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
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
	m_filter_edit->setPlaceholderText(tr("Nom, label ou information de l'élément…"));
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

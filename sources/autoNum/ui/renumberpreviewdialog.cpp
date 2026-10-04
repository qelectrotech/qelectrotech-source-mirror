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
#include "renumberpreviewdialog.h"

#include "../../diagram.h"
#include "../../qetgraphicsitem/element.h"
#include "../../qetinformation.h"

#include <QBrush>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
QString labelOf(const DiagramContext &info)
{
	return info.value(QETInformation::ELMT_LABEL).toString();
}

QString folioOf(const Element *element)
{
	return element && element->diagram()
			? QString::number(element->diagram()->folioIndex() + 1)
			: QString();
}
} // namespace

/**
	@brief RenumberPreviewDialog::changedLabelCount
	@return how many of @p changes give their element another label
*/
int RenumberPreviewDialog::changedLabelCount(
		const QVector<RenumberElementsCommand::ElementChange> &changes)
{
	int count = 0;
	for (const auto &change : changes) {
		if (labelOf(change.old_infos) != labelOf(change.new_infos)) {
			++count;
		}
	}
	return count;
}

/**
	@brief RenumberPreviewDialog::ask
	@param intro : text above the list
	@param changes : what the operation would do to elements
	@param left_alone : the elements it leaves as they are, each with the
	reason shown in place of its new name
	@param replace_text : when not empty, a third button of that text
	which goes on and also replaces the elements left alone
	@return what the user chose
*/
RenumberPreviewDialog::Answer RenumberPreviewDialog::ask(
		QWidget *parent,
		const QString &title,
		const QString &intro,
		const QVector<RenumberElementsCommand::ElementChange> &changes,
		const QVector<LeftAlone> &left_alone,
		const QString &replace_text)
{
	QDialog dialog(parent);
	dialog.setWindowTitle(title);
	dialog.resize(640, 460);
	auto *layout = new QVBoxLayout(&dialog);

	const int changed = changedLabelCount(changes);
	QString text = intro;
	if (changes.size() > changed) {
		text += QLatin1Char('\n') + tr("%n élément(s) gardent le nom qu'ils ont.", "", changes.size() - changed);
	}
	if (!left_alone.isEmpty()) {
		text += QLatin1Char('\n') + tr("%n élément(s) restent comme ils sont.", "", left_alone.size());
	}
	auto *intro_label = new QLabel(text, &dialog);
	intro_label->setWordWrap(true);
	layout->addWidget(intro_label);

	auto *table = new QTableWidget(&dialog);
	table->setColumnCount(4);
	table->setHorizontalHeaderLabels({tr("Folio"), tr("Élément"), tr("Nom actuel"), tr("Nouveau nom")});
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->verticalHeader()->setVisible(false);
	table->horizontalHeader()->setStretchLastSection(true);
	table->setAlternatingRowColors(true);

	const auto add_row = [table](const QStringList &cells, bool grey) {
		const int row = table->rowCount();
		table->insertRow(row);
		for (int column = 0 ; column < cells.size() ; ++column) {
			auto *item = new QTableWidgetItem(cells.at(column));
			if (grey) {
				QFont font = item->font();
				font.setItalic(true);
				item->setFont(font);
				item->setForeground(QBrush(Qt::gray));
			}
			table->setItem(row, column, item);
		}
	};

	for (const auto &change : changes) {
		const QString before = labelOf(change.old_infos);
		const QString after = labelOf(change.new_infos);
		if (before != after) {
			add_row({folioOf(change.element),
					 change.element ? change.element->name() : QString(),
					 before, after}, false);
		}
	}
	for (const LeftAlone &left : left_alone) {
		if (left.element) {
			add_row({folioOf(left.element), left.element->name(),
					 labelOf(left.element->elementInformations()), left.note}, true);
		}
	}
	table->resizeColumnsToContents();
	layout->addWidget(table, 1);

		//With no label to change, OK would do nothing: only Cancel (and, where
		//there is one, the button which replaces what is left alone) is offered
	const bool something_to_do = !changes.isEmpty();
	auto *buttons = new QDialogButtonBox(
				something_to_do ? (QDialogButtonBox::Ok | QDialogButtonBox::Cancel)
								: QDialogButtonBox::Cancel, &dialog);
	Answer answer = Answer::Cancel;
	QPushButton *replace = nullptr;
	if (!replace_text.isEmpty()) {
		replace = buttons->addButton(replace_text, QDialogButtonBox::ActionRole);
	}
	QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	if (replace) {
		QObject::connect(replace, &QPushButton::clicked, &dialog, [&]() {
			answer = Answer::GoAndReplace;
			dialog.accept();
		});
	}
	layout->addWidget(buttons);

	if (dialog.exec() == QDialog::Accepted && answer != Answer::GoAndReplace) {
		answer = Answer::Go;
	}
	return answer;
}

/**
	@brief RenumberPreviewDialog::confirm
	@param intro : text above the list
	@param changes : what the operation would do to elements
	@param frozen : the elements it leaves as they are, a frozen label
	@return true if the user agrees to go on
*/
bool RenumberPreviewDialog::confirm(
		QWidget *parent,
		const QString &title,
		const QString &intro,
		const QVector<RenumberElementsCommand::ElementChange> &changes,
		const QVector<Element *> &frozen)
{
	QVector<LeftAlone> left;
	for (Element *el : frozen) {
		left.append({el, tr("(figé)")});
	}
	return ask(parent, title, intro, changes, left) == Answer::Go;
}

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
#include "devicetypeeditor.h"

#include "../ElementsCollection/elementslocation.h"
#include "../devicetype/devicetypecheck.h"
#include "../elementdialog.h"
#include "../qetapp.h"
#include "../qetinformation.h"
#include "../qetproject.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDomElement>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
	enum Column {PartColumn, RoleColumn, KindColumn, StateColumn, SymbolColumn, PinsColumn};

		/// A combo box of fixed words: each item shows a text, keeps a word
	QComboBox *wordBox(const QList<QPair<QString, QString>> &words, bool editable,
					   const QString &current, QWidget *parent)
	{
		auto *box = new QComboBox(parent);
		box->setEditable(editable);
		for (const auto &word : words)
			box->addItem(word.second, word.first);
		const int index = box->findData(current);
		if (index >= 0)
			box->setCurrentIndex(index);
		else if (editable)
			box->setEditText(current);
		return box;
	}

		/// The word a combo box of wordBox() holds: its item's, else what is typed
	QString wordOf(const QComboBox *box)
	{
		if (!box) return QString();
		const int index = box->findText(box->currentText());
		if (index >= 0) return box->itemData(index).toString();
		return box->currentText().trimmed();
	}

	QList<QPair<QString, QString>> roles()
	{
		return {{QStringLiteral("master"), DeviceTypeEditor::tr("Master")},
				{QStringLiteral("slave"), DeviceTypeEditor::tr("Slave")},
				{QStringLiteral("simple"), DeviceTypeEditor::tr("Simple")},
				{QStringLiteral("terminal"), DeviceTypeEditor::tr("Terminal")}};
	}

	QList<QPair<QString, QString>> kinds()
	{
		return {{QString(), QString()},
				{QStringLiteral("coil"), DeviceTypeEditor::tr("Coil")},
				{QStringLiteral("protection"), DeviceTypeEditor::tr("Protection")},
				{QStringLiteral("commutator"), DeviceTypeEditor::tr("Commutator")},
				{QStringLiteral("plc"), DeviceTypeEditor::tr("PLC")},
				{QStringLiteral("simple"), DeviceTypeEditor::tr("Simple contact")},
				{QStringLiteral("power"), DeviceTypeEditor::tr("Power contact")},
				{QStringLiteral("delayOn"), DeviceTypeEditor::tr("Delay on")},
				{QStringLiteral("delayOff"), DeviceTypeEditor::tr("Delay off")},
				{QStringLiteral("delayOnOff"), DeviceTypeEditor::tr("Delay on and off")}};
	}

	QList<QPair<QString, QString>> states()
	{
		return {{QString(), QString()},
				{QStringLiteral("NO"), DeviceTypeEditor::tr("NO")},
				{QStringLiteral("NC"), DeviceTypeEditor::tr("NC")},
				{QStringLiteral("SW"), DeviceTypeEditor::tr("Changeover")}};
	}

	QString cellText(const QTableWidget *table, int row, int column)
	{
		const QTableWidgetItem *item = table->item(row, column);
		return item ? item->text().trimmed() : QString();
	}
}

/**
	@brief DeviceTypeEditor::DeviceTypeEditor
	@param type : the type to change; a null type to make a new one (its
	name may be set, as when a type is duplicated)
	@param project : the project whose symbols (embed://) a part may name
	@param taken_names : the names a new type may not take
	@param parent : parent widget
*/
DeviceTypeEditor::DeviceTypeEditor(const DeviceType &type, QETProject *project,
								   const QStringList &taken_names, QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_taken(taken_names),
	m_lang(QETApp::langFromSetting())
{
	setWindowTitle(tr("Device type"));
	auto *layout = new QVBoxLayout(this);

	auto *form = new QFormLayout();
	m_name = new QLineEdit(this);
	m_name->setObjectName(QStringLiteral("deviceTypeName"));
	m_name->setToolTip(tr("The name devices give in their Device type field, such as CONTR10"));
	form->addRow(tr("Name:"), m_name);
	m_description = new QLineEdit(this);
	m_description->setObjectName(QStringLiteral("deviceTypeDescription"));
	form->addRow(tr("Description:"), m_description);
	layout->addLayout(form);

	auto *given = new QGroupBox(tr("Given to the symbols of its devices"), this);
	auto *given_layout = new QVBoxLayout(given);
	m_informations = new QTableWidget(0, 2, given);
	m_informations->setObjectName(QStringLiteral("deviceTypeInformations"));
	m_informations->setHorizontalHeaderLabels({tr("Information"), tr("Value")});
	m_informations->horizontalHeader()->setStretchLastSection(true);
	m_informations->verticalHeader()->hide();
	m_informations->setMaximumHeight(120);
	given_layout->addWidget(m_informations);
	auto *given_buttons = new QHBoxLayout();
	auto *add_info = new QPushButton(tr("Add"), given);
	auto *remove_info = new QPushButton(tr("Remove"), given);
	given_buttons->addWidget(add_info);
	given_buttons->addWidget(remove_info);
	given_buttons->addStretch();
	given_layout->addLayout(given_buttons);
	connect(add_info, &QPushButton::clicked, this, &DeviceTypeEditor::addInformation);
	connect(remove_info, &QPushButton::clicked, this, &DeviceTypeEditor::removeInformation);
	layout->addWidget(given);

	auto *parts = new QGroupBox(tr("Parts"), this);
	auto *parts_layout = new QVBoxLayout(parts);
	auto *parts_help = new QLabel(
		tr("One row per way to draw a part. Rows with the same part number are "
		   "the ways one part may be drawn, such as a contact as NO, NC or "
		   "changeover. The first master part is the device's head."), parts);
	parts_help->setWordWrap(true);
	parts_layout->addWidget(parts_help);
	m_parts = new QTableWidget(0, 6, parts);
	m_parts->setObjectName(QStringLiteral("deviceTypeParts"));
	m_parts->setHorizontalHeaderLabels({tr("Part"), tr("Role"), tr("Kind"), tr("State"),
										tr("Symbol"), tr("Pins")});
	m_parts->horizontalHeader()->setSectionResizeMode(SymbolColumn, QHeaderView::Stretch);
		//Wide enough for the words of the drop-down lists
	m_parts->setColumnWidth(PartColumn, 50);
	m_parts->setColumnWidth(RoleColumn, 110);
	m_parts->setColumnWidth(KindColumn, 150);
	m_parts->setColumnWidth(StateColumn, 120);
	m_parts->setColumnWidth(PinsColumn, 120);
	m_parts->verticalHeader()->hide();
	m_parts->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_parts->setSelectionMode(QAbstractItemView::SingleSelection);
	parts_layout->addWidget(m_parts);
	auto *part_buttons = new QHBoxLayout();
	auto *add_part = new QPushButton(tr("Add a part"), parts);
	add_part->setObjectName(QStringLiteral("deviceTypeAddPart"));
	auto *add_drawing = new QPushButton(tr("Add a way to draw it"), parts);
	add_drawing->setToolTip(tr("Another way to draw the selected part, such as the same contact as NC"));
	auto *remove = new QPushButton(tr("Remove"), parts);
	auto *up = new QPushButton(tr("Up"), parts);
	auto *down = new QPushButton(tr("Down"), parts);
	auto *symbol = new QPushButton(tr("Choose the symbol..."), parts);
	symbol->setObjectName(QStringLiteral("deviceTypeChooseSymbol"));
	for (QPushButton *button : {add_part, add_drawing, remove, up, down, symbol})
		part_buttons->addWidget(button);
	part_buttons->addStretch();
	parts_layout->addLayout(part_buttons);
	connect(add_part, &QPushButton::clicked, this, &DeviceTypeEditor::addPart);
	connect(add_drawing, &QPushButton::clicked, this, &DeviceTypeEditor::addDrawing);
	connect(remove, &QPushButton::clicked, this, &DeviceTypeEditor::removeRow);
	connect(up, &QPushButton::clicked, this, [this]() {moveRow(-1);});
	connect(down, &QPushButton::clicked, this, [this]() {moveRow(1);});
	connect(symbol, &QPushButton::clicked, this, &DeviceTypeEditor::chooseSymbol);
	layout->addWidget(parts, 1);

	m_problems = new QLabel(this);
	m_problems->setObjectName(QStringLiteral("deviceTypeProblems"));
	m_problems->setWordWrap(true);
	m_problems->setStyleSheet(QStringLiteral("color: #c0392b;"));
	m_problems->hide();
	layout->addWidget(m_problems);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &DeviceTypeEditor::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &DeviceTypeEditor::reject);
	layout->addWidget(buttons);

	resize(760, 560);
	setType(type);
}

/**
	@brief DeviceTypeEditor::setType
	Show @p type. A type which has parts is an existing one: its name is
	kept; a type without parts (new, or only named) may be named.
*/
void DeviceTypeEditor::setType(const DeviceType &type)
{
	m_original = type;
	m_name->setText(type.name);
		//A type with parts is an existing one, which keeps its name
	setNameEditable(type.isNull() || type.parts.isEmpty());
	m_description->setText(type.names.contains(m_lang) ? type.names.value(m_lang) : type.text(m_lang));

	m_informations->setRowCount(0);
	for (const auto &info : type.informations) {
		const int row = m_informations->rowCount();
		m_informations->insertRow(row);
		m_informations->setItem(row, 0, new QTableWidgetItem(info.first));
		m_informations->setItem(row, 1, new QTableWidgetItem(info.second));
	}

	m_parts->setRowCount(0);
	for (int i = 0 ; i < type.parts.size() ; ++i) {
		const DeviceType::Part &part = type.parts.at(i);
		if (part.symbols.isEmpty()) {
			addRow(i + 1, part.role, part.kind, part.state, QString(), QString());
			continue;
		}
		for (const DeviceType::Symbol &symbol : part.symbols)
			addRow(i + 1, part.role, part.kind,
				   symbol.state.isEmpty() ? part.state : symbol.state,
				   symbol.location, symbol.pins.join(QLatin1Char(' ')));
	}
}

/**
	@brief DeviceTypeEditor::setNameEditable
	A new type, or a duplicate, may be named; an existing type keeps its
	name, the one its devices give
*/
void DeviceTypeEditor::setNameEditable(bool editable)
{
	m_new = editable;
	m_name->setReadOnly(!editable);
}

/**
	@brief DeviceTypeEditor::type
	@return the type as shown: rows of one part number are one part, in the
	order of their first row; a part drawn one way only keeps its state on
	the part, a part drawn several ways on each way
*/
DeviceType DeviceTypeEditor::type() const
{
	DeviceType type = m_original;
	type.name = m_name->text().trimmed();
	const QString description = m_description->text().trimmed();
	if (description.isEmpty())
		type.names.remove(m_lang);
	else
		type.names.insert(m_lang, description);

	type.informations.clear();
	for (int row = 0 ; row < m_informations->rowCount() ; ++row) {
		const QString key = cellText(m_informations, row, 0);
		if (!key.isEmpty())
			type.informations << qMakePair(key, cellText(m_informations, row, 1));
	}

	type.parts.clear();
	QList<int> numbers;
	QList<QList<int>> rows_of;
	for (int row = 0 ; row < m_parts->rowCount() ; ++row) {
		const int number = cellText(m_parts, row, PartColumn).toInt();
		int index = int(numbers.indexOf(number));
		if (index < 0) {
			numbers << number;
			rows_of << QList<int>();
			index = int(numbers.size()) - 1;
		}
		rows_of[index] << row;
	}
	for (const QList<int> &rows : std::as_const(rows_of)) {
		const int first = rows.first();
		DeviceType::Part part;
		part.role = wordOf(qobject_cast<QComboBox *>(m_parts->cellWidget(first, RoleColumn)));
		part.kind = wordOf(qobject_cast<QComboBox *>(m_parts->cellWidget(first, KindColumn)));
		const bool several = rows.size() > 1;
		for (int row : rows) {
			const QString state = wordOf(qobject_cast<QComboBox *>(m_parts->cellWidget(row, StateColumn)));
			const QString location = cellText(m_parts, row, SymbolColumn);
			if (!several) part.state = state;
			if (location.isEmpty()) continue;
			DeviceType::Symbol symbol;
			symbol.location = location;
			symbol.pins = cellText(m_parts, row, PinsColumn).split(QLatin1Char(' '), Qt::SkipEmptyParts);
			if (several) symbol.state = state;
			part.symbols << symbol;
		}
		type.parts << part;
	}
	return type;
}

/**
	@brief DeviceTypeEditor::terminalNames
	@param location : a symbol, as a part names it
	@param project : the project an embed:// symbol is in
	@param count : the number of its terminals, -1 when the symbol cannot be read
	@return the names of its terminals which have one, in their order
*/
QStringList DeviceTypeEditor::terminalNames(const QString &location, QETProject *project, int *count)
{
	if (count) *count = -1;
	QStringList names;
	const ElementsLocation symbol = DeviceTypeCheck::symbolLocation(location, project);
	if (!symbol.exist() || !symbol.isElement()) return names;
	const QDomElement definition = symbol.xml();
	if (definition.isNull()) return names;
	const QDomNodeList terminals = definition.elementsByTagName(QStringLiteral("terminal"));
	if (count) *count = terminals.size();
	for (int i = 0 ; i < terminals.size() ; ++i) {
		const QString name = terminals.at(i).toElement().attribute(QStringLiteral("name")).trimmed();
		if (!name.isEmpty()) names << name;
	}
	return names;
}

/**
	@brief DeviceTypeEditor::problems
	@return what keeps @p type from being kept, in words; empty when none
*/
QStringList DeviceTypeEditor::problems(const DeviceType &type, QETProject *project,
									   const QStringList &taken_names)
{
	QStringList list;
	if (type.name.isEmpty())
		list << tr("Give the device type a name.");
	else if (taken_names.contains(type.name))
		//: %1 is a device type's name, such as "CONTR10"
		list << tr("A device type named %1 already exists.").arg(type.name);
	if (type.parts.isEmpty())
		list << tr("A device type needs at least one part.");
	for (int i = 0 ; i < type.parts.size() ; ++i) {
		const DeviceType::Part &part = type.parts.at(i);
		if (part.role.isEmpty())
			//: %1 is a part's number in the device type
			list << tr("Part %1 has no role.").arg(i + 1);
		for (const DeviceType::Symbol &symbol : part.symbols) {
			int terminals = -1;
			terminalNames(symbol.location, project, &terminals);
			if (terminals < 0)
				//: %1 is a part's number, %2 the symbol it names
				list << tr("Part %1: the symbol %2 cannot be found.").arg(i + 1).arg(symbol.location);
			else if (!symbol.pins.isEmpty() && symbol.pins.size() != terminals)
				//: %1 is a part's number, %2 its pins, %3 the symbol's terminals
				list << tr("Part %1: %2 pins for a symbol with %3 terminals.")
						.arg(i + 1).arg(symbol.pins.size()).arg(terminals);
		}
	}
	return list;
}

/**
	@brief DeviceTypeEditor::accept
	Close only when the type can be kept; else say why
*/
void DeviceTypeEditor::accept()
{
	const QStringList found = problems(type(), m_project, m_new ? m_taken : QStringList());
	if (!found.isEmpty()) {
		m_problems->setText(found.join(QLatin1Char('\n')));
		m_problems->show();
		return;
	}
	QDialog::accept();
}

void DeviceTypeEditor::addRow(int part, const QString &role, const QString &kind,
							  const QString &state, const QString &location, const QString &pins)
{
	const int row = m_parts->rowCount();
	m_parts->insertRow(row);
	m_parts->setItem(row, PartColumn, new QTableWidgetItem(QString::number(part)));
	m_parts->setCellWidget(row, RoleColumn, wordBox(roles(), false, role, m_parts));
	m_parts->setCellWidget(row, KindColumn, wordBox(kinds(), true, kind, m_parts));
	m_parts->setCellWidget(row, StateColumn, wordBox(states(), false, state, m_parts));
	m_parts->setItem(row, SymbolColumn, new QTableWidgetItem(location));
	m_parts->setItem(row, PinsColumn, new QTableWidgetItem(pins));
	m_parts->selectRow(row);
}

/**
	@brief DeviceTypeEditor::addPart
	A new part, after the last: the first one a master, the others slaves
*/
void DeviceTypeEditor::addPart()
{
	int last = 0;
	for (int row = 0 ; row < m_parts->rowCount() ; ++row)
		last = qMax(last, cellText(m_parts, row, PartColumn).toInt());
	addRow(last + 1, last ? QStringLiteral("slave") : QStringLiteral("master"),
		   last ? QStringLiteral("simple") : QStringLiteral("coil"),
		   last ? QStringLiteral("NO") : QString(), QString(), QString());
}

/**
	@brief DeviceTypeEditor::addDrawing
	Another way to draw the selected part, below its last row
*/
void DeviceTypeEditor::addDrawing()
{
	const int current = m_parts->currentRow();
	if (current < 0) {
		addPart();
		return;
	}
	const int part = cellText(m_parts, current, PartColumn).toInt();
	addRow(part, wordOf(qobject_cast<QComboBox *>(m_parts->cellWidget(current, RoleColumn))),
		   wordOf(qobject_cast<QComboBox *>(m_parts->cellWidget(current, KindColumn))),
		   QStringLiteral("NC"), QString(), QString());
}

void DeviceTypeEditor::removeRow()
{
	const int current = m_parts->currentRow();
	if (current >= 0) m_parts->removeRow(current);
}

/**
	@brief DeviceTypeEditor::moveRow
	Move the selected row up (-1) or down (1); its part number goes with it
*/
void DeviceTypeEditor::moveRow(int by)
{
	const int current = m_parts->currentRow();
	const int other = current + by;
	if (current < 0 || other < 0 || other >= m_parts->rowCount()) return;
	QList<QStringList> rows;
	for (int row = 0 ; row < m_parts->rowCount() ; ++row)
		rows << QStringList({cellText(m_parts, row, PartColumn),
							 wordOf(qobject_cast<QComboBox *>(m_parts->cellWidget(row, RoleColumn))),
							 wordOf(qobject_cast<QComboBox *>(m_parts->cellWidget(row, KindColumn))),
							 wordOf(qobject_cast<QComboBox *>(m_parts->cellWidget(row, StateColumn))),
							 cellText(m_parts, row, SymbolColumn), cellText(m_parts, row, PinsColumn)});
	rows.swapItemsAt(current, other);
	m_parts->setRowCount(0);
	for (const QStringList &r : std::as_const(rows))
		addRow(r.at(0).toInt(), r.at(1), r.at(2), r.at(3), r.at(4), r.at(5));
	m_parts->selectRow(other);
}

/**
	@brief DeviceTypeEditor::chooseSymbol
	The symbol the selected row draws with, from the collections; its
	terminals' names become the row's pins when it has none yet
*/
void DeviceTypeEditor::chooseSymbol()
{
	const int current = m_parts->currentRow();
	if (current < 0) return;
	const ElementsLocation chosen = ElementDialog::getOpenElementLocation(this);
	if (!chosen.isElement()) return;
	const QString location = chosen.collectionPath(true);
	m_parts->setItem(current, SymbolColumn, new QTableWidgetItem(location));
	if (cellText(m_parts, current, PinsColumn).isEmpty())
		m_parts->setItem(current, PinsColumn, new QTableWidgetItem(
							 terminalNames(location, m_project).join(QLatin1Char(' '))));
}

void DeviceTypeEditor::addInformation()
{
	const int row = m_informations->rowCount();
	m_informations->insertRow(row);
	m_informations->setItem(row, 0, new QTableWidgetItem(QETInformation::ELMT_DESCRIPTION));
	m_informations->setItem(row, 1, new QTableWidgetItem());
	m_informations->setCurrentCell(row, 1);
}

void DeviceTypeEditor::removeInformation()
{
	const int current = m_informations->currentRow();
	if (current >= 0) m_informations->removeRow(current);
}

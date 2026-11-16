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
#include "placelabelpartdialog.h"

#include "../devicetype/devicetype.h"
#include "../devicetype/devicetypeassign.h"
#include "../devicetype/devicetypecheck.h"
#include "../devicetype/devicetypeplacement.h"
#include "../diagram.h"
#include "../diagramevent/macrolabels.h"
#include "../materiallist/findpartdialog.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/structureboxitem.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "../utils/qetsettings.h"
#include "devicetypechooser.h"

#include "../materiallist/partsearch.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QUndoCommand>
#include <QVBoxLayout>

namespace
{
		/// How many parts the popup of the Part field lists
	const int SuggestionCount = 15;
		/// The role of a suggestion holding its part number
	const int PartNumberRole = Qt::UserRole + 1;

		/// The strip and the number of the last terminal placed in @p project:
		/// kept on the project itself, so it goes with it
	const char *LastTerminal = "placeLabelPartLastTerminal";

	QPair<QString, int> lastTerminal(QETProject *project)
	{
		if (!project) return {};
		const QStringList last = project->property(LastTerminal).toStringList();
		if (last.size() != 2) return {};
		return {last.at(0), last.at(1).toInt()};
	}

	void setLastTerminal(QETProject *project, const QString &strip, int number)
	{
		if (project)
			project->setProperty(LastTerminal, QStringList{strip, QString::number(number)});
	}

	QETProject *projectOf(Element *element)
	{
		return element && element->diagram() ? element->diagram()->project() : nullptr;
	}

	QString withoutSign(QString text)
	{
		text = text.trimmed();
		if (text.startsWith(QLatin1Char('-'))) text.remove(0, 1);
		return text.trimmed();
	}

		/// X1:3 is the strip X1 and the number 3; false when it is not a terminal's label
	bool splitTerminal(const QString &label, QString *strip, QString *number)
	{
		const QString text = withoutSign(label);
		const int colon = text.lastIndexOf(QLatin1Char(':'));
		if (colon <= 0) return false;
		if (strip) *strip = text.left(colon).trimmed();
		if (number) *number = text.mid(colon + 1).trimmed();
		return true;
	}

		/// The numbers the terminals of the project hold in the strip @p strip
	QSet<QString> numbersOf(Element *symbol, const QString &strip)
	{
		QSet<QString> numbers;
		QETProject *project = projectOf(symbol);
		if (!project) return numbers;
		for (Diagram *diagram : project->diagrams()) {
			for (Element *element : diagram->elements()) {
				if (element == symbol || !(element->linkType() & Element::Terminale)) continue;
				QString other, number;
				if (splitTerminal(element->elementInformations()
								  .value(QETInformation::ELMT_LABEL).toString(), &other, &number)
					&& other.compare(strip, Qt::CaseInsensitive) == 0)
					numbers << number;
			}
		}
		return numbers;
	}

		/// Can a drawing with these terminal names take these pins?
	bool pinsFit(const QStringList &names, const QStringList &pins)
	{
		if (pins.isEmpty()) return false;
		if (names.size() == pins.size()) return true;
		QStringList distinct = names;
		distinct.removeDuplicates();
		return distinct.size() == pins.size();
	}

	QStringList namesOf(Element *symbol)
	{
		QStringList names;
		for (Terminal *terminal : symbol->terminals())
			names << terminal->baseName().trimmed();
		return names;
	}

		/// The device type of another symbol of the project labelled @p label
	QString typeOfDevice(Element *symbol, const QString &label, Element **other)
	{
		QETProject *project = projectOf(symbol);
		const QString wanted = withoutSign(label);
		if (!project || wanted.isEmpty()) return QString();
		for (Diagram *diagram : project->diagrams()) {
			for (Element *element : diagram->elements()) {
				if (element == symbol) continue;
				const DiagramContext info = element->elementInformations();
				if (withoutSign(info.value(QETInformation::ELMT_LABEL).toString()).compare(wanted, Qt::CaseInsensitive) != 0)
					continue;
				const QString type = DeviceTypeCheck::typeNameOf(info);
				if (type.isEmpty()) continue;
				if (other) *other = element;
				return type;
			}
		}
		return QString();
	}

		/// The symbol of the project, another than @p symbol, whose label is
		/// @p label: the head of the device it names, a symbol with a
		/// device type before one without
	Element *deviceHead(Element *symbol, const QString &label)
	{
		QETProject *project = projectOf(symbol);
		const QString wanted = withoutSign(label);
		if (!project || wanted.isEmpty() || (symbol->linkType() & Element::Terminale)) return nullptr;
		Element *best = nullptr;
		int best_score = 0;
		for (Diagram *diagram : project->diagrams()) {
			for (Element *element : diagram->elements()) {
				if (element == symbol || (element->linkType() & (Element::AllReport | Element::Terminale))) continue;
				const DiagramContext info = element->elementInformations();
				if (withoutSign(info.value(QETInformation::ELMT_LABEL).toString()).compare(wanted, Qt::CaseInsensitive) != 0)
					continue;
				const int score = 1 + (element->linkType() & Element::Master ? 1 : 0)
						+ (DeviceTypeCheck::typeNameOf(info).isEmpty() ? 0 : 2);
				if (score > best_score) {
					best = element;
					best_score = score;
				}
			}
		}
		return best;
	}

	QString bare(QString text)
	{
		text = text.trimmed();
		if (text.startsWith(QLatin1Char('=')) || text.startsWith(QLatin1Char('+'))) text.remove(0, 1);
		return text.trimmed();
	}

		/// A label a symbol has by itself (autonumbering): "_" is none
	QString ownLabel(Element *symbol)
	{
		const QString label = symbol->elementInformations().value(QETInformation::ELMT_LABEL).toString().trimmed();
		return label == QLatin1String("_") ? QString() : label;
	}
}

/**
	@brief PlaceLabelPartDialog::PlaceLabelPartDialog
	@param symbol : the symbol just placed
	@param parent : parent widget
*/
PlaceLabelPartDialog::PlaceLabelPartDialog(Element *symbol, QWidget *parent) :
	QDialog(parent),
	m_symbol(symbol)
{
	setWindowTitle(tr("Label and part"));
	auto *layout = new QVBoxLayout(this);
	auto *form = new QFormLayout();
	layout->addLayout(form);

	m_label = new QLineEdit(this);
	m_label->setObjectName(QStringLiteral("placeLabel"));
	m_label->setText(proposedLabel(symbol));
	form->addRow(tr("Label"), m_label);

	QString path = MaterialList::configuredPath();
	if (path.isEmpty()) path = MaterialList::defaultPath();
	if (!MaterialList::isEmptyFile(path)) MaterialList::load(path, &m_data);
	m_used = usedParts(symbol);
	m_part = new QLineEdit(this);
	m_part->setObjectName(QStringLiteral("placePart"));
		//The popup is ours: it lists the parts the words typed find, not
		//the parts beginning with the text
	m_suggestions = new QStandardItemModel(this);
	m_completer = new QCompleter(m_suggestions, this);
	m_completer->setWidget(m_part);
	m_completer->setCompletionMode(QCompleter::PopupCompletion);
	m_completer->setCaseSensitivity(Qt::CaseInsensitive);
	m_completer->setMaxVisibleItems(SuggestionCount);
	auto *choose = new QPushButton(tr("Choose..."), this);
	choose->setObjectName(QStringLiteral("placePartChoose"));
	choose->setAutoDefault(false);
	auto *part_row = new QHBoxLayout();
	part_row->addWidget(m_part, 1);
	part_row->addWidget(choose);
	form->addRow(tr("Part number"), part_row);

	m_info = new QLabel(this);
	m_info->setObjectName(QStringLiteral("placePartInfo"));
	m_info->setWordWrap(true);
	layout->addWidget(m_info);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, this, &PlaceLabelPartDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &PlaceLabelPartDialog::reject);
	connect(choose, &QPushButton::clicked, this, &PlaceLabelPartDialog::choosePart);
	connect(m_part, &QLineEdit::textEdited, this, &PlaceLabelPartDialog::partEdited);
	connect(m_part, &QLineEdit::textEdited, this, &PlaceLabelPartDialog::suggest);
	connect(m_label, &QLineEdit::textChanged, this, &PlaceLabelPartDialog::showPart);
	connect(m_completer, QOverload<const QModelIndex &>::of(&QCompleter::activated),
			this, &PlaceLabelPartDialog::suggestionChosen);
	m_label->selectAll();
	showPart();
}

QString PlaceLabelPartDialog::label() const {return m_label->text().trimmed();}
QString PlaceLabelPartDialog::part() const {return m_part->text().trimmed();}
MaterialRecord PlaceLabelPartDialog::record() const {return m_record;}
void PlaceLabelPartDialog::setLabel(const QString &label) {m_label->setText(label);}

/**
	Types @p part in the field, as when it is typed
*/
void PlaceLabelPartDialog::setPart(const QString &part)
{
	m_part->setText(part);
	partEdited(part);
}

/**
	@return the number of @p record: its reference, the machine's else
*/
QString PlaceLabelPartDialog::partNumberOf(const MaterialRecord &record)
{
	for (const QString &column : {QStringLiteral("manufacturer_reference"),
								  QStringLiteral("machine_manufacturer_reference"),
								  QStringLiteral("designation")}) {
		const QString value = record.value(column).trimmed();
		if (!value.isEmpty()) return value;
	}
	return QString();
}

/**
	Lists in the popup of the Part field the parts the words typed find,
	best first, as "NUMBER - description - manufacturer". Nothing is
	listed when the text is a part number as it is: Return then accepts
	the dialog at once.
*/
void PlaceLabelPartDialog::suggest(const QString &text)
{
	m_suggestions->clear();
	const QString wanted = text.trimmed();
	if (wanted.isEmpty() || !m_record.values.isEmpty()) {
		m_completer->popup()->hide();
		return;
	}
	const QList<int> found = PartSearch::rank(m_data.records, wanted, SuggestionCount, m_used);
	for (int index : found) {
		const MaterialRecord &record = m_data.records.at(index);
		const QString number = PartSearch::partNumber(record);
		QStringList columns{number};
		const QString description = record.value(QStringLiteral("description")).trimmed();
		const QString manufacturer = record.value(QStringLiteral("manufacturer")).trimmed();
		if (!description.isEmpty()) columns << description;
		if (!manufacturer.isEmpty()) columns << manufacturer;
		if (m_used.contains(number, Qt::CaseInsensitive))
			columns[0] = QString(QChar(0x2605)) + QLatin1Char(' ') + number;
		auto *item = new QStandardItem(columns.join(QStringLiteral(" \u2014 ")));
		item->setData(number, PartNumberRole);
		m_suggestions->appendRow(item);
	}
	if (m_suggestions->rowCount() == 0) {
		m_completer->popup()->hide();
		return;
	}
	m_completer->setCompletionPrefix(QString());
		//As wide as its lines, so the descriptions can be read
	QAbstractItemView *popup = m_completer->popup();
	const int wanted_width = popup->sizeHintForColumn(0)
			+ popup->verticalScrollBar()->sizeHint().width()
			+ 2 * popup->frameWidth() + 8;
	QRect rect = m_part->rect();
	rect.setWidth(qBound(m_part->width(), wanted_width, 720));
	m_completer->complete(rect);
}

/**
	A part of the popup is chosen: its number is the field's text
*/
void PlaceLabelPartDialog::suggestionChosen(const QModelIndex &index)
{
	const QString number = index.data(PartNumberRole).toString();
	if (number.isEmpty()) return;
	m_part->setText(number);
	partEdited(number);
}

/**
	The part typed is a line of the material list, when one has that number
*/
void PlaceLabelPartDialog::partEdited(const QString &text)
{
	const QString wanted = text.trimmed();
	m_record = MaterialRecord();
	if (!wanted.isEmpty()) {
		for (const MaterialRecord &record : std::as_const(m_data.records)) {
			if (record.value(QStringLiteral("manufacturer_reference")).trimmed().compare(wanted, Qt::CaseInsensitive) == 0
				|| record.value(QStringLiteral("machine_manufacturer_reference")).trimmed().compare(wanted, Qt::CaseInsensitive) == 0) {
				m_record = record;
				break;
			}
		}
	}
	showPart();
}

/**
	"Choose...": the Find part window, searching what is typed in the
	field when it is not a part number as it is
*/
void PlaceLabelPartDialog::choosePart()
{
	if (m_data.records.isEmpty()) {
		QString path = MaterialList::configuredPath();
		if (path.isEmpty()) path = MaterialList::defaultPath();
		QMessageBox::information(this, tr("No materials list"),
			tr("There is no materials list file at this location:\n%1").arg(path));
		return;
	}
	const QString label = m_symbol ? resolvedLabel(m_symbol, m_label->text()) : QString();
	FindPartDialog dialog(m_data, m_symbol, label, m_used, this);
	if (m_record.values.isEmpty()) dialog.setSearchText(m_part->text().trimmed());
	if (dialog.exec() != QDialog::Accepted) return;
	m_record = dialog.selectedRecord();
	if (m_record.values.isEmpty()) return;
	m_part->setText(partNumberOf(m_record));
	showPart();
		//Back in the Part field, Return accepts the dialog: on the
		//Choose... button it would open the window again
	m_part->setFocus();
}

void PlaceLabelPartDialog::showPart()
{
	QStringList lines;
	const QString description = m_record.value(QStringLiteral("description")).trimmed();
	if (!description.isEmpty()) lines << description;
	const QString type = m_record.value(QStringLiteral("device_type")).trimmed();
	if (!type.isEmpty()) lines << tr("Device type: %1").arg(type);
	if (m_symbol) {
		const QString label = resolvedLabel(m_symbol, m_label->text());
		const QString place = placeToJoin(m_symbol, label);
		if (!place.isEmpty()) lines << tr("Place: %1").arg(place);
		const Pins pins = pinsFor(m_symbol, label, m_record);
		if (!pins.type.isEmpty() && type.isEmpty()) lines << tr("Device type: %1").arg(pins.type);
		if (pins.none_free)
			lines << tr("No free part of %1 for this symbol").arg(pins.type);
		else if (pins.part >= 0)
			lines << tr("Part: %1").arg(pins.names.join(QLatin1Char(' ')));
	}
	m_info->setText(lines.join(QLatin1Char('\n')));
}

/**
	OK: a terminal number already held in its strip is asked about; Yes
	keeps it, No puts the next free one in the field and stays here
*/
void PlaceLabelPartDialog::accept()
{
	const QString text = resolvedLabel(m_symbol, m_label->text());
	m_label->setText(text);
	QString strip;
	if (m_symbol && (m_symbol->linkType() & Element::Terminale) && splitTerminal(text, &strip, nullptr)
		&& terminalExists(m_symbol, text)) {
		if (QMessageBox::question(this, windowTitle(),
				//: %1 is a terminal's label, such as "X1:3"
				tr("Terminal %1 already exists. Use it again?").arg(text),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
			m_label->setText(strip + QLatin1Char(':') + nextFreeTerminal(m_symbol, strip));
			m_label->setFocus();
			return;
		}
	}
	QDialog::accept();
}

/**
	@return the part numbers the elements of @p symbol's project carry,
	@p symbol apart: the parts already used in the project
*/
QStringList PlaceLabelPartDialog::usedParts(Element *symbol)
{
	QStringList parts;
	QETProject *project = projectOf(symbol);
	if (!project) return parts;
	QSet<QString> seen;
	for (Diagram *diagram : project->diagrams()) {
		for (Element *element : diagram->elements()) {
			if (element == symbol) continue;
			const DiagramContext info = element->elementInformations();
			for (const QString &key : {QETInformation::ELMT_MANUFACTURER_REF,
									   QETInformation::ELMT_MACHINE_MANUFACTURER_REF}) {
				const QString part = info.value(key).toString().trimmed();
				if (!part.isEmpty() && !seen.contains(part.toLower())) {
					seen << part.toLower();
					parts << part;
				}
			}
		}
	}
	return parts;
}

/**
	@return whether placing @p symbol asks for its label and its part
*/
bool PlaceLabelPartDialog::asksFor(Element *symbol)
{
	return symbol && QetSettings::askLabelAndPartPlacingSymbol()
			&& (symbol->linkType() & (Element::Master | Element::Simple | Element::Terminale))
			&& !(symbol->linkType() & (Element::Slave | Element::AllReport));
}

/**
	@return the lowest number above @p from free in the strip @p strip
	of @p symbol's project
*/
QString PlaceLabelPartDialog::nextFreeTerminal(Element *symbol, const QString &strip)
{
	const QSet<QString> held = numbersOf(symbol, withoutSign(strip));
	const QPair<QString, int> last = lastTerminal(projectOf(symbol));
	int number = last.first.compare(withoutSign(strip), Qt::CaseInsensitive) == 0 ? last.second + 1 : 1;
	while (held.contains(QString::number(number))) ++number;
	return QString::number(number);
}

bool PlaceLabelPartDialog::terminalExists(Element *symbol, const QString &label)
{
	QString strip, number;
	return splitTerminal(label, &strip, &number) && numbersOf(symbol, strip).contains(number);
}

/**
	@return what is proposed as @p symbol's label
*/
QString PlaceLabelPartDialog::proposedLabel(Element *symbol)
{
	if (!symbol) return QString();
	if (symbol->linkType() & Element::Terminale) {
		const QString own = ownLabel(symbol);
		if (!own.isEmpty()) return own;
		const QPair<QString, int> last = lastTerminal(projectOf(symbol));
		const QString strip = last.first.isEmpty() ? QStringLiteral("X1") : last.first;
		return strip + QLatin1Char(':') + nextFreeTerminal(symbol, strip);
	}
	const QString own = ownLabel(symbol);
	if (!own.isEmpty()) return own;
	QString prefix = withoutSign(symbol->getPrefix());
	if (prefix.isEmpty() || !symbol->diagram()) return QString();
	QSet<QString> typed;
	const QSet<QString> held = MacroLabels::taken(symbol->diagram(), &typed);
	const QString label = MacroLabels::nextFree(prefix + QLatin1Char('?'), MacroLabels::placeOf(symbol->diagram()), held);
		//Written with the sign when the prefix has it, or the labels of the project do
	bool signed_label = symbol->getPrefix().trimmed().startsWith(QLatin1Char('-'));
	for (const QString &other : std::as_const(typed))
		if (other.startsWith(prefix, Qt::CaseInsensitive)) signed_label = true;
	return signed_label ? QLatin1Char('-') + label : label;
}

/**
	@return @p text as the label of @p symbol: for a terminal, a strip
	name alone (X2) is the strip's next free terminal (X2:1)
*/
QString PlaceLabelPartDialog::resolvedLabel(Element *symbol, const QString &text)
{
	const QString label = text.trimmed();
	if (label.isEmpty() || !symbol || !(symbol->linkType() & Element::Terminale)
		|| label.contains(QLatin1Char(':')))
		return label;
	return label + QLatin1Char(':') + nextFreeTerminal(symbol, label);
}

/**
	@return the undo step giving @p symbol the label @p label (when it
	is not its own), the values of @p part as the articles tab writes
	them, and the device type the part names, when the project or the
	user's library has it; nullptr if nothing changes
*/
QUndoCommand *PlaceLabelPartDialog::apply(Element *symbol, const QString &label,
										  const MaterialRecord &part)
{
	if (!symbol) return nullptr;
	const DiagramContext old_info = symbol->elementInformations();
	DiagramContext new_info = old_info;
	const QString new_label = label.trimmed();
	if (!new_label.isEmpty() && new_label != old_info.value(QETInformation::ELMT_LABEL).toString()) {
			//A label typed no longer follows a numbering's formula
		new_info.addValue(QETInformation::ELMT_LABEL, new_label);
		new_info.remove(QETInformation::ELMT_FORMULA);
		new_info.remove(QETInformation::ELMT_FORMULA_ID);
	}
		//Joining a device elsewhere: its place, the folio's and the boxes' notwithstanding
	if (!new_label.isEmpty()) {
		if (Element *head = deviceHead(symbol, new_label)) {
			const StructureBoxItem::Place wanted = StructureBoxItem::placeOf(head);
			const StructureBoxItem::Place own = StructureBoxItem::placeOf(symbol);
			if (bare(wanted.plant) != bare(own.plant) && !bare(wanted.plant).isEmpty())
				new_info.addValue(QETInformation::ELMT_PLANT, bare(wanted.plant));
			if (bare(wanted.location) != bare(own.location) && !bare(wanted.location).isEmpty())
				new_info.addValue(QETInformation::ELMT_LOCATION, bare(wanted.location));
		}
	}

	QString type;
	const QHash<QString, QString> values = MaterialList::elementInfoFor(part, 0);
	QETProject *project = projectOf(symbol);
	const QString named = values.value(QETInformation::ELMT_DEVICE_TYPE);
	if (!named.isEmpty() && project
		&& (!project->deviceType(named).isNull() || !DeviceTypeLibrary::load(named).isNull()))
		type = named;
	for (auto it = values.cbegin(); it != values.cend(); ++it)
		new_info.addValue(it.key(), it.value());

		//The type first, with the copy of a type of the library; the
		//information written after it holds the type's name as well
	QUndoCommand *undo = type.isEmpty() ? nullptr : DeviceTypeChooser::give(symbol, type);
	if (!undo) undo = new QUndoCommand;
	undo->setText(QObject::tr("Set the label and the part"));
	if (new_info != old_info)
		new ChangeElementInformationCommand(symbol, old_info, new_info, undo);
	if (!undo->childCount()) {
		delete undo;
		return nullptr;
	}

	QString strip, number;
	if ((symbol->linkType() & Element::Terminale) && splitTerminal(new_label, &strip, &number)) {
		bool numeric = false;
		const int n = number.toInt(&numeric);
		if (numeric) setLastTerminal(project, strip, n);
	}
	return undo;
}

/**
	@return the place @p symbol takes joining the device @p label names
*/
QString PlaceLabelPartDialog::placeToJoin(Element *symbol, const QString &label)
{
	Element *head = deviceHead(symbol, label);
	if (!head) return QString();
	const StructureBoxItem::Place wanted = StructureBoxItem::placeOf(head);
	const StructureBoxItem::Place own = StructureBoxItem::placeOf(symbol);
	if (bare(wanted.plant) == bare(own.plant) && bare(wanted.location) == bare(own.location))
		return QString();
	QString place;
	if (!bare(wanted.plant).isEmpty()) place += QLatin1Char('=') + bare(wanted.plant);
	if (!bare(wanted.location).isEmpty()) place += QLatin1Char('+') + bare(wanted.location);
	return place;
}

/**
	@return the pins @p symbol would take given @p label and @p part; see Pins
*/
PlaceLabelPartDialog::Pins PlaceLabelPartDialog::pinsFor(Element *symbol, const QString &label,
														 const MaterialRecord &part)
{
	Pins pins;
	QETProject *project = projectOf(symbol);
	if (!project || !symbol->diagram()) return pins;
	Element *other = nullptr;
	QString name = part.value(QStringLiteral("device_type")).trimmed();
		//No part: the device the label names gives the type
	if (part.values.isEmpty())
		name = typeOfDevice(symbol, label, &other);
	if (name.isEmpty()) return pins;
	DeviceType type = project->deviceType(name);
	if (type.isNull()) type = DeviceTypeLibrary::load(name);
	if (type.isNull()) return pins;
	pins.type = name;
	if (!other) typeOfDevice(symbol, label, &other);

		//The parts no other symbol of the device draws, in the type's order
	QList<int> free;
	for (int i = 0 ; i < type.parts.size() ; ++i) free << i;
	if (other && DeviceTypeCheck::typeNameOf(other->elementInformations()) == name) {
		const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(other);
		if (device.known() && device.type.name == name) {
			QList<int> drawn;
			for (int s = 0 ; s < device.symbols.size() ; ++s)
				if (device.symbols.at(s).data() != symbol && device.drawn.value(s, -1) >= 0)
					drawn << device.drawn.at(s);
			for (int part_drawn : std::as_const(drawn)) free.removeAll(part_drawn);
		}
	}
	const QStringList names = namesOf(symbol);
	for (int i : std::as_const(free)) {
		const DeviceType::Part &p = type.parts.at(i);
		for (int way = 0 ; way < p.symbols.size() ; ++way) {
			if (!pinsFit(names, p.symbols.at(way).pins)) continue;
			pins.part = i;
			pins.way = way;
			pins.names = p.symbols.at(way).pins;
			pins.already = names == pins.names;
			return pins;
		}
	}
	pins.none_free = true;
	return pins;
}

/**
	@brief PlaceLabelPartDialog::applyAndPush
	The undo step of apply(), and the replacement of @p symbol by the
	drawing with its part's pin names (DeviceTypeAssign), in one step of
	the folio's undo stack.
*/
Element *PlaceLabelPartDialog::applyAndPush(Element *symbol, const QString &label,
											const MaterialRecord &part, bool *none_free)
{
	if (none_free) *none_free = false;
	if (!symbol || !symbol->diagram()) return symbol;
	Diagram *diagram = symbol->diagram();
	QETProject *project = diagram->project();
	QUndoCommand *undo = apply(symbol, label, part);
	const QString typed = label.trimmed();
	const Pins pins = pinsFor(symbol, typed.isEmpty() ? ownLabel(symbol) : typed, part);
	if (!undo && pins.type.isEmpty()) return symbol;
	const QString text = QObject::tr("Set the label and the part");
	diagram->undoStack().beginMacro(text);
	if (undo) diagram->undoStack().push(undo);
	Element *result = symbol;
	if (!pins.type.isEmpty() && pins.part < 0 && none_free) *none_free = true;
	if (pins.part >= 0 && !pins.already) {
		const DeviceType type = project->deviceType(pins.type);
		const DeviceTypeCheck::Device device = DeviceTypeAssign::deviceOf(symbol, type);
		for (const DeviceTypeAssign::Choice &choice : DeviceTypeAssign::choices(symbol, device)) {
			if (choice.part != pins.part || choice.way != pins.way || !choice.own_drawing || !choice.possible)
				continue;
			if (Element *fresh = DeviceTypeAssign::apply(symbol, device, choice)) result = fresh;
			break;
		}
	}
	diagram->undoStack().endMacro();
	return result;
}

/**
	@return whether the device type question is still to be asked for
	@p symbol: its project follows the device types, it is a master and
	has no type yet (a part may have brought one)
*/
bool PlaceLabelPartDialog::asksDeviceType(Element *symbol)
{
	QETProject *project = projectOf(symbol);
	return project && project->followDeviceTypes() && (symbol->linkType() & Element::Master)
			&& DeviceTypeCheck::typeNameOf(symbol->elementInformations()).isEmpty();
}

/**
	@brief PlaceLabelPartDialog::askFor
	Ask @p symbol's label and part, give them in one undo step; then the
	device type question, unless the part brought the type, and the parts
	of that type. Cancel leaves the symbol as it was placed.
*/
void PlaceLabelPartDialog::askFor(Element *symbol, QWidget *parent)
{
	if (!symbol || !symbol->diagram()) return;
	QPointer<Element> placed = symbol;
	PlaceLabelPartDialog dialog(symbol, parent);
	if (dialog.exec() == QDialog::Accepted && placed && placed->diagram()) {
			//The symbol may be replaced by one with its part's pin names
		placed = applyAndPush(placed, dialog.label(), dialog.record());
	}
	if (!placed) return;
	if (asksDeviceType(placed)) DeviceTypeChooser::askFor(placed, parent);
	if (placed && (placed->linkType() & Element::Master))
		DeviceTypePlacement::offerParts(placed, parent);
}

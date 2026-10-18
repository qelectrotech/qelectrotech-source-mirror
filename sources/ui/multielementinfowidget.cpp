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
#include "multielementinfowidget.h"

#include "../dataBase/projectdatabase.h"
#include "../diagram.h"
#include "../qet.h"
#include "../qeticons.h"
#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/elementtextitemgroup.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../undocommand/changeelementinformationcommand.h"

#include <QCheckBox>
#include <QCompleter>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSet>
#include <QSqlQuery>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
/**
	The change of one editing session: the keystrokes applied one by one in
	the selection panel make one undo step, but never join a change made
	before, by the same editor or another one, to the same elements.
*/
class MultiElementInfoCommand : public ChangeElementInformationCommand
{
	public:
		MultiElementInfoCommand(
				const QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> &map,
				quint64 session) :
			ChangeElementInformationCommand(map),
			m_session(session)
		{}

		int id() const override {return 1439;}
		bool mergeWith(const QUndoCommand *other) override
		{
			if (other->id() != id()
					|| static_cast<const MultiElementInfoCommand *>(other)->m_session != m_session) {
				return false;
			}
			return ChangeElementInformationCommand::mergeWith(other);
		}

	private:
		quint64 m_session;
};

quint64 s_last_session = 0;

const QString EXCLUDE_FROM_BOM    = QStringLiteral("exclude_from_bom");
const QString AUTO_NUM_LOCKED     = QStringLiteral("auto_num_locked");
const QString POTENTIAL_ISOLATING = QStringLiteral("potential_isolating");

QString flagText(bool on)
{
	return on ? QStringLiteral("true") : QStringLiteral("false");
}
}

/**
	@brief MultiElementInfoWidget::MultiElementInfoWidget
	@param elements : the elements to edit
	@param other_items : how many other selected items this editor leaves
	alone, only shown to the user
	@param parent
*/
MultiElementInfoWidget::MultiElementInfoWidget(const QList<Element *> &elements,
											   int other_items,
											   QWidget *parent) :
	PropertiesEditorWidget(parent)
{
	buildInterface();
	setElements(elements, other_items);
}

MultiElementInfoWidget::~MultiElementInfoWidget()
{
	qDeleteAll(m_rows);
	qDeleteAll(m_flags);
}

/**
	@brief MultiElementInfoWidget::takesInformations
	@return true if @p element has information the user can edit: the
	element kinds ElementPropertiesWidget gives an Information tab.
	Plain slaves and folio reports take theirs from what they are linked to.
*/
bool MultiElementInfoWidget::takesInformations(const Element *element)
{
	if (!element) {
		return false;
	}
	switch (element->linkType()) {
		case Element::Simple:
		case Element::Thumbnail:
		case Element::Master:
		case Element::Terminale:
			return true;
		default:
			return false;
	}
}

/**
	@brief MultiElementInfoWidget::informationElements
	@return the elements of @p items which take information
*/
QList<Element *> MultiElementInfoWidget::informationElements(const QList<QGraphicsItem *> &items)
{
	QList<Element *> list;
	for (QGraphicsItem *qgi : items) {
		if (qgi->type() != Element::Type) {
			continue;
		}
		auto element = static_cast<Element *>(qgi);
		if (takesInformations(element)) {
			list << element;
		}
	}
	return list;
}

/**
	@brief MultiElementInfoWidget::withoutOwnTexts
	@return @p items without the texts and text groups of elements which
	are in @p items too: select all takes an element's texts with it, and
	they are part of the element rather than other items.
*/
QList<QGraphicsItem *> MultiElementInfoWidget::withoutOwnTexts(const QList<QGraphicsItem *> &items)
{
	QSet<QGraphicsItem *> elements;
	for (QGraphicsItem *qgi : items) {
		if (qgi->type() == Element::Type) {
			elements.insert(qgi);
		}
	}
	if (elements.isEmpty()) {
		return items;
	}

	QList<QGraphicsItem *> list;
	for (QGraphicsItem *qgi : items)
	{
		Element *owner = nullptr;
		if (qgi->type() == DynamicElementTextItem::Type) {
			owner = static_cast<DynamicElementTextItem *>(qgi)->parentElement();
		} else if (auto group = dynamic_cast<ElementTextItemGroup *>(qgi)) {
			owner = group->parentElement();
		}
		if (!owner || !elements.contains(owner)) {
			list << qgi;
		}
	}
	return list;
}

/**
	@brief MultiElementInfoWidget::setElements
	Edit @p elements, forgetting any pending change.
*/
void MultiElementInfoWidget::setElements(const QList<Element *> &elements, int other_items)
{
	for (const auto &element : std::as_const(m_elements)) {
		if (element) {
			disconnect(element.data(), &Element::elementInfoChange,
					   this, &MultiElementInfoWidget::elementInfoChanged);
		}
	}

	m_elements.clear();
	for (Element *element : elements) {
		m_elements << QPointer<Element>(element);
		connect(element, &Element::elementInfoChange,
				this, &MultiElementInfoWidget::elementInfoChanged);
	}
	m_other_items = other_items;

	updateUi();
}

/**
	@brief MultiElementInfoWidget::elements
	@return the edited elements still alive
*/
QList<Element *> MultiElementInfoWidget::elements() const
{
	QList<Element *> list;
	for (const auto &element : m_elements) {
		if (element) {
			list << element.data();
		}
	}
	return list;
}

/**
	@brief MultiElementInfoWidget::apply
	Push the change of every element, as one undo step.
*/
void MultiElementInfoWidget::apply()
{
	QUndoCommand *undo = associatedUndo();
	if (!undo) {
		return;
	}

	const auto list = elements();
	if (list.isEmpty() || !list.first()->diagram()) {
		delete undo;
		return;
	}

	m_applying = true;
	list.first()->diagram()->undoStack().push(undo);
	m_applying = false;
}

/**
	@brief MultiElementInfoWidget::associatedUndo
	@return the command giving each element its new information, or
	nullptr if no element would change.
	A field left alone gives each element back the value it had when the
	editor was filled, so restoring a field in live edit undoes it.
*/
QUndoCommand *MultiElementInfoWidget::associatedUndo() const
{
	QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> changes;

	for (Element *element : elements())
	{
		const DiagramContext old_info = element->elementInformations();
		const DiagramContext snap = m_snapshot.value(element, old_info);
		DiagramContext new_info = old_info;

		for (const Row *row : m_rows)
		{
			QString txt;
			switch (row->state) {
				case Keep:
					if (snap.contains(row->key)) {
						new_info.addValue(row->key, snap.value(row->key),
										  snap.keyMustShow(row->key));
					} else {
						new_info.remove(row->key);
					}
					continue;
				case Clear:
					new_info.remove(row->key);
					continue;
				case Set:
					if (!row->edit->hasAcceptableInput()) {
						continue;
					}
					txt = row->edit->text();
					txt.remove(QStringLiteral("\r"));
					txt.remove(QStringLiteral("\n"));
					if (txt.isEmpty()) {
						new_info.remove(row->key);
					} else {
						new_info.addValue(row->key, txt,
										  old_info.contains(row->key)
										  ? old_info.keyMustShow(row->key)
										  : true);
					}
					continue;
			}
		}

			//The element keeps following its numbering scheme as long as
			//its formula is left as it was; a formula given by hand is its own.
		if (new_info.value(QETInformation::ELMT_FORMULA)
				== snap.value(QETInformation::ELMT_FORMULA)
				&& snap.contains(QETInformation::ELMT_FORMULA_ID)) {
			new_info.addValue(QETInformation::ELMT_FORMULA_ID,
							  snap.value(QETInformation::ELMT_FORMULA_ID), false);
		} else {
			new_info.remove(QETInformation::ELMT_FORMULA_ID);
		}

		for (const Flag *flag : m_flags)
		{
			if (flag->check->isHidden()) {
				continue;
			}
			if (!flag->changed) {
				if (snap.contains(flag->key)) {
					new_info.addValue(flag->key, snap.value(flag->key),
									  snap.keyMustShow(flag->key));
				} else {
					new_info.remove(flag->key);
				}
			} else {
				new_info.addValue(flag->key, flagText(flag->check->isChecked()));
			}
		}

		if (new_info != old_info) {
			changes.insert(QPointer<Element>(element), qMakePair(old_info, new_info));
		}
	}

	if (changes.isEmpty()) {
		return nullptr;
	}
	return new MultiElementInfoCommand(changes, m_session);
}

/**
	@brief MultiElementInfoWidget::reset
	Bring back every field to what the elements had, and in live edit
	give it back to the elements.
*/
void MultiElementInfoWidget::reset()
{
	m_updating = true;
	for (Row *row : std::as_const(m_rows)) {
		restoreRow(row);
	}
	for (Flag *flag : std::as_const(m_flags)) {
		restoreFlag(flag);
	}
	m_updating = false;
	updateLabelRow();
	changed();
}

/**
	@brief MultiElementInfoWidget::updateUi
	Read the elements again, forgetting any pending change.
*/
void MultiElementInfoWidget::updateUi()
{
	takeSnapshot();
	fillRows();
	updateHeader();
	updateSuggestions();
}

/**
	@brief MultiElementInfoWidget::fieldState
	@return what the field @p key does on apply
*/
MultiElementInfoWidget::FieldState MultiElementInfoWidget::fieldState(const QString &key) const
{
	const Row *row = rowForKey(key);
	return row ? row->state : Keep;
}

/**
	@brief MultiElementInfoWidget::setFieldText
	Type @p text in the field @p key, as the user would.
*/
void MultiElementInfoWidget::setFieldText(const QString &key, const QString &text)
{
	if (Row *row = rowForKey(key)) {
		row->edit->setText(text);
	}
}

/**
	@brief MultiElementInfoWidget::setFieldCleared
	Tick or untick the erase box of the field @p key, as the user would.
*/
void MultiElementInfoWidget::setFieldCleared(const QString &key, bool cleared)
{
	if (Row *row = rowForKey(key)) {
		row->erase->setChecked(cleared);
		rowErased(row);
	}
}

/**
	@brief MultiElementInfoWidget::restoreField
	Click the restore button of the field @p key.
*/
void MultiElementInfoWidget::restoreField(const QString &key)
{
	if (Row *row = rowForKey(key)) {
		restoreRow(row);
		changed();
	}
}

/**
	@brief MultiElementInfoWidget::fieldIsMixed
	@return true if the elements had different values for @p key
*/
bool MultiElementInfoWidget::fieldIsMixed(const QString &key) const
{
	const Row *row = rowForKey(key);
	return row && row->mixed;
}

/**
	@brief MultiElementInfoWidget::setLiveEdit
	@param live_edit : true to apply every change at once
	@return always true
*/
bool MultiElementInfoWidget::setLiveEdit(bool live_edit)
{
	m_live_edit = live_edit;
	return true;
}

/**
	@brief MultiElementInfoWidget::buildInterface
	One row per information key: name, text, erase box, restore button.
	The numbering scheme, label freezing and number picker of the
	one-element editor are left out: they are about one element.
*/
void MultiElementInfoWidget::buildInterface()
{
	auto main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(0, 0, 0, 0);

	m_header = new QLabel(this);
	m_header->setWordWrap(true);
	m_header->setContentsMargins(5, 5, 5, 0);
	main_layout->addWidget(m_header);

	auto flags_layout = new QVBoxLayout();
	main_layout->addLayout(flags_layout);

	const QList<QPair<QString, QString>> flags {
		{EXCLUDE_FROM_BOM,    tr("Exclude from the bill of materials")},
		{POTENTIAL_ISOLATING, tr("Function separation")},
		{AUTO_NUM_LOCKED,     tr("Exclude from automatic numbering")}
	};
	for (const auto &pair : flags)
	{
		auto flag = new Flag;
		flag->key = pair.first;
		flag->check = new QCheckBox(pair.second, this);
		flag->restore = new QToolButton(this);
		flag->restore->setIcon(QET::Icons::EditUndo);
		flag->restore->setAutoRaise(true);
		flag->restore->setToolTip(tr("Restore the values the elements had"));
		auto row_layout = new QHBoxLayout();
		row_layout->setContentsMargins(5, 0, 5, 0);
		row_layout->addWidget(flag->check, 1);
		row_layout->addWidget(flag->restore);
		flags_layout->addLayout(row_layout);
		m_flags << flag;

		connect(flag->check, &QCheckBox::clicked, this, [this, flag]() {
			flagClicked(flag);
		});
		connect(flag->restore, &QToolButton::clicked, this, [this, flag]() {
			restoreFlag(flag);
			changed();
		});
	}

	auto scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	auto content = new QWidget(scroll);
	auto grid = new QGridLayout(content);
	grid->setColumnStretch(1, 1);

	QStringList keys = QETInformation::elementInfoKeys();
		//It has its own check box above, see ElementInfoWidget::buildInterface
	keys.removeAll(EXCLUDE_FROM_BOM);

	int line = 0;
	for (const QString &key : std::as_const(keys))
	{
		auto row = new Row;
		row->key = key;
		row->label = new QLabel(QETInformation::translatedInfoKey(key), content);
		row->edit = new QLineEdit(content);
		row->edit->setClearButtonEnabled(true);
		row->erase = new QCheckBox(content);
		row->erase->setToolTip(tr("Delete this text"));
		row->restore = new QToolButton(content);
		row->restore->setIcon(QET::Icons::EditUndo);
		row->restore->setAutoRaise(true);
		row->restore->setToolTip(tr("Restore the values the elements had"));

		if (key == QETInformation::ELMT_WIDTH
				|| key == QETInformation::ELMT_HEIGHT
				|| key == QETInformation::ELMT_DEPTH) {
			row->edit->setValidator(new QETInformation::NumericInfoValidator(row->edit));
			row->edit->setToolTip(tr("Decimal number with a dot as separator (e.g. 80.5)"));
		}

		grid->addWidget(row->label,   line, 0);
		grid->addWidget(row->edit,    line, 1);
		grid->addWidget(row->erase,   line, 2);
		grid->addWidget(row->restore, line, 3);
		++line;
		m_rows << row;

		connect(row->edit, &QLineEdit::textChanged, this, [this, row]() {
			if (!m_updating) {
				rowEdited(row);
			}
		});
		connect(row->erase, &QCheckBox::clicked, this, [this, row]() {
			rowErased(row);
		});
		connect(row->restore, &QToolButton::clicked, this, [this, row]() {
			restoreRow(row);
			changed();
		});
	}
	grid->setRowStretch(line, 1);

	scroll->setWidget(content);
	main_layout->addWidget(scroll, 1);
}

/**
	@brief MultiElementInfoWidget::takeSnapshot
	Keep the information every element has now: the values a field left
	alone gives back.
*/
void MultiElementInfoWidget::takeSnapshot()
{
	m_session = ++s_last_session;
	m_snapshot.clear();
	for (Element *element : elements()) {
		m_snapshot.insert(element, element->elementInformations());
	}
}

/**
	@brief MultiElementInfoWidget::fillRows
	Show in each field the value shared by every element, or that the
	values differ.
*/
void MultiElementInfoWidget::fillRows()
{
	const auto list = elements();
	m_updating = true;

	for (Row *row : std::as_const(m_rows))
	{
		row->mixed = false;
		row->initial.clear();
		bool first = true;
		for (Element *element : list) {
			const QString value = m_snapshot.value(element).value(row->key).toString();
			if (first) {
				row->initial = value;
				first = false;
			} else if (value != row->initial) {
				row->mixed = true;
				row->initial.clear();
				break;
			}
		}
		restoreRow(row);
	}

	bool all_terminals = !list.isEmpty();
	for (Element *element : list) {
		if (element->elementData().m_type != ElementData::Terminal) {
			all_terminals = false;
			break;
		}
	}

	for (Flag *flag : std::as_const(m_flags))
	{
		flag->check->setVisible(flag->key == EXCLUDE_FROM_BOM || all_terminals);
		flag->restore->setVisible(flag->check->isVisible());

		int on = 0;
		for (Element *element : list) {
			if (QET::infoFlagIsTrue(m_snapshot.value(element).value(flag->key).toString())) {
				++on;
			}
		}
		flag->initial = on == 0 ? Qt::Unchecked
								: on == list.size() ? Qt::Checked
													: Qt::PartiallyChecked;
		restoreFlag(flag);
	}

	m_updating = false;
	updateLabelRow();
}

/**
	@brief MultiElementInfoWidget::updateHeader
	Say how many elements are edited, and that the rest of the selection
	is not.
*/
void MultiElementInfoWidget::updateHeader()
{
	QString text = tr("Elements edited: %1").arg(elements().size());
	if (m_other_items > 0) {
		text += QStringLiteral("\n")
				+ tr("Other selected items, left unchanged: %1").arg(m_other_items);
	}
	m_header->setText(text);
}

/**
	@brief MultiElementInfoWidget::updateSuggestions
	Offer, while typing, the values the project already uses,
	the way ElementInfoWidget::updateSuggestions does.
*/
void MultiElementInfoWidget::updateSuggestions()
{
	const auto list = elements();
	Diagram *diagram = list.isEmpty() ? nullptr : list.first()->diagram();
	QETProject *project = diagram ? diagram->project() : nullptr;
	if (!project || !project->dataBase()) {
		return;
	}

		//Only a column of element_info can be queried; the key becomes
		//part of the SQL text, so it is checked against that list.
	const QStringList columns = QETInformation::elementInfoKeys();
	for (Row *row : std::as_const(m_rows))
	{
		if (row->key == QETInformation::ELMT_LABEL || !columns.contains(row->key)) {
			continue;
		}

		QStringList values;
		auto query = project->dataBase()->newQuery(QStringLiteral(
			"SELECT v, MAX(n) FROM ("
				"SELECT \"%1\" AS v, COUNT(*) AS n FROM element_info "
				"WHERE \"%1\" IS NOT NULL AND \"%1\" != '' "
				"GROUP BY \"%1\") "
			"GROUP BY v COLLATE NOCASE "
			"ORDER BY v COLLATE NOCASE").arg(row->key));
		while (query.next()) {
			values << query.value(0).toString();
		}

		if (values.isEmpty()) {
			row->edit->setCompleter(nullptr);
			continue;
		}
		auto completer = new QCompleter(values, row->edit);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setFilterMode(Qt::MatchContains);
		row->edit->setCompleter(completer);
	}
}

/**
	@brief MultiElementInfoWidget::rowEdited
	The user typed in a field: its text goes to every element, unless
	it is back to what they all had.
*/
void MultiElementInfoWidget::rowEdited(Row *row)
{
	const QString text = row->edit->text();
	if (row->mixed ? text.isEmpty() : text == row->initial) {
		row->state = Keep;
	} else {
		row->state = Set;
	}
	updateRowLook(row);
	if (row->key == QETInformation::ELMT_FORMULA) {
		updateLabelRow();
	}
	changed();
}

/**
	@brief MultiElementInfoWidget::rowErased
	The erase box of a field was ticked or unticked.
*/
void MultiElementInfoWidget::rowErased(Row *row)
{
	m_updating = true;
	if (row->erase->isChecked()) {
		row->state = Clear;
		row->edit->clear();
	} else {
		row->state = Keep;
		row->edit->setText(row->initial);
	}
	m_updating = false;
	updateRowLook(row);
	if (row->key == QETInformation::ELMT_FORMULA) {
		updateLabelRow();
	}
	changed();
}

/**
	@brief MultiElementInfoWidget::restoreRow
	Bring a field back to what the elements had. Does not apply.
*/
void MultiElementInfoWidget::restoreRow(Row *row)
{
	const bool was_updating = m_updating;
	m_updating = true;
	row->state = Keep;
	row->erase->setChecked(false);
	row->edit->setText(row->initial);
	m_updating = was_updating;
	updateRowLook(row);
	if (!m_updating && row->key == QETInformation::ELMT_FORMULA) {
		updateLabelRow();
	}
}

/**
	@brief MultiElementInfoWidget::restoreFlag
	Bring a check box back to what the elements had. Does not apply.
*/
void MultiElementInfoWidget::restoreFlag(Flag *flag)
{
	flag->changed = false;
	flag->check->setTristate(flag->initial == Qt::PartiallyChecked);
	flag->check->setCheckState(flag->initial);
	flag->restore->setEnabled(false);
}

/**
	@brief MultiElementInfoWidget::flagClicked
	A click gives the box's state to every element. A box which started
	"partly checked" goes on as a plain on/off box: cycling back through
	"partly" is what its restore button is for.
*/
void MultiElementInfoWidget::flagClicked(Flag *flag)
{
	if (flag->check->isTristate()) {
		flag->check->setTristate(false);
		if (flag->check->checkState() == Qt::PartiallyChecked) {
			flag->check->setCheckState(Qt::Checked);
		}
	}
	flag->changed = flag->check->checkState() != flag->initial;
	flag->restore->setEnabled(flag->changed);
	changed();
}

/**
	@brief MultiElementInfoWidget::updateRowLook
	A field whose values differ says so; a field which will change
	something has its restore button on and its name in bold.
*/
void MultiElementInfoWidget::updateRowLook(Row *row)
{
	row->edit->setDisabled(row->state == Clear);
	if (row->state == Clear) {
		row->edit->setPlaceholderText(tr("Delete this text"));
	} else if (row->mixed) {
		row->edit->setPlaceholderText(tr("Multiple values"));
	} else {
		row->edit->setPlaceholderText(QString());
	}
	row->restore->setEnabled(row->state != Keep);
	QFont font = row->label->font();
	font.setBold(row->state != Keep);
	row->label->setFont(font);
}

/**
	@brief MultiElementInfoWidget::updateLabelRow
	As in the one-element editor, the label cannot be typed while the
	formula field holds a formula: the formula makes the label.
*/
void MultiElementInfoWidget::updateLabelRow()
{
	Row *formula = rowForKey(QETInformation::ELMT_FORMULA);
	Row *label = rowForKey(QETInformation::ELMT_LABEL);
	if (formula && label) {
		const bool has_formula = !formula->edit->text().isEmpty();
		label->edit->setEnabled(!has_formula && label->state != Clear);
		label->erase->setEnabled(!has_formula);
	}
}

/**
	@brief MultiElementInfoWidget::changed
	In live edit, every change is applied at once.
*/
void MultiElementInfoWidget::changed()
{
	if (m_live_edit && !m_updating) {
		apply();
	}
}

/**
	@brief MultiElementInfoWidget::elementInfoChanged
	The information of an element was changed by something else than
	this editor (undo, another editor): read the elements again.
*/
void MultiElementInfoWidget::elementInfoChanged()
{
	if (m_applying || m_refresh_pending) {
		return;
	}
		//An undo changes every element one after the other:
		//read them once, when it is done.
	m_refresh_pending = true;
	QTimer::singleShot(0, this, [this]() {
		m_refresh_pending = false;
		updateUi();
	});
}

MultiElementInfoWidget::Row *MultiElementInfoWidget::rowForKey(const QString &key) const
{
	for (Row *row : m_rows) {
		if (row->key == key) {
			return row;
		}
	}
	return nullptr;
}

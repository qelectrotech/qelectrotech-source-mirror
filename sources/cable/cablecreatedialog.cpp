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
#include "cablecreatedialog.h"

#include "editcablecommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetproject.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QTabWidget>
#include <QVBoxLayout>

#include <utility>

/**
	@brief CableFilterProxy::CableFilterProxy
	@param parent
*/
CableFilterProxy::CableFilterProxy(QObject *parent) :
	QSortFilterProxyModel(parent)
{
}

/**
	@brief CableFilterProxy::setTokens
	@param tokens every word the user typed, all of which have to be found
*/
void CableFilterProxy::setTokens(const QStringList &tokens)
{
	if (m_tokens == tokens) return;
	m_tokens = tokens;
	beginFilterChange();
	endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

/**
	@brief CableFilterProxy::setHideUnsuitable
	@param hide true to keep the rows which are of no use out of sight,
	false to show them -- struck through while a line was just drawn,
	plain while a type is being changed, where the loss was explained
	instead of being refused
*/
void CableFilterProxy::setHideUnsuitable(bool hide)
{
	if (m_hide_unsuitable == hide) return;
	m_hide_unsuitable = hide;
	beginFilterChange();
	endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

/**
	@brief CableFilterProxy::filterAcceptsRow
	@param source_row
	@param source_parent
	@return
*/
bool CableFilterProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	const QModelIndex first = sourceModel()->index(source_row, 0, source_parent);

	if (m_hide_unsuitable && first.data(UnsuitableRole).toBool()) {
		return false;
	}
	if (m_tokens.isEmpty()) {
		return true;
	}

	const int columns = sourceModel()->columnCount();
	for (const QString &token : m_tokens)
	{
		bool found = false;
		for (int column = 0; column < columns; ++column)
		{
			const QString value = sourceModel()->index(source_row, column, source_parent)
									  .data(Qt::DisplayRole).toString();
			if (value.contains(token, Qt::CaseInsensitive))
			{
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}

/**
	@brief CableCreateDialog::CableCreateDialog
	@param diagram the folio the line was drawn on
	@param crossings the conductors that line runs across, in drawing order
	@param part the section which will carry the trunk line and its slashes
	@param parent
*/
CableCreateDialog::CableCreateDialog(Diagram *diagram,
									 const QList<CableCrossing> &crossings,
									 const CablePartData &part,
									 QWidget *parent) :
	QDialog(parent),
	m_diagram(diagram),
	m_crossings(crossings),
	m_part(part)
{
	build();
}

/**
	@brief CableCreateDialog::CableCreateDialog
	@param diagram the folio whose undo stack takes the change
	@param cable the cable whose type is being changed
	@param parent
*/
CableCreateDialog::CableCreateDialog(Diagram *diagram,
									 Cable *cable,
									 QWidget *parent) :
	QDialog(parent),
	m_diagram(diagram),
	m_cable(cable),
	m_change(cable)
{
	build();
}

/**
	@brief CableCreateDialog::build
	Everything the two modes share: the header saying what this window
	is about, the tabs, the property fields and the buttons. What the
	header says and which tabs are built is where they part company --
	a window which settles a line just drawn knows how many conductors
	that line crossed, one which changes a cable knows how many cores
	that cable has.
*/
void CableCreateDialog::build()
{
	setWindowTitle(tr("Cable"));

	auto *layout = new QVBoxLayout(this);

	if (changeMode())
	{
			//What this window is for, and how many cores the cable has
			//right now: the type he picks is measured against that number.
		const QString name = m_change->type().isEmpty()
			? tr("(no type)") : m_change->type();
		m_header = new QLabel(tr("Change the type: %1, %n core(s)",
								 "which type this window changes, and how many cores that cable has now",
								 m_change->coreCount()).arg(name), this);
	}
	else
	{
		m_header = new QLabel(tr("%n recognized core(s)",
								 "how many cores the line just crossed",
								 m_crossings.size()), this);
	}
	QFont header_font = m_header->font();
	header_font.setBold(true);
	m_header->setFont(header_font);
	layout->addWidget(m_header);

	m_tabs = new QTabWidget(this);
	layout->addWidget(m_tabs);

	buildNewTab();
		//A window changing an existing cable has no line just drawn, so
		//there is no other cable to hand that line over to: the second
		//tab would only offer cables this one is to be replaced by,
		//which is not what changing a type means.
	if (!changeMode()) {
		buildExistingTab();
	}
	buildPropertyRow();
	layout->addWidget(m_property_row);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &CableCreateDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &CableCreateDialog::reject);
	layout->addWidget(m_buttons);

	connect(m_tabs, &QTabWidget::currentChanged, this, [this]() {selectionChanged();});

	loadTypeFile();
	refreshExisting();
	selectionChanged();

	resize(780, 540);
}

/**
	@brief CableCreateDialog::cell
	One read only cell of one of the two tables. A row which cannot be
	used is drawn struck through and greyed out, so the checkbox which
	keeps such rows visible never leaves the user to guess why they are
	there.
	@param value
	@param unsuitable
	@return
*/
QStandardItem *CableCreateDialog::cell(const QString &value, bool unsuitable) const
{
	auto *item = new QStandardItem(value);
	item->setEditable(false);
	if (unsuitable)
	{
		QFont font = item->font();
		font.setStrikeOut(true);
		item->setFont(font);
		item->setForeground(QBrush(palette().color(QPalette::Disabled, QPalette::Text)));
	}
	return item;
}

/**
	@brief CableCreateDialog::buildNewTab
	The tab choosing the type of a brand new cable, read from the cable
	type file.
*/
void CableCreateDialog::buildNewTab()
{
	auto *page = new QWidget(m_tabs);
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(4, 4, 4, 4);

	auto *search_row = new QHBoxLayout;
	search_row->addWidget(new QLabel(tr("Search:"), page));
	m_new_search = new QLineEdit(page);
	m_new_search->setClearButtonEnabled(true);
	m_new_search->setPlaceholderText(tr("Filter cable types"));
	search_row->addWidget(m_new_search, 1);
	layout->addLayout(search_row);

		//What the checkbox hides is what cannot be taken: too few cores
		//to carry the line which was just drawn, and -- when a type is
		//being changed rather than drawn -- too few cores to keep this
		//cable as it stands, since taking such a type takes cores off it.
	m_new_hide = new QCheckBox(changeMode()
							   ? tr("Hide types which have fewer cores "
									"than this cable")
							   : tr("Hide types which have fewer cores "
									"than recognized conductors"), page);
	m_new_hide->setChecked(true);
	layout->addWidget(m_new_hide);

		//Said up front rather than only when a struck through row is
		//picked: the user sees the lines crossed out the moment the
		//checkbox shows them, and asks why straight away.
	auto *why = new QLabel(changeMode()
						   ? tr("Uncheck to also see the types which have "
								"fewer cores: taking them makes the cores "
								"beyond their number leave the cable.")
						   : tr("A struck through line is a type which has "
								"fewer cores than recognized conductors: it "
								"cannot carry this cable."), page);
	why->setWordWrap(true);
	layout->addWidget(why);

	m_new_model = new QStandardItemModel(this);
	m_new_proxy = new CableFilterProxy(this);
	m_new_proxy->setSourceModel(m_new_model);

	m_new_table = new QTableView(page);
	m_new_table->setModel(m_new_proxy);
	m_new_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_new_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_new_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_new_table->setAlternatingRowColors(true);
	m_new_table->setSortingEnabled(true);
	m_new_table->horizontalHeader()->setStretchLastSection(true);
	m_new_table->verticalHeader()->setVisible(false);
	layout->addWidget(m_new_table, 1);

	m_new_note = new QLabel(page);
	m_new_note->setWordWrap(true);
	m_new_note->setVisible(false);
	layout->addWidget(m_new_note);

		//A new line for the cable type file, filled in the way the
		//material window fills in one of its own articles: he adds the
		//type right here rather than leaving the window to find out how.
		//Beside it, the button rewriting the line he picked -- which
		//stays out of reach until a line really is picked, since there
		//would then be nothing to rewrite.
	auto *add_row = new QHBoxLayout;
	add_row->addStretch(1);
	auto *add_button = new QPushButton(tr("New entry"), page);
	add_button->setToolTip(tr("Add a new cable type to the list"));
	connect(add_button, &QPushButton::clicked,
			this, &CableCreateDialog::addTypeRecord);
	add_row->addWidget(add_button);

	m_edit_button = new QPushButton(tr("Edit entry"), page);
	m_edit_button->setToolTip(tr("Edit the cable type selected in "
								 "the list."));
	m_edit_button->setEnabled(false);
	connect(m_edit_button, &QPushButton::clicked,
			this, &CableCreateDialog::editTypeRecord);
	add_row->addWidget(m_edit_button);

	layout->addLayout(add_row);

	m_tabs->addTab(page, tr("New cable"));

	connect(m_new_search, &QLineEdit::textChanged, this, [this](const QString &text) {
		m_new_proxy->setTokens(text.split(QLatin1Char(' '), Qt::SkipEmptyParts));
	});
	connect(m_new_hide, &QCheckBox::toggled, m_new_proxy, &CableFilterProxy::setHideUnsuitable);
	connect(m_new_table->selectionModel(), &QItemSelectionModel::currentChanged,
			this, [this]() {selectionChanged();});
		//A double click settles the choice, exactly the way clicking
		//OK would: he has come to pick that cable, and making him press
		//OK after every pick is one click too many. A row which cannot
		//carry this cable goes on being refused the same way it is
		//refused by that button, greyed out as it is.
	connect(m_new_table, &QTableView::doubleClicked, this, [this]() {
		if (m_buttons->button(QDialogButtonBox::Ok)->isEnabled()) {
			accept();
		}
	});
}

/**
	@brief CableCreateDialog::buildExistingTab
	The tab giving more cores to a cable the project already holds. That
	cable keeps its type, its designation, its installation and its
	location: choosing it is what inherits them.
*/
void CableCreateDialog::buildExistingTab()
{
	auto *page = new QWidget(m_tabs);
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(4, 4, 4, 4);

	auto *search_row = new QHBoxLayout;
	search_row->addWidget(new QLabel(tr("Search:"), page));
	m_existing_search = new QLineEdit(page);
	m_existing_search->setClearButtonEnabled(true);
	m_existing_search->setPlaceholderText(tr("Filter existing cables"));
	search_row->addWidget(m_existing_search, 1);
	layout->addLayout(search_row);

	m_existing_hide = new QCheckBox(tr("Hide cables which have fewer "
									   "free cores than recognized conductors"), page);
	m_existing_hide->setChecked(true);
	layout->addWidget(m_existing_hide);

		//Same as on the first tab: the reason a line is crossed out is
		//said where the crossed out lines appear, not only afterwards.
	auto *why = new QLabel(tr("A struck through line is a cable which has "
							  "fewer free cores than recognized "
							  "conductors: it cannot receive them."), page);
	why->setWordWrap(true);
	layout->addWidget(why);

	m_existing_model = new QStandardItemModel(this);
	m_existing_proxy = new CableFilterProxy(this);
	m_existing_proxy->setSourceModel(m_existing_model);

	m_existing_table = new QTableView(page);
	m_existing_table->setModel(m_existing_proxy);
	m_existing_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_existing_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_existing_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_existing_table->setAlternatingRowColors(true);
	m_existing_table->setSortingEnabled(true);
	m_existing_table->horizontalHeader()->setStretchLastSection(true);
	m_existing_table->verticalHeader()->setVisible(false);
	layout->addWidget(m_existing_table, 1);

	m_existing_note = new QLabel(page);
	m_existing_note->setWordWrap(true);
	m_existing_note->setVisible(false);
	layout->addWidget(m_existing_note);

	m_tabs->addTab(page, tr("Existing cables"));

	connect(m_existing_search, &QLineEdit::textChanged, this, [this](const QString &text) {
		m_existing_proxy->setTokens(text.split(QLatin1Char(' '), Qt::SkipEmptyParts));
	});
	connect(m_existing_hide, &QCheckBox::toggled, m_existing_proxy, &CableFilterProxy::setHideUnsuitable);
	connect(m_existing_table->selectionModel(), &QItemSelectionModel::currentChanged,
			this, [this]() {selectionChanged();});
		//A double click settles the choice here too: the cable goes
		//straight onto the line he has just drawn, without a stop at
		//the OK button on the way -- and a cable with no room left for
		//these cores goes on being refused, since that button is greyed
		//out for it.
	connect(m_existing_table, &QTableView::doubleClicked, this, [this]() {
		if (m_buttons->button(QDialogButtonBox::Ok)->isEnabled()) {
			accept();
		}
	});
}

/**
	@brief CableCreateDialog::buildPropertyRow
	The properties of the cable itself, which belong to it rather than to
	one of its parts: whatever is typed here applies to the whole cable.

	There is one single field for the number of the cable, not one per
	name it goes by: the Désignation is the tag of the cable -- the thing
	one is tempted to call its BMK -- and it is what gets drawn next to
	the line and written into the conductors.
*/
void CableCreateDialog::buildPropertyRow()
{
	m_property_row = new QWidget(this);
	auto *form = new QFormLayout(m_property_row);
	form->setContentsMargins(0, 0, 0, 0);

	m_designation = new QLineEdit(m_property_row);
	m_installation = new QLineEdit(m_property_row);
	m_location = new QLineEdit(m_property_row);

	form->addRow(tr("Designation"), m_designation);
	form->addRow(tr("Plant (=)"), m_installation);
	form->addRow(tr("Location (+)"), m_location);
}

/**
	@brief CableCreateDialog::loadTypeFile
	Read the cable type file the same way the material file is read, and
	fill the first tab with it. A file which is not there yet simply
	leaves the tab empty and says so.
*/
void CableCreateDialog::loadTypeFile()
{
	m_new_model->clear();

	QString path = CableTypeList::configuredPath();
	if (path.isEmpty()) {
		path = CableTypeList::defaultPath();
	}
	m_types = CableTypeListData();
	CableTypeList::load(path, &m_types);

	m_new_model->setHorizontalHeaderLabels(CableTypeList::translatedHeader(m_types.columns));

	const int needed = neededCores();
	int current_row = -1;
	for (const CableTypeRecord &record : std::as_const(m_types.records))
	{
		const int cores = CableTypeList::coreCount(record);
			//A type saying how many cores it has, but fewer than the line
			//crosses, cannot carry the cable: too few cores were chosen.
			//When a type is being changed, such a row is not refused --
			//he may well want a cable with fewer cores -- but it is the
			//one the checkbox hides, and the one which has something to
			//say about what it takes off the cable.
		const bool fewer = cores > 0 && cores < needed;
		const bool unsuitable = fewer && !changeMode();
		if (changeMode() && !m_change->type().isEmpty()
			&& CableTypeList::designation(record) == m_change->type()) {
			current_row = m_types.records.indexOf(record);
		}

		QList<QStandardItem *> items;
		items.reserve(m_types.columns.size());
		for (const QString &column : std::as_const(m_types.columns))
		{
			items.append(cell(record.value(column), unsuitable));
		}
		if (!items.isEmpty())
		{
			items.first()->setData(fewer, CableFilterProxy::UnsuitableRole);
			m_new_model->appendRow(items);
		}
	}

		//The type the cable already has is the one he came here to
		//change, so it is the one standing under his finger from the
		//start -- and OK is enabled with something to say yes to.
	if (current_row >= 0 && m_new_proxy)
	{
		const QModelIndex source = m_new_model->index(current_row, 0);
		m_new_table->setCurrentIndex(m_new_proxy->mapFromSource(source));
		m_new_table->scrollTo(m_new_table->currentIndex());
	}
}

/**
	@brief CableCreateDialog::refreshExisting
	Fill the second tab with the cables the project already holds,
	saying how many of their cores are wired and how many are left.
*/
void CableCreateDialog::refreshExisting()
{
		//No second tab when a type is being changed, so nothing to fill
		//-- and nothing was built which could be filled either.
	if (changeMode()) return;

	m_existing_model->clear();
	m_existing.clear();

	if (!m_diagram || !m_diagram->project()) return;

	m_existing_model->setHorizontalHeaderLabels({
		tr("Designation"),
		tr("Type"),
		tr("Cores"),
		tr("Plant"),
		tr("Location")
	});

	const int needed = m_crossings.size();
	for (Cable *cable : m_diagram->project()->cables())
	{
		if (!cable) continue;

		const bool unsuitable = cable->freeCoreCount() < needed;
		QList<QStandardItem *> items;
		items << cell(cable->designation(), unsuitable)
			  << cell(cable->type(), unsuitable)
			  << cell(tr("%1 / %2")
						  .arg(cable->usedCoreCount())
						  .arg(cable->coreCount()), unsuitable)
			  << cell(cable->installation(), unsuitable)
			  << cell(cable->location(), unsuitable);
		items.first()->setData(unsuitable, CableFilterProxy::UnsuitableRole);

		m_existing_model->appendRow(items);
		m_existing.append(cable);
	}
}

/**
	@brief CableCreateDialog::currentSourceIndex
	@return the row currently chosen on whichever tab is in front, in the
	coordinates of the model behind the filter
*/
QModelIndex CableCreateDialog::currentSourceIndex() const
{
	const bool on_new_tab = m_tabs->currentIndex() == 0;
	QTableView *table = on_new_tab ? m_new_table : m_existing_table;
	CableFilterProxy *proxy = on_new_tab ? m_new_proxy : m_existing_proxy;

	const QModelIndex proxy_index = table->currentIndex();
	if (!proxy_index.isValid()) return QModelIndex();
	return proxy->mapToSource(proxy_index);
}

/**
	@brief CableCreateDialog::currentSelectionIsUnsuitable
	@return true when the chosen row is one the filter only shows when
	the checkbox is off, that is one which cannot carry this cable
*/
bool CableCreateDialog::currentSelectionIsUnsuitable() const
{
	const QModelIndex index = currentSourceIndex();
	if (!index.isValid()) return false;

	const bool on_new_tab = m_tabs->currentIndex() == 0;
	CableFilterProxy *proxy = on_new_tab ? m_new_proxy : m_existing_proxy;
	return proxy->index(index.row(), 0).data(CableFilterProxy::UnsuitableRole).toBool();
}

/**
	@brief CableCreateDialog::unsuitableNote
	@return what has to be said about the struck through row which was
	chosen, so the user learns why it cannot be taken
*/
QString CableCreateDialog::unsuitableNote() const
{
	const int needed = neededCores();
	const QModelIndex index = currentSourceIndex();
	if (!index.isValid()) return QString();

	if (m_tabs->currentIndex() == 0)
	{
		const int cores = CableTypeList::coreCount(m_types.records.at(index.row()));
			//Changing a type may well take a cable with fewer cores:
			//nothing is refused, but what that costs has to be said out
			//loud before he says yes to it.
		if (changeMode())
		{
			return tr("This type has fewer cores than this cable: %1 instead of %2. "
					  "The cores beyond %1 leave the cable and their marks "
					  "disappear from the drawing.")
				.arg(cores)
				.arg(needed);
		}

		return tr("Too many cores selected: the type offers only %1, "
				  "%2 conductors are recognized")
			.arg(cores)
			.arg(needed);
	}

	const Cable *cable = m_existing.value(index.row());
	return tr("Only %1 free cores, %2 requested")
		.arg(cable ? cable->freeCoreCount() : 0)
		.arg(needed);
}

/**
	@brief CableCreateDialog::selectionChanged
	Show the reason a struck through row cannot be taken, keep the OK
	button in step with it, and fill the property fields from whatever is
	chosen -- an existing cable hands its properties over, a new one gets
	its number worked out.
*/
void CableCreateDialog::selectionChanged()
{
	for (QLabel *note : {m_new_note, m_existing_note})
	{
		if (!note) continue;
		note->clear();
		note->hide();
	}

	const bool on_new_tab = m_tabs->currentIndex() == 0;
	QLabel *note = on_new_tab ? m_new_note : m_existing_note;
	const QModelIndex index = currentSourceIndex();

	bool acceptable = index.isValid();

	if (on_new_tab && m_types.columns.isEmpty())
	{
		note->setText(tr("No cable type available: configure the cable "
						 "type file in the preferences."));
		note->show();
		acceptable = false;
	}
	else if (acceptable && currentSelectionIsUnsuitable())
	{
		note->setText(unsuitableNote());
		note->show();
			//Changing a type may well take one with fewer cores: what
			//that costs is said out loud here, and he may still take it.
		acceptable = !changeMode();
	}
	else if (!acceptable)
	{
		note->setText(on_new_tab ? tr("Choose a cable type.")
								 : tr("Choose an existing cable."));
		note->show();
	}

		//A type with more cores than this cable holds now loses nothing:
		//the extra ones wait in the list of cores which are not placed
		//yet, and he brings them in one by one.
	if (acceptable && on_new_tab && changeMode() && index.isValid()
		&& index.row() < m_types.records.size())
	{
		const int cores = CableTypeList::coreCount(m_types.records.at(index.row()));
		if (cores > m_change->coreCount())
		{
			note->setText(tr("%1 cores instead of %2: the extra cores "
							 "stay unplaced.")
						  .arg(cores)
						  .arg(m_change->coreCount()));
			note->show();
		}
	}

	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(acceptable);

		//The button rewriting a line of the file works on the line he
		//picked and on nothing else: greyed out while his finger has
		//chosen no line yet, and greyed out on the tab holding the
		//cables of the project, which are not lines of that file.
	if (m_edit_button)
	{
		m_edit_button->setEnabled(on_new_tab && index.isValid()
								  && index.row() < m_types.records.size());
	}

	if (!acceptable) return;

	if (on_new_tab)
	{
		if (changeMode())
		{
				//The cable already has its number, its installation and
				//its place: those are what it holds, not something this
				//window works out again.
			fillDesignation(m_change->designation(),
							m_change->designationByHand());
			m_installation->setText(m_change->installation());
			m_location->setText(m_change->location());
			return;
		}

		fillDesignation(m_diagram
			? CableManager::nextDesignation(m_diagram->project(), m_diagram.data())
			: QString(),
			false);
		m_installation->clear();
		m_location->clear();
		return;
	}

	if (const Cable *cable = m_existing.value(index.row()))
	{
		fillDesignation(cable->designation(), cable->designationByHand());
		m_installation->setText(cable->installation());
		m_location->setText(cable->location());
	}
}

/**
	@brief CableCreateDialog::fillDesignation
	Put a name into the field and keep hold of what this window put
	there: whatever he types over it is a name he chose rather than one
	the program handed out.
	@param text the name to show
	@param by_hand whether that name was typed in by him
*/
void CableCreateDialog::fillDesignation(const QString &text, bool by_hand)
{
	m_designation->setText(text);
	m_designation_prefill = text;
	m_designation_prefill_by_hand = by_hand;
}

/**
	@brief CableCreateDialog::nextFreeSlots
	@param count how many cores are needed
	@return the lowest core numbers no conductor shows yet
*/
QList<int> CableCreateDialog::nextFreeSlots(int count) const
{
	QList<int> core_slots;
	if (!m_cable) return core_slots;

	const int total = m_cable->coreCount();
	for (int core = 0; core < total && core_slots.size() < count; ++core)
	{
		if (!m_cable->hasCore(core)) {
			core_slots.append(core);
		}
	}
	return core_slots;
}

/**
	@brief CableCreateDialog::assignCrossings
	Give the recognised conductors to the cable, one after the other
	along the line, so each core takes the colour the type lists at its
	place. A conductor another cable was holding is taken away from it:
	a piece of wire belongs to one cable only.
	@param slots the core numbers being handed out, in drawing order
*/
void CableCreateDialog::assignCrossings(const QList<int> &core_slots)
{
	if (!m_cable || !m_diagram || !m_diagram->project()) return;
	QETProject *project = m_diagram->project();

	int position = 0;
	for (const CableCrossing &crossing : std::as_const(m_crossings))
	{
		if (position >= core_slots.size()) break;

		Conductor *conductor = crossing.conductor;
		if (!conductor) continue;

			//Already one of this cable's cores: it keeps the core it has,
			//drawing the same cable twice must not give it two.
		if (conductor->properties().m_cable_uuid == m_cable->uuid())
		{
			++position;
			continue;
		}

		CableManager::claimConductor(project, conductor, m_cable);

			//A cable binds a conductor by its identity, so a conductor
			//read back from a file written before it had a persistent one
			//is given one to keep from now on.
		conductor->setUuid(conductor->uuid());

		CableCore core;
		core.core = core_slots.at(position);
		core.part = m_part.uuid;
		core.conductor = conductor->uuid();
		core.position = crossing.position;
		m_cable->setCore(core);
		++position;
	}
}

/**
	@brief CableCreateDialog::coresTakenFromOthers
	Which cores of other cables the wires this line crosses already
	belong to.

	A conductor is one piece of wire, so handing it to the cable being
	drawn takes that core away from whichever cable held it before. The
	list is worked out before anything is changed, both to be able to
	ask the user first and to be able to give those cores back when he
	undoes the line.
	@param target the cable about to claim those conductors; a null one
	is a cable which does not exist yet, and then every core another
	cable holds is one which would be taken away
	@return one entry per core which is about to change hands
*/
QList<CableCoreTaken> CableCreateDialog::coresTakenFromOthers(const QUuid &target) const
{
	QList<CableCoreTaken> taken;
	if (!m_diagram || !m_diagram->project()) return taken;

	QETProject *project = m_diagram->project();
	for (const CableCrossing &crossing : std::as_const(m_crossings))
	{
		Conductor *conductor = crossing.conductor;
		if (!conductor) continue;

		const auto properties = conductor->properties();
		if (properties.m_cable_uuid.isNull()
			|| properties.m_cable_uuid == target) {
			continue;
		}

		Cable *previous = CableManager::cableByUuid(project, properties.m_cable_uuid);
		if (!previous || !previous->hasCore(properties.m_cable_slot)) continue;

		CableCoreTaken item;
		item.cable = previous;
		item.core = previous->core(properties.m_cable_slot);
		taken.append(item);
	}
	return taken;
}

/**
	@brief CableCreateDialog::accept
	Settle the cable the drawing gets: a new one from the chosen type, or
	the chosen existing one, filled with the recognised conductors.

	Nothing is written before the user has been asked about the cores
	other cables are about to lose: saying yes to that question has to be
	the last thing he does before the change really happens, so that
	saying no leaves the project exactly as it was.
*/
void CableCreateDialog::accept()
{
		//Changing the type of a cable which already exists has nothing
		//to do with conductors a line crosses: it takes the chosen type
		//onto that cable, and that alone.
	if (changeMode())
	{
		acceptTypeChange();
		return;
	}

	if (!m_diagram || !m_diagram->project()) {
		QDialog::reject();
		return;
	}
	QETProject *project = m_diagram->project();
	const int needed = m_crossings.size();
	QList<int> core_slots;

	const QModelIndex index = currentSourceIndex();
	if (!index.isValid() || currentSelectionIsUnsuitable()) return;

	Cable *chosen = nullptr;
	if (m_tabs->currentIndex() == 0)
	{
		if (index.row() >= m_types.records.size()) return;
		const CableTypeRecord record = m_types.records.at(index.row());
		const int cores = CableTypeList::coreCount(record);
		if (cores > 0 && cores < needed) return;
	}
	else
	{
		chosen = m_existing.value(index.row());
		if (!chosen) return;
		if (chosen->freeCoreCount() < needed) return;
	}

		//The wires this line crosses which another cable already holds
		//are about to change hands, and the drawing already shows that
		//as the label of the first cable vanishing: better to say so
		//here than to let the user discover it afterwards.
	m_taken = coresTakenFromOthers(chosen ? chosen->uuid() : QUuid());
	if (!m_taken.isEmpty())
	{
		const QString text = tr(
			"%n crossed conductor(s) already bound to another cable\n\n"
			"If you confirm, the other cable will lose these cores. Do you want to continue?",
			"ask before taking cores away from another cable",
			m_taken.size());

		if (QMessageBox::warning(this, tr("Cable"), text,
								 QMessageBox::Yes | QMessageBox::No,
								 QMessageBox::No) != QMessageBox::Yes) {
			m_taken.clear();
			return;
		}
	}

	if (m_tabs->currentIndex() == 0)
	{
		const CableTypeRecord record = m_types.records.at(index.row());
		const int cores = CableTypeList::coreCount(record);

		m_cable = project->newCable();
		m_cable->setType(CableTypeList::designation(record));
			//A type saying how many cores it has keeps its own count, so
			//"5 of 8" stays true; a type saying nothing gets the number of
			//conductors which were recognised, since that is all we know.
		m_cable->setCoreCount(qMax(cores, needed));
		m_cable->setCoreColors(CableTypeList::coreColors(record));
		m_cable->setDesignation(CableManager::nextDesignation(project, m_diagram.data()));
		m_cable->addPart(m_part);
		core_slots = nextFreeSlots(needed);
	}
	else
	{
		m_cable = chosen;
		if (!m_cable->hasPart(m_part.uuid)) {
			m_cable->addPart(m_part);
		}
		core_slots = nextFreeSlots(needed);
	}

	m_cable->setDesignation(m_designation->text().trimmed());
		//A name this window filled in is the program's, a name he typed
		//over it is his own -- and it stays his own when he puts the
		//very same name back into the field
	m_cable->setDesignationByHand(
		m_designation->text().trimmed() != m_designation_prefill
			|| m_designation_prefill_by_hand);
	m_cable->setInstallation(m_installation->text().trimmed());
	m_cable->setLocation(m_location->text().trimmed());

	assignCrossings(core_slots);

	QDialog::accept();
}

/**
	@brief CableCreateDialog::neededCores
	@return how many cores whatever he picks has to hold at least: the
	conductors the line just crossed, or -- when a type is being changed
	-- the cores this cable holds right now
*/
int CableCreateDialog::neededCores() const
{
	return changeMode() ? m_change->coreCount() : m_crossings.size();
}

/**
	@brief CableCreateDialog::typeFilePath
	@return where the cable type file stands, read the same way the type
	tab reads it
*/
QString CableCreateDialog::typeFilePath() const
{
	QString path = CableTypeList::configuredPath();
	if (path.isEmpty()) {
		path = CableTypeList::defaultPath();
	}
	return path;
}

/**
	@brief CableCreateDialog::addTypeRecord
	Fill in a new line of the cable type file, the way the material
	window adds one of its own articles: the form is appended to the
	file, the search is cleared so nothing hides what was just written,
	and the file is read again so the new line comes back from where it
	really stands -- and it is the one under his finger.
*/
void CableCreateDialog::addTypeRecord()
{
	QStringList columns = m_types.columns;
	if (columns.isEmpty()) {
		columns = CableTypeList::defaultColumns();
	}

	CableTypeEntryDialog entry(columns, this);
	if (entry.exec() != QDialog::Accepted) {
		return;
	}

	const QString path = typeFilePath();
	QString error;
	if (!CableTypeList::appendRecord(path, entry.record(), &error))
	{
		QMessageBox::critical(this,
							  tr("Cannot be written"),
							  tr("Cannot write to the file:\n%1\n%2")
								  .arg(path, error));
		return;
	}

		//A search hiding the line he just filled in would leave him
		//with a form he completed and no line to see.
	m_new_search->clear();
	loadTypeFile();

	const CableTypeRecord written = entry.record();
	for (int row = 0; row < m_types.records.size(); ++row)
	{
		if (!(m_types.records.at(row) == written)) continue;

		const QModelIndex source = m_new_model->index(row, 0);
		m_new_table->setCurrentIndex(m_new_proxy->mapFromSource(source));
		m_new_table->scrollTo(m_new_table->currentIndex());
		break;
	}

	selectionChanged();
}

/**
	@brief CableCreateDialog::editTypeRecord
	Rewrite the line of the cable type file he picked: the very window
	which adds a line opens, already filled in with what that line says,
	and only his yes writes it back -- saying no leaves the file exactly
	as it was.

	What he writes replaces the line he started from and nothing else.
	The other lines of the file are read again first, so whatever he
	saved from his spreadsheet in the meantime is written back too
	rather than being thrown away.
*/
void CableCreateDialog::editTypeRecord()
{
	const QModelIndex index = currentSourceIndex();
	if (!index.isValid() || index.row() >= m_types.records.size()) return;

	const CableTypeRecord before = m_types.records.at(index.row());

	QStringList columns = m_types.columns;
	if (columns.isEmpty()) {
		columns = CableTypeList::defaultColumns();
	}

	CableTypeEntryDialog entry(columns, this);
	entry.setValues(before);
	if (entry.exec() != QDialog::Accepted) {
		return;
	}

	CableTypeRecord after = entry.record();
		//Cells standing beyond the header belong to the user, and the
		//form knows nothing about them: they must not vanish from the
		//line just because they could not be shown.
	after.extra = before.extra;

	const QString path = typeFilePath();
	QString error;
	if (!CableTypeList::updateRecord(path, before, after, &error))
	{
		QMessageBox::critical(this,
							  tr("Cannot be written"),
							  tr("Cannot write to the file:\n%1\n%2")
								  .arg(path, error));
		return;
	}

		//A search hiding the line he just wrote would leave him with a
		//form he filled in and no line to see.
	m_new_search->clear();
	loadTypeFile();

	for (int row = 0; row < m_types.records.size(); ++row)
	{
		if (!(m_types.records.at(row) == after)) continue;

		const QModelIndex source = m_new_model->index(row, 0);
		m_new_table->setCurrentIndex(m_new_proxy->mapFromSource(source));
		m_new_table->scrollTo(m_new_table->currentIndex());
		break;
	}

	selectionChanged();
}

/**
	@brief CableCreateDialog::acceptTypeChange
	Take the chosen type onto the cable whose type is being changed:
	its name, how many cores it has and the colour of each of them,
	together with the cores that cable holds -- the ones beyond the new
	count leave it, the ones which stay keep their place and their wire,
	and the added ones wait in the list of cores not placed yet.

	Everything lands as one single undo step, together with whatever he
	typed into the property fields: the type, the cores and the fields
	all come back together or not at all.
*/
void CableCreateDialog::acceptTypeChange()
{
	if (!m_change) {
		QDialog::reject();
		return;
	}

	const QModelIndex index = currentSourceIndex();
	if (!index.isValid() || index.row() >= m_types.records.size()) return;

	const CableTypeRecord record = m_types.records.at(index.row());

	const QString type = CableTypeList::designation(record);
	int count = CableTypeList::coreCount(record);
		//A type saying how many cores it has takes that count; one
		//saying nothing leaves this cable the size it already is.
	if (count <= 0) count = m_change->coreCount();
	const QStringList colors = CableTypeList::coreColors(record);

	const CableTypeState before = m_change->typeState();
	CableTypeState after = before;
	after.type = type;
	after.core_count = count;
	after.core_colors = colors;
	after.cores.clear();
	for (const CableCore &core : std::as_const(before.cores))
	{
		if (core.core < count) {
			after.cores.append(core);
		}
	}

		//Cores beyond the new count take their mark off the drawing.
		//He is only asked about that when a wire really loses its
		//label: a core which stood nowhere was not on the drawing at
		//all, and nothing he can see goes away with it.
	int wired = 0;
	for (const CableCore &core : std::as_const(before.cores))
	{
		if (core.core >= count && !core.conductor.isNull()) {
			++wired;
		}
	}
	if (wired > 0)
	{
		const QString text = tr(
			"%n core(s) of this cable bound to a conductor. "
			"With the chosen type, they leave this cable and their mark "
			"disappears from the drawing. Continue?",
			"ask before cores lose the mark they show on the drawing",
			wired);

		if (QMessageBox::warning(this, tr("Cable"), text,
								 QMessageBox::Yes | QMessageBox::No,
								 QMessageBox::Yes) != QMessageBox::Yes) {
			return;
		}
	}

		//The type itself is written by the type step rather than by the
		//fields, so the two cannot write it twice and undo cannot leave
		//half of it standing.
	const CableProperties properties_before = m_change->properties();
	CableProperties properties_after = properties_before;
	properties_after.designation = m_designation->text().trimmed();
		//A name this window filled in is the one the cable already had;
		//anything he typed over it is a name of his own
	properties_after.designation_by_hand =
		properties_after.designation != m_designation_prefill
			|| m_designation_prefill_by_hand;
	properties_after.installation = m_installation->text().trimmed();
	properties_after.location = m_location->text().trimmed();

	QList<QUndoCommand *> steps;
	if (properties_after != properties_before) {
		steps.append(new ChangeCablePropertiesCommand(m_change,
													  properties_before,
													  properties_after));
	}
	if (before != after) {
		steps.append(new ChangeCableTypeCommand(m_change, before, after));
	}

		//Nothing changed at all: saying yes to an unchanged form is
		//still saying yes, but it is not a step which has to be kept.
	if (steps.isEmpty())
	{
		QDialog::accept();
		return;
	}

	QUndoCommand *step = steps.size() == 1
		? steps.takeFirst()
		: new BatchCommand(steps, QCoreApplication::translate("ChangeCableTypeCommand",
															  "Change the cable type"));

		//The folio is the usual place this step is taken back from. When
		//no folio is known -- a cable picked without a line -- the
		//project keeps them all anyway, and only when there is neither
		//does the change stand without a way back.
	QUndoStack *stack = m_diagram ? &m_diagram->undoStack() : nullptr;
	if (!stack)
	{
		auto *project = qobject_cast<QETProject *>(m_change->parent());
		if (project && project->undoStack()) {
			stack = project->undoStack();
		}
	}

	if (stack) {
		stack->push(step);
	} else {
		if (properties_after != properties_before) {
			m_change->setProperties(properties_after);
		}
		m_change->applyTypeState(after);
		delete step;
	}

	QDialog::accept();
}

/**
	@brief CableTypeEntryDialog::CableTypeEntryDialog
	Build one field per column of the cable type file. Two columns get
	the shape they need -- a number which counts, and colours written
	the way the file separates them -- and every other column is a plain
	field, since it may be one the user invented rather than one
	QElectroTech knows.
	@param columns the columns of the file, in file order
	@param parent
*/
CableTypeEntryDialog::CableTypeEntryDialog(const QStringList &columns, QWidget *parent) :
	QDialog(parent),
	m_columns(columns)
{
	setWindowTitle(tr("New entry"));

	auto *main_layout = new QVBoxLayout(this);

	auto *intro = new QLabel(tr("Enter the cable type to add. Fields "
								"left empty stay empty in the file."), this);
	intro->setWordWrap(true);
	m_intro = intro;
	main_layout->addWidget(intro);

	auto *grid = new QGridLayout;
	for (int i = 0; i < m_columns.size(); ++i)
	{
		const QString column = m_columns.at(i);

		auto *label = new QLabel(CableTypeList::translatedColumn(column), this);
		label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

		QWidget *field = nullptr;
		if (column == QLatin1String("cores"))
		{
			auto *spin = new QSpinBox(this);
			spin->setRange(0, 99);
			spin->setSpecialValueText(QStringLiteral("—"));
			spin->setToolTip(tr("How many cores this type has. The dash means "
								"the file does not specify."));
			field = spin;
		}
		else
		{
			auto *edit = new QLineEdit(this);
			edit->setClearButtonEnabled(true);
			if (column == QLatin1String("core_colors"))
			{
				edit->setPlaceholderText(QStringLiteral("br,sw,gr,bl,gnge"));
				edit->setToolTip(tr("The color of each core, separated by a "
									"comma and in the order of the cores."));
			}
			field = edit;
		}

		label->setBuddy(field);
		grid->addWidget(label, i, 0, Qt::AlignRight | Qt::AlignVCenter);
		grid->addWidget(field, i, 1);
		m_fields.append(field);
	}
	grid->setColumnStretch(1, 1);
	main_layout->addLayout(grid);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Ok)->setText(tr("Save"));
	buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	connect(buttons, &QDialogButtonBox::accepted, this, &CableTypeEntryDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &CableTypeEntryDialog::reject);
	main_layout->addWidget(buttons);

	if (!m_fields.isEmpty()) {
		m_fields.first()->setFocus();
	}

	setMinimumWidth(420);
}

/**
	@brief CableTypeEntryDialog::record
	@return the line the form describes. Empty fields are left out: an
	empty column never means anything for a type being created.
*/
CableTypeRecord CableTypeEntryDialog::record() const
{
	CableTypeRecord record;

	for (int i = 0; i < m_columns.size() && i < m_fields.size(); ++i)
	{
		const QString column = m_columns.at(i);

		QString value;
		if (auto *spin = qobject_cast<QSpinBox *>(m_fields.at(i)))
		{
				//Zero means "not said": a type may leave its size to the
				//drawing, and an empty cell says exactly that.
			if (spin->value() > 0) {
				value = QString::number(spin->value());
			}
		}
		else if (auto *edit = qobject_cast<QLineEdit *>(m_fields.at(i)))
		{
			value = edit->text();
			value.remove(QLatin1Char('\r'));
			value.remove(QLatin1Char('\n'));
			value = value.trimmed();
		}

		if (!value.isEmpty()) {
			record.setValue(column, value);
		}
	}

	return record;
}

/**
	@brief CableTypeEntryDialog::setValues
	Fill the form with a line which is already in the file. The window
	then says it edits rather than adds, so the two can never be told
	apart by mistake: what he writes is only written when he says yes to
	it, and only about the line he started from.
	@param values the line as it stands in the file
*/
void CableTypeEntryDialog::setValues(const CableTypeRecord &values)
{
	setWindowTitle(tr("Edit entry"));
	if (m_intro)
	{
		m_intro->setText(tr("Edit the cable type selected in the list. "
							"Fields left empty stay empty in the file."));
	}

	for (int i = 0; i < m_columns.size() && i < m_fields.size(); ++i)
	{
		const QString value = values.value(m_columns.at(i));

		if (auto *spin = qobject_cast<QSpinBox *>(m_fields.at(i)))
		{
			bool ok = false;
			const int count = value.toInt(&ok);
			spin->setValue(ok ? qBound(spin->minimum(), count, spin->maximum())
							  : spin->minimum());
		}
		else if (auto *edit = qobject_cast<QLineEdit *>(m_fields.at(i)))
		{
			edit->setText(value);
		}
	}
}

/**
	@brief CableTypeEntryDialog::accept
	Refuse an empty form: a type with nothing said about it is not worth
	a line in the file.
*/
void CableTypeEntryDialog::accept()
{
	if (record().values.isEmpty())
	{
		QMessageBox::warning(this,
							 tr("No details"),
							 tr("Enter at least one detail "
								"to create an entry."));
		return;
	}

	QDialog::accept();
}

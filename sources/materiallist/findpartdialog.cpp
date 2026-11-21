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
#include "findpartdialog.h"

#include "../ElementsCollection/elementslocation.h"
#include "../devicetype/devicetype.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"
#include "partcategory.h"
#include "partsearch.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QHash>
#include <QPair>
#include <QCollator>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace
{
	const QChar Star = QChar(0x2605);
	const QChar Times = QChar(0x00D7);
	const char *const FacetColumns[] = {
		nullptr, "manufacturer", "device_type", "voltage_rating", "current_rating", "supplier"
	};

		/// The chips of @p filter: a key to remove it by, and its text
	QList<QPair<QString, QString>> chipsOf(const FindPartDialog::Filter &filter)
	{
		QList<QPair<QString, QString>> chips;
		if (!filter.text.trimmed().isEmpty())
			chips << qMakePair(QStringLiteral("text"),
							   QObject::tr("Search: %1").arg(filter.text.trimmed()));
		if (filter.suits)
			chips << qMakePair(QStringLiteral("suits"), QObject::tr("Suits this symbol"));
		const QStringList names = {
			QObject::tr("Category"), QObject::tr("Manufacturer"), QObject::tr("Device type"),
			QObject::tr("Voltage"), QObject::tr("Current"), QObject::tr("Supplier")};
		for (int f = 0 ; f < FindPartDialog::FacetCount ; ++f) {
			if (!filter.isActive(FindPartDialog::Facet(f))) continue;
			chips << qMakePair(QString::number(f),
							   //: %1 is a filter ("Category"), %2 its value ("Relay")
							   QObject::tr("%1: %2").arg(names.at(f),
								   FindPartDialog::facetText(FindPartDialog::Facet(f), filter.values[f])));
		}
		return chips;
	}

	QString ratingOf(const MaterialRecord &record)
	{
		QStringList parts;
		const QString voltage = record.value(QStringLiteral("voltage_rating")).trimmed();
		const QString current = record.value(QStringLiteral("current_rating")).trimmed();
		if (!voltage.isEmpty()) parts << voltage;
		if (!current.isEmpty()) parts << current;
		return parts.join(QStringLiteral(" / "));
	}
}

int FindPartDialog::Filter::activeCount() const
{
	return chipsOf(*this).size();
}

/**
	@param data : the material list
	@param symbol : the symbol being placed, to tell which parts suit it; may be null
	@param label : its label, shown in the title; may be empty
	@param usedParts : the part numbers already used in the project
	@param parent : parent widget
*/
FindPartDialog::FindPartDialog(const MaterialListData &data, Element *symbol, const QString &label,
							   const QStringList &usedParts, QWidget *parent) :
	QDialog(parent),
	m_data(data),
	m_symbol(symbol),
	m_used(usedParts)
{
	setWindowTitle(label.trimmed().isEmpty() ? tr("Find part")
											 : tr("Find part for %1").arg(label.trimmed()));
	resize(820, 520);
	auto *layout = new QVBoxLayout(this);

	m_search = new QLineEdit(this);
	m_search->setObjectName(QStringLiteral("findPartSearch"));
	m_search->setClearButtonEnabled(true);
	m_search->setPlaceholderText(tr("Type words: part number, description, manufacturer..."));
	m_search->installEventFilter(this);
	layout->addWidget(m_search);

	const QStringList titles = {tr("Category"), tr("Manufacturer"), tr("Device type"),
								tr("Voltage"), tr("Current"), tr("Supplier")};
	const QStringList names = {QStringLiteral("findPartCategory"), QStringLiteral("findPartManufacturer"),
							   QStringLiteral("findPartDeviceType"), QStringLiteral("findPartVoltage"),
							   QStringLiteral("findPartCurrent"), QStringLiteral("findPartSupplier")};
	auto *filters = new QGridLayout();
	for (int f = 0 ; f < FacetCount ; ++f) {
		m_combos[f] = new QComboBox(this);
		m_combos[f]->setObjectName(names.at(f));
		m_combos[f]->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		m_combos[f]->setMinimumContentsLength(12);
		auto *title = new QLabel(titles.at(f), this);
		title->setBuddy(m_combos[f]);
		filters->addWidget(title, (f / 3) * 2, f % 3);
		filters->addWidget(m_combos[f], (f / 3) * 2 + 1, f % 3);
		connect(m_combos[f], QOverload<int>::of(&QComboBox::activated), this, [this]() {
			readWidgets();
			update();
		});
	}
	layout->addLayout(filters);

	m_suitable = suitingRows(m_data, m_symbol);
	m_any_suits = !m_suitable.isEmpty();
	m_suits = new QCheckBox(tr("Suits this symbol"), this);
	m_suits->setObjectName(QStringLiteral("findPartSuits"));
	m_suits->setVisible(m_symbol != nullptr);
	m_suits->setEnabled(m_any_suits);
	m_suits->setChecked(m_any_suits);
	if (m_symbol && !m_any_suits)
		m_suits->setToolTip(tr("No device type lists this symbol: every part is offered"));
	auto *reset = new QPushButton(tr("Reset filters"), this);
	reset->setObjectName(QStringLiteral("findPartReset"));
	reset->setAutoDefault(false);
	auto *all = new QPushButton(tr("Show all parts"), this);
	all->setObjectName(QStringLiteral("findPartShowAll"));
	all->setAutoDefault(false);
	auto *options = new QHBoxLayout();
	options->addWidget(m_suits);
	options->addStretch(1);
	options->addWidget(reset);
	options->addWidget(all);
	layout->addLayout(options);

	m_chips = new QWidget(this);
	m_chips->setObjectName(QStringLiteral("findPartChips"));
	auto *chips_layout = new QHBoxLayout(m_chips);
	chips_layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_chips);

	m_count = new QLabel(this);
	m_count->setObjectName(QStringLiteral("findPartCount"));
	layout->addWidget(m_count);

	m_pages = new QStackedWidget(this);
	m_list = new QTreeWidget(m_pages);
	m_list->setObjectName(QStringLiteral("findPartList"));
	m_list->setRootIsDecorated(false);
	m_list->setAlternatingRowColors(true);
	m_list->setSelectionMode(QAbstractItemView::SingleSelection);
	m_list->setHeaderLabels({tr("Part number"), tr("Description"), tr("Manufacturer"),
							 tr("Category"), tr("Rating")});
	m_list->header()->setStretchLastSection(false);
	m_list->header()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_list->setColumnWidth(0, 170);
	m_list->setColumnWidth(2, 130);
	m_list->setColumnWidth(3, 110);
		//Sorted on a click of a header; the order of the search until then
	m_list->setSortingEnabled(true);
	m_list->header()->setSortIndicator(-1, Qt::AscendingOrder);
	m_pages->addWidget(m_list);

	m_empty = new QWidget(m_pages);
	m_empty->setObjectName(QStringLiteral("findPartEmpty"));
	auto *empty_layout = new QVBoxLayout(m_empty);
	empty_layout->addStretch(1);
	auto *empty_text = new QLabel(tr("No parts match."), m_empty);
	empty_text->setAlignment(Qt::AlignCenter);
	QFont big = empty_text->font();
	big.setPointSizeF(big.pointSizeF() * 1.3);
	empty_text->setFont(big);
	empty_layout->addWidget(empty_text);
	m_empty_filters = new QLabel(m_empty);
	m_empty_filters->setObjectName(QStringLiteral("findPartEmptyFilters"));
	m_empty_filters->setAlignment(Qt::AlignCenter);
	m_empty_filters->setWordWrap(true);
	empty_layout->addWidget(m_empty_filters);
	auto *clear = new QPushButton(tr("Clear filters"), m_empty);
	clear->setObjectName(QStringLiteral("findPartClearFilters"));
	clear->setAutoDefault(false);
	auto *search_all = new QPushButton(tr("Search all parts"), m_empty);
	search_all->setObjectName(QStringLiteral("findPartSearchAll"));
	search_all->setAutoDefault(false);
	auto *empty_buttons = new QHBoxLayout();
	empty_buttons->addStretch(1);
	empty_buttons->addWidget(clear);
	empty_buttons->addWidget(search_all);
	empty_buttons->addStretch(1);
	empty_layout->addLayout(empty_buttons);
	empty_layout->addStretch(1);
	m_pages->addWidget(m_empty);
	layout->addWidget(m_pages, 1);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(m_buttons);

	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_search, &QLineEdit::textChanged, this, [this]() {
		readWidgets();
		update();
	});
	connect(m_suits, &QCheckBox::toggled, this, [this]() {
		readWidgets();
		update();
	});
	connect(reset, &QPushButton::clicked, this, &FindPartDialog::resetFilters);
	connect(all, &QPushButton::clicked, this, &FindPartDialog::showAllParts);
	connect(clear, &QPushButton::clicked, this, &FindPartDialog::clearFacets);
	connect(search_all, &QPushButton::clicked, this, &FindPartDialog::searchAllParts);
	connect(m_list, &QTreeWidget::currentItemChanged, this, &FindPartDialog::selectionChanged);
	connect(m_list, &QTreeWidget::itemDoubleClicked, this, [this]() {
		if (m_list->currentItem()) accept();
	});

	readWidgets();
	update();
	m_search->setFocus();
}

QString FindPartDialog::starred(const QString &number)
{
	return QString(Star) + QLatin1Char(' ') + number;
}

/**
	The up and down keys of the search line move in the list
*/
bool FindPartDialog::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == m_search && event->type() == QEvent::KeyPress && m_list) {
		const int key = static_cast<QKeyEvent *>(event)->key();
		if (key == Qt::Key_Down || key == Qt::Key_Up || key == Qt::Key_PageDown || key == Qt::Key_PageUp) {
			QCoreApplication::sendEvent(m_list, event);
			return true;
		}
	}
	return QDialog::eventFilter(watched, event);
}

MaterialRecord FindPartDialog::selectedRecord() const
{
	const QTreeWidgetItem *item = m_list->currentItem();
	if (!item || m_pages->currentWidget() != m_list) return MaterialRecord();
	return m_data.records.value(item->data(0, Qt::UserRole).toInt());
}

QString FindPartDialog::partNumber() const
{
	return PartSearch::partNumber(selectedRecord());
}

void FindPartDialog::setSearchText(const QString &text)
{
	m_search->setText(text);
}

QString FindPartDialog::facetValue(const MaterialRecord &record, Facet facet)
{
	if (facet == Category) return PartCategory::categoryOf(record);
	return record.value(QString::fromLatin1(FacetColumns[facet])).trimmed();
}

QString FindPartDialog::facetText(Facet facet, const QString &value)
{
	return facet == Category ? PartCategory::translated(value) : value;
}

namespace
{
	bool keeps(const FindPartDialog::Filter &filter, int row, const QString *values, int skip)
	{
		if (filter.suits && !filter.suitable.contains(row)) return false;
		for (int f = 0 ; f < FindPartDialog::FacetCount ; ++f)
			if (f != skip && filter.isActive(FindPartDialog::Facet(f)) && values[f] != filter.values[f])
				return false;
		return true;
	}
}

QList<int> FindPartDialog::matches(const MaterialListData &data, const Filter &filter, const QStringList &used)
{
	QList<int> rows;
	const QList<int> ranked = PartSearch::rank(data.records, filter.text, -1, used);
	for (int row : ranked) {
		QString values[FacetCount];
		const MaterialRecord &record = data.records.at(row);
		for (int f = 0 ; f < FacetCount ; ++f)
			if (filter.isActive(Facet(f))) values[f] = facetValue(record, Facet(f));
		if (keeps(filter, row, values, -1)) rows << row;
	}
	return rows;
}

QList<QMap<QString, int>> FindPartDialog::facetCounts(const MaterialListData &data, const Filter &filter)
{
	QList<QMap<QString, int>> counts;
	for (int f = 0 ; f < FacetCount ; ++f) counts << QMap<QString, int>();
	const QList<int> ranked = PartSearch::rank(data.records, filter.text);
	for (int row : ranked) {
		QString values[FacetCount];
		const MaterialRecord &record = data.records.at(row);
		for (int f = 0 ; f < FacetCount ; ++f) values[f] = facetValue(record, Facet(f));
		for (int f = 0 ; f < FacetCount ; ++f)
			if (!values[f].isEmpty() && keeps(filter, row, values, f)) counts[f][values[f]]++;
	}
	return counts;
}

/**
	The element a part's device type draws is the symbol's: the same
	location, or a file of the same name
*/
QSet<int> FindPartDialog::suitingRows(const MaterialListData &data, Element *symbol)
{
	QSet<int> rows;
	if (!symbol) return rows;
	const QString location = symbol->location().toString();
	if (location.isEmpty()) return rows;
	const QString file = QFileInfo(location).fileName();
	QETProject *project = symbol->diagram() ? symbol->diagram()->project() : nullptr;

	QHash<QString, bool> suits;     // device type name -> draws the symbol
	for (int row = 0 ; row < data.records.size() ; ++row) {
		const QString name = data.records.at(row).value(QStringLiteral("device_type")).trimmed();
		if (name.isEmpty()) continue;
		if (!suits.contains(name)) {
			DeviceType type = project ? project->deviceType(name) : DeviceType();
			if (type.isNull()) type = DeviceTypeLibrary::load(name);
			bool found = false;
			for (const DeviceType::Part &part : std::as_const(type.parts)) {
				for (const DeviceType::Symbol &drawn : part.symbols) {
					if (drawn.location == location
						|| (!file.isEmpty() && QFileInfo(drawn.location).fileName() == file)) {
						found = true;
						break;
					}
				}
				if (found) break;
			}
			suits.insert(name, found);
		}
		if (suits.value(name)) rows << row;
	}
	return rows;
}

QStringList FindPartDialog::describe(const Filter &filter)
{
	QStringList texts;
	for (const auto &chip : chipsOf(filter)) texts << chip.second;
	return texts;
}

void FindPartDialog::readWidgets()
{
	m_filter.text = m_search->text();
	for (int f = 0 ; f < FacetCount ; ++f)
		m_filter.values[f] = m_combos[f]->currentData().toString();
	m_filter.suits = m_suits->isEnabled() && m_suits->isChecked();
	m_filter.suitable = m_suitable;
}

/**
	Shows what @a m_filter gives: the combos with what still gives a
	result, the chips, the list or the empty state
*/
void FindPartDialog::update()
{
	if (m_updating) return;
	m_updating = true;
	m_results = matches(m_data, m_filter, m_used);
	const QList<QMap<QString, int>> counts = facetCounts(m_data, m_filter);
	for (int f = 0 ; f < FacetCount ; ++f) fillCombo(Facet(f), counts.at(f));
	fillChips();
	fillList();
	m_count->setText(tr("%1 of %2 parts").arg(m_results.size()).arg(m_data.records.size()));
	m_empty_filters->setVisible(m_filter.activeCount() > 0);
	m_empty_filters->setText(tr("Filters on: %1").arg(describe(m_filter).join(QStringLiteral(", "))));
	m_pages->setCurrentWidget(m_results.isEmpty() ? m_empty : static_cast<QWidget *>(m_list));
	m_updating = false;
	selectionChanged();
}

void FindPartDialog::fillCombo(Facet facet, const QMap<QString, int> &counts)
{
	QComboBox *combo = m_combos[facet];
	const QString selected = m_filter.values[facet];
	QMap<QString, int> shown = counts;
	if (!selected.isEmpty() && !shown.contains(selected)) shown.insert(selected, 0);

	QStringList values = shown.keys();
	QCollator collator;
	collator.setNumericMode(true);
	collator.setCaseSensitivity(Qt::CaseInsensitive);
	std::sort(values.begin(), values.end(), [&](const QString &a, const QString &b) {
		return collator.compare(facetText(facet, a), facetText(facet, b)) < 0;
	});

	const QSignalBlocker blocker(combo);
	combo->clear();
	combo->addItem(tr("All"), QString());
	for (const QString &value : std::as_const(values))
		combo->addItem(QStringLiteral("%1 (%2)").arg(facetText(facet, value)).arg(shown.value(value)), value);
	const int index = combo->findData(selected);
	combo->setCurrentIndex(index < 0 ? 0 : index);
}

void FindPartDialog::fillChips()
{
		//A chip may be the sender of the click which brings us here
	const QList<QToolButton *> old = m_chips->findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly);
	for (QToolButton *button : old) {
		button->hide();
		button->setParent(nullptr);
		button->deleteLater();
	}
	auto *layout = static_cast<QHBoxLayout *>(m_chips->layout());
	while (layout->count()) delete layout->takeAt(0);
	for (const auto &chip : chipsOf(m_filter)) {
		auto *button = new QToolButton(m_chips);
		button->setObjectName(QStringLiteral("findPartChip"));
		button->setAutoRaise(true);
			//"&&": a button would take a single "&" for a shortcut key
		button->setText(QString(chip.second).replace(QLatin1Char('&'), QStringLiteral("&&"))
						+ QLatin1Char(' ') + QString(Times));
		button->setToolTip(tr("Remove this filter"));
		button->setProperty("chip", chip.first);
		connect(button, &QToolButton::clicked, this, [this, key = chip.first]() {clearChip(key);});
		layout->addWidget(button);
	}
	layout->addStretch(1);
	m_chips->setVisible(m_filter.activeCount() > 0);
}

void FindPartDialog::fillList()
{
	m_list->setUpdatesEnabled(false);
	m_list->clear();
	for (int row : std::as_const(m_results)) {
		const MaterialRecord &record = m_data.records.at(row);
		const QString number = PartSearch::partNumber(record);
		const bool used = m_used.contains(number, Qt::CaseInsensitive);
		auto *item = new QTreeWidgetItem(m_list);
		item->setText(0, used ? starred(number) : number);
		item->setText(1, record.value(QStringLiteral("description")).trimmed());
		item->setText(2, record.value(QStringLiteral("manufacturer")).trimmed());
		item->setText(3, PartCategory::translated(PartCategory::categoryOf(record)));
		item->setText(4, ratingOf(record));
		item->setData(0, Qt::UserRole, row);
		if (used) item->setToolTip(0, tr("Already used in this project"));
	}
	if (m_list->topLevelItemCount()) m_list->setCurrentItem(m_list->topLevelItem(0));
	m_list->setUpdatesEnabled(true);
}

void FindPartDialog::selectionChanged()
{
	if (auto *ok = m_buttons->button(QDialogButtonBox::Ok))
		ok->setEnabled(m_pages->currentWidget() == m_list && m_list->currentItem());
}

void FindPartDialog::clearChip(const QString &chip)
{
	m_updating = true;
	if (chip == QLatin1String("text")) m_search->clear();
	else if (chip == QLatin1String("suits")) m_suits->setChecked(false);
	else m_combos[chip.toInt()]->setCurrentIndex(0);
	m_updating = false;
	readWidgets();
	update();
}

void FindPartDialog::clearFacets()
{
	m_updating = true;
	for (QComboBox *combo : m_combos) combo->setCurrentIndex(0);
	m_updating = false;
	readWidgets();
	update();
}

/**
	The filters off, the search text kept
*/
void FindPartDialog::resetFilters()
{
	m_updating = true;
	for (QComboBox *combo : m_combos) combo->setCurrentIndex(0);
	m_suits->setChecked(false);
	m_updating = false;
	readWidgets();
	update();
}

/**
	The filters and the search text off
*/
void FindPartDialog::showAllParts()
{
	m_updating = true;
	m_search->clear();
	m_updating = false;
	resetFilters();
}

/**
	From the empty state: the same parts without the filters, the text kept
*/
void FindPartDialog::searchAllParts()
{
	resetFilters();
}

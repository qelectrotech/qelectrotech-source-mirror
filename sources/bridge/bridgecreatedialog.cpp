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
#include "bridgecreatedialog.h"

#include "../qetproject.h"
#include "bridge.h"
#include "../TerminalStrip/terminalstripbridge.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPixmap>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <kcolorcombo.h>

namespace {
		///Item data of the bridge list: the bridge itself, and whether
		///it cannot be chosen at all
	constexpr int bridge_role = Qt::UserRole;
	constexpr int unsuitable_role = Qt::UserRole + 1;
}

/**
	@brief BridgeCreateDialog::BridgeCreateDialog
	@param diagram the folio the bridge is drawn on
	@param crossings the terminals the line has just crossed
	@param parent
*/
BridgeCreateDialog::BridgeCreateDialog(Diagram *diagram,
									   const QList<BridgeCrossing> &crossings,
									   QWidget *parent) :
	QDialog(parent),
	m_diagram(diagram),
	m_crossings(crossings)
{
	int klemmen = 0;
	int references = 0;
	for (const BridgeCrossing &crossing : m_crossings) {
		if (crossing.reference) {
			++references;
		} else {
			++klemmen;
		}
	}
	setWindowTitle(tr("The bridge over %n Klemme(n)", "window title", klemmen));

	auto *layout = new QVBoxLayout(this);
	QString counted = tr("%n Klemme(n) recognized", "how many terminals the line crossed", klemmen);
	if (references) {
		counted += tr(", %n sheet reference(s) on the way to the next sheet",
					  "the way to the next sheet", references);
	}
	auto *header = new QLabel(counted, this);
	layout->addWidget(header);

	m_tabs = new QTabWidget(this);
	m_tabs->setObjectName(QStringLiteral("bridge_tabs"));
	layout->addWidget(m_tabs);

		//The page for a brand new bridge: what it may hold and which
		//colour it is drawn in. Nothing else is typed in: a bridge takes
		//its function and its potential from the first terminal it lands
		//on and keeps the form it is drawn with.
	auto *new_page = new QWidget(m_tabs);
	auto *new_layout = new QVBoxLayout(new_page);

	auto *max_label = new QLabel(tr("Terminals this bridge may hold:"), new_page);
	new_layout->addWidget(max_label);
	m_max_sb = new QSpinBox(new_page);
	m_max_sb->setObjectName(QStringLiteral("bridge_max_sb"));
	m_max_sb->setRange(Bridge::unlimited_connections, 99);
	m_max_sb->setSpecialValueText(tr("Unlimited"));
	m_max_sb->setValue(Bridge::unlimited_connections);
	new_layout->addWidget(m_max_sb);

	auto *width_label = new QLabel(tr("Line width:"), new_page);
	new_layout->addWidget(width_label);
	m_width_sb = new QDoubleSpinBox(new_page);
	m_width_sb->setObjectName(QStringLiteral("bridge_width_sb"));
		//The same numbers as the size of a conductor: the bridge is
		//drawn as thick as the wires it sits across
	m_width_sb->setRange(0.4, 20.0);
	m_width_sb->setSingleStep(0.2);
	m_width_sb->setValue(Bridge::default_line_width);
	new_layout->addWidget(m_width_sb);

	auto *name_label = new QLabel(tr("Name:"), new_page);
	new_layout->addWidget(name_label);
	m_name_ed = new QLineEdit(new_page);
	m_name_ed->setObjectName(QStringLiteral("bridge_name_ed"));
	m_name_ed->setToolTip(tr("A name to tell this bridge from the next one, "
							 "like 24V or B1. Empty when nobody names it."));
	new_layout->addWidget(m_name_ed);

	auto *color_label = new QLabel(tr("Bridge colour:"), new_page);
	new_layout->addWidget(color_label);
	m_color_cb = new KColorCombo(new_page);
	m_color_cb->setObjectName(QStringLiteral("bridge_color_cb"));
	m_color_cb->setColors(TerminalStripBridge::bridgeColor().toList());
	m_color_cb->setColor(QColor(Qt::black));
	new_layout->addWidget(m_color_cb);
	new_layout->addStretch();

	m_tabs->addTab(new_page, tr("New bridge"));

		//The page for a bridge already there: what cannot take these
		//terminals is struck through and cannot be chosen at all
	auto *existing_page = new QWidget(m_tabs);
	auto *existing_layout = new QVBoxLayout(existing_page);

	m_hide_full = new QCheckBox(tr("Hide bridges which are full"), existing_page);
	m_hide_full->setObjectName(QStringLiteral("bridge_hide_full_cb"));
	m_hide_full->setChecked(true);
	existing_layout->addWidget(m_hide_full);

	m_hide_small = new QCheckBox(tr("Hide bridges with fewer free places than terminals recognized"), existing_page);
	m_hide_small->setObjectName(QStringLiteral("bridge_hide_small_cb"));
	m_hide_small->setChecked(true);
	existing_layout->addWidget(m_hide_small);

	m_list = new QListWidget(existing_page);
	m_list->setObjectName(QStringLiteral("bridge_list"));
	existing_layout->addWidget(m_list);

	m_note = new QLabel(existing_page);
	m_note->setObjectName(QStringLiteral("bridge_note"));
	m_note->setWordWrap(true);
	existing_layout->addWidget(m_note);

	m_tabs->addTab(existing_page, tr("Bridges already there"));

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, this, &BridgeCreateDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &BridgeCreateDialog::reject);

	connect(m_hide_full, &QCheckBox::toggled, this, &BridgeCreateDialog::rebuildList);
	connect(m_hide_small, &QCheckBox::toggled, this, &BridgeCreateDialog::rebuildList);
	connect(m_list, &QListWidget::itemSelectionChanged, this, &BridgeCreateDialog::selectionChanged);
		//Taking the bridge he means is one double click away, the way a
		//cable is chosen: the row which cannot take anything answers
		//nothing at all
	connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
		if (item && (item->flags() & Qt::ItemIsSelectable)) {
			accept();
		}
	});

	rebuildList();
}

/**
	@brief BridgeCreateDialog::BridgeCreateDialog
	The window of a bridge already drawn, opened by double clicking one
	of its lines: only its own properties, colour and how many terminals
	it may hold. Nothing here lands on anything.
	@param bridge
	@param parent
*/
BridgeCreateDialog::BridgeCreateDialog(Bridge *bridge, QWidget *parent) :
	QDialog(parent),
	m_editing(bridge)
{
	setWindowTitle(tr("The bridge"));

	auto *layout = new QVBoxLayout(this);
	QString counted = tr("%n Klemme(n) on this bridge", "how many terminals the bridge lands on",
						 bridge ? bridge->terminalCount() : 0);
	if (bridge && !bridge->references().isEmpty()) {
		counted += tr(", %n sheet reference(s) on the way to the next sheet",
					  "the way to the next sheet", bridge->references().size());
	}
	layout->addWidget(new QLabel(counted, this));

	layout->addWidget(new QLabel(tr("Terminals this bridge may hold:"), this));
	m_max_sb = new QSpinBox(this);
	m_max_sb->setObjectName(QStringLiteral("bridge_max_sb"));
	m_max_sb->setRange(Bridge::unlimited_connections, 99);
	m_max_sb->setSpecialValueText(tr("Unlimited"));
	m_max_sb->setValue(bridge ? bridge->maxConnections() : Bridge::unlimited_connections);
	layout->addWidget(m_max_sb);

	layout->addWidget(new QLabel(tr("Line width:"), this));
	m_width_sb = new QDoubleSpinBox(this);
	m_width_sb->setObjectName(QStringLiteral("bridge_width_sb"));
		//The same numbers as the size of a conductor: the bridge is
		//drawn as thick as the wires it sits across
	m_width_sb->setRange(0.4, 20.0);
	m_width_sb->setSingleStep(0.2);
	m_width_sb->setValue(bridge ? bridge->lineWidth() : Bridge::default_line_width);
	layout->addWidget(m_width_sb);

	layout->addWidget(new QLabel(tr("Name:"), this));
	m_name_ed = new QLineEdit(this);
	m_name_ed->setObjectName(QStringLiteral("bridge_name_ed"));
	m_name_ed->setToolTip(tr("A name to tell this bridge from the next one, "
							 "like 24V or B1. Empty when nobody names it."));
	if (bridge) {
		m_name_ed->setText(bridge->name());
	}
	layout->addWidget(m_name_ed);

	layout->addWidget(new QLabel(tr("Bridge colour:"), this));
	m_color_cb = new KColorCombo(this);
	m_color_cb->setObjectName(QStringLiteral("bridge_color_cb"));
	m_color_cb->setColors(TerminalStripBridge::bridgeColor().toList());
	m_color_cb->setColor(bridge ? bridge->color() : QColor(Qt::black));
	layout->addWidget(m_color_cb);

		//What the bridge carries: the voltage/protocol of the wires it
		//sits on, and only that. Taken over from them, never typed in.
	if (bridge) {
		auto *information = new QLabel(
					tr("Voltage: %1", "what the bridge carries as information")
					.arg(bridge->tensionText().isEmpty() ? tr("—") : bridge->tensionText()),
					this);
		information->setObjectName(QStringLiteral("bridge_information"));
		layout->addWidget(information);
	}

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, this, &BridgeCreateDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &BridgeCreateDialog::reject);
}

/**
	@brief BridgeCreateDialog::askForInformation
	@param sets the distinct sets of information the terminals carry
	@param ok set to false when he cancelled
	@param parent
	@return the set the bridge takes

	A bridge which carries nothing takes what is there without being
	asked -- only a real disagreement between what the candidates carry
	is worth the question.
*/
BridgeInformation BridgeCreateDialog::askForInformation(const QList<BridgeInformation> &sets,
														bool *ok,
														QWidget *parent)
{
	BridgeInformation taken;
	if (ok) {
		*ok = false;
	}

		//What is worth keeping: a candidate saying nothing at all is no
		//candidate, it only ever stands for "nothing to take"
	QList<BridgeInformation> real;
	for (const BridgeInformation &set : sets) {
		if (set.tension.isEmpty() || real.contains(set)) {
			continue;
		}
		real << set;
	}

	if (real.isEmpty()) {
		if (ok) {
			*ok = true;
		}
		return taken;
	}
	if (real.size() == 1) {
		if (ok) {
			*ok = true;
		}
		return real.first();
	}

	QStringList items;
	for (const BridgeInformation &set : real) {
		items << set.tension;
	}

	bool accepted = false;
	const QString chosen = QInputDialog::getItem(
				parent,
				tr("Different potentials"),
				tr("The terminals carry different information: choose which the bridge takes."),
				items, 0, false, &accepted);
	if (!accepted) {
		return taken;
	}
	const int at = items.indexOf(chosen);
	if (at < 0) {
		return taken;
	}
	if (ok) {
		*ok = true;
	}
	return real.at(at);
}

/**
	@return the bridge to land on, or nullptr when a new one is built
*/
Bridge *BridgeCreateDialog::bridge() const {
	return m_bridge;
}

/**
	@return the colour of the new bridge
*/
QColor BridgeCreateDialog::newColor() const {
	return m_color_cb ? m_color_cb->color() : QColor(Qt::black);
}

/**
	@return how many terminals the new bridge may hold,
	Bridge::unlimited_connections (0) when there is no limit
*/
int BridgeCreateDialog::newMaxConnections() const {
	return m_max_sb ? m_max_sb->value() : Bridge::unlimited_connections;
}

/**
	@return how thick the lines of the bridge are drawn, in the same way
	a conductor takes its size
*/
qreal BridgeCreateDialog::newLineWidth() const {
	return m_width_sb ? m_width_sb->value() : Bridge::default_line_width;
}

/**
	@return the name the hand gives the bridge, empty for none
*/
QString BridgeCreateDialog::bridgeName() const {
	return m_name_ed ? m_name_ed->text() : QString();
}

/**
	@brief BridgeCreateDialog::problem
	@return why what is chosen now cannot be taken, or an empty string
	when it can: a new bridge may not be given a limit below the
	terminals just crossed, and a bridge already there may not be chosen
	struck through (it cannot be chosen at all) nor be overfilled.
*/
QString BridgeCreateDialog::problem() const
{
	const int needed = m_crossings.size();

		//The window of a bridge already drawn: a limit below what it
		//already holds would cut off terminals which are on it
	if (m_editing) {
		if (newMaxConnections() != Bridge::unlimited_connections
			&& newMaxConnections() < m_editing->terminalCount()) {
			return tr("This bridge holds %1 terminal(s): it may not be given a limit "
					  "below that.")
					.arg(m_editing->terminalCount());
		}
		return {};
	}

	if (m_tabs->currentIndex() == 0) {
		if (newMaxConnections() != Bridge::unlimited_connections
			&& newMaxConnections() < needed) {
			return tr("A bridge holding %1 terminal(s) at most cannot take the %2 "
					  "just recognized. Give it a higher limit, no limit at all, or "
					  "draw another bridge.")
					.arg(newMaxConnections()).arg(needed);
		}
		return {};
	}

	QListWidgetItem *item = m_list->currentItem();
	if (!item || item->data(unsuitable_role).toBool()) {
		return tr("Choose a bridge which can take the %1 terminal(s) just "
				  "recognized, or draw a new one.")
				.arg(needed);
	}
	Bridge *chosen = reinterpret_cast<Bridge *>(item->data(bridge_role).value<quintptr>());
	if (!chosen || chosen->wouldExceed(needed)) {
		return tr("Choose a bridge which can take the %1 terminal(s) just "
				  "recognized, or draw a new one.")
				.arg(needed);
	}
	return {};
}

/**
	@brief BridgeCreateDialog::accept
	Only a bridge which can really be built is taken, and what refuses
	one is said before the window goes away.
*/
void BridgeCreateDialog::accept()
{
	const QString problem_ = problem();
	if (!problem_.isEmpty()) {
		QMessageBox::warning(this, tr("That bridge cannot be built"), problem_);
		return;
	}

	if (m_editing) {
		QDialog::accept();
		return;
	}

	m_bridge = (m_tabs->currentIndex() == 0)
			? nullptr
			: reinterpret_cast<Bridge *>(m_list->currentItem()->data(bridge_role).value<quintptr>());

	QDialog::accept();
}

/**
	@brief BridgeCreateDialog::rebuildList
	The bridges of the project, with what they hold and what is left of
	them. What cannot take the terminals just crossed is struck through
	and cannot be chosen -- shown only when the boxes say so.
*/
void BridgeCreateDialog::rebuildList()
{
	m_list->clear();

	QETProject *project = m_diagram ? m_diagram->project() : nullptr;
	if (!project) {
		return;
	}

	const int needed = m_crossings.size();
	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges)
	{
		if (!bridge) {
			continue;
		}
		const bool full = isFull(bridge);
		const bool small = hasTooFewPlaces(bridge);
		if (full && m_hide_full->isChecked()) {
			continue;
		}
		if (small && !full && m_hide_small->isChecked()) {
			continue;
		}

		const QString places = (bridge->maxConnections() == Bridge::unlimited_connections)
				? tr("unlimited places")
				: tr("%1 of %2 places")
				  .arg(bridge->maxConnections() - bridge->klemmCount())
				  .arg(bridge->maxConnections());
			//Told apart the way a place is named on a drawing: which
			//sheet, which column and which row -- and what it carries
		const QString location = BridgeManager::locationOf(
					m_diagram ? m_diagram->project() : nullptr, bridge);
		QStringList lines;
		lines << (bridge->name().isEmpty() ? tr("without a name") : bridge->name())
			  + (location.isEmpty() ? QString() : QStringLiteral(" \u2013 %1").arg(location));
		lines << (bridge->tensionText().isEmpty() ? tr("no voltage") : bridge->tensionText())
			  + QStringLiteral(" \u2013 ")
			  + tr("%1 Klemmen, %2", "how much a bridge holds and what is left of it")
				.arg(bridge->klemmCount()).arg(places);
		auto *item = new QListWidgetItem(lines.join(QLatin1Char('\n')), m_list);
		item->setIcon(swatch(bridge->color()));
		item->setData(bridge_role, quintptr(bridge));

		const bool unsuitable = full || small;
		item->setData(unsuitable_role, unsuitable);
		if (unsuitable) {
			QFont font = item->font();
			font.setStrikeOut(true);
			item->setFont(font);
				//Struck through means: not choosable at all, not with a
				//warning either -- a bridge which cannot be built is not
				//drawn.
			item->setFlags(item->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
			item->setToolTip(tr("Not enough free places for the %1 terminal(s) just recognized", "", needed));
		}
	}

	selectionChanged();
}

/**
	@brief BridgeCreateDialog::selectionChanged
*/
void BridgeCreateDialog::selectionChanged()
{
	const int needed = m_crossings.size();
	QListWidgetItem *item = m_list->currentItem();
	Bridge *bridge = item
			? reinterpret_cast<Bridge *>(item->data(bridge_role).value<quintptr>())
			: nullptr;

	if (m_tabs->currentIndex() == 0) {
		m_note->setText(tr("The bridge is drawn where you pulled the line and keeps the form you give it."));
		return;
	}

	if (!bridge) {
		m_note->setText(tr("Choose a bridge to land on, or draw a new one."));
	} else if (isFull(bridge)) {
		m_note->setText(tr("This bridge is full."));
	} else if (hasTooFewPlaces(bridge)) {
		m_note->setText(tr("This bridge has %1 free place(s) left, %2 requested.")
						.arg(bridge->maxConnections() - bridge->terminalCount()).arg(needed));
	} else {
		m_note->setText(tr("The %1 terminal(s) join this bridge, drawn as another line of it.")
						.arg(needed));
	}
}

/**
	@param bridge
	@return true when the bridge has no free place at all
*/
bool BridgeCreateDialog::isFull(Bridge *bridge) const
{
	return bridge && bridge->maxConnections() != Bridge::unlimited_connections
			&& bridge->terminalCount() >= bridge->maxConnections();
}

/**
	@param bridge
	@return true when the bridge has free places, but fewer than the
	terminals just crossed
*/
bool BridgeCreateDialog::hasTooFewPlaces(Bridge *bridge) const
{
	return bridge && !isFull(bridge) && bridge->wouldExceed(m_crossings.size());
}

/**
	@param color
	@return a small square of that colour, for the list
*/
QIcon BridgeCreateDialog::swatch(const QColor &color)
{
	QPixmap pixmap(16, 16);
	pixmap.fill(color);
	return QIcon(pixmap);
}

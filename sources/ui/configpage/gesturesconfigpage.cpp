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
#include "gesturesconfigpage.h"

#include "../../gesturesettings.h"
#include "../../qeticons.h"
#include "../../shortcutmanager.h"

#include <QAction>
#include <QComboBox>
#include <QDataStream>
#include <QDragEnterEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtMath>

namespace {
const int ring_radius = 100;
const int ring_inner = 26;
const int slot_radius = 68;
const int slot_icon = 24;
	/// What QListWidget puts on a drag
const QString list_mime = QStringLiteral("application/x-qabstractitemmodeldatalist");

/**
	@return the command id carried by a drag from the command list, or an
	empty string
*/
QString droppedId(const QMimeData *mime)
{
	if (!mime || !mime->hasFormat(list_mime)) {
		return QString();
	}
	QByteArray data = mime->data(list_mime);
	QDataStream stream(&data, QIODevice::ReadOnly);
	int row, column;
	QMap<int, QVariant> roles;
	stream >> row >> column >> roles;
	return roles.value(Qt::UserRole).toString();
}
}

GestureRingEditor::GestureRingEditor(QWidget *parent) :
	QWidget(parent)
{
	setAcceptDrops(true);
	setFocusPolicy(Qt::StrongFocus);
	setMinimumSize(sizeHint());
}

QSize GestureRingEditor::sizeHint() const
{
	return QSize(2 * ring_radius + 8, 2 * ring_radius + 8);
}

void GestureRingEditor::setIds(const QStringList &ids)
{
	m_ids = ids;
	update();
}

void GestureRingEditor::setDirections(int directions)
{
	m_directions = directions == 4 ? 4 : 8;
	setCurrentSlot(qMin(m_current, m_directions - 1));
	update();
}

void GestureRingEditor::setDescriptions(const QHash<QString, QString> &descriptions)
{
	m_descriptions = descriptions;
	update();
}

void GestureRingEditor::setCurrentSlot(int slot)
{
	if (slot < 0 || slot >= m_directions) {
		return;
	}
	const bool changed = slot != m_current;
	m_current = slot;
	update();
	if (changed) {
		emit currentSlotChanged(slot);
	}
}

/**
	@return the slot in the direction of @a pos from the centre, 0 at the
	top then clockwise, or -1 at the centre or outside the ring
*/
int GestureRingEditor::slotAt(const QPoint &pos) const
{
	const QPointF d = QPointF(pos) - QPointF(rect().center());
	const qreal distance = qHypot(d.x(), d.y());
	if (distance < ring_inner || distance > ring_radius) {
		return -1;
	}
	qreal angle = qRadiansToDegrees(qAtan2(d.x(), -d.y()));
	if (angle < 0) {
		angle += 360;
	}
	return int(qRound(angle / (360.0 / m_directions))) % m_directions;
}

/**
	@brief GestureRingEditor::place
	Put @a id in the current slot. A command already elsewhere on the ring
	moves here, so a command has one direction.
*/
void GestureRingEditor::place(const QString &id)
{
	if (id.isEmpty()) {
		return;
	}
	const int old = m_ids.indexOf(id);
	if (old >= 0) {
		m_ids[old].clear();
	}
	while (m_ids.count() <= m_current) {
		m_ids << QString();
	}
	m_ids[m_current] = id;
	update();
	emit edited();
}

void GestureRingEditor::clearCurrent()
{
	if (m_current < m_ids.count() && !m_ids.at(m_current).isEmpty()) {
		m_ids[m_current].clear();
		update();
		emit edited();
	}
}

QString GestureRingEditor::idAt(int slot) const
{
	return slot < m_ids.count() ? m_ids.at(slot) : QString();
}

QPointF GestureRingEditor::slotCenter(int slot) const
{
	const qreal a = qDegreesToRadians(slot * 360.0 / m_directions);
	const QPointF c = QRectF(rect()).center();
	return QPointF(c.x() + slot_radius * qSin(a), c.y() - slot_radius * qCos(a));
}

void GestureRingEditor::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	const QPointF c = QRectF(rect()).center();
	const qreal step = 360.0 / m_directions;
	const QRectF outer(c.x() - ring_radius, c.y() - ring_radius,
			   2 * ring_radius, 2 * ring_radius);

	QColor line = palette().color(QPalette::Text);
	line.setAlpha(110);
	painter.setPen(line);
	painter.setBrush(palette().color(QPalette::Base));
	painter.drawEllipse(outer);

		//The chosen slot
	QPainterPath wedge;
	wedge.moveTo(c);
	wedge.arcTo(outer, 90 - m_current * step - step / 2, step);
	wedge.closeSubpath();
	painter.setPen(Qt::NoPen);
	QColor highlight = palette().color(QPalette::Highlight);
	if (!hasFocus()) {
		highlight.setAlpha(140);
	}
	painter.setBrush(highlight);
	painter.drawPath(wedge);

		//Lines between the slots
	painter.setPen(line);
	for (int i = 0 ; i < m_directions ; ++i) {
		const qreal a = qDegreesToRadians(i * step + step / 2);
		painter.drawLine(QPointF(c.x() + ring_inner * qSin(a), c.y() - ring_inner * qCos(a)),
				 QPointF(c.x() + ring_radius * qSin(a), c.y() - ring_radius * qCos(a)));
	}
	painter.setBrush(palette().color(QPalette::Window));
	painter.drawEllipse(c, ring_inner, ring_inner);

	for (int i = 0 ; i < m_directions ; ++i)
	{
		const QString id = idAt(i);
		if (id.isEmpty()) {
			continue;
		}
		const QPointF p = slotCenter(i);
		const QRect r(int(p.x()) - slot_icon / 2, int(p.y()) - slot_icon / 2,
			      slot_icon, slot_icon);
		QAction *action = ShortcutManager::instance().action(id, nullptr);
		if (action && !action->icon().isNull()) {
			action->icon().paint(&painter, r);
		} else {
			painter.setPen(palette().color(i == m_current ? QPalette::HighlightedText
								      : QPalette::Text));
			painter.drawText(r.adjusted(-12, 0, 12, 0), Qt::AlignCenter,
					 m_descriptions.value(id, id).left(3));
		}
	}
}

void GestureRingEditor::mousePressEvent(QMouseEvent *event)
{
	setCurrentSlot(slotAt(event->position().toPoint()));
	event->accept();
}

void GestureRingEditor::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
		clearCurrent();
	} else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Down) {
		setCurrentSlot((m_current + 1) % m_directions);
	} else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Up) {
		setCurrentSlot((m_current + m_directions - 1) % m_directions);
	} else {
		QWidget::keyPressEvent(event);
	}
}

void GestureRingEditor::dragEnterEvent(QDragEnterEvent *event)
{
	if (!droppedId(event->mimeData()).isEmpty()) {
		event->acceptProposedAction();
	}
}

void GestureRingEditor::dragMoveEvent(QDragMoveEvent *event)
{
	const int slot = slotAt(event->position().toPoint());
	if (slot < 0) {
		event->ignore();
		return;
	}
	setCurrentSlot(slot);
	event->acceptProposedAction();
}

void GestureRingEditor::dropEvent(QDropEvent *event)
{
	const int slot = slotAt(event->position().toPoint());
	const QString id = droppedId(event->mimeData());
	if (slot < 0 || id.isEmpty()) {
		event->ignore();
		return;
	}
	setCurrentSlot(slot);
	place(id);
	event->acceptProposedAction();
}

/**
	@brief GesturesConfigPage::GesturesConfigPage
	@param parent
*/
GesturesConfigPage::GesturesConfigPage(QWidget *parent) :
	ConfigPage(parent)
{
	const QStringList available = ShortcutBarSettings::availableIds();
	for (const ShortcutManager::ShortcutInfo &info :
	     ShortcutManager::instance().allShortcuts()) {
		if (available.contains(info.id)) {
			m_descriptions.insert(info.id, info.description);
		}
	}
	for (const ShortcutBarSettings::Context c : ShortcutBarSettings::contexts()) {
		m_pending.insert(c, GestureSettings::ids(c));
		m_custom.insert(c, GestureSettings::isCustom(c));
	}

	auto *explanation = new QLabel(
		tr("Drag with the right mouse button on a sheet: a ring of "
		   "commands opens, release towards the one to run. Until you "
		   "change the ring for a context, it shows the commands of the "
		   "shortcut bar. Gestures are turned on or off in the General "
		   "page."), this);
	explanation->setWordWrap(true);

	m_directions = new QComboBox(this);
	m_directions->setObjectName(QStringLiteral("directionsCombo"));
	m_directions->addItem(tr("8 directions"), 8);
	m_directions->addItem(tr("4 directions"), 4);
	m_directions->setCurrentIndex(m_directions->findData(GestureSettings::directions()));

	m_context = new QComboBox(this);
	m_context->setObjectName(QStringLiteral("contextCombo"));
	for (const ShortcutBarSettings::Context c : ShortcutBarSettings::contexts()) {
		m_context->addItem(ShortcutBarSettings::title(c), c);
	}

	m_available = new QListWidget(this);
	m_available->setObjectName(QStringLiteral("availableList"));
	m_available->setSortingEnabled(true);
	m_available->setDragEnabled(true);
	m_available->setDragDropMode(QAbstractItemView::DragOnly);
	for (auto it = m_descriptions.cbegin(); it != m_descriptions.cend(); ++it) {
		QIcon icon;
		if (QAction *action = ShortcutManager::instance().action(it.key(), nullptr)) {
			icon = action->icon();
		}
		auto *item = new QListWidgetItem(icon, it.value(), m_available);
		item->setData(Qt::UserRole, it.key());
	}

	m_ring = new GestureRingEditor(this);
	m_ring->setObjectName(QStringLiteral("ringEditor"));
	m_ring->setDescriptions(m_descriptions);
	m_ring->setDirections(GestureSettings::directions());

	m_slot_name = new QLabel(this);
	m_slot_name->setAlignment(Qt::AlignCenter);
	m_slot_name->setWordWrap(true);
	m_state = new QLabel(this);
	m_state->setWordWrap(true);

	auto *place = new QPushButton(tr("Place →"), this);
	place->setObjectName(QStringLiteral("placeButton"));
	auto *clear = new QPushButton(tr("Clear the direction"), this);
	auto *follow = new QPushButton(tr("Same as the shortcut bar"), this);
	follow->setObjectName(QStringLiteral("followBarButton"));

	auto *ring_column = new QVBoxLayout();
	ring_column->addWidget(m_ring, 0, Qt::AlignHCenter);
	ring_column->addWidget(m_slot_name);
	ring_column->addWidget(clear);
	ring_column->addStretch();

	auto *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Available commands"), this), 0, 0);
	grid->addWidget(new QLabel(tr("Ring, the chosen direction highlighted"), this), 0, 2);
	grid->addWidget(m_available, 1, 0);
	grid->addWidget(place, 1, 1, Qt::AlignVCenter);
	grid->addLayout(ring_column, 1, 2);
	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(2, 1);

	auto *top_row = new QHBoxLayout();
	top_row->addWidget(new QLabel(tr("Directions:"), this));
	top_row->addWidget(m_directions);
	top_row->addSpacing(12);
	top_row->addWidget(new QLabel(tr("Context:"), this));
	top_row->addWidget(m_context, 1);

	auto *state_row = new QHBoxLayout();
	state_row->addWidget(m_state, 1);
	state_row->addWidget(follow);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(explanation);
	layout->addLayout(top_row);
	layout->addLayout(state_row);
	layout->addLayout(grid);

	connect(m_directions, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
		m_ring->setDirections(m_directions->currentData().toInt());
		updateState();
	});
	connect(m_context, qOverload<int>(&QComboBox::currentIndexChanged),
		this, &GesturesConfigPage::showContext);
	connect(m_ring, &GestureRingEditor::edited, this, [this]() {
		m_pending.insert(m_shown, m_ring->ids());
		m_custom.insert(m_shown, true);
		updateState();
	});
	connect(m_ring, &GestureRingEditor::currentSlotChanged,
		this, &GesturesConfigPage::updateState);
	connect(place, &QPushButton::clicked, this, &GesturesConfigPage::placeSelected);
	connect(m_available, &QListWidget::itemDoubleClicked,
		this, &GesturesConfigPage::placeSelected);
	connect(clear, &QPushButton::clicked, m_ring, &GestureRingEditor::clearCurrent);
	connect(follow, &QPushButton::clicked, this, &GesturesConfigPage::followBar);

	showContext();
}

/**
	@brief GesturesConfigPage::applyConf
	Save the directions and every context's ring.
*/
void GesturesConfigPage::applyConf()
{
	GestureSettings::setDirections(m_directions->currentData().toInt());
	for (const ShortcutBarSettings::Context c : ShortcutBarSettings::contexts()) {
		if (m_custom.value(c)) {
			GestureSettings::setIds(c, m_pending.value(c));
		} else {
			GestureSettings::reset(c);
		}
	}
}

QString GesturesConfigPage::title() const
{
	return tr("Mouse gestures", "configuration page title");
}

QIcon GesturesConfigPage::icon() const
{
	return QET::Icons::ConfigureShortcuts;
}

void GesturesConfigPage::showContext()
{
	m_shown = static_cast<ShortcutBarSettings::Context>(
		m_context->currentData().toInt());
	m_ring->setIds(m_pending.value(m_shown));
	updateState();
}

void GesturesConfigPage::placeSelected()
{
	if (QListWidgetItem *item = m_available->currentItem()) {
		m_ring->place(item->data(Qt::UserRole).toString());
	}
}

/**
	@brief GesturesConfigPage::followBar
	Make the shown context's ring follow the shortcut bar again.
*/
void GesturesConfigPage::followBar()
{
	m_custom.insert(m_shown, false);
	m_pending.insert(m_shown, GestureSettings::defaultIds(m_shown));
	m_ring->setIds(m_pending.value(m_shown));
	updateState();
}

/**
	@brief GesturesConfigPage::updateState
	Name the chosen direction's command, and say whether the shown ring is
	the user's or the shortcut bar's.
*/
void GesturesConfigPage::updateState()
{
	const int slot = m_ring->currentSlot();
	const QStringList ids = m_ring->ids();
	const QString id = slot < ids.count() ? ids.at(slot) : QString();
	m_slot_name->setText(id.isEmpty() ? tr("(empty direction)")
					  : m_descriptions.value(id, id));
	m_state->setText(m_custom.value(m_shown)
			 ? tr("Custom ring for this context.")
			 : tr("This ring follows the shortcut bar."));
}

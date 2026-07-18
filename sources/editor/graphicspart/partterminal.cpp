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
#include "partterminal.h"

#include "../elementscene.h"
#include "../../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../../qetgraphicsitem/terminal.h"

/**
	@brief PartTerminal::PartTerminal
	@param editor :
	L'editeur d'element concerne
	@param parent :
	Le QGraphicsItem parent de cette borne
*/
PartTerminal::PartTerminal(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent)
{
	d = new TerminalData(this);
	d -> m_orientation = Qet::North;
	d -> m_uuid = QUuid::createUuid(); // if part is loaded this uuid will be overwritten, but being sure that terminal has a uuid
	updateSecondPoint();
	setZValue(100000);
}

/// Destructeur
PartTerminal::~PartTerminal()
{
}

/**
	Import terminal properties from an XML element
	Importe les proprietes d'une borne depuis un element XML
	@param xml_elmt Element XML a lire / XML element to read
*/
void PartTerminal::fromXml(const QDomElement &xml_elmt) {
	d -> fromXml(xml_elmt);
	setPos(d -> m_pos);
	updateSecondPoint();
}

/**
	Exporte la borne en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant la borne
*/
const QDomElement PartTerminal::toXml(QDomDocument &xml_document) const
{
	return d -> toXml(xml_document);
}

/**
	Dessine la borne
	@param painter QPainter a utiliser pour rendre le dessin
	@param options Options pour affiner le rendu
	@param widget Widget sur lequel le rendu est effectue
*/
void PartTerminal::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options,
		QWidget *widget)
{
	Q_UNUSED(widget)
	painter -> save();

	// annulation des renderhints
	painter -> setRenderHint(QPainter::Antialiasing,          false);
	painter -> setRenderHint(QPainter::TextAntialiasing,      false);
	painter -> setRenderHint(QPainter::SmoothPixmapTransform, false);

	QPen t;
	t.setWidthF(1.0);

	t.setCosmetic(options && options -> levelOfDetailFromTransform(painter->worldTransform()) < 1.0);

	// dessin de la borne en rouge
	t.setColor(isSelected() ? Terminal::neutralColor : Qt::red);
	painter -> setPen(t);
	painter -> drawLine(QPointF(0.0, 0.0), d -> m_second_point);

	// dessin du point d'amarrage au conducteur en bleu
	t.setColor(isSelected() ? Qt::red : Terminal::neutralColor);
	painter -> setPen(t);
	painter -> setBrush(Terminal::neutralColor);
	painter -> drawPoint(QPointF(0.0, 0.0));
	painter -> restore();

	if (m_hovered)
		drawShadowShape(painter);

	if (d->m_show_name && !d->m_name.isEmpty()) {
		painter->save();
		painter->setFont(d->m_label_font);
		painter->setPen(d->m_label_color);
		painter->setRenderHint(QPainter::Antialiasing, true);
		painter->setRenderHint(QPainter::TextAntialiasing, true);

		QPointF label_pos = d->m_label_pos;
		QRectF text_rect;
		QFontMetrics fm(d->m_label_font);
		QSizeF text_size = fm.size(Qt::TextSingleLine, d->m_name);

		auto compute_rect = [&]() {
			qreal dx = 0, dy = 0;
			if (d->m_label_halignment & Qt::AlignLeft) dx = 0;
			else if (d->m_label_halignment & Qt::AlignHCenter) dx = -text_size.width() / 2.0;
			else if (d->m_label_halignment & Qt::AlignRight) dx = -text_size.width();
			if (d->m_label_valignment & Qt::AlignTop) dy = 0;
			else if (d->m_label_valignment & Qt::AlignVCenter) dy = -text_size.height() / 2.0;
			else if (d->m_label_valignment & Qt::AlignBottom) dy = -text_size.height();
			return QRectF(label_pos + QPointF(dx, dy), text_size);
		};

		if (d->m_label_rotation != 0.0) {
			painter->translate(label_pos);
			painter->rotate(d->m_label_rotation);
			text_rect = QRectF(-text_size.width()/2.0, -text_size.height()/2.0,
							   text_size.width(), text_size.height());
			if (d->m_label_frame) {
				painter->drawRect(text_rect.adjusted(-1, -1, 1, 1));
			}
			painter->drawText(text_rect, static_cast<int>(d->m_label_halignment | d->m_label_valignment), d->m_name);
		} else {
			text_rect = compute_rect();
			if (d->m_label_frame) {
				painter->drawRect(text_rect.adjusted(-1, -1, 1, 1));
			}
			painter->drawText(text_rect, static_cast<int>(Qt::AlignLeft | Qt::AlignTop), d->m_name);
		}

		if (isSelected() || m_hovered) {
			QPen outline_pen(Qt::darkBlue, 0, Qt::DashLine);
			painter->setPen(outline_pen);
			painter->setBrush(Qt::NoBrush);
			if (d->m_label_rotation != 0.0) {
				painter->drawRect(text_rect.adjusted(-1, -1, 1, 1));
			} else {
				painter->drawRect(text_rect.adjusted(-1, -1, 1, 1));
			}
		}

		painter->restore();
	}
}

/**
	@brief PartTerminal::shape
	@return the shape of this item
*/
QPainterPath PartTerminal::shape() const
{
	QPainterPath shape;
	shape.lineTo(d -> m_second_point);

	QPainterPathStroker pps;
	pps.setWidth(1);

	QPainterPath path = pps.createStroke(shape);

	if (d->m_show_name && !d->m_name.isEmpty()) {
		path.addRect(labelRect());
	}

	return path;
}

/**
	@brief PartTerminal::shadowShape
	@return the hover outline shape (terminal line only, no label rect)
*/
QPainterPath PartTerminal::shadowShape() const
{
	QPainterPath shape;
	shape.lineTo(d -> m_second_point);

	QPainterPathStroker pps;
	pps.setWidth(1);
	return pps.createStroke(shape);
}

/**
	@brief PartTerminal::boundingRect
	@return the bounding rect of this item
*/
QRectF PartTerminal::boundingRect() const
{
	QRectF br(QPointF(0, 0), d -> m_second_point);
	br = br.normalized();

	qreal adjust = (SHADOWS_HEIGHT + 1) / 2;
	br.adjust(-adjust, -adjust, adjust, adjust);

	if (d->m_show_name && !d->m_name.isEmpty()) {
		br = br.united(labelRect());
	}

	return(br);
}

/**
	@brief PartTerminal::labelRect
	@return the rectangle of the label text (in item coordinates),
	or an empty rect if the label is not shown
*/
QRectF PartTerminal::labelRect() const
{
	if (!d->m_show_name || d->m_name.isEmpty())
		return QRectF();

	QFontMetrics fm(d->m_label_font);
	QSizeF text_size = fm.size(Qt::TextSingleLine, d->m_name);
	QPointF label_pos = d->m_label_pos;

	qreal dx = 0, dy = 0;
	if (d->m_label_halignment & Qt::AlignLeft) dx = 0;
	else if (d->m_label_halignment & Qt::AlignHCenter) dx = -text_size.width() / 2.0;
	else if (d->m_label_halignment & Qt::AlignRight) dx = -text_size.width();

	if (d->m_label_valignment & Qt::AlignTop) dy = 0;
	else if (d->m_label_valignment & Qt::AlignVCenter) dy = -text_size.height() / 2.0;
	else if (d->m_label_valignment & Qt::AlignBottom) dy = -text_size.height();

	return QRectF(label_pos + QPointF(dx, dy), text_size).adjusted(-3, -3, 3, 3);
}


/**
	Definit l'orientation de la borne
	@param ori la nouvelle orientation de la borne
*/
void PartTerminal::setOrientation(Qet::Orientation ori) {
	if (d -> m_orientation == ori) return;
	prepareGeometryChange();
	d -> m_orientation = ori;
	updateSecondPoint();
	emit orientationChanged(); // all terminal-signals call "updateForm"
}

/**
	Redefines setRotation to call setOrientation
	@param angle
*/
void PartTerminal::setRotation(qreal angle) {
	qreal angle_mod = std::fmod(angle,360);
	Qet::Orientation new_ori = Qet::North;

	if      (0   <= angle_mod && angle_mod < 90 ) new_ori = Qet::North;
	else if (90  <= angle_mod && angle_mod < 180) new_ori = Qet::East;
	else if (180 <= angle_mod && angle_mod < 270) new_ori = Qet::South;
	else new_ori = Qet::West;

	qreal diffAngle = qRound((angle - rotation()) * 100.0) / 100.0;

	auto p1 = QTransform().rotate(diffAngle).map(pos());
	d->m_pos.setX(p1.x()); d->m_pos.setY(p1.y());
	setPos(d->m_pos);
	setOrientation(new_ori);
	updateSecondPoint();
	prepareGeometryChange();
	emit orientationChanged(); // all terminal-signals call "updateForm"
}

/**
	@brief PartTerminal::rotation
	@return current rotation-angle in degrees
*/
qreal PartTerminal::rotation() const {
	switch (d->m_orientation) {
		case Qet::North : return 0;
		case Qet::East  : return 90;
		case Qet::South : return 180;
		case Qet::West  : return 270;
	}
	return 0;
}

/**
	@brief PartTerminal::flip
	turn part upside down
*/
void PartTerminal::flip() {
	d->m_pos.setX(         pos().x());
	d->m_pos.setY((-1.0) * pos().y());
	switch (d->m_orientation) {
		case Qet::North : setOrientation(Qet::South);
						  break;
		case Qet::East  : break;
		case Qet::South : setOrientation(Qet::North);
						  break;
		case Qet::West  : break;
	}
	setPos(d->m_pos);
	updateSecondPoint();
	prepareGeometryChange();
	emit yChanged(); // all terminal-signals call "updateForm"
}

/**
	@brief PartTerminal::mirror
	turn part from left to right
*/
void PartTerminal::mirror() {
	d->m_pos.setX((-1.0) * pos().x());
	d->m_pos.setY(         pos().y());
	switch (d->m_orientation) {
		case Qet::North : break;
		case Qet::East  : setOrientation(Qet::West);
						  break;
		case Qet::South : break;
		case Qet::West  : setOrientation(Qet::East);
						  break;
	}
	setPos(d->m_pos);
	updateSecondPoint();
	prepareGeometryChange();
	emit xChanged(); // all terminal-signals call "updateForm"
}

void PartTerminal::nextOrientation()
{
    switch (d->m_orientation) {
        case Qet::North :
            setOrientation(Qet::East); break;
        case Qet::East :
            setOrientation(Qet::South); break;
        case Qet::South :
            setOrientation(Qet::West); break;
        case Qet::West  :
            setOrientation(Qet::North); break;
    }
}

void PartTerminal::previousOrientation()
{
    switch (d->m_orientation) {
    case Qet::North :
        setOrientation(Qet::West); break;
    case Qet::East :
        setOrientation(Qet::North); break;
    case Qet::South :
        setOrientation(Qet::East); break;
    case Qet::West  :
        setOrientation(Qet::South); break;
    }
}


/**
	@brief PartTerminal::setTerminalName
	@param name
*/
void PartTerminal::setTerminalName(const QString& name) {
	if (d -> m_name == name) return;
	d -> m_name = name;
	emit nameChanged(); // all terminal-signals call "updateForm"
}

/**
 * @brief PartTerminal::setTerminalType
 * Set the type of terminal to 'type'
 * @param type
 */
void PartTerminal::setTerminalType(TerminalData::Type type)
{
	if (d->m_type == type) {
		return;
	}
	d->m_type = type;
	emit terminalTypeChanged(); // all terminal-signals call "updateForm"
}

void PartTerminal::setNewUuid()
{
	d -> m_uuid = QUuid::createUuid();
}

void PartTerminal::setShowName(bool show)
{
	if (d->m_show_name == show) return;
	prepareGeometryChange();
	d->m_show_name = show;
	update();
	emit showNameChanged();
}

void PartTerminal::setLabelPos(QPointF pos)
{
	if (d->m_label_pos == pos) return;
	prepareGeometryChange();
	d->m_label_pos = pos;
	update();
	emit labelPosChanged();
}

void PartTerminal::setLabelFont(QFont font)
{
	if (d->m_label_font == font) return;
	prepareGeometryChange();
	d->m_label_font = font;
	update();
	emit labelFontChanged();
}

void PartTerminal::setLabelRotation(qreal rotation)
{
	if (qFuzzyCompare(d->m_label_rotation, rotation)) return;
	prepareGeometryChange();
	d->m_label_rotation = rotation;
	update();
	emit labelRotationChanged();
}

void PartTerminal::setLabelHAlignment(Qt::Alignment align)
{
	if (d->m_label_halignment == align) return;
	prepareGeometryChange();
	d->m_label_halignment = align;
	update();
	emit labelHAlignmentChanged();
}

void PartTerminal::setLabelVAlignment(Qt::Alignment align)
{
	if (d->m_label_valignment == align) return;
	prepareGeometryChange();
	d->m_label_valignment = align;
	update();
	emit labelVAlignmentChanged();
}

void PartTerminal::setLabelFrame(bool frame)
{
	if (d->m_label_frame == frame) return;
	prepareGeometryChange();
	d->m_label_frame = frame;
	update();
	emit labelFrameChanged();
}

void PartTerminal::setLabelColor(QColor color)
{
	if (d->m_label_color == color) return;
	d->m_label_color = color;
	update();
	emit labelColorChanged();
}

void PartTerminal::setUseMasterLabel(bool use)
{
	if (d->m_use_master_label == use) return;
	d->m_use_master_label = use;
	update();
	emit useMasterLabelChanged();
}

void PartTerminal::setMasterLabelIndex(int index)
{
	if (d->m_master_label_index == index) return;
	d->m_master_label_index = index;
	update();
	emit masterLabelIndexChanged();
}

/**
	Updates the position of the second point according to the position
	and orientation of the terminal.
	Met a jour la position du second point en fonction de la position et de
	l'orientation de la borne.
*/
void PartTerminal::updateSecondPoint()
{
	qreal ts = 4.0; // terminal size
	switch(d -> m_orientation) {
		case Qet::North: d -> m_second_point = QPointF(0.0,  ts); break;
		case Qet::East : d -> m_second_point = QPointF(-ts, 0.0); break;
		case Qet::South: d -> m_second_point = QPointF(0.0, -ts); break;
		case Qet::West : d -> m_second_point = QPointF(ts,  0.0); break;
	}
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Une borne est toujours pertinente ; cette fonction renvoie donc
	toujours false
*/
bool PartTerminal::isUseless() const
{
	return(false);
}

/**
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartTerminal::sceneGeometricRect() const
{
	return(sceneBoundingRect());
}

/**
	Start the user-induced transformation, provided this primitive is contained
	within the \a initial_selection_rect bounding rectangle.
*/
void PartTerminal::startUserTransformation(const QRectF &initial_selection_rect) {
	Q_UNUSED(initial_selection_rect)
	saved_position_ = scenePos();
}

/**
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
*/
void PartTerminal::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) {
	QPointF mapped_point = mapPoints(
		initial_selection_rect, new_selection_rect, QList<QPointF>() << saved_position_).first();
	setPos(mapped_point);
}

void PartTerminal::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && d->m_show_name && !d->m_name.isEmpty()) {
		QPointF local_click = event->pos();
		// Only start label drag when click is on label AND NOT on terminal line
		if (!shadowShape().contains(local_click) && labelRect().contains(local_click)) {
			m_dragging_label = true;
			m_original_label_pos = d->m_label_pos;
		}
	}
	CustomElementGraphicPart::mousePressEvent(event);
}

void PartTerminal::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_dragging_label) {
		QPointF delta = event->pos() - event->buttonDownPos(Qt::LeftButton);
		QPointF new_pos = m_original_label_pos + delta;
		if (!(event->modifiers() & Qt::ControlModifier)) {
			ElementScene *scene = elementScene();
			if (scene) {
				QPointF scene_pos = mapToScene(new_pos);
				new_pos = mapFromScene(scene->snapToGrid(scene_pos));
			}
		}
		setLabelPos(new_pos);
		update();
		event->accept();
		return;
	}
	CustomElementGraphicPart::mouseMoveEvent(event);
}

void PartTerminal::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_dragging_label) {
		m_dragging_label = false;
		if (m_original_label_pos != d->m_label_pos) {
			auto undo = new QPropertyUndoCommand(this, "label_pos",
				QVariant(m_original_label_pos), QVariant(d->m_label_pos));
			undo->setText(tr("Déplacer le label d'une borne"));
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
		event->accept();
		return;
	}
	CustomElementGraphicPart::mouseReleaseEvent(event);
}

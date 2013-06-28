/*
	Copyright 2006-2012 Xavier Guerrin
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
#include "hotspoteditor.h"

/**
	Constructeur
	@param parent QWidget parent de cet editeur de hotspot
*/
HotspotEditor::HotspotEditor(QWidget *parent) :
	QWidget(parent),
	parts_rect_enabled(false)
{
	informations_label_ = new QLabel(
		tr(
			"L'\351l\351ment doit \352tre assez grand pour contenir tout sa "
			"repr\351sentation graphique."
		)
	);
	
	sb_width = new QSpinBox();
	sb_width -> setMinimum(1);
	sb_width -> setMaximum(1000);
	sb_width -> setValue(3);
	sb_width -> setSuffix(tr(" \32710 px"));
	sb_height = new QSpinBox();
	sb_height -> setMinimum(1);
	sb_height -> setMaximum(1000);
	sb_height -> setValue(7);
	sb_height -> setSuffix(tr(" \32710 px"));
	
	sb_hotspot_x = new QSpinBox();
	sb_hotspot_x -> setValue(15);
	sb_hotspot_x -> setSuffix(tr(" px"));
	sb_hotspot_x -> setSingleStep(10);
	sb_hotspot_y = new QSpinBox();
	sb_hotspot_y -> setValue(35);
	sb_hotspot_y -> setSuffix(tr(" px"));
	sb_hotspot_y -> setSingleStep(10);
	
	diagram_scene = new Diagram();
	diagram_scene -> border_and_titleblock.setColumnsWidth(50.0);
	diagram_scene -> border_and_titleblock.setColumnsCount(3);
	diagram_scene -> border_and_titleblock.setRowsHeight(60.0);
	diagram_scene -> border_and_titleblock.setRowsCount(2);
	diagram_scene -> border_and_titleblock.displayRows(false);
	diagram_scene -> border_and_titleblock.displayTitleBlock(false);
	
	diagram_view = new QGraphicsView(diagram_scene);
	diagram_view -> setMinimumSize(
		qRound(diagram_scene -> border_and_titleblock.diagramWidth()  + (3 * Diagram::margin)),
		qRound(diagram_scene -> border_and_titleblock.diagramHeight() + (3 * Diagram::margin))
	);
	diagram_view -> setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	diagram_view -> setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	diagram_view -> setAlignment(Qt::AlignLeft | Qt::AlignTop);
	
	hotspot_sync = new QCheckBox(tr("D\351placer l'\351l\351ment avec le hotspot"));
	hotspot_sync -> setChecked(true);
	// TODO setvisible to disable checkbox hotspot_sync
	// one for remove bug in element editor after un move hotspot_sync, selection is not focused on primitives
	// two is not specialy used
	// and for future auto boudinrect hotspot
	hotspot_sync -> setVisible(false);
	
	connect(sb_width,     SIGNAL(valueChanged(int)), this, SLOT(updateHotspotLimits()));
	connect(sb_height,    SIGNAL(valueChanged(int)), this, SLOT(updateHotspotLimits()));
	connect(sb_width,     SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
	connect(sb_height,    SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
	connect(sb_hotspot_x, SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
	connect(sb_hotspot_y, SIGNAL(valueChanged(int)), this, SLOT(updateScene()));
	connect(hotspot_sync, SIGNAL(stateChanged(int)), this, SLOT(updateScene()));
	
	QGridLayout *grid_layout = new QGridLayout();
	grid_layout -> addWidget(new QLabel(tr("<span style=\"text-decoration:underline;\">Dimensions</span>")),   0, 0);
	grid_layout -> addWidget(new QLabel(tr("Largeur :")),      1, 0);
	grid_layout -> addWidget(sb_width,                         1, 1);
	grid_layout -> addWidget(new QLabel(tr("Hauteur :")),      2, 0);
	grid_layout -> addWidget(sb_height,                        2, 1);
	grid_layout -> addWidget(new QLabel(tr("<span style=\"text-decoration:underline;\">Hotspot</span>")),      3, 0);
	grid_layout -> addWidget(new QLabel(tr("Abscisse :")),     4, 0);
	grid_layout -> addWidget(sb_hotspot_x,                     4, 1);
	grid_layout -> addWidget(new QLabel(tr("Ordonn\351e :")),  5, 0);
	grid_layout -> addWidget(sb_hotspot_y,                     5, 1);
	grid_layout -> setRowStretch(6, 500);
	grid_layout -> setSpacing(3);
	
	QHBoxLayout *hlayout = new QHBoxLayout();
	hlayout -> addLayout(grid_layout,    1);
	hlayout -> addWidget(diagram_view, 500);
	
	vlayout = new QVBoxLayout(this);
	vlayout -> setSpacing(0);
	vlayout -> addWidget(informations_label_);
	vlayout -> addLayout(hlayout, 1);
	
	updateScene();
	updateHotspotLimits();
}

/// @param w Nouvelle largeur de l'element, en dizaines de pixels
void HotspotEditor::setElementWidth(uint w) {
	sb_width -> setValue(w);
}

/// @param h Nouvelle hauteur de l'element, en dizaines de pixels
void HotspotEditor::setElementHeight(uint h) {
	sb_height -> setValue(h);
}

/// @return la Largeur de l'element en dizaines de pixels
uint HotspotEditor::elementWidth10px() const {
	return(sb_width -> value());
}

/// @return la hauteur de l'element en dizaines de pixels
uint HotspotEditor::elementHeight10px() const {
	return(sb_height -> value());
}

/// @return la Largeur de l'element en pixels
uint HotspotEditor::elementWidth() const {
	return(sb_width -> value() * 10);
}

/// @return la hauteur de l'element en pixels
uint HotspotEditor::elementHeight() const {
	return(sb_height -> value() * 10);
}

/// @param size La nouvelle taille de l'element, en dizaines de pixels
void HotspotEditor::setElementSize(const QSize &size) {
	setElementWidth(size.width());
	setElementWidth(size.height());
}

/// @return la taille de l'element, en dizaines de pixels
QSize HotspotEditor::elementSize10px() const {
	return(QSize(elementWidth10px(), elementHeight10px()));
}

/// @return la taille de l'element, en pixels
QSize HotspotEditor::elementSize() const {
	return(QSize(elementWidth(), elementHeight()));
}

/// @param hs Nouvelle position du hotspot
void HotspotEditor::setHotspot(const QPoint &hs) {
	sb_hotspot_x -> setValue(hs.x());
	sb_hotspot_y -> setValue(hs.y());
}

/// @return les coordonnees du hotspot de l'element
QPoint HotspotEditor::hotspot() const {
	return(QPoint(sb_hotspot_x -> value(), sb_hotspot_y -> value()));
}

/// @param o_h l'ancien hotspot en cas d'edition du hotspot
void HotspotEditor::setOldHotspot(const QPoint &o_h) {
	old_hotspot = o_h;
}

/// @return l'ancien hotspot en cas d'edition du hotspot
QPoint HotspotEditor::oldHotspot() const {
	return(old_hotspot);
}

/**
	Specifie le rectangle delimitant les parties de l'element
	@param rect rectangle delimitant les parties de l'element
*/
void HotspotEditor::setPartsRect(const QRectF &rect) {
	parts_rect = rect;
}

/// @return le rectangle delimitant les parties de l'element
QRectF HotspotEditor::partsRect() const {
	return(parts_rect);
}

/**
	@param a true pour activer l'affichage du rectangle representant l'element,
	false pour le desactiver
*/
void HotspotEditor::setPartsRectEnabled(bool a) {
	if (a != parts_rect_enabled) {
		if (a) vlayout -> addWidget(hotspot_sync);
		else vlayout -> removeWidget(hotspot_sync);
	}
	parts_rect_enabled = a;
	updateScene();
}

/**
	@return true si l'affichage du rectangle representant l'element est active,
	false sinon
*/
bool HotspotEditor::partsRectEnabled() {
	return(parts_rect_enabled);
}

/// @return true s'il sera necessaire d'appliquer une translation aux parties de l'elements
bool HotspotEditor::mustTranslateParts() const {
	// il sera necessaire d'appliquer une translation aux parties de l'elements
	// si le hotspot a ete change et si l'element ne suit pas le hotspot
	bool hotspot_change = !old_hotspot.isNull() && old_hotspot != hotspot();
	return(hotspot_change && !hotspot_sync -> isChecked());
}

/// @return La translation a faire subir aux parties de l'element apres l'edition du hotspot
QPoint HotspotEditor::offsetParts() const {
	// si le hotspot n'a pas ete change ou si l'element doit suivre le hotspot,
	// il n'est pas necessaire d'appliquer une translation aux parties de
	// l'elements
	if (!mustTranslateParts()) return(QPoint(0,0));
	else return(old_hotspot - hotspot());
}

/**
	@return true si ce widget est en lecture seule, false sinon
*/
bool HotspotEditor::isReadOnly() const {
	return(sb_width -> isReadOnly());
}

/**
	@param ro true pour passer ce widget en lecture seule, false sinon
*/
void HotspotEditor::setReadOnly(bool ro) {
	sb_width     -> setReadOnly(ro);
	sb_height    -> setReadOnly(ro);
	sb_hotspot_x -> setReadOnly(ro);
	sb_hotspot_y -> setReadOnly(ro);
	hotspot_sync -> setDisabled(ro);
}

/**
	Met a jour le schema
*/
void HotspotEditor::updateScene() {
	// nettoie la scene
	foreach (QGraphicsItem *qgi, diagram_scene -> items()) {
		diagram_scene -> removeItem(qgi);
		delete qgi;
	}
	
	int elmt_width  = sb_width  -> value() * 10;
	int elmt_height = sb_height -> value() * 10;
	int hotspot_x   = sb_hotspot_x -> value();
	int hotspot_y   = sb_hotspot_y -> value();
	int margin_x = 10;
	int margin_y = 30;
	
	// dessin du cadre representant les dimensions de l'element
	diagram_scene -> addRect(QRectF(margin_x, margin_y, elmt_width, elmt_height));
	
	// dessin des deux segments representant le point de saisie de l'element
	QPen hotspot_pen(Qt::red);
	QGraphicsLineItem *line_hotspot_x = diagram_scene -> addLine(
		QLine(
			margin_x,
			margin_y + hotspot_y,
			margin_x + elmt_width,
			margin_y + hotspot_y
		),
		hotspot_pen
	);
	QGraphicsLineItem *line_hotspot_y = diagram_scene -> addLine(
		QLine(
			margin_x + hotspot_x,
			margin_y,
			margin_x + hotspot_x,
			margin_y + elmt_height
		),
		hotspot_pen
	);
	line_hotspot_x -> setZValue(10);
	line_hotspot_y -> setZValue(10);
	
	// dessin eventuel du rectangle representant l'element
	if (parts_rect_enabled) {
		QPen element_pen(Qt::blue);
		QBrush element_brush(QColor(0, 0, 255, 100));
		QRectF parts_rect_to_draw;
		
		if (!hotspot_sync -> isChecked() && !old_hotspot.isNull()) {
			// coordonnees de l'ancien hotspot sur le schema
			QPointF current_old_hotspotf(margin_x + old_hotspot.x(), margin_y + old_hotspot.y());
			QPoint current_old_hotspot(current_old_hotspotf.toPoint());
			
			// laisse l'element en place par rapport au coin superieur gauche
			parts_rect_to_draw = parts_rect.translated(current_old_hotspot);
		} else {
			// coordonnees du nouveau hotspot sur le schema
			QPointF current_hotspotf(line_hotspot_y -> line().x1(), line_hotspot_x -> line().y1());
			QPoint current_hotspot(current_hotspotf.toPoint());
			
			// deplace l'element en meme temps que le hotspot
			parts_rect_to_draw = parts_rect.translated(current_hotspot);
		}
		QGraphicsRectItem *rect_element = diagram_scene -> addRect(parts_rect_to_draw, element_pen, element_brush);
		rect_element -> setZValue(5);
	}
	
	diagram_scene -> setSceneRect(QRect(0, 0, elmt_width + (2 * margin_x) + 15, elmt_height + (2 * margin_y)));
	diagram_scene -> update();
}

/**
	Met a jour les limites des QSpinBox
*/
void HotspotEditor::updateHotspotLimits() {
	sb_hotspot_x -> setMaximum(sb_width  -> value() * 10);
	sb_hotspot_y -> setMaximum(sb_height -> value() * 10);
}

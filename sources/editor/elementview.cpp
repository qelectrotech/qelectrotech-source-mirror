/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "elementview.h"
#include "qetelementeditor.h"
#include "editorcommands.h"
/**
	Constructeur
	@param scene ElementScene visualisee par cette ElementView
	@param parent QWidget parent de cette ElementView
*/
ElementView::ElementView(ElementScene *scene, QWidget *parent) :
	QGraphicsView(scene, parent),
	scene_(scene),
	offset_paste_count_(0)
{
	setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setInteractive(true);
	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	zoomReset();
	connect(scene_, SIGNAL(pasteAreaDefined(const QRectF &)), this, SLOT(pasteAreaDefined(const QRectF &)));
	connect(scene_, SIGNAL(needZoomFit()), this, SLOT(zoomFit()));
}

/// Destructeur
ElementView::~ElementView() {
}

/// @return l'ElementScene visualisee par cette ElementView
ElementScene *ElementView::scene() const {
	return(scene_);
}

/**
	@return le rectangle de l'element visualise par cet ElementView
*/
QRectF ElementView::viewedSceneRect() const {
	// recupere la taille du widget viewport
	QSize viewport_size = viewport() -> size();
	
	// recupere la transformation viewport -> scene
	QTransform view_to_scene   = viewportTransform().inverted();
	
	// mappe le coin superieur gauche et le coin inferieur droit de la viewport sur la scene
	QPointF scene_left_top     = view_to_scene.map(QPointF(0.0, 0.0));
	QPointF scene_right_bottom = view_to_scene.map(QPointF(viewport_size.width(), viewport_size.height()));
	
	// en deduit le rectangle visualise par la scene
	return(QRectF(scene_left_top, scene_right_bottom));
}

void ElementView::contextMenuEvent(QContextMenuEvent *event) {
	scene_ -> contextMenu(event);
}

/**
	Definit l'ElementScene visualisee par cette ElementView
	@param s l'ElementScene visualisee par cette ElementView
*/
void ElementView::setScene(ElementScene *s) {
	QGraphicsView::setScene(s);
	scene_ = s;
}

/**
	Set the Diagram in visualisation mode
*/
void ElementView::setVisualisationMode() {
	setDragMode(ScrollHandDrag);
	setInteractive(false);
	emit(modeChanged());
}

/**
	Set the Diagram in Selection mode
*/
void ElementView::setSelectionMode() {
	setDragMode(RubberBandDrag);
	setInteractive(true);
	emit(modeChanged());
}

/**
	Agrandit le schema (+33% = inverse des -25 % de zoomMoins())
*/
void ElementView::zoomIn() {
	adjustSceneRect();
	scale(4.0/3.0, 4.0/3.0);
}

/**
	Retrecit le schema (-25% = inverse des +33 % de zoomPlus())
*/
void ElementView::zoomOut() {
	adjustSceneRect();
	scale(0.75, 0.75);
}

/**
	Agrandit ou rectrecit le schema de facon a ce que tous les elements du
	schema soient visibles a l'ecran. S'il n'y a aucun element sur le schema,
	le zoom est reinitialise
*/
void ElementView::zoomFit() {
	resetSceneRect();
	fitInView(sceneRect(), Qt::KeepAspectRatio);
}

/**
	Reinitialise le zoom
*/
void ElementView::zoomReset() {
	resetSceneRect();
	resetMatrix();
	scale(4.0, 4.0);
}

/**
	Ajuste le sceneRect (zone du schéma visualisée par l'ElementView) afin que
	celui-ci inclut à la fois les primitives de l'élément ainsi que le viewport
	de la scène avec une marge de 1/4 d'elle-même.
*/
void ElementView::adjustSceneRect() {
	QRectF esgr = scene_ -> elementSceneGeometricRect();
	QRectF vpbr = mapToScene(this -> viewport()->rect()).boundingRect();
	QRectF new_scene_rect = vpbr.adjusted(-vpbr.width()/4, -vpbr.height()/4, vpbr.width()/4, vpbr.height()/4);
	setSceneRect(new_scene_rect.united(esgr));
}

/**
 * @brief ElementView::resetSceneRect
 * reset le sceneRect (zone du schéma visualisée par l'ElementView) afin que
 * celui-ci inclut uniquement les primitives de l'élément dessiné.
 */
void ElementView::resetSceneRect() {
	setSceneRect(scene_ -> elementSceneGeometricRect());
}

/**
	Gere le fait de couper la selection = l'exporter en XML dans le
	presse-papier puis la supprimer.
*/
void ElementView::cut() {
	// delegue cette action a la scene
	scene_ -> cut();
	offset_paste_count_ = -1;
}

/**
	Gere le fait de copier la selection = l'exporter en XML dans le
	presse-papier.
*/
void ElementView::copy() {
	// delegue cette action a la scene
	scene_ -> copy();
	offset_paste_count_ = 0;
}

/**
	Gere le fait de coller le contenu du presse-papier = l'importer dans
	l'element. Cette methode examine le contenu du presse-papier. Si celui-ci
	semble avoir ete copie depuis cet element, il est colle a cote de sa zone
	d'origine ; s'il est recolle, il sera colle un cran a cote de la zone deja
	recollee, etc.
	Sinon, cette methode demande a l'utilisateur de definir la zone ou le
	collage devra s'effectuer.
	@see pasteAreaDefined(const QRectF &)
*/
void ElementView::paste() {
	QString clipboard_text = QApplication::clipboard() -> text();
	if (clipboard_text.isEmpty()) return;
	
	QDomDocument document_xml;
	if (!document_xml.setContent(clipboard_text)) return;
	
	if (scene_ -> wasCopiedFromThisElement(clipboard_text)) {
		// copier/coller avec decalage
		pasteWithOffset(document_xml);
	} else {
		// copier/coller par choix de la zone de collage
		QRectF pasted_content_bounding_rect = scene_ -> boundingRectFromXml(document_xml);
		if (pasted_content_bounding_rect.isEmpty()) return;
		
		to_paste_in_area_ = clipboard_text;
		getPasteArea(pasted_content_bounding_rect);
	}
}

/**
	Colle le contenu du presse-papier en demandant systematiquement a
	l'utilisateur de choisir une zone de collage
*/
void ElementView::pasteInArea() {
	QString clipboard_text = QApplication::clipboard() -> text();
	if (clipboard_text.isEmpty()) return;
	
	QDomDocument document_xml;
	if (!document_xml.setContent(clipboard_text)) return;
	
	QRectF pasted_content_bounding_rect = scene_ -> boundingRectFromXml(document_xml);
	if (pasted_content_bounding_rect.isEmpty()) return;
	
	// copier/coller par choix de la zone de collage
	to_paste_in_area_ = clipboard_text;
	getPasteArea(pasted_content_bounding_rect);
}

/**
	Gere le fait de coller le contenu du presse-papier = l'importer dans
	l'element. Cette methode examine le contenu du presse-papier. Si celui-ci
	est exploitable, elle le colle a la position passee en parametre.
	@see pasteAreaDefined(const QRectF &)
	@param position Point de collage
*/
ElementContent ElementView::paste(const QPointF &position) {
	QString clipboard_text = QApplication::clipboard() -> text();
	if (clipboard_text.isEmpty()) return(ElementContent());
	
	QDomDocument document_xml;
	if (!document_xml.setContent(clipboard_text)) return(ElementContent());
	
	// objet pour recuperer le contenu ajoute au schema par le coller
	return(paste(document_xml, position));
}

/**
	@param to_paste Rectangle englobant les parties a coller
*/
void ElementView::getPasteArea(const QRectF &to_paste) {
	// on copie le rectangle fourni - on s'interesse a ses dimensions, pas a sa position
	QRectF used_rect(to_paste);
	
	// on lui attribue pour centre l'origine du repere
	if (underMouse()) {
		used_rect.moveCenter(mapToScene(mapFromGlobal(QCursor::pos())));
	} else {
		used_rect.moveCenter(QPointF(0.0, 0.0));
	}
	scene_ -> getPasteArea(used_rect);
}

/**
	Slot appele lorsque la scene annonce avoir defini une zone de collage
	@param target_rect Rectangle cible pour le collage
*/
ElementContent ElementView::pasteAreaDefined(const QRectF &target_rect) {
	if (to_paste_in_area_.isEmpty()) return(ElementContent());
	
	QDomDocument xml_document;
	if (!xml_document.setContent(to_paste_in_area_)) {
		to_paste_in_area_.clear();
		return(ElementContent());
	} else {
		return(paste(xml_document, target_rect.topLeft()));
	}
}

/**
	Colle le document XML xml_document a la position pos
	@param xml_document Document XML a coller
	@param pos Coin superieur gauche du rectangle cible
*/
ElementContent ElementView::paste(const QDomDocument &xml_document, const QPointF &pos) {
	// objet pour recuperer le contenu ajoute au schema par le coller
	ElementContent content_pasted;
	scene_ -> fromXml(xml_document, pos, false, &content_pasted);
	
	// si quelque chose a effectivement ete ajoute au schema, on cree un objet d'annulation
	if (content_pasted.count()) {
		scene_ -> clearSelection();
		PastePartsCommand *undo_object = new PastePartsCommand(this, content_pasted);
		scene_ -> undoStack().push(undo_object);
	}
	return(content_pasted);
}

/**
	Colle le document XML xml_document a la position pos
	@param xml_document Document XML a coller
*/
ElementContent ElementView::pasteWithOffset(const QDomDocument &xml_document) {
	// objet pour recuperer le contenu ajoute au schema par le coller
	ElementContent content_pasted;
	
	// rectangle source
	QRectF pasted_content_bounding_rect = scene_ -> boundingRectFromXml(xml_document);
	if (pasted_content_bounding_rect.isEmpty()) return(content_pasted);
	
	// copier/coller avec decalage
	QRectF final_pasted_content_bounding_rect;
	++ offset_paste_count_;
	if (!offset_paste_count_) {
		// the pasted content was cut
		start_top_left_corner_ = pasted_content_bounding_rect.topLeft();
		final_pasted_content_bounding_rect = pasted_content_bounding_rect;
	}
	else {
		// the pasted content was copied
		if (offset_paste_count_ == 1) {
			start_top_left_corner_ = pasted_content_bounding_rect.topLeft();
		} else {
			pasted_content_bounding_rect.moveTopLeft(start_top_left_corner_);
		}
		
		// on applique le decalage qui convient
		final_pasted_content_bounding_rect = applyMovement(
			pasted_content_bounding_rect,
			QETElementEditor::pasteOffset()
		);
	}
	QPointF old_start_top_left_corner = start_top_left_corner_;
	start_top_left_corner_ = final_pasted_content_bounding_rect.topLeft();
	scene_ -> fromXml(xml_document, start_top_left_corner_, false, &content_pasted);
	
	// si quelque chose a effectivement ete ajoute au schema, on cree un objet d'annulation
	if (content_pasted.count()) {
		scene_ -> clearSelection();
		PastePartsCommand *undo_object = new PastePartsCommand(this, content_pasted);
		undo_object -> setOffset(offset_paste_count_ - 1, old_start_top_left_corner, offset_paste_count_, start_top_left_corner_);
		scene_ -> undoStack().push(undo_object);
	}
	return(content_pasted);
}

/**
	Gere les clics sur la vue - permet de coller lorsaue l'on enfonce le bouton
	du milieu de la souris.
	@param e QMouseEvent decrivant l'evenement souris
*/
void ElementView::mousePressEvent(QMouseEvent *e) {
	// Select visualisation or selection mode
	if (e -> buttons() == Qt::MidButton) {
		setCursor(Qt::ClosedHandCursor);
		reference_view_ = mapToScene(e -> pos());
		center_view_ = mapToScene(this -> viewport() -> rect()).boundingRect().center();
		return;
	}
	QGraphicsView::mousePressEvent(e);
}

/**
 * @brief ElementView::mouseMoveEvent
 * Manage the event move mouse
 */
void ElementView::mouseMoveEvent(QMouseEvent *e) {
	if ((e -> buttons() & Qt::MidButton) == Qt::MidButton) {
		QPointF move = reference_view_ - mapToScene(e -> pos());
		this -> centerOn(center_view_ + move);
		center_view_ = mapToScene(this -> viewport() -> rect()).boundingRect().center();
		adjustSceneRect();
		return;
	}
	QGraphicsView::mouseMoveEvent(e);
}

/**
 * @brief ElementView::mouseReleaseEvent
 * Manage event release click mouse
 */
void ElementView::mouseReleaseEvent(QMouseEvent *e) {
	if (e -> button() == Qt::MidButton) {
		setCursor(Qt::ArrowCursor);
		return;
	}
	QGraphicsView::mouseReleaseEvent(e);
}

/**
	Gere les actions liees a la rollette de la souris
	@param e QWheelEvent decrivant l'evenement rollette
*/
void ElementView::wheelEvent(QWheelEvent *e) {
	//Zoom and scrolling
	if (e->buttons() != Qt::MidButton) {
		if (!(e -> modifiers() & Qt::ControlModifier)) {
			if (e -> delta() > 0){
				zoomIn();
			}
			else{
				zoomOut();
			}
		}
		else {
			QAbstractScrollArea::wheelEvent(e);
		}
	}
}

/**
	Dessine l'arriere-plan de l'editeur, cad la grille.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void ElementView::drawBackground(QPainter *p, const QRectF &r) {
	p -> save();
	
	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	p -> setBrush(Qt::white);
	p -> drawRect(r);
		
	// determine le zoom en cours
	qreal zoom_factor = matrix().m11();
	
	// choisit la granularite de la grille en fonction du zoom en cours
	int drawn_x_grid = 1;//scene_ -> xGrid();
	int drawn_y_grid = 1;//scene_ -> yGrid();
	bool draw_grid = true;
	bool draw_cross = false;

	if (zoom_factor < (4.0/3.0)) { //< no grid
		draw_grid = false;
	} else if (zoom_factor < 4.0) { //< grid 10*10
		drawn_x_grid *= 10;
		drawn_y_grid *= 10;
	}else if (zoom_factor < 8.0) { //< grid 5*5
		drawn_x_grid *= 5;
		drawn_y_grid *= 5;
		draw_cross = true;
	} else if (zoom_factor < 10.0) { //< grid 2*2
		drawn_x_grid *= 2;
		drawn_y_grid *= 2;
		draw_cross = true;	
	} else { //< grid 1*1
		draw_cross = true;
	}

	scene_->setGrid(drawn_x_grid, drawn_y_grid);
	
	if (draw_grid) {
		// draw the dot of the grid
		p -> setPen(Qt::black);
		p -> setBrush(Qt::NoBrush);
		qreal limite_x = r.x() + r.width();
		qreal limite_y = r.y() + r.height();
		
		int g_x = (int)ceil(r.x());
		while (g_x % drawn_x_grid) ++ g_x;
		int g_y = (int)ceil(r.y());
		while (g_y % drawn_y_grid) ++ g_y;
		
		for (int gx = g_x ; gx < limite_x ; gx += drawn_x_grid) {
			for (int gy = g_y ; gy < limite_y ; gy += drawn_y_grid) {
				if (draw_cross) {
					if (!(gx % 10) && !(gy % 10)) {
						p -> drawLine(QLineF(gx - 0.25, gy, gx + 0.25, gy));
						p -> drawLine(QLineF(gx, gy - 0.25, gx, gy + 0.25));
					} else {
						p -> drawPoint(gx, gy);
					}
				} else {
					p -> drawPoint(gx, gy);
				}
			}
		}
	}
	p -> restore();
}

/**
	Applique le decalage offset dans le sens movement au rectangle start
	@param start rectangle a decaler
	@param movement Orientation du decalage a appliquer
	@param offset Decalage a appliquer
*/
QRectF ElementView::applyMovement(const QRectF &start, const QPointF &offset) {
	// calcule le decalage a appliquer a partir de l'offset
	QPointF final_offset;
	final_offset.rx() =  start.width() + offset.x();

	// applique le decalage ainsi calcule
	return(start.translated(final_offset));
}

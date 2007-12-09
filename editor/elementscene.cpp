/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "elementscene.h"
#include "qetelementeditor.h"
#include <cmath>
#include "partline.h"
#include "partellipse.h"
#include "partcircle.h"
#include "partpolygon.h"
#include "partterminal.h"
#include "parttext.h"
#include "parttextfield.h"
#include "partarc.h"
#include "hotspoteditor.h"
#include "editorcommands.h"

const int ElementScene::xGrid = 10;
const int ElementScene::yGrid = 10;

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent le Widget parent
*/
ElementScene::ElementScene(QETElementEditor *editor, QObject *parent) :
	QGraphicsScene(parent),
	_width(3),
	_height(7),
	_hotspot(15, 35),
	internal_connections(false),
	qgi_manager(this),
	element_editor(editor)
{
	current_polygon = NULL;
	undo_stack.setClean();
	connect(this, SIGNAL(changed(const QList<QRectF> &)), this, SLOT(slot_checkSelectionChanged()));
}

/// Destructeur
ElementScene::~ElementScene() {
}

/**
	Passe la scene en mode "selection et deplacement de parties"
*/
void ElementScene::slot_move() {
	behavior = Normal;
}

/**
	Passe la scene en mode "ajout de ligne"
*/
void ElementScene::slot_addLine() {
	behavior = Line;
}

/**
	Passe la scene en mode "ajout de cercle"
*/
void ElementScene::slot_addCircle() {
	behavior = Circle;
}

/**
	Passe la scene en mode "ajout d'ellipse"
*/
void ElementScene::slot_addEllipse() {
	behavior = Ellipse;
}

/**
	Passe la scene en mode "ajout de polygone"
*/
void ElementScene::slot_addPolygon() {
	behavior = Polygon;
}


/**
	Passe la scene en mode "ajout de texte statique"
*/
void ElementScene::slot_addText() {
	behavior = Text;
}

/**
	Passe la scene en mode "ajout de borne"
*/
void ElementScene::slot_addTerminal() {
	behavior = Terminal;
}


/**
	Passe la scene en mode "ajout d'arc de cercle"
*/
void ElementScene::slot_addArc() {
	behavior = Arc;
}

/**
	Passe la scene en mode "ajout de champ de texte"
*/
void ElementScene::slot_addTextField() {
	behavior = TextField;
}

/**
	Gere les mouvements de la souris
	@param e objet decrivant l'evenement
*/
void ElementScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	QRectF temp_rect;
	qreal radius;
	QPointF temp_point;
	QPolygonF temp_polygon;
	if (e -> buttons() & Qt::LeftButton) {
		switch(behavior) {
			case Line:
				current_line -> setLine(QLineF(current_line -> line().p1(), e -> scenePos()));
				break;
			case Ellipse:
				temp_rect = current_ellipse -> rect();
				temp_rect.setBottomRight(e -> scenePos());
				current_ellipse -> setRect(temp_rect);
				break;
			case Arc:
				temp_rect = current_arc -> rect();
				temp_rect.setBottomRight(e -> scenePos());
				current_arc -> setRect(temp_rect);
				break;
			case Circle:
				temp_rect = current_circle -> rect();
				temp_point = e -> scenePos() - current_circle -> mapToScene(temp_rect.center());
				radius = sqrt(pow(temp_point.x(), 2) + pow(temp_point.y(), 2));
				temp_rect = QRectF(
					temp_rect.center() - QPointF(radius, radius),
					QSizeF(2.0 * radius, 2.0 * radius)
				);
				current_circle -> setRect(temp_rect);
				break;
			case Polygon:
				if (current_polygon == NULL) break;
				temp_polygon = current_polygon -> polygon();
				temp_polygon.pop_back();
				temp_polygon << e -> scenePos();
				current_polygon -> setPolygon(temp_polygon);
				break;
			case Normal:
			default:
				QGraphicsScene::mouseMoveEvent(e);
		}
	} else if (behavior == Polygon && current_polygon != NULL) {
		temp_polygon = current_polygon -> polygon();
		temp_polygon.pop_back();
		temp_polygon << e -> scenePos();
		current_polygon -> setPolygon(temp_polygon);
	} else QGraphicsScene::mouseMoveEvent(e);
}

/**
	Gere les appuis sur les boutons de la souris
	@param e objet decrivant l'evenement
*/
void ElementScene::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	QPolygonF temp_polygon;
	if (e -> button() & Qt::LeftButton) {
		switch(behavior) {
			case Line:
				current_line = new PartLine(element_editor, 0, this);
				current_line -> setLine(QLineF(e -> scenePos(), e -> scenePos()));
				break;
			case Ellipse:
				current_ellipse = new PartEllipse(element_editor, 0, this);
				current_ellipse -> setRect(QRectF(e -> scenePos(), QSizeF(0.0, 0.0)));
				current_ellipse -> setProperty("antialias", true);
				break;
			case Arc:
				current_arc = new PartArc(element_editor, 0, this);
				current_arc -> setRect(QRectF(e -> scenePos(), QSizeF(0.0, 0.0)));
				current_arc -> setProperty("antialias", true);
				break;
			case Circle:
				current_circle = new PartCircle(element_editor, 0, this);
				current_circle -> setRect(QRectF(e -> scenePos(), QSizeF(0.0, 0.0)));
				current_circle -> setProperty("antialias", true);
				break;
			case Polygon:
				if (current_polygon == NULL) {
					current_polygon = new PartPolygon(element_editor, 0, this);
					temp_polygon = QPolygonF(0);
				} else temp_polygon = current_polygon -> polygon();
				// au debut, on insere deux points
				if (!temp_polygon.count()) temp_polygon << e -> scenePos();
				temp_polygon << e -> scenePos();
				current_polygon -> setPolygon(temp_polygon);
				break;
			case Normal:
			default:
				QGraphicsScene::mousePressEvent(e);
				if (!selectedItems().isEmpty()) fsi_pos = selectedItems().first() -> scenePos();
		}
	} else QGraphicsScene::mousePressEvent(e);
}

/**
	Gere les relachements de boutons de la souris
	@param e objet decrivant l'evenement
*/
void ElementScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	PartTerminal *terminal;
	PartText *text;
	PartTextField *textfield;
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	if (e -> button() & Qt::LeftButton) {
		switch(behavior) {
			case Line:
				undo_stack.push(new AddPartCommand(tr("ligne"), this, current_line));
				emit(partsAdded());
				break;
			case Ellipse:
				current_ellipse -> setRect(current_ellipse -> rect().normalized());
				undo_stack.push(new AddPartCommand(tr("ellipse"), this, current_ellipse));
				emit(partsAdded());
				break;
			case Arc:
				current_arc-> setRect(current_arc -> rect().normalized());
				undo_stack.push(new AddPartCommand(tr("arc"), this, current_arc));
				emit(partsAdded());
				break;
			case Circle:
				current_circle -> setRect(current_circle -> rect().normalized());
				undo_stack.push(new AddPartCommand(tr("cercle"), this, current_circle));
				emit(partsAdded());
				break;
			case Terminal:
				terminal = new PartTerminal(element_editor, 0, this);
				terminal -> setPos(e -> scenePos());
				undo_stack.push(new AddPartCommand(tr("borne"), this, terminal));
				emit(partsAdded());
				break;
			case Text:
				text = new PartText(element_editor, 0, this);
				text -> setPos(e -> scenePos());
				undo_stack.push(new AddPartCommand(tr("texte"), this, text));
				emit(partsAdded());
				break;
			case TextField:
				textfield = new PartTextField(element_editor, 0, this);
				textfield -> setPos(e -> scenePos());
				undo_stack.push(new AddPartCommand(tr("champ de texte"), this, textfield));
				emit(partsAdded());
				break;
			case Normal:
			default:
				QGraphicsScene::mouseReleaseEvent(e);
				// detecte les deplacements de parties
				if (!selectedItems().isEmpty()) {
					QPointF movement = selectedItems().first() -> scenePos() - fsi_pos;
					if (!movement.isNull()) {
						undo_stack.push(new MovePartsCommand(movement, this, selectedItems()));
					}
				}
		}
	} else if (e -> button() & Qt::RightButton) {
		if (behavior == Polygon) {
			behavior = Normal;
			undo_stack.push(new AddPartCommand(tr("polygone"), this, current_polygon));
			current_polygon = NULL;
			emit(partsAdded());
			emit(needNormalMode());
		} else QGraphicsScene::mouseReleaseEvent(e);
	} else QGraphicsScene::mouseReleaseEvent(e);
}

/**
	Dessine l'arriere-plan de l'editeur, cad la grille.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void ElementScene::drawBackground(QPainter *p, const QRectF &r) {
	p -> save();
	
	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	p -> setBrush(Qt::white);
	p -> drawRect(r);
	
	// encadre la zone dessinable de l'element
	QRectF drawable_area(-_hotspot.x(), -_hotspot.y(), width(), height());
	p -> setPen(Qt::black);
	p -> setBrush(Qt::NoBrush);
	p -> drawRect(drawable_area);
	
	if (r.width() < 2500 && r.height() < 2500) {
		// dessine les points de la grille
		p -> setPen(Qt::black);
		p -> setBrush(Qt::NoBrush);
		qreal limite_x = r.x() + r.width();
		qreal limite_y = r.y() + r.height();
		
		int g_x = (int)ceil(r.x());
		while (g_x % xGrid) ++ g_x;
		int g_y = (int)ceil(r.y());
		while (g_y % yGrid) ++ g_y;
		
		for (int gx = g_x ; gx < limite_x ; gx += xGrid) {
			for (int gy = g_y ; gy < limite_y ; gy += yGrid) {
				p -> drawPoint(gx, gy);
			}
		}
	}
	p -> restore();
}

/**
	Dessine l'arriere-plan de l'editeur, cad l'indicateur de hotspot.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void ElementScene::drawForeground(QPainter *p, const QRectF &) {
	p -> save();
	
	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	p -> setPen(Qt::red);
	p -> setBrush(Qt::NoBrush);
	p -> drawLine(QLineF(0.0, -_hotspot.y(), 0.0, height() - _hotspot.y()));
	p -> drawLine(QLineF(-_hotspot.x(), 0.0, width() - _hotspot.x(),  0.0));
	p -> restore();
}

/**
	Exporte l'element en XML
	@return un document XML decrivant l'element
*/
const QDomDocument ElementScene::toXml() const {
	// document XML
	QDomDocument xml_document;
	
	// racine du document XML
	QDomElement root = xml_document.createElement("definition");
	root.setAttribute("type",        "element");
	root.setAttribute("width",       QString("%1").arg(_width  * 10));
	root.setAttribute("height",      QString("%1").arg(_height * 10));
	root.setAttribute("hotspot_x",   QString("%1").arg(_hotspot.x()));
	root.setAttribute("hotspot_y",   QString("%1").arg(_hotspot.y()));
	root.setAttribute("orientation", ori.toString());
	root.setAttribute("version", QET::version);
	if (internal_connections) root.setAttribute("ic", "true");
	
	// noms de l'element
	root.appendChild(_names.toXml(xml_document));
	
	QDomElement description = xml_document.createElement("description");
	// description de l'element
	foreach(QGraphicsItem *qgi, zItems(true)) {
		if (CustomElementPart *ce = dynamic_cast<CustomElementPart *>(qgi)) {
			description.appendChild(ce -> toXml(xml_document));
		}
	}
	root.appendChild(description);
	
	xml_document.appendChild(root);
	return(xml_document);
}

/**
	Lit un element depuis un document XML
	@param xml_document un document XML decrivant l'element
*/
void ElementScene::fromXml(const QDomDocument &xml_document) {
	
	QString error_message;
	bool state = true;
	
	// la racine est supposee etre une definition d'element
	QDomElement root = xml_document.documentElement();
	if (root.tagName() != "definition" || root.attribute("type") != "element") {
		state = false;
		error_message = tr("Ce document XML n'est pas une definition d'\351l\351ment.");
	}
	
	// dimensions et hotspot
	if (state) {
		// ces attributs doivent etre presents et valides
		int w, h, hot_x, hot_y;
		if (
			!QET::attributeIsAnInteger(root, QString("width"),     &w) ||\
			!QET::attributeIsAnInteger(root, QString("height"),    &h) ||\
			!QET::attributeIsAnInteger(root, QString("hotspot_x"), &hot_x) ||\
			!QET::attributeIsAnInteger(root, QString("hotspot_y"), &hot_y)
		) {
			state = false;
			error_message = tr("Les dimensions ou le point de saisie ne sont pas valides.");
		} else {
			setWidth(w);
			setHeight(h);
			setHotspot(QPoint(hot_x, hot_y));
		}
	}
	
	// orientations et connexions internes
	if (state) {
		internal_connections = (root.attribute("ic") == "true");
		
		if (!ori.fromString(root.attribute("orientation"))) {
			state = false;
			error_message = tr("Les orientations ne sont pas valides.");
		}
	}
	
	// extrait les noms de la definition XML
	if (state) {
		_names.fromXml(root);
	}
	
	// parcours des enfants de la definition : parties de l'element
	if (state) {
		for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
			QDomElement elmts = node.toElement();
			if (elmts.isNull()) continue;
			if (elmts.tagName() == "description") {
				// gestion de la description graphique de l'element
				//  = parcours des differentes parties du dessin
				int z = 1;
				for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
					QDomElement qde = n.toElement();
					if (qde.isNull()) continue;
					CustomElementPart *cep;
					if      (qde.tagName() == "line")     cep = new PartLine     (element_editor, 0, this);
					else if (qde.tagName() == "ellipse")  cep = new PartEllipse  (element_editor, 0, this);
					else if (qde.tagName() == "circle")   cep = new PartCircle   (element_editor, 0, this);
					else if (qde.tagName() == "polygon")  cep = new PartPolygon  (element_editor, 0, this);
					else if (qde.tagName() == "terminal") cep = new PartTerminal (element_editor, 0, this);
					else if (qde.tagName() == "text")     cep = new PartText     (element_editor, 0, this);
					else if (qde.tagName() == "input")    cep = new PartTextField(element_editor, 0, this);
					else if (qde.tagName() == "arc")      cep = new PartArc      (element_editor, 0, this);
					else continue;
					if (QGraphicsItem *qgi = dynamic_cast<QGraphicsItem *>(cep)) qgi -> setZValue(z++);
					cep -> fromXml(qde);
				}
			}
		}
	}
}

/**
	@return un rectangle englobant toutes les parties ainsi que le
	"bounding rect" de l'element
*/
QRectF ElementScene::sceneContent() const {
	return(itemsBoundingRect().unite(QRectF(-_hotspot, QSizeF(width(), height()))));
}

/**
	@return la pile d'annulations de cet editeur d'element
*/
QUndoStack &ElementScene::undoStack() {
	return(undo_stack);
}

/**
	@return le gestionnaire de QGraphicsItem de cet editeur d'element
*/
QGIManager &ElementScene::qgiManager() {
	return(qgi_manager);
}

/**
	Detecte les changements de selection
*/
void ElementScene::slot_checkSelectionChanged() {
	static QList<QGraphicsItem *> cache_selecteditems = QList<QGraphicsItem *>();
	QList<QGraphicsItem *> selecteditems = selectedItems();
	if (cache_selecteditems != selecteditems) emit(selectionChanged());
	cache_selecteditems = selecteditems;
}

/**
	Selectionne tout
*/
void ElementScene::slot_selectAll() {
	foreach(QGraphicsItem *qgi, items()) qgi -> setSelected(true);
}

/**
	Deselectionne tout
*/
void ElementScene::slot_deselectAll() {
	clearSelection();
}

/**
	Inverse la selection
*/
void ElementScene::slot_invertSelection() {
	foreach(QGraphicsItem *qgi, items()) qgi -> setSelected(!qgi -> isSelected());
}

/**
	Supprime les elements selectionnes
*/
void ElementScene::slot_delete() {
	// verifie qu'il y a qqc de selectionne
	QList<QGraphicsItem *> selected_items = selectedItems();
	if (selected_items.isEmpty()) return;
	
	// efface tout ce qui est selectionne
	undo_stack.push(new DeletePartsCommand(this, selected_items));
	emit(partsRemoved());
}

/**
	Lance un dialogue pour editer les dimensions et le point de saisie
	(hotspot) de l'element.
*/
void ElementScene::slot_editSizeHotSpot() {
	// cree un dialogue
	QDialog dialog_sh;
	dialog_sh.setModal(true);
	dialog_sh.setMinimumSize(400, 230);
	dialog_sh.setWindowTitle(tr("\311diter la taille et le point de saisie"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog_sh);
	
	// ajoute un HotspotEditor au dialogue
	HotspotEditor *hotspot_editor = new HotspotEditor();
	hotspot_editor -> setElementWidth(static_cast<uint>(width() / 10));
	hotspot_editor -> setElementHeight(static_cast<uint>(height() / 10));
	hotspot_editor -> setHotspot(hotspot());
	hotspot_editor -> setOldHotspot(hotspot());
	hotspot_editor -> setPartsRect(itemsBoundingRect());
	hotspot_editor -> setPartsRectEnabled(true);
	dialog_layout -> addWidget(hotspot_editor);
	
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),    &dialog_sh, SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),    &dialog_sh, SLOT(reject()));
	
	// lance le dialogue
	if (dialog_sh.exec() != QDialog::Accepted) return;
	QSize new_size(hotspot_editor -> elementSize());
	QSize old_size(width(), height());
	QPoint new_hotspot(hotspot_editor -> hotspot());
	QPoint old_hotspot(_hotspot);
	
	if (new_size != old_size || new_hotspot != old_hotspot) {
		undo_stack.push(new ChangeHotspotCommand(this, old_size, new_size, old_hotspot, new_hotspot, hotspot_editor -> offsetParts()));
	}
}

/**
	Lance un dialogue pour editer les noms de cete element
*/
void ElementScene::slot_editOrientations() {
	
	// cree un dialogue
	QDialog dialog_ori;
	dialog_ori.setModal(true);
	dialog_ori.setFixedSize(400, 260);
	dialog_ori.setWindowTitle(tr("\311diter les orientations"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog_ori);
	
	// ajoute un champ explicatif au dialogue
	QLabel *information_label = new QLabel(tr("L'orientation par d\351faut est l'orientation dans laquelle s'effectue la cr\351ation de l'\351l\351ment."));
	information_label -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	information_label -> setWordWrap(true);
	dialog_layout -> addWidget(information_label);
	
	// ajoute un OrientationSetWidget au dialogue
	OrientationSetWidget *ori_widget = new OrientationSetWidget();
	ori_widget -> setOrientationSet(ori);
	dialog_layout -> addWidget(ori_widget);
	
	// ajoute une case a cocher pour les connexions internes
	QCheckBox *ic_checkbox = new QCheckBox(tr("Autoriser les connexions internes"));
	ic_checkbox -> setChecked(internal_connections);
	dialog_layout -> addWidget(ic_checkbox);
	
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),    &dialog_ori, SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),    &dialog_ori, SLOT(reject()));
	
	// lance le dialogue
	if (dialog_ori.exec() == QDialog::Accepted) {
		OrientationSet new_ori = ori_widget -> orientationSet();
		if (new_ori != ori) {
			undoStack().push(new ChangeOrientationsCommand(this, ori, new_ori));
		}
		if (ic_checkbox -> isChecked() != internal_connections) {
			undoStack().push(new AllowInternalConnectionsCommand(this, ic_checkbox -> isChecked()));
		}
	}
}

/**
	Lance un dialogue pour editer les noms de cet element
*/
void ElementScene::slot_editNames() {
	
	// cree un dialogue
	QDialog dialog;
	dialog.setModal(true);
	dialog.setFixedSize(400, 330);
	dialog.setWindowTitle(tr("\311diter les noms"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog);
	
	// ajoute un champ explicatif au dialogue
	QLabel *information_label = new QLabel(tr("Vous pouvez sp\351cifier le nom de l'\351l\351ment dans plusieurs langues."));
	information_label -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	information_label -> setWordWrap(true);
	dialog_layout -> addWidget(information_label);
	
	// ajoute un NamesListWidget au dialogue
	NamesListWidget *names_widget = new NamesListWidget();
	names_widget -> setNames(_names);
	dialog_layout -> addWidget(names_widget);
	
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),     names_widget, SLOT(check()));
	connect(names_widget,   SIGNAL(inputChecked()), &dialog,      SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),     &dialog,      SLOT(reject()));
	
	// lance le dialogue
	if (dialog.exec() == QDialog::Accepted) {
		NamesList new_names(names_widget -> names());
		if (new_names != _names) undoStack().push(new ChangeNamesCommand(this, _names, new_names));
	}
}

/**
	Amene les elements selectionnes au premier plan
*/
void ElementScene::slot_bringForward() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::BringForward));
	emit(partsZValueChanged());
}

/**
	Remonte les elements selectionnes d'un plan
*/
void ElementScene::slot_raise() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::Raise));
	emit(partsZValueChanged());
}

/**
	Descend les elements selectionnes d'un plan
*/
void ElementScene::slot_lower() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::Lower));
	emit(partsZValueChanged());
}

/**
	Envoie les elements selectionnes au fond
*/
void ElementScene::slot_sendBackward() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::SendBackward));
	emit(partsZValueChanged());
}

/**
	@param include_terminals true pour inclure les bornes, false sinon
	@return les parties de l'element ordonnes par zValue croissante
*/
QList<QGraphicsItem *> ElementScene::zItems(bool include_terminals) const {
	// recupere les elements
	QList<QGraphicsItem *> all_items_list(items());
	
	// enleve les bornes
	QList<QGraphicsItem *> terminals;
	foreach(QGraphicsItem *qgi, all_items_list) {
		if (qgraphicsitem_cast<PartTerminal *>(qgi)) {
			all_items_list.removeAt(all_items_list.indexOf(qgi));
			terminals << qgi;
		}
	}
	
	// ordonne les parties par leur zValue
	QMultiMap<qreal, QGraphicsItem *> mm;
	foreach(QGraphicsItem *qgi, all_items_list) mm.insert(qgi -> zValue(), qgi);
	all_items_list.clear();
	QMapIterator<qreal, QGraphicsItem *> i(mm);
	while (i.hasNext()) {
		i.next();
		all_items_list << i.value();
	}
	
	// rajoute eventuellement les bornes
	if (include_terminals) all_items_list += terminals;
	return(all_items_list);
}

/**
	Supprime les parties de l'element et les objets d'annulations.
	Les autres caracteristiques sont conservees.
*/
void ElementScene::reset() {
	// supprime les objets d'annulation
	undoStack().clear();
	// enleve les elements de la scene
	foreach (QGraphicsItem *qgi, items()) {
		removeItem(qgi);
		qgiManager().release(qgi);
	}
}

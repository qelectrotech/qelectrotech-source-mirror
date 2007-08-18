#include "editorscene.h"
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
#define GRILLE_X 10
#define GRILLE_Y 10

EditorScene::EditorScene(QObject *parent) :
	QGraphicsScene(parent),
	_width(3),
	_height(7),
	_hotspot(15, 35)
{
	current_polygon = NULL;
	connect(this, SIGNAL(changed(const QList<QRectF> &)), this, SLOT(slot_checkSelectionChanged()));
}

EditorScene::~EditorScene() {
	qDebug() << "~EditorScene()";
}

void EditorScene::slot_move() {
	behavior = Normal;
}

void EditorScene::slot_addLine() {
	behavior = Line;
}

void EditorScene::slot_addCircle() {
	behavior = Circle;
}

void EditorScene::slot_addEllipse() {
	behavior = Ellipse;
}

void EditorScene::slot_addPolygon() {
	behavior = Polygon;
}

void EditorScene::slot_addText() {
	behavior = Text;
}

void EditorScene::slot_addTerminal() {
	behavior = Terminal;
}

void EditorScene::slot_addArc() {
	behavior = Arc;
}

void EditorScene::slot_addTextField() {
	behavior = TextField;
}

void EditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	QRectF temp_rect;
	qreal radius;
	QPointF temp_point;
	QPolygonF temp_polygon;
	if (e -> buttons() & Qt::LeftButton) {
		switch(behavior) {
			case Normal:
				QGraphicsScene::mouseMoveEvent(e);
				break;
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

void EditorScene::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	QPolygonF temp_polygon;
	if (e -> button() & Qt::LeftButton) {
		switch(behavior) {
			case Normal:
				QGraphicsScene::mousePressEvent(e);
				break;
			case Line:
				current_line = new PartLine(0, this);
				current_line -> setLine(QLineF(e -> scenePos(), e -> scenePos()));
				break;
			case Ellipse:
				current_ellipse = new PartEllipse(0, this);
				current_ellipse -> setRect(QRectF(e -> scenePos(), QSizeF(0.0, 0.0)));
				break;
			case Arc:
				current_arc = new PartArc(0, this);
				current_arc -> setRect(QRectF(e -> scenePos(), QSizeF(0.0, 0.0)));
				break;
			case Circle:
				current_circle = new PartCircle(0, this);
				current_circle -> setRect(QRectF(e -> scenePos(), QSizeF(0.0, 0.0)));
				break;
			case Polygon:
				if (current_polygon == NULL) {
					current_polygon = new PartPolygon(0, this);
					temp_polygon = QPolygonF(0);
				} else temp_polygon = current_polygon -> polygon();
				// au debut, on insere deux points
				if (!temp_polygon.count()) temp_polygon << e -> scenePos();
				temp_polygon << e -> scenePos();
				current_polygon -> setPolygon(temp_polygon);
				break;
			default:
				QGraphicsScene::mousePressEvent(e);
		}
	} else QGraphicsScene::mousePressEvent(e);
}

void EditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	PartTerminal *terminal;
	PartText *text;
	PartTextField *textfield;
	if (behavior != Polygon && current_polygon != NULL) current_polygon = NULL;
	if (e -> button() & Qt::LeftButton) {
		switch(behavior) {
			case Normal:
				QGraphicsScene::mouseReleaseEvent(e);
				break;
			case Line:
				break;
			case Ellipse:
				current_ellipse -> setRect(current_ellipse -> rect().normalized());
				break;
			case Arc:
				current_arc-> setRect(current_arc -> rect().normalized());
				break;
			case Circle:
				current_circle -> setRect(current_circle -> rect().normalized());
				break;
			case Terminal:
				terminal = new PartTerminal(0, this);
				terminal -> setPos(e -> scenePos());
				break;
			case Text:
				text = new PartText(0, this);
				text -> setPos(e -> scenePos());
				break;
			case TextField:
				textfield = new PartTextField(0, this);
				textfield -> setPos(e -> scenePos());
				break;
			default:
				QGraphicsScene::mouseReleaseEvent(e);
		}
	} else if (e -> button() & Qt::RightButton) {
		if (behavior == Polygon) {
			behavior = Normal;
			current_polygon = NULL;
			emit(needNormalMode());
		} else QGraphicsScene::mouseReleaseEvent(e);
	} else QGraphicsScene::mouseReleaseEvent(e);
}

/**
	Dessine l'arriere-plan de l'editeur, cad la grille.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void EditorScene::drawBackground(QPainter *p, const QRectF &r) {
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
		while (g_x % GRILLE_X) ++ g_x;
		int g_y = (int)ceil(r.y());
		while (g_y % GRILLE_Y) ++ g_y;
		
		for (int gx = g_x ; gx < limite_x ; gx += GRILLE_X) {
			for (int gy = g_y ; gy < limite_y ; gy += GRILLE_Y) {
				p -> drawPoint(gx, gy);
			}
		}
	}
	p -> restore();
}

/**
	Dessine l'arriere-plan de l'editeur, cad l'indicateur de hotspotó.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void EditorScene::drawForeground(QPainter *p, const QRectF &) {
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

const QDomDocument EditorScene::toXml() const {
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
	
	// noms de l'element
	root.appendChild(_names.toXml(xml_document));
	
	QDomElement description = xml_document.createElement("description");
	// description de l'element
	foreach(QGraphicsItem *qgi, items()) {
		if (CustomElementPart *ce = dynamic_cast<CustomElementPart *>(qgi)) {
			description.appendChild(ce -> toXml(xml_document));
		}
	}
	root.appendChild(description);
	
	xml_document.appendChild(root);
	return(xml_document);
}

void EditorScene::fromXml(const QDomDocument &xml_document) {
	
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
	
	// orientations
	if (state) {
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
					if      (qde.tagName() == "line")     cep = new PartLine     (0, this);
					else if (qde.tagName() == "ellipse")  cep = new PartEllipse  (0, this);
					else if (qde.tagName() == "circle")   cep = new PartCircle   (0, this);
					else if (qde.tagName() == "polygon")  cep = new PartPolygon  (0, this);
					else if (qde.tagName() == "terminal") cep = new PartTerminal (0, this);
					else if (qde.tagName() == "text")     cep = new PartText     (0, this);
					else if (qde.tagName() == "input")    cep = new PartTextField(0, this);
					else if (qde.tagName() == "arc")      cep = new PartArc      (0, this);
					else continue;
					if (QGraphicsItem *qgi = dynamic_cast<QGraphicsItem *>(cep)) qgi -> setZValue(z++);
					cep -> fromXml(qde);
				}
			}
		}
	}
}

void EditorScene::slot_checkSelectionChanged() {
	static QList<QGraphicsItem *> cache_selecteditems = QList<QGraphicsItem *>();
	QList<QGraphicsItem *> selecteditems = selectedItems();
	if (cache_selecteditems != selecteditems) emit(selectionChanged());
	cache_selecteditems = selecteditems;
}

void EditorScene::slot_selectAll() {
	foreach(QGraphicsItem *qgi, items()) qgi -> setSelected(true);
}

void EditorScene::slot_deselectAll() {
	clearSelection();
}

void EditorScene::slot_invertSelection() {
	foreach(QGraphicsItem *qgi, items()) qgi -> setSelected(!qgi -> isSelected());
}

void EditorScene::slot_delete() {
	// verifie qu'il y a qqc de selectionne
	QList<QGraphicsItem *> selected_items = selectedItems();
	if (selected_items.isEmpty()) return;
	
	// efface tout ce qui est selectionne
	foreach(QGraphicsItem *qgi, selected_items) {
		removeItem(qgi);
		delete qgi;
	}
}

void EditorScene::slot_editSizeHotSpot() {
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
	if (dialog_sh.exec() == QDialog::Accepted) {
		setWidth(hotspot_editor -> elementWidth());
		setHeight(hotspot_editor -> elementHeight());
		setHotspot(hotspot_editor -> hotspot());
		if (hotspot_editor -> mustTranslateParts()) {
			QPoint translation = hotspot_editor -> offsetParts();
			foreach(QGraphicsItem *qgi, items()) {
				qgi -> translate(translation.x(), translation.y());
			}
		}
	}
}

void EditorScene::slot_editOrientations() {
	
	// cree un dialogue
	QDialog dialog_ori;
	dialog_ori.setModal(true);
	dialog_ori.setFixedSize(400, 230);
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
	
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),    &dialog_ori, SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),    &dialog_ori, SLOT(reject()));
	
	// lance le dialogue
	if (dialog_ori.exec() == QDialog::Accepted) ori = ori_widget -> orientationSet();
}

void EditorScene::slot_editNames() {
	
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
	if (dialog.exec() == QDialog::Accepted) _names = names_widget -> names();
}


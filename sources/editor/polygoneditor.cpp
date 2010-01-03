/*
	Copyright 2006-2010 Xavier Guerrin
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
#include "polygoneditor.h"
#include "partpolygon.h"
#include "elementscene.h"
#include "editorcommands.h"
#include "qetmessagebox.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param p Le polygone a editer
	@param parent le Widget parent
*/
PolygonEditor::PolygonEditor(QETElementEditor *editor, PartPolygon *p, QWidget *parent) :
	ElementItemEditor(editor, parent),
	points_list(this),
	close_polygon(tr("Polygone ferm\351"), this)
{
	part = p;
	// prepare la liste de points
	points_list.setColumnCount(2);
	QStringList headers;
	headers << tr("x") << tr("y");
	points_list.setHeaderLabels(headers);
	points_list.setRootIsDecorated(false);
	updateForm();
	
	// layout
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout -> addWidget(new QLabel(tr("Points du polygone :")));
	layout -> addWidget(&points_list);
	layout -> addWidget(&close_polygon);
	
	updateForm();
}

/// Destructeur
PolygonEditor::~PolygonEditor() {
}

/**
	Met a jour le polygone a partir des donnees du formulaire : points et etat ferme ou non
*/
void PolygonEditor::updatePolygon() {
	updatePolygonPoints();
	updatePolygonClosedState();
}

/**
	Met a jour les points du polygone et cree un objet d'annulation
*/
void PolygonEditor::updatePolygonPoints() {
	QVector<QPointF> points = getPointsFromTree();
	if (points.count() < 2) {
		QET::MessageBox::warning(
			this,
			tr("Erreur", "message box title"),
			tr("Le polygone doit comporter au moins deux points.", "message box content")
		);
		return;
	}
	undoStack().push(new ChangePolygonPointsCommand(part, part -> polygon(), points));
}

/**
	Met a jour l'etat ferme ou non du polygone
*/
void PolygonEditor::updatePolygonClosedState() {
	undoStack().push(
		new ChangePartCommand(
			tr("fermeture du polygone"),
			part,
			"closed",
			QVariant(!close_polygon.isChecked()),
			QVariant(close_polygon.isChecked())
		)
	);
}

/**
	Met a jour le formulaire d'edition
*/
void PolygonEditor::updateForm() {
	activeConnections(false);
	while(points_list.takeTopLevelItem(0)) {}
	foreach(QPointF point, part -> polygon()) {
		point = part -> mapToScene(point);
		QStringList qsl;
		qsl << QString("%1").arg(point.x()) << QString("%1").arg(point.y());
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(qsl);
		qtwi -> setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		points_list.addTopLevelItem(qtwi);
	}
	close_polygon.setChecked(part -> isClosed());
	activeConnections(true);
}

/**
	@return Un vecteur contenant les points composant le polygone a partir du
	formulaire d'edition
*/
QVector<QPointF> PolygonEditor::getPointsFromTree() {
	QVector<QPointF> points;
	for(int i = 0 ; i < points_list.topLevelItemCount() ; ++ i) {
		QTreeWidgetItem *qtwi = points_list.topLevelItem(i);
		bool x_convert_ok, y_convert_ok;
		qreal x = qtwi -> text(0).toDouble(&x_convert_ok);
		qreal y = qtwi -> text(1).toDouble(&y_convert_ok);
		if (!x_convert_ok || !y_convert_ok) continue;
		points << part -> mapFromScene(QPointF(x, y));
	}
	return(points);
}

/**
	@param qtwi QTreeWidgetItem a valider
	@param column Colonne exacte du QTreeWidgetItem a valider
*/
void PolygonEditor::validColumn(QTreeWidgetItem *qtwi, int column) {
	bool convert_ok;
	qtwi -> text(column).toDouble(&convert_ok);
	if (convert_ok) {
		points_list.closePersistentEditor(qtwi, column);
		updatePolygonPoints();
	} else points_list.openPersistentEditor(qtwi, column);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void PolygonEditor::activeConnections(bool active) {
	if (active) {
		connect(&close_polygon, SIGNAL(stateChanged(int)),                   this, SLOT(updatePolygonClosedState()));
		connect(&points_list,   SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(validColumn(QTreeWidgetItem *, int)));
	} else {
		disconnect(&close_polygon, SIGNAL(stateChanged(int)),                   this, SLOT(updatePolygonClosedState()));
		disconnect(&points_list,   SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(validColumn(QTreeWidgetItem *, int)));
	}
}

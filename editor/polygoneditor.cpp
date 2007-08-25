#include "polygoneditor.h"
#include "partpolygon.h"
#include "elementscene.h"
#include "editorcommands.h"

/**
	Constructeur
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
	
	// connexions signaux/slots
	connect(&close_polygon, SIGNAL(stateChanged(int)),                   this, SLOT(updatePolygonClosedState()));
	connect(&points_list,   SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(validColumn(QTreeWidgetItem *, int)));
}

void PolygonEditor::updatePolygon() {
	updatePolygonPoints();
	updatePolygonClosedState();
}

void PolygonEditor::updatePolygonPoints() {
	QVector<QPointF> points = getPointsFromTree();
	if (points.count() < 2) {
		QMessageBox::warning(
			this,
			tr("Erreur"),
			tr("Le polygone doit comporter au moins deux points.")
		);
		return;
	}
	undoStack().push(new ChangePolygonPointsCommand(part, part -> polygon(), points));
}

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

void PolygonEditor::updateForm() {
	while(points_list.takeTopLevelItem(0));
	foreach(QPointF point, part -> polygon()) {
		point = part -> mapToScene(point);
		QStringList qsl;
		qsl << QString("%1").arg(point.x()) << QString("%1").arg(point.y());
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(qsl);
		qtwi -> setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		points_list.addTopLevelItem(qtwi);
	}
	close_polygon.setChecked(part -> isClosed());
}

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

void PolygonEditor::validColumn(QTreeWidgetItem *qtwi, int column) {
	bool convert_ok;
	qtwi -> text(column).toDouble(&convert_ok);
	if (convert_ok) {
		points_list.closePersistentEditor(qtwi, column);
		updatePolygonPoints();
	} else points_list.openPersistentEditor(qtwi, column);
}

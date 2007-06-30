#include "lineeditor.h"
#include "partline.h"

LineEditor::LineEditor(PartLine *line, QWidget *parent) : QWidget(parent) {
	
	part = line;
	
	x1 = new QLineEdit();
	y1 = new QLineEdit();
	x2 = new QLineEdit();
	y2 = new QLineEdit();
	
	QGridLayout *grid = new QGridLayout(this);
	grid -> addWidget(new QLabel("x1"), 0, 0);
	grid -> addWidget(x1,               0, 1);
	grid -> addWidget(new QLabel("y1"), 0, 2);
	grid -> addWidget(y1,               0, 3);
	grid -> addWidget(new QLabel("x2"), 1, 0);
	grid -> addWidget(x2,               1, 1);
	grid -> addWidget(new QLabel("y2"), 1, 2);
	grid -> addWidget(y2,               1, 3);
	
	updateForm();
	
	connect(x1, SIGNAL(editingFinished()), this, SLOT(updateLine()));
	connect(y1, SIGNAL(editingFinished()), this, SLOT(updateLine()));
	connect(x2, SIGNAL(editingFinished()), this, SLOT(updateLine()));
	connect(y2, SIGNAL(editingFinished()), this, SLOT(updateLine()));
}

LineEditor::~LineEditor() {
	qDebug() << "~LineEditor()";
}

void LineEditor::updateLine() {
	part -> setLine(
		QLineF(
			part -> mapFromScene(
				x1 -> text().toDouble(),
				y1 -> text().toDouble()
			),
			part -> mapFromScene(
				x2 -> text().toDouble(),
				y2 -> text().toDouble()
			)
		)
	);
}

void LineEditor::updateForm() {
	QPointF p1(part -> sceneP1());
	QPointF p2(part -> sceneP2());
	x1 -> setText(QString("%1").arg(p1.x()));
	y1 -> setText(QString("%1").arg(p1.y()));
	x2 -> setText(QString("%1").arg(p2.x()));
	y2 -> setText(QString("%1").arg(p2.y()));
}

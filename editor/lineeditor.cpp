#include "lineeditor.h"
#include "partline.h"

LineEditor::LineEditor(QETElementEditor *editor, PartLine *line, QWidget *parent) : ElementItemEditor(editor, parent) {
	
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
}

LineEditor::~LineEditor() {
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

void LineEditor::updateLineX1() { addChangePartCommand(tr("abscisse point 1"),    part, "x1", x1 -> text().toDouble()); }
void LineEditor::updateLineY1() { addChangePartCommand(tr("ordonn\351e point 1"), part, "y1", y1 -> text().toDouble()); }
void LineEditor::updateLineX2() { addChangePartCommand(tr("abscisse point 2"),    part, "x2", x2 -> text().toDouble()); }
void LineEditor::updateLineY2() { addChangePartCommand(tr("ordonn\351e point 2"), part, "y2", y2 -> text().toDouble()); }

void LineEditor::updateForm() {
	activeConnections(false);
	QPointF p1(part -> sceneP1());
	QPointF p2(part -> sceneP2());
	x1 -> setText(QString("%1").arg(p1.x()));
	y1 -> setText(QString("%1").arg(p1.y()));
	x2 -> setText(QString("%1").arg(p2.x()));
	y2 -> setText(QString("%1").arg(p2.y()));
	activeConnections(true);
}

void LineEditor::activeConnections(bool active) {
	if (active) {
		connect(x1, SIGNAL(editingFinished()), this, SLOT(updateLineX1()));
		connect(y1, SIGNAL(editingFinished()), this, SLOT(updateLineY1()));
		connect(x2, SIGNAL(editingFinished()), this, SLOT(updateLineX2()));
		connect(y2, SIGNAL(editingFinished()), this, SLOT(updateLineY2()));
	} else {
		connect(x1, SIGNAL(editingFinished()), this, SLOT(updateLineX1()));
		connect(y1, SIGNAL(editingFinished()), this, SLOT(updateLineY1()));
		connect(x2, SIGNAL(editingFinished()), this, SLOT(updateLineX2()));
		connect(y2, SIGNAL(editingFinished()), this, SLOT(updateLineY2()));
	}
}

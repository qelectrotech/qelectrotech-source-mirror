#include "circleeditor.h"
#include "partcircle.h"

CircleEditor::CircleEditor(PartCircle *circle, QWidget *parent) : QWidget(parent) {
	
	part = circle;
	
	x = new QLineEdit();
	y = new QLineEdit();
	r = new QLineEdit();
	
// 	QDoubleValidator *format = new QDoubleValidator(-1000.0, -1000.0, 4, this);
// 	x -> setValidator(new QDoubleValidator(-1000.0, 1000.0, 4, this));
// 	y -> setValidator(new QDoubleValidator(-1000.0, 1000.0, 4, this));
// 	h -> setValidator(new QDoubleValidator(0.0, 1000.0, 4, this));
	
	
	QGridLayout *grid = new QGridLayout(this);
	grid -> addWidget(new QLabel(tr("Centre : ")),       0, 0);
	grid -> addWidget(new QLabel("x"),                   1, 0);
	grid -> addWidget(x,                                 1, 1);
	grid -> addWidget(new QLabel("y"),                   1, 2);
	grid -> addWidget(y,                                 1, 3);
	grid -> addWidget(new QLabel(tr("Diam\350tre : ")),  2, 0);
	grid -> addWidget(r,                                 2, 1);
	
	updateForm();
	
	connect(x, SIGNAL(editingFinished()), this, SLOT(updateCircle()));
	connect(y, SIGNAL(editingFinished()), this, SLOT(updateCircle()));
	connect(r, SIGNAL(editingFinished()), this, SLOT(updateCircle()));
}

CircleEditor::~CircleEditor() {
	qDebug() << "~CircleEditor()";
}

void CircleEditor::updateCircle() {
	qreal _x = x -> text().toDouble();
	qreal _y = y -> text().toDouble();
	qreal _d = r -> text().toDouble();
	part -> setRect(
		QRectF(
			part -> mapFromScene(QPointF(_x - _d / 2.0, _y - _d / 2.0)),
			QSizeF(_d, _d)
		)
	);
}

void CircleEditor::updateForm() {
	qreal _d = part -> rect().width();
	QPointF top_left(part -> sceneTopLeft());
	x -> setText(QString("%1").arg(top_left.x() + _d / 2.0));
	y -> setText(QString("%1").arg(top_left.y() + _d / 2.0));
	r -> setText(QString("%1").arg(_d));
}

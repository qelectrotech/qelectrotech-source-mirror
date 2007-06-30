#include "ellipseeditor.h"
#include "partellipse.h"

EllipseEditor::EllipseEditor(PartEllipse *ellipse, QWidget *parent) : QWidget(parent) {
	
	part = ellipse;
	
	x = new QLineEdit();
	y = new QLineEdit();
	h = new QLineEdit();
	v = new QLineEdit();
	
// 	QDoubleValidator *format = new QDoubleValidator(-1000.0, -1000.0, 4, this);
// 	x -> setValidator(new QDoubleValidator(-1000.0, 1000.0, 4, this));
// 	y -> setValidator(new QDoubleValidator(-1000.0, 1000.0, 4, this));
// 	h -> setValidator(new QDoubleValidator(0.0, 1000.0, 4, this));
// 	v -> setValidator(new QDoubleValidator(0.0, 1000.0, 4, this));
	
	
	QGridLayout *grid = new QGridLayout(this);
	grid -> addWidget(new QLabel(tr("Centre : ")),       0, 0);
	grid -> addWidget(new QLabel("x"),                   1, 0);
	grid -> addWidget(x,                                 1, 1);
	grid -> addWidget(new QLabel("y"),                   1, 2);
	grid -> addWidget(y,                                 1, 3);
	grid -> addWidget(new QLabel(tr("Diam\350tres : ")), 2, 0);
	grid -> addWidget(new QLabel(tr("horizontal :")),    3, 0);
	grid -> addWidget(h,                                 3, 1);
	grid -> addWidget(new QLabel(tr("vertical :")),      4, 0);
	grid -> addWidget(v,                                 4, 1);
	
	updateForm();
	
	connect(x, SIGNAL(editingFinished()), this, SLOT(updateEllipse()));
	connect(y, SIGNAL(editingFinished()), this, SLOT(updateEllipse()));
	connect(h, SIGNAL(editingFinished()), this, SLOT(updateEllipse()));
	connect(v, SIGNAL(editingFinished()), this, SLOT(updateEllipse()));
}

EllipseEditor::~EllipseEditor() {
	qDebug() << "~EllipseEditor()";
}

void EllipseEditor::updateEllipse() {
	qreal _x = x -> text().toDouble();
	qreal _y = y -> text().toDouble();
	qreal _h = h -> text().toDouble();
	qreal _v = v -> text().toDouble();
	_v = _v < 0 ? -_v : _v;
	part -> setRect(
		QRectF(
			part -> mapFromScene(QPointF(_x - (_h / 2.0), _y - (_v / 2.0))),
			QSizeF(_h, _v)
		)
	);
}

void EllipseEditor::updateForm() {
	qreal _h = part -> rect().width();
	qreal _v = part -> rect().height();
	QPointF top_left(part -> sceneTopLeft());
	x -> setText(QString("%1").arg(top_left.x() + (_h / 2.0)));
	y -> setText(QString("%1").arg(top_left.y() + (_v / 2.0)));
	h -> setText(QString("%1").arg(_h));
	v -> setText(QString("%1").arg(_v));
}

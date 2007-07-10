#include "arceditor.h"
#include "partarc.h"

ArcEditor::ArcEditor(PartArc *arc, QWidget *parent) : QWidget(parent) {
	
	part = arc;
	
	x = new QLineEdit();
	y = new QLineEdit();
	h = new QLineEdit();
	v = new QLineEdit();
	start_angle = new QSpinBox();
	angle       = new QSpinBox();
	start_angle -> setRange(-360, 360);
	angle       -> setRange(-360, 360);
	
	QGridLayout *grid = new QGridLayout(this);
	grid -> addWidget(new QLabel(tr("Centre : ")),            0, 0);
	grid -> addWidget(new QLabel("x"),                        1, 0);
	grid -> addWidget(x,                                      1, 1);
	grid -> addWidget(new QLabel("y"),                        1, 2);
	grid -> addWidget(y,                                      1, 3);
	grid -> addWidget(new QLabel(tr("Diam\350tres : ")),      2, 0);
	grid -> addWidget(new QLabel(tr("horizontal :")),         3, 0);
	grid -> addWidget(h,                                      3, 1);
	grid -> addWidget(new QLabel(tr("vertical :")),           4, 0);
	grid -> addWidget(v,                                      4, 1);
	grid -> addWidget(new QLabel(tr("Angle de d\351part :")), 5, 0);
	grid -> addWidget(start_angle,                            5, 1);
	grid -> addWidget(new QLabel(tr("Angle :")),              6, 0);
	grid -> addWidget(angle,                                  6, 1);
	updateForm();
	
	connect(x,           SIGNAL(editingFinished()), this, SLOT(updateArc()));
	connect(y,           SIGNAL(editingFinished()), this, SLOT(updateArc()));
	connect(h,           SIGNAL(editingFinished()), this, SLOT(updateArc()));
	connect(v,           SIGNAL(editingFinished()), this, SLOT(updateArc()));
	connect(start_angle, SIGNAL(valueChanged(int)), this, SLOT(updateArc()));
	connect(angle,       SIGNAL(valueChanged(int)), this, SLOT(updateArc()));
}

ArcEditor::~ArcEditor() {
	qDebug() << "~ArcEditor()";
}

void ArcEditor::updateArc() {
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
	part -> setStartAngle(-start_angle -> value() + 90);
	part -> setAngle(-angle -> value());
}

void ArcEditor::updateForm() {
	qreal _h = part -> rect().width();
	qreal _v = part -> rect().height();
	QPointF top_left(part -> sceneTopLeft());
	x -> setText(QString("%1").arg(top_left.x() + (_h / 2.0)));
	y -> setText(QString("%1").arg(top_left.y() + (_v / 2.0)));
	h -> setText(QString("%1").arg(_h));
	v -> setText(QString("%1").arg(_v));
	disconnect(start_angle, SIGNAL(valueChanged(int)), this, SLOT(updateArc()));
	disconnect(angle,       SIGNAL(valueChanged(int)), this, SLOT(updateArc()));
	start_angle -> setValue(-part -> startAngle() + 90);
	angle -> setValue(-part -> angle());
	connect(start_angle, SIGNAL(valueChanged(int)), this, SLOT(updateArc()));
	connect(angle,       SIGNAL(valueChanged(int)), this, SLOT(updateArc()));
}

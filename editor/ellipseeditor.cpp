#include "ellipseeditor.h"
#include "partellipse.h"

EllipseEditor::EllipseEditor(QETElementEditor *editor, PartEllipse *ellipse, QWidget *parent) : ElementItemEditor(editor, parent) {
	
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
	
	activeConnections(true);
	updateForm();
}

EllipseEditor::~EllipseEditor() {
}

void EllipseEditor::updateEllipse() {
	part -> setProperty("x", x -> text().toDouble());
	part -> setProperty("y", x -> text().toDouble());
	part -> setProperty("diameter_h", x -> text().toDouble());
	part -> setProperty("diameter_v", x -> text().toDouble());
}

void EllipseEditor::updateEllipseX() { addChangePartCommand(tr("abscisse"),               part, "x",           x -> text().toDouble());       }
void EllipseEditor::updateEllipseY() { addChangePartCommand(tr("ordonn\351e"),            part, "y",           y -> text().toDouble());       }
void EllipseEditor::updateEllipseH() { addChangePartCommand(tr("diam\350tre horizontal"), part, "diameter_h",  h -> text().toDouble());       }
void EllipseEditor::updateEllipseV() { addChangePartCommand(tr("diam\350tre vertical"),   part, "diameter_v",  v -> text().toDouble());       }

void EllipseEditor::updateForm() {
	activeConnections(false);
	x -> setText(part -> property("x").toString());
	y -> setText(part -> property("y").toString());
	h -> setText(part -> property("diameter_h").toString());
	v -> setText(part -> property("diameter_v").toString());
	activeConnections(true);
}

void EllipseEditor::activeConnections(bool active) {
	if (active) {
		connect(x, SIGNAL(editingFinished()), this, SLOT(updateEllipseX()));
		connect(y, SIGNAL(editingFinished()), this, SLOT(updateEllipseY()));
		connect(h, SIGNAL(editingFinished()), this, SLOT(updateEllipseH()));
		connect(v, SIGNAL(editingFinished()), this, SLOT(updateEllipseV()));
	} else {
		disconnect(x, SIGNAL(editingFinished()), this, SLOT(updateEllipseX()));
		disconnect(y, SIGNAL(editingFinished()), this, SLOT(updateEllipseY()));
		disconnect(h, SIGNAL(editingFinished()), this, SLOT(updateEllipseH()));
		disconnect(v, SIGNAL(editingFinished()), this, SLOT(updateEllipseV()));
	}
}

/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "qtextorientationspinboxwidget.h"

/**
	Constructeur
	@param parent QWidget parent de ce QTextOrientationSpinBoxWidget
*/
QTextOrientationSpinBoxWidget::QTextOrientationSpinBoxWidget(QWidget *parent) :
	QWidget(parent)
{
	build();
}

/**
	Destructeur
*/
QTextOrientationSpinBoxWidget::~QTextOrientationSpinBoxWidget() {
}

/**
	@return un pointeur vers le QTextOrientationWidget
*/
QTextOrientationWidget *QTextOrientationSpinBoxWidget::orientationWidget() const {
	return(orientation_widget_);
}

/**
	@return un pointeur vers le QSpinBox
*/
QDoubleSpinBox *QTextOrientationSpinBoxWidget::spinBox() const {
	return(spin_box_);
}

/**
	@return l'orientation en cours
*/
double QTextOrientationSpinBoxWidget::orientation() const {
	return(orientation_widget_ -> orientation());
}

/**
	Synonyme pour orientation()
	@return l'orientation en cours
	@see orientation()
*/
double QTextOrientationSpinBoxWidget::value() const {
	return(orientation());
}

/**
	@return true si le widget est en mode "lecture seule", false sinon
*/
bool QTextOrientationSpinBoxWidget::isReadOnly() const {
	return(orientation_widget_ -> isReadOnly());
}

/**
	@param value Nouvelle valeur de l'orientation a afficher
*/
void QTextOrientationSpinBoxWidget::setOrientation(const double &value) {
	orientation_widget_ -> setOrientation(value);
	spin_box_           -> setValue(value);
}

/**
	Synonyme pour setOrientation(value)
	@param value Nouvelle valeur de l'orientation a afficher
	@see setOrientation
*/
void QTextOrientationSpinBoxWidget::setValue(const double &value) {
	setOrientation(value);
}

/**
	@param ro true pour passer le widget en mode "lecture seule", false sinon
*/
void QTextOrientationSpinBoxWidget::setReadOnly(bool ro) {
	orientation_widget_ -> setReadOnly(ro);
	spin_box_           -> setReadOnly(ro);
}

/**
	Construit le widget
*/
void QTextOrientationSpinBoxWidget::build() {
	orientation_widget_ = new QTextOrientationWidget();
	orientation_widget_ -> setMinimumSize(90.0, 90.0);
	
	spin_box_ = new QDoubleSpinBox();
	spin_box_ -> setRange(-360.0, 360.0);
	spin_box_ -> setSuffix("\260");
	
	// met en place les relations entre le SpinBox et le QTextOrientationWidget
	connect(spin_box_,           SIGNAL(valueChanged(double)),       orientation_widget_, SLOT(setOrientation(double)));
	connect(orientation_widget_, SIGNAL(orientationChanged(double)), spin_box_,           SLOT(setValue(double)));
	
	// cliquer sur un des carres du QTextOrientationWidget revient a finir une saisie dans le SpinBox
	connect(orientation_widget_, SIGNAL(orientationChanged(double)), spin_box_, SIGNAL(editingFinished()));
	
	// lorsque l'utilisateur a change l'orientation, on emet un signal avec la valeur de la nouvelle orientation
	connect(spin_box_, SIGNAL(editingFinished()), this, SLOT(emitChangeSignals()));
	
	// dispose les widgets : le QTextOrientationWidget a gauche, le SpinBox a droite
	QHBoxLayout *main_layout = new QHBoxLayout();
	main_layout -> addWidget(orientation_widget_);
	main_layout -> addWidget(spin_box_);
	main_layout -> addStretch();
	setLayout(main_layout);
}


/**
	Emet le signal orientationEditingFinished avec la valeur de l'orientation en cours
*/
void QTextOrientationSpinBoxWidget::emitChangeSignals() {
	emit(editingFinished(orientation()));
	emit(editingFinished());
}

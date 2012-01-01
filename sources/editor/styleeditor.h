/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef STYLE_EDITOR_H
#define STYLE_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class CustomElementGraphicPart;
/**
	Cette classe represente un widget d'edition des styles que peut comporter
	une partie d'elements (couleur, epaisseur et style du trait, remplissage,
	antialiasing). Elle peut accueillir un widget sous cette interface grace a
	la methode appendWidget.
*/
class StyleEditor : public ElementItemEditor {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	StyleEditor(QETElementEditor *, CustomElementGraphicPart * = 0, QWidget * = 0);
	virtual ~StyleEditor();
	
	private:
	StyleEditor(const StyleEditor &);
	
	// attributs
	private:
	CustomElementGraphicPart *part;
	QVBoxLayout *main_layout;
	QButtonGroup *color, *style, *weight, *filling;
	QRadioButton *black_color, *white_color, *normal_style, *dashed_style, *dotted_style;
	QRadioButton *none_weight, *thin_weight, *normal_weight, *no_filling;
	QRadioButton *black_filling, *white_filling;
	QCheckBox *antialiasing;
	
	// methodes
	public:
	virtual bool setPart(CustomElementPart *);
	virtual CustomElementPart *currentPart() const;
	
	public slots:
	void updatePart();
	void updateForm();
	void updatePartAntialiasing();
	void updatePartColor();
	void updatePartLineStyle();
	void updatePartLineWeight();
	void updatePartFilling();
	
	private:
	void activeConnections(bool);
};
#endif

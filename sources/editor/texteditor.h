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
#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartText;
class QTextOrientationSpinBoxWidget;
/**
	Cette classe represente un editeur de champ de texte non editable
	Elle permet d'editer a travers une interface graphique les
	proprietes d'un champ de texte non editable.
*/
class TextEditor : public ElementItemEditor {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	TextEditor(QETElementEditor *, PartText * = 0, QWidget * = 0);
	virtual ~TextEditor();
	private:
	TextEditor(const TextEditor &);
	
	// attributs
	private:
	PartText *part;
	QLineEdit *qle_x, *qle_y, *qle_text;
	QSpinBox *font_size;
	QButtonGroup *color_;
	QRadioButton *black_color_, *white_color_;
	QTextOrientationSpinBoxWidget *rotation_angle_;
	
	// methodes
	public:
	virtual bool setPart(CustomElementPart *);
	virtual CustomElementPart *currentPart() const;
	
	public slots:
	void updateText();
	void updateTextX();
	void updateTextY();
	void updateTextT();
	void updateTextS();
	void updateTextC();
	void updateTextRotationAngle();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif

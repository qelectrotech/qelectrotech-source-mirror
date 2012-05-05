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
#ifndef TERMINAL_EDITOR_H
#define TERMINAL_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartTerminal;
/**
	Cette classe represente un editeur de borne.
	Elle permet d'editer a travers une interface graphique les
	proprietes d'une borne d'element.
*/
class TerminalEditor : public ElementItemEditor {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	TerminalEditor(QETElementEditor *, PartTerminal * = 0, QWidget * = 0);
	virtual ~TerminalEditor();
	private:
	TerminalEditor(const TerminalEditor &);
	
	// attributs
	private:
	PartTerminal *part;
	QLineEdit *qle_x, *qle_y;
	QComboBox *orientation;
	
	// methodes
	public:
	virtual bool setPart(CustomElementPart *);
	virtual CustomElementPart *currentPart() const;
	
	public slots:
	void updateTerminal();
	void updateTerminalX();
	void updateTerminalY();
	void updateTerminalO();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif

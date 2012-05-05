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
#ifndef DIAGRAMS_CHOOSER_H
#define DIAGRAMS_CHOOSER_H
#include <QtGui>
class QETProject;
class Diagram;
/**
	Cette classe represente un widget permettant de choisir 0 a n schemas parmi
	ceux d'un projet.
*/
class DiagramsChooser : public QScrollArea {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	DiagramsChooser(QETProject *, QWidget * = 0);
	virtual ~DiagramsChooser();
	private:
	DiagramsChooser(const DiagramsChooser &);
	
	// methodes
	public:
	QETProject *project() const;
	QList<Diagram *> selectedDiagrams() const;
	QList<Diagram *> nonSelectedDiagrams() const;
	bool diagramIsSelected(Diagram * const) const;
	void setSelectedDiagrams(const QList<Diagram *> &, bool = true, bool = true);
	void setSelectedAllDiagrams(bool = true);
	
	public slots:
	void updateList();
	
	signals:
	void selectionChanged();
	
	private:
	void buildLayout();
	
	// attributs
	private:
	QETProject *project_;
	QWidget *widget0_;
	QVBoxLayout *vlayout0_;
	QHash<Diagram *, QCheckBox *> diagrams_;
};
#endif

/*
	Copyright 2006-2008 Xavier Guerrin
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
#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H
#include <QtGui>
#include "diagram.h"
class QSvgGenerator;
/**
	Cette classe represente le dialogue permettant d'exporter un schema
	sous forme d'image selon les desirs de l'utilisateur
*/
class ExportDialog : public QDialog {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ExportDialog(Diagram *, QWidget * = 0);
	virtual ~ExportDialog();
	
	private:
	ExportDialog(const ExportDialog &);
	
	// attributs
	private:
	// elements graphiques
	QLineEdit *filename;
	QPushButton *button_browse;
	QComboBox *format;
	QSpinBox *width;
	QSpinBox *height;
	QCheckBox *keep_aspect_ratio;
	QCheckBox *draw_grid;
	QCheckBox *draw_border;
	QCheckBox *draw_inset;
	QCheckBox *draw_columns;
	QCheckBox *draw_terminals;
	QRadioButton *export_elements;
	QRadioButton *export_border;
	QDialogButtonBox *buttons;
	QGraphicsScene *preview_scene;
	QGraphicsView *preview_view;
	
	// booleens pour ne pas avoir de boucle lors de l'edition des dimensions de l'image
	bool dontchangewidth;
	bool dontchangeheight;
	
	// elements relatifs au traitement effectue par le dialogue
	Diagram *diagram;
	QSize diagram_size;
	qreal diagram_ratio;
	QVector<QRgb> ColorTab;
	
	// methodes
	private:
	QWidget *leftPart();
	QWidget *rightPart();
	QGroupBox *setupDimensionsGroupBox();
	QGroupBox *setupOptionsGroupBox();
	void saveReloadDiagramParameters(bool = true);
	void generateSvg(QFile &file);
	QImage generateImage();
	
	public slots:
	void slot_correctWidth();
	void slot_correctHeight();
	void slot_chooseAFile();
	void slot_check();
	void slot_changeUseBorder();
	void slot_refreshPreview();
};
#endif

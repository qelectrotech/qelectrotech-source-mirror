/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "qetproject.h"
class QSvgGenerator;
/**
	Cette classe represente le dialogue permettant d'exporter un schema
	sous forme d'image selon les desirs de l'utilisateur
*/
class ExportDialog : public QDialog {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ExportDialog(QETProject *, QWidget * = 0);
	virtual ~ExportDialog();
	
	private:
	ExportDialog(const ExportDialog &);
	
	// methodes
	public:
	int diagramsToExportCount() const;
	
	// classes privees
	private:
	class ExportDiagramLine {
		public:
		ExportDiagramLine(Diagram *);
		virtual ~ExportDiagramLine();
		QBoxLayout *sizeLayout();
		Diagram *diagram;
		QCheckBox *must_export;
		QLabel *title_label;
		QLineEdit *file_name;
		QSpinBox *width;
		QLabel *x_label;
		QSpinBox *height;
		QPushButton *keep_ratio;
		QPushButton *reset_size;
		QPushButton *preview;
	};
	
	// attributs
	private:
	QHash<int, ExportDialog::ExportDiagramLine *> diagram_lines_;
	// elements graphiques
	QGridLayout *diagrams_list_layout_;
	QLineEdit *dirpath;
	QPushButton *button_browse;
	QComboBox *format;
	QCheckBox *draw_grid;
	QCheckBox *draw_border;
	QCheckBox *draw_inset;
	QCheckBox *draw_terminals;
	QRadioButton *export_elements;
	QRadioButton *export_border;
	QDialogButtonBox *buttons;
	// mappers
	QSignalMapper *preview_mapper_;
	QSignalMapper *width_mapper_;
	QSignalMapper *height_mapper_;
	QSignalMapper *ratio_mapper_;
	QSignalMapper *reset_mapper_;
	
	// elements relatifs au traitement effectue par le dialogue
	QETProject *project_;
	
	// methodes
	private:
	QWidget *initDiagramsListPart();
	QWidget *leftPart();
	QGroupBox *setupOptionsGroupBox();
	void saveReloadDiagramParameters(Diagram *, bool = true);
	void generateSvg(Diagram *, int, int, bool, QFile &);
	QImage generateImage(Diagram *, int, int, bool);
	void exportDiagram(ExportDiagramLine *);
	qreal diagramRatio(Diagram *);
	QSize diagramSize(Diagram *);
	
	public slots:
	void slot_correctWidth(int);
	void slot_correctHeight(int);
	void slot_keepRatioChanged(int);
	void slot_resetSize(int);
	void slot_chooseADirectory();
	void slot_export();
	void slot_changeUseBorder();
	void slot_checkDiagramsCount();
	void slot_changeFilesExtension(bool = false);
	void slot_previewDiagram(int);
};
#endif

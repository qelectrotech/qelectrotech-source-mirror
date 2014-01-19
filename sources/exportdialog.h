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
#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H
#include <QtGui>
#include "diagram.h"
#include "qetproject.h"
class QSvgGenerator;
class ExportPropertiesWidget;
/**
	This class provides a dialog enabling users to export 1 to n diagrams from
	a project as image files, with features like preview, copy to clipboard,
	resize, etc.
*/
class ExportDialog : public QDialog {
	Q_OBJECT
	
	// constructors, destructor
	public:
	ExportDialog(QETProject *, QWidget * = 0);
	virtual ~ExportDialog();
	
	private:
	ExportDialog(const ExportDialog &);
	
	// methods
	public:
	int diagramsToExportCount() const;
	
	private:
	class ExportDiagramLine {
		public:
		ExportDiagramLine(Diagram *, QSize);
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
		QPushButton *clipboard;
	};
	
	// attributes
	private:
	QHash<int, ExportDialog::ExportDiagramLine *> diagram_lines_;
	// visual items
	QGridLayout *diagrams_list_layout_;
	ExportPropertiesWidget *epw;
	QDialogButtonBox *buttons;
	// mappers
	QSignalMapper *preview_mapper_;
	QSignalMapper *width_mapper_;
	QSignalMapper *height_mapper_;
	QSignalMapper *ratio_mapper_;
	QSignalMapper *reset_mapper_;
	QSignalMapper *clipboard_mapper_;
	
	// project whose diagrams are to be exported
	QETProject *project_;
	
	// methods
	private:
	QWidget *initDiagramsListPart();
	void saveReloadDiagramParameters(Diagram *, bool = true);
	void generateSvg(Diagram *, int, int, bool, QIODevice &);
	void generateDxf(Diagram *, int, int, bool, QString &);
	QImage generateImage(Diagram *, int, int, bool);
	void exportDiagram(ExportDiagramLine *);
	qreal diagramRatio(Diagram *);
	QSize diagramSize(Diagram *);
	QPointF rotation_transformed(qreal, qreal, qreal, qreal, qreal);
	void drawDxfArcEllipse(QString, qreal, qreal, qreal, qreal, qreal, qreal, qreal, qreal, qreal);
	
	public slots:
	void slot_correctWidth(int);
	void slot_correctHeight(int);
	void slot_keepRatioChanged(int);
	void slot_resetSize(int);
	void slot_export();
	void slot_changeUseBorder();
	void slot_checkDiagramsCount();
	void slot_changeFilesExtension(bool = false);
	void slot_previewDiagram(int);
	void slot_exportToClipBoard(int);
};
#endif

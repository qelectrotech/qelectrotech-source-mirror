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
#ifndef DIAGRAM_PRINT_DIALOG_H
#define DIAGRAM_PRINT_DIALOG_H
#include <QtGui>
#include "qetproject.h"
#include "diagram.h"
#include "exportproperties.h"
/**
	Cette classe represente le dialogue de configuration de l'impression d'un
	schema electrique.
	Elle se charge egalement de l'impression elle-meme
*/
class DiagramPrintDialog : public QWidget {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	DiagramPrintDialog(QETProject *, QWidget * = 0);
	virtual ~DiagramPrintDialog();
	private:
	DiagramPrintDialog(const DiagramPrintDialog &);
	
	// methodes
	public:
	void setFileName(const QString &);
	QString fileName() const;
	void setDocName(const QString &);
	QString docName() const;
	QRect diagramRect(Diagram *, const ExportProperties &) const;
	int pagesCount(Diagram *, const ExportProperties &, bool = false) const;
	int horizontalPagesCount(Diagram *, const ExportProperties &, bool = false) const;
	int verticalPagesCount(Diagram *, const ExportProperties &, bool = false) const;
	void exec();
	
	private:
	void buildPrintTypeDialog();
	void buildDialog();
	void saveReloadDiagramParameters(Diagram *, const ExportProperties, bool);
	void savePageSetupForCurrentPrinter();
	void loadPageSetupForCurrentPrinter();
	QString settingsSectionName(const QPrinter *);
	
	private slots:
	void print(const QList<Diagram *> &, bool, const ExportProperties);
	void printDiagram(Diagram *, bool, const ExportProperties &, QPainter *, QPrinter * = 0);
	void updatePrintTypeDialog();
	void acceptPrintTypeDialog();
	void browseFilePrintTypeDialog();
	
	// attributs
	private:
	QETProject *project_;
	QPrinter *printer_;
	QString doc_name_;
	QString file_name_;
	
	/// Attributs relatifs au 1er dialogue
	QDialog *dialog_;
	QLabel *printtype_label_;
	QGridLayout *glayout0_;
	QVBoxLayout *vlayout0_;
	QHBoxLayout *hlayout0_;
	QLabel *printer_icon_;
	QLabel *pdf_icon_;
	QLabel *ps_icon_;
	QButtonGroup *printtype_choice_;
	QRadioButton *printer_choice_;
	QRadioButton *pdf_choice_;
	QRadioButton *ps_choice_;
	QLineEdit *filepath_field_;
	QPushButton *browse_button_;
	QDialogButtonBox *buttons_;
};
#endif

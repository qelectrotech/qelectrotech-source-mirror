/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "diagram.h"
/**
	Cette classe represente le dialogue de configuration de l'impression d'un
	schema electrique.
	Elle se charge egalement de l'impression elle-meme
*/
class DiagramPrintDialog : public QWidget {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	DiagramPrintDialog(Diagram *, QWidget * = 0);
	virtual ~DiagramPrintDialog();
	private:
	DiagramPrintDialog(const DiagramPrintDialog &);
	
	// methodes
	public:
	void setPDFName(const QString &);
	QString PDFName() const;
	int pagesCount(bool = false) const;
	int horizontalPagesCount(bool = false) const;
	int verticalPagesCount(bool = false) const;
	void exec();
	
	private:
	void buildDialog();
	void print();
	
	private slots:
	void updateDialog();
	void checkStartPage();
	void checkEndPage();
	void setPagesRangeVisible(bool);
	
	// attributs
	private:
	Diagram *diagram;
	QPrinter *printer;
	QString pdf_name;
	QDialog *dialog;
	QLabel *options_label;
	QLabel *range_from_label;
	QLabel *to_label;
	QCheckBox *use_full_page;
	QCheckBox *fit_diagram_to_page;
	QSpinBox *start_page;
	QSpinBox *end_page;
	QDialogButtonBox *buttons;
};
#endif

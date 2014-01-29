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
#ifndef CLOSE_DIAGRAMS_DIALOG_H
#define CLOSE_DIAGRAMS_DIALOG_H
#include <QDialog>
#include <QSignalMapper>
class QAbstractButton;
class QDialogButtonBox;
class QCheckBox;
class QLabel;
class QPushButton;
class Diagram;
class QGridLayout;
class QScrollArea;

/**
	This class represents a dialog asking users whether they want to save
	their modified project when it is being closed and what they wish to save
	in it.
*/
class CloseDiagramsDialog : public QDialog {
	Q_OBJECT
	public:
	enum Actions {
		Unknown,
		Save,
		DoNotSave,
		Remove
	};
	
	// Constructors, destructor
	public:
	CloseDiagramsDialog(const QList<Diagram *> &diagrams, QWidget *parent = 0);
	virtual ~CloseDiagramsDialog();
	private:
	CloseDiagramsDialog(const CloseDiagramsDialog &);
	
	// methods
	public:
	int answer() const;
	int actionForDiagram(Diagram *);
	QList<Diagram *> diagramsByAction(Actions);
	
	private:
	void initWidgets();
	void initLayout();
	void loadDiagramsList();
	void addDiagram(Diagram *, int);
	QCheckBox *getCheckBoxForDiagram(Diagram *);
	QString diagramTitle(Diagram *);
	QString diagramStatus(Diagram *);
	QString diagramAction(Diagram *);
	
	signals:
	void showDiagram(Diagram *);
	
	private slots:
	void lambdaCheckBoxChangedState(int);
	void topCheckBoxChangedState(int);
	void setCheckedAll(bool);
	void requireShowDiagram(int);
	void storeAnswer(QAbstractButton *);
	
	// attributes
	private:
	QList<Diagram *> diagrams_list_;     ///< List of (modified or newly added) diagrams displayed by the dialog
	QLabel *informative_label1_;         ///< Informative label when there are modified diagrams
	QLabel *informative_label2_;         ///< Informative label when there is no modified diagram
	QCheckBox *all_checkbox_;            ///< Header checkbox to check/uncheck all other checkboxes
	QScrollArea *diagrams_list_area_;    ///< Scroll area to make the diagrams visual list fit in the dialog
	QWidget *diagrams_list_widget_;      ///< Scrolled widget
	QGridLayout *diagrams_list_layout_;  ///< Layout used to list diagrams
	QDialogButtonBox *buttons_;          ///< Buttons for users to input their final choice
	int answer_;                         ///< Reflects the user answer once the diagram has been executed
	QSignalMapper show_mapper_;          ///< Signal mapper for the "show diagram" buttons to work
};
#endif

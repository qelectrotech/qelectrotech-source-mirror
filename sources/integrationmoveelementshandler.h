/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef INTEGRATION_MOVE_ELEMENTS_HANDLER_H
#define INTEGRATION_MOVE_ELEMENTS_HANDLER_H
#include "basicmoveelementshandler.h"
#include <QtWidgets>
/**
	This class implements the MoveElementsHandler Strategy class.
	It acts like a BasiMoveElementsHandler configured to answer QET::Abort to any question.
	Please note this class was designed with the context of integrating an element definition into a project in mind.
	For this purpose, the elementAlreadyExists method was redefined to ask users whether they wish to:
	  * erase a different, already-existing element,
	  * keep the already-existing element by renaming the new one,
	  * or cancel the integration.
*/
class IntegrationMoveElementsHandler : public BasicMoveElementsHandler {
	Q_OBJECT
	
	// constructors, destructor
	public:
	IntegrationMoveElementsHandler(QWidget * = 0);
	virtual ~IntegrationMoveElementsHandler();
	private:
	IntegrationMoveElementsHandler(const IntegrationMoveElementsHandler &);
	
	// methods
	public:
	virtual QET::Action elementAlreadyExists(ElementDefinition *, ElementDefinition *);
	virtual QString nameForRenamingOperation();
	
	private:
	QString dateString() const;
	QString newNameForElement(const ElementDefinition *);
	QET::Action askUser(ElementDefinition *, ElementDefinition *);
	void initDialog();
	void radioButtonleftMargin(QRadioButton *);
	
	private slots:
	void correctRadioButtons();
	
	// attributes
	private:
	QWidget *parent_widget_;              ///< Widget to be used as parent when displaying dialogs
	QString rename_;                      ///< Name to be used when renaming the integrated element
	QDialog *integ_dialog_;               ///< Dialog in case of conflict when integration an element
	QLabel *dialog_label_;
	QVBoxLayout *dialog_vlayout_;
	QGridLayout *dialog_glayout;
	QDialogButtonBox *buttons_;
	QRadioButton *use_existing_elmt_;
	QRadioButton *integrate_new_element_;
	QRadioButton *erase_element_;
	QRadioButton *integrate_both_;
	QButtonGroup *button_group1_;
	QButtonGroup *button_group2_;
};
#endif

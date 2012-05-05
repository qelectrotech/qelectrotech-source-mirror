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
#ifndef INTEGRATION_MOVE_ELEMENTS_HANDLER_H
#define INTEGRATION_MOVE_ELEMENTS_HANDLER_H
#include "basicmoveelementshandler.h"
#include <QtGui>
/**
	Cette classe implemente la classe strategie MoveElementsHandler.
	Elle correspond a un BasiMoveElementsHandler configure pour repondre
	QET::Abort a toutes les questions.
	Elle redefinit seulement la methode elementAlreadyExists pour, dans le cadre
	de l'integration d'un element dans un projet demander a l'utilisateur s'il souahite :
	  * ecraser l'element precedent, manifestement different
	  * conserver l'element precedent, en renommant le nouveau
	  * annuler l'integration de l'element dans le projet
*/
class IntegrationMoveElementsHandler : public BasicMoveElementsHandler {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	IntegrationMoveElementsHandler(QWidget * = 0);
	virtual ~IntegrationMoveElementsHandler();
	private:
	IntegrationMoveElementsHandler(const IntegrationMoveElementsHandler &);
	
	// methodes
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
	
	// attributs
	private:
	QWidget *parent_widget_;              ///< Widget a utiliser comme parent pour l'affichage des dialogues
	QString rename_;                      ///< Nom a utiliser lors d'une operation de renommage
	QDialog *integ_dialog_;               ///< Dialogue en cas de conflit lors de l'integration d'un element
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

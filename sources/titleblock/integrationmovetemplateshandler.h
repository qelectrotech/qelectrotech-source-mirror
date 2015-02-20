/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_INTEGRATION_MOVE_TEMPLATES_HANDLER_H
#define TITLEBLOCK_SLASH_INTEGRATION_MOVE_TEMPLATES_HANDLER_H
#include "movetemplateshandler.h"
#include <QtGui>
/**
	This class implements the interface defined by
	MoveTitleBlockTemplatesHandler to ease the integration of title block
	templates from files-based collections into projects.
*/
class IntegrationMoveTitleBlockTemplatesHandler : public MoveTitleBlockTemplatesHandler {
	Q_OBJECT
	
	// constructors, destructor
	public:
	IntegrationMoveTitleBlockTemplatesHandler(QWidget * = 0);
	virtual ~IntegrationMoveTitleBlockTemplatesHandler();
	private:
	IntegrationMoveTitleBlockTemplatesHandler(const IntegrationMoveTitleBlockTemplatesHandler &);
	
	// methods
	public:
	virtual QET::Action templateAlreadyExists(const TitleBlockTemplateLocation &src, const TitleBlockTemplateLocation &dst);
	virtual QET::Action errorWithATemplate(const TitleBlockTemplateLocation &, const QString &);
	virtual QString nameForRenamingOperation();
	
	private:
	QString dateString() const;
	QString newNameForTemplate(const TitleBlockTemplateLocation &);
	QET::Action askUser(const TitleBlockTemplateLocation &, const TitleBlockTemplateLocation &);
	void initDialog();
	void radioButtonleftMargin(QRadioButton *);
	
	private slots:
	void correctRadioButtons();
	
	// attributes
	private:
	QWidget *parent_widget_;              ///< Widget used as parent to display dialogs
	QString rename_;                      ///< Name to be used when renaming a title block template
	QDialog *integ_dialog_;               ///< Dialog in case of conflict when integrating a title block template
	QLabel *dialog_label_;
	QVBoxLayout *dialog_vlayout_;
	QGridLayout *dialog_glayout;
	QDialogButtonBox *buttons_;
	QRadioButton *use_existing_template_;    ///< Radio button the user may click to use the existing template and stop the integration
	QRadioButton *integrate_new_template_;   ///< Radio button the user may click to integrate the template
	QRadioButton *erase_template_;           ///< Radio button the user may click for the integrated template to erase the existing one
	/*
		Radio button the user may click for the integrated template to be
		automatically renamed in order to be stored along with the existing one.
	*/
	QRadioButton *integrate_both_;
	QButtonGroup *button_group1_;
	QButtonGroup *button_group2_;
};
#endif

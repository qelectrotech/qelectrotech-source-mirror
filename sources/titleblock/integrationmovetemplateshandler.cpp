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
#include "integrationmovetemplateshandler.h"
#include "templatescollection.h"
#include "qetmessagebox.h"

/**
	Constructor
	@param parent Qwidget used as parent when showing the user dialog.
*/
IntegrationMoveTitleBlockTemplatesHandler::IntegrationMoveTitleBlockTemplatesHandler(QWidget *parent) :
	MoveTitleBlockTemplatesHandler(parent),
	parent_widget_(parent),
	integ_dialog_(0)
{
}

/**
	Destructor
*/
IntegrationMoveTitleBlockTemplatesHandler::~IntegrationMoveTitleBlockTemplatesHandler() {
}

/**
	@param src Source template
	@param dst Target template
	@return the action to be done if the target template already exists
*/
QET::Action IntegrationMoveTitleBlockTemplatesHandler::templateAlreadyExists(const TitleBlockTemplateLocation &src, const TitleBlockTemplateLocation &dst) {
	QString no_parent_collection_error_message(tr("Impossible d'acc\351der \340 la cat\351gorie parente", "error message"));
	QString cant_get_xml_description_error_message(tr("Impossible d'obtenir la description XML de ce mod\350le", "error message"));
	
	// we'll need the parent collection of both templates
	TitleBlockTemplatesCollection *src_tbt_parent_collection = src.parentCollection();
	if (!src_tbt_parent_collection) return(errorWithATemplate(src, no_parent_collection_error_message));
	
	TitleBlockTemplatesCollection *dst_tbt_parent_collection = dst.parentCollection();
	if (!dst_tbt_parent_collection) return(errorWithATemplate(dst, no_parent_collection_error_message));
	
	
	// first, we compare templates (actually we compare their XML code... sadly not the most efficient approach)
	QDomElement src_xml_elmt = src.getTemplateXmlDescription();
	if (src_xml_elmt.isNull()) return(errorWithATemplate(src, cant_get_xml_description_error_message));
	QDomElement dst_xml_elmt = dst.getTemplateXmlDescription();
	if (dst_xml_elmt.isNull()) return(errorWithATemplate(dst, cant_get_xml_description_error_message));
	
	QDomDocument src_tbt_document;
	src_tbt_document.appendChild(src_tbt_document.importNode(src_xml_elmt, true));
	QDomDocument dst_tbt_document;
	dst_tbt_document.appendChild(dst_tbt_document.importNode(dst_xml_elmt, true));
	
	
	if (src_tbt_document.toString(0) == dst_tbt_document.toString(0)) {
		// the templates are the same, consider the integration is done
		qDebug() << Q_FUNC_INFO << "Not integrating" << src.parentCollection() << "/" << src.name()<< "because it is already present in the project";
		return(QET::Managed);
	} else {
		return(askUser(src, dst));
	}
}

/**
	Display an error message related to a specific title block template.
	@param tbt Problematic title block template
	@param message Error message.
*/
QET::Action IntegrationMoveTitleBlockTemplatesHandler::errorWithATemplate(const TitleBlockTemplateLocation &tbt, const QString &message) {
	QString error_message = QString("Une erreur s'est produite avec le mod\350le %1\240: %2").arg(tbt.toString()).arg(message);
	QET::MessageBox::critical(
		parent_widget_,
		tr("Erreur", "message box title"),
		error_message,
		QMessageBox::Ok,
		QMessageBox::Ok
	);
	return(QET::Ignore);
}

/**
	@return the name to be used when this object returns QET::Rename
	@see QET::Action
*/
QString IntegrationMoveTitleBlockTemplatesHandler::nameForRenamingOperation() {
	return(rename_);
}

/**
	@return the current date with a filename-friendly format
*/
QString IntegrationMoveTitleBlockTemplatesHandler::dateString() const {
	return(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
}

/**
	@param tbt A title block template location
	@return a name to be used in order to duplicate the title block template.
	This name is based on the current date.
*/
QString IntegrationMoveTitleBlockTemplatesHandler::newNameForTemplate(const TitleBlockTemplateLocation &tbt) {
	return(QString("%1-%2.elmt").arg(tbt.name()).arg(dateString()));
}

/**
	Ask the use whether they wish to erase the already existing template, rename it or cancel the operation.
	@param src Source title block template
	@param dst Target title block template
	@return the user answer
*/
QET::Action IntegrationMoveTitleBlockTemplatesHandler::askUser(const TitleBlockTemplateLocation &src, const TitleBlockTemplateLocation &dst) {
	Q_UNUSED(src)
	initDialog();
	int result = integ_dialog_ -> exec();
	if (result == QDialog::Accepted) {
		if (use_existing_template_ -> isChecked()) {
			return(QET::Managed);
		} else if (erase_template_ -> isChecked()) {
			return(QET::Erase);
		} else {
			rename_ = newNameForTemplate(dst);
			return(QET::Rename);
		}
	} else {
		return(QET::Abort);
	}
}

/**
	Initialize the user dialog.
*/
void IntegrationMoveTitleBlockTemplatesHandler::initDialog() {
	if (integ_dialog_) return;
	integ_dialog_ = new QDialog(parent_widget_);
	integ_dialog_ -> setWindowTitle(tr("Int\351gration d'un mod\350le de cartouche"));
	
	dialog_label_ = new QLabel(
		QString(
			tr(
				"Le mod\350le a d\351j\340 \351t\351 "
				"int\351gr\351 dans le projet. Toutefois, la version que vous "
				"tentez d'appliquer semble diff\351rente. Que souhaitez-vous "
				"faire ?",
				"dialog content - %1 is a title block template name"
			)
		)
	);
	
	use_existing_template_ = new QRadioButton(
		QString(
			tr(
				"Utiliser le mod\350le d\351j\340 int\351gr\351",
				"dialog content"
			)
		)
	);
	
	integrate_new_template_ = new QRadioButton(
		QString(
			tr(
				"Int\351grer le mod\350le d\351pos\351",
				"dialog content"
			)
		)
	);
	radioButtonleftMargin(integrate_new_template_);
	
	erase_template_ = new QRadioButton(
		QString(
			tr(
				"\311craser le mod\350le d\351j\340 int\351gr\351",
				"dialog content"
			)
		)
	);
	radioButtonleftMargin(erase_template_);
	
	integrate_both_ = new QRadioButton(
		QString(
			tr(
				"Faire cohabiter les deux mod\350les",
				"dialog content"
			)
		)
	);
	
	button_group1_ = new QButtonGroup(this);
	button_group1_ -> addButton(use_existing_template_);
	button_group1_ -> addButton(integrate_new_template_);
	button_group2_ = new QButtonGroup(this);
	button_group2_ -> addButton(erase_template_);
	button_group2_ -> addButton(integrate_both_);
	
	integrate_new_template_ -> setChecked(true);
	integrate_both_ -> setChecked(true);
	
	buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	dialog_glayout = new QGridLayout();
	dialog_glayout -> setColumnMinimumWidth(0, 20);
	dialog_glayout -> addWidget(erase_template_,  0, 1);
	dialog_glayout -> addWidget(integrate_both_, 1, 1);
	
	dialog_vlayout_ = new QVBoxLayout(integ_dialog_);
	dialog_vlayout_ -> addWidget(dialog_label_);
	dialog_vlayout_ -> addWidget(use_existing_template_);
	dialog_vlayout_ -> addWidget(integrate_new_template_);
	dialog_vlayout_ -> addLayout(dialog_glayout);
	dialog_vlayout_ -> addWidget(buttons_);
	
	connect(use_existing_template_,  SIGNAL(toggled(bool)), this,          SLOT(correctRadioButtons()));
	connect(integrate_new_template_, SIGNAL(toggled(bool)), this,          SLOT(correctRadioButtons()));
	connect(buttons_,                SIGNAL(accepted()),    integ_dialog_, SLOT(accept()));
	connect(buttons_,                SIGNAL(rejected()),    integ_dialog_, SLOT(reject()));
}

/**
	Increase the left margin of a radiob utton.
	@param button Radio button
*/
void IntegrationMoveTitleBlockTemplatesHandler::radioButtonleftMargin(QRadioButton *button) {
	int a, b, c, d;
	button -> getContentsMargins(&a, &b, &c, &d);
	button -> setContentsMargins(a + 15, b, c, d);
}

/**
	Ensure the dialog remains consistent.
*/
void IntegrationMoveTitleBlockTemplatesHandler::correctRadioButtons() {
	erase_template_ -> setEnabled(integrate_new_template_ -> isChecked());
	integrate_both_ -> setEnabled(integrate_new_template_ -> isChecked());
}


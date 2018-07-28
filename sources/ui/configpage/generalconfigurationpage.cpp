/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "generalconfigurationpage.h"
#include "ui_generalconfigurationpage.h"
#include "qeticons.h"
#include "qetapp.h"

#include <QSettings>
#include <QFontDialog>

/**
 * @brief GeneralConfigurationPage::GeneralConfigurationPage
 * @param parent
 */
GeneralConfigurationPage::GeneralConfigurationPage(QWidget *parent) :
	ConfigPage(parent),
	ui(new Ui::GeneralConfigurationPage)
{
	ui->setupUi(this);
	
	QSettings settings;
	
	ui->m_use_system_color_cb->setChecked(settings.value("usesystemcolors", "true").toBool());
	bool tabbed = settings.value("diagrameditor/viewmode", "tabbed") == "tabbed";
	if(tabbed)
		ui->m_use_tab_mode_rb->setChecked(true);
	else
		ui->m_use_windows_mode_rb->setChecked(true);
	ui->m_zoom_out_beyond_folio->setChecked(settings.value("diagrameditor/zoom-out-beyond-of-folio", false).toBool());
	ui->m_use_gesture_trackpad->setChecked(settings.value("diagramview/gestures", false).toBool());
	ui->m_save_label_paste->setChecked(settings.value("diagramcommands/erase-label-on-copy", true).toBool());
	ui->m_use_folio_label->setChecked(settings.value("genericpanel/folio", true).toBool());
	ui->m_export_terminal->setChecked(settings.value("nomenclature-exportlist", true).toBool());
	ui->m_autosave_sb->setValue(settings.value("diagrameditor/autosave-interval", 0).toInt());
	
	QString fontInfos = settings.value("diagramitemfont").toString() + " " +
                        settings.value("diagramitemsize").toString() + " (" +
                        settings.value("diagramitemstyle").toString() + ")";
	ui->m_font_pb->setText(fontInfos);
	
	QString foliolistfontInfos = settings.value("foliolistfont").toString() + " " +
                        settings.value("foliolistsize").toString() + " (" +
                        settings.value("folioliststyle").toString() + ")";
	ui->m_folio_list_pb->setText(foliolistfontInfos);
	
	ui->m_highlight_integrated_elements->setChecked(settings.value("diagrameditor/highlight-integrated-elements", true).toBool());
	ui->m_default_elements_info->setPlainText(settings.value("elementeditor/default-informations", "").toString());
	
	QString path = settings.value("elements-collections/common-collection-path", "default").toString();
	if (path != "default")
	{
		ui->m_common_elmt_path_cb->blockSignals(true);
		ui->m_common_elmt_path_cb->setCurrentIndex(1);
		ui->m_common_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		ui->m_common_elmt_path_cb->blockSignals(false);
	}

	path = settings.value("elements-collections/custom-collection-path", "default").toString();
	if (path != "default")
	{
		ui->m_custom_elmt_path_cb->blockSignals(true);
		ui->m_custom_elmt_path_cb->setCurrentIndex(1);
		ui->m_custom_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		ui->m_custom_elmt_path_cb->blockSignals(false);
	}
	
	fillLang();
	
#ifdef Q_OS_MACOS
	ui->m_common_elmt_path_cb ->setDisabled(true);
	ui->m_custom_elmt_path_cb ->setDisabled(true);
#endif
	
}

GeneralConfigurationPage::~GeneralConfigurationPage()
{
	delete ui;
}

/**
 * @brief GeneralConfigurationPage::applyConf
 * Write all configuration in settings file
 */
void GeneralConfigurationPage::applyConf()
{
	QSettings settings;
	
	bool was_using_system_colors = settings.value("usesystemcolors", "true").toBool();
	bool must_use_system_colors  = ui->m_use_system_color_cb->isChecked();
	settings.setValue("usesystemcolors", must_use_system_colors);
	if (was_using_system_colors != must_use_system_colors) {
		QETApp::instance()->useSystemPalette(must_use_system_colors);
	}

	settings.setValue("lang", ui->m_lang_cb->itemData(ui->m_lang_cb->currentIndex()).toString());
	
	QString view_mode = ui->m_use_tab_mode_rb->isChecked() ? "tabbed" : "windowed";
	settings.setValue("diagrameditor/viewmode", view_mode) ;
	
	settings.setValue("diagrameditor/highlight-integrated-elements", ui->m_highlight_integrated_elements->isChecked());
	settings.setValue("elementeditor/default-informations", ui->m_default_elements_info->toPlainText());
	settings.setValue("diagramview/gestures", ui->m_use_gesture_trackpad->isChecked());
	settings.setValue("diagramcommands/erase-label-on-copy", ui->m_save_label_paste->isChecked());
	settings.setValue("diagrameditor/zoom-out-beyond-of-folio", ui->m_zoom_out_beyond_folio->isChecked());
	settings.setValue("genericpanel/folio",ui->m_use_folio_label->isChecked());
	settings.setValue("nomenclature/terminal-exportlist",ui->m_export_terminal->isChecked());
	settings.setValue("diagrameditor/autosave-interval", ui->m_autosave_sb->value());
	
	if (ui->m_common_elmt_path_cb->currentIndex() == 1)
	{
		QString path = ui->m_common_elmt_path_cb->currentText();
		QDir dir(path);
		settings.setValue("elements-collections/common-collection-path",
						  dir.exists() ? path : "default");
	}
	else {
		settings.setValue("elements-collections/common-collection-path", "default");
	}
	
	if (ui->m_custom_elmt_path_cb->currentIndex() == 1)
	{
		QString path = ui->m_custom_elmt_path_cb->currentText();
		QDir dir(path);
		settings.setValue("elements-collections/custom-collection-path",
						  dir.exists() ? path : "default");
	}
	else {
		settings.setValue("elements-collections/custom-collection-path", "default");
	}
}

/**
 * @brief GeneralConfigurationPage::title
 * @return The title of this page
 */
QString GeneralConfigurationPage::title() const {
	return(tr("Général", "configuration page title"));
}

/**
 * @brief GeneralConfigurationPage::icon
 * @return The icon of this page
 */
QIcon GeneralConfigurationPage::icon() const {
	return(QET::Icons::Settings);
}

/**
 * @brief GeneralConfigurationPage::fillLang
 * fill all available lang
 */
void GeneralConfigurationPage::fillLang()
{
	ui->m_lang_cb->addItem(QET::Icons::translation,  tr("Système"), "system");
	ui->m_lang_cb->insertSeparator(1);

		// all lang available on lang directory
	ui->m_lang_cb->addItem(QET::Icons::ar,           tr("Arabe"), "ar");
	ui->m_lang_cb->addItem(QET::Icons::br,           tr("Brézilien"), "pt_br");
	ui->m_lang_cb->addItem(QET::Icons::catalonia,    tr("Catalan"), "ca");
	ui->m_lang_cb->addItem(QET::Icons::cs,           tr("Tchèque"), "cs");
	ui->m_lang_cb->addItem(QET::Icons::de,           tr("Allemand"), "de");
	ui->m_lang_cb->addItem(QET::Icons::da,           tr("Danois"), "da");
	ui->m_lang_cb->addItem(QET::Icons::gr,           tr("Grec"), "el");
	ui->m_lang_cb->addItem(QET::Icons::en,           tr("Anglais"), "en");
	ui->m_lang_cb->addItem(QET::Icons::es,           tr("Espagnol"), "es");
	ui->m_lang_cb->addItem(QET::Icons::fr,           tr("Français"), "fr");
	ui->m_lang_cb->addItem(QET::Icons::hr,           tr("Croate"), "hr");
	ui->m_lang_cb->addItem(QET::Icons::it,           tr("Italien"), "it");
	ui->m_lang_cb->addItem(QET::Icons::pl,           tr("Polonais"), "pl");
	ui->m_lang_cb->addItem(QET::Icons::pt,           tr("Portugais"), "pt");
	ui->m_lang_cb->addItem(QET::Icons::ro,           tr("Roumains"), "ro");
	ui->m_lang_cb->addItem(QET::Icons::ru,           tr("Russe"), "ru");
	ui->m_lang_cb->addItem(QET::Icons::sl,           tr("Slovène"), "sl");
	ui->m_lang_cb->addItem(QET::Icons::nl,           tr("Pays-Bas"), "nl");
	ui->m_lang_cb->addItem(QET::Icons::be,           tr("Belgique-Flemish"), "be");

		//set current index to the lang found in setting file
		//if lang doesn't exist set to system
	QSettings settings;
	for (int i=0; i<ui->m_lang_cb->count(); i++)
	{
		if (ui->m_lang_cb->itemData(i).toString() == settings.value("lang").toString())
		{
			ui->m_lang_cb->setCurrentIndex(i);
			return;
		}
	}
	ui->m_lang_cb->setCurrentIndex(0);
}

/**
 * @brief GeneralConfigurationPage::on_m_font_pb_clicked
 * Apply font to config
 */
void GeneralConfigurationPage::on_m_font_pb_clicked()
{
	bool ok;
	QSettings settings;
	QFont font = QFontDialog::getFont(&ok, QFont("Sans Serif", 9), this);
	if (ok)
	{
		settings.setValue("diagramitemfont", font.family());
		settings.setValue("diagramitemsize", font.pointSize());
		settings.setValue("diagramitemweight", font.weight());
		settings.setValue("diagramitemstyle", font.styleName());
		QString fontInfos = settings.value("diagramitemfont").toString() + " " +
                            settings.value("diagramitemsize").toString() + " (" +
                            settings.value("diagramitemstyle").toString() + ")";
        ui->m_font_pb->setText(fontInfos);
	}
}

/**
 * @brief GeneralConfigurationPage::on_m_folio_list_pb_clicked
 * Apply font to summary pages
 */
void GeneralConfigurationPage::on_m_folio_list_pb_clicked()
{
	bool ok;
	QSettings settings;
	QFont font = QFontDialog::getFont(&ok, QFont("Sans Serif", 9), this);
	if (ok)
	{
		settings.setValue("foliolistfont", font.family());
		settings.setValue("foliolistsize", font.pointSize());
		settings.setValue("foliolistweight", font.weight());
		settings.setValue("folioliststyle", font.styleName());
		QString fontInfos = settings.value("foliolistfont").toString() + " " +
                            settings.value("foliolistsize").toString() + " (" +
                            settings.value("folioliststyle").toString() + ")";
        ui->m_folio_list_pb->setText(fontInfos);
	}
}

#include <QFileDialog>
void GeneralConfigurationPage::on_m_common_elmt_path_cb_currentIndexChanged(int index)
{
    if (index == 1)
	{
		QString path = QFileDialog::getExistingDirectory(this, tr("Chemin de la collection commune"), QDir::homePath());
		if (!path.isEmpty()) {
			ui->m_common_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		}
		else {
			ui->m_common_elmt_path_cb->setCurrentIndex(0);
		}
	}
}

void GeneralConfigurationPage::on_m_custom_elmt_path_cb_currentIndexChanged(int index)
{
    if (index == 1)
	{
		QString path = QFileDialog::getExistingDirectory(this, tr("Chemin de la collection utilisateur"), QDir::homePath());
		if (!path.isEmpty()) {
			ui->m_custom_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		}
		else {
			ui->m_custom_elmt_path_cb->setCurrentIndex(0);
		}
	}
}

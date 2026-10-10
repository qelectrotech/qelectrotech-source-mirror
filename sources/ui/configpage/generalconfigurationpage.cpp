/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "../../scripting/liveserver.h"
#include "../../scripting/assistantinfo.h"

#include "../../qetapp.h"
#include "../../qeticons.h"
#include "ui_generalconfigurationpage.h"
#include "../../materiallist/materiallist.h"
#include "../../utils/qetsettings.h"
#include "../../utils/qetutils.h"
#include "../../qetmessagebox.h"
#include "../../textgrid.h"
#include "../../wiringrules.h"
#include "../wiringruleswarning.h"
#include "../../editor/terminalnamecheck.h"
#include "../../ElementsCollection/qetlabelsfile.h"
#include "../prefixconfigurationdialog.h"
#include "../nokde/kcolorbutton.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QMessageBox>
#include <QSettings>

/**
	@brief GeneralConfigurationPage::GeneralConfigurationPage
	@param parent
*/
GeneralConfigurationPage::GeneralConfigurationPage(QWidget *parent) :
	ConfigPage(parent),
	ui(new Ui::GeneralConfigurationPage)
{
	ui->setupUi(this);
	
	QSettings settings;
	
		//Appearance tab
	ui->m_hdpi_round_policy_cb->addItem(tr("Rounding up for 0.5 and more"), QLatin1String("Round"));
	ui->m_hdpi_round_policy_cb->addItem(tr("Always top rounding"), QLatin1String("Ceil"));
	ui->m_hdpi_round_policy_cb->addItem(tr("Always rounded down"), QLatin1String("Floor"));
	ui->m_hdpi_round_policy_cb->addItem(tr("Rounding up for 0.75 and more"), QLatin1String("RoundPreferFloor"));
	ui->m_hdpi_round_policy_cb->addItem(tr("No rounding"), QLatin1String("PassThrough"));
	switch (QetSettings::hdpiScaleFactorRoundingPolicy()) {
		case Qt::HighDpiScaleFactorRoundingPolicy::Round:
			ui->m_hdpi_round_policy_cb->setCurrentIndex(0);
			break;
		case Qt::HighDpiScaleFactorRoundingPolicy::Ceil:
			ui->m_hdpi_round_policy_cb->setCurrentIndex(1);
			break;
		case Qt::HighDpiScaleFactorRoundingPolicy::Floor:
			ui->m_hdpi_round_policy_cb->setCurrentIndex(2);
			break;
		case Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor:
			ui->m_hdpi_round_policy_cb->setCurrentIndex(3);
			break;
		default:
			ui->m_hdpi_round_policy_cb->setCurrentIndex(4);
			break;
	}

	ui->grid_startup_cb->setChecked(settings.value("diagrameditor/grid_display_startup", true).toBool());
	ui->guides_startup_cb->setChecked(settings.value("diagrameditor/guides_display_startup", false).toBool());
		//Stored as "inserts" but presented as "edits", so the default (insert)
		//is the unchecked state -- a preference reads better as an opt-out.
	ui->m_collection_dblclick_edits->setChecked(!settings.value("elementscollection/double-click-inserts", true).toBool());
	ui->m_collection_search_flat_cb->setChecked(settings.value("elementscollection/search-flat-list", true).toBool());
	ui->m_context_toolbar_cb->setChecked(settings.value("diagrameditor/context_toolbar", true).toBool());
	ui->m_mouse_gestures_cb->setChecked(settings.value("diagrameditor/mouse_gestures", true).toBool());
	ui->DiagramEditor_xGrid_sb->setValue(settings.value("diagrameditor/Xgrid", 10).toInt());
	ui->DiagramEditor_yGrid_sb->setValue(settings.value("diagrameditor/Ygrid", 10).toInt());
	for (const qreal divisor : TextGrid::divisors)
		ui->DiagramEditor_textGrid_cb->addItem(
					divisor > 0 ? TextGrid::ratioLabel(divisor) : tr("Disabled", "fr: Désactivée"),
					divisor);
	int text_grid_index = ui->DiagramEditor_textGrid_cb->findData(
				settings.value(TextGrid::settings_key, 1).toReal());
	if (text_grid_index < 0)
		text_grid_index = ui->DiagramEditor_textGrid_cb->findData(qreal(1));
	ui->DiagramEditor_textGrid_cb->setCurrentIndex(text_grid_index);
	ui->DiagramEditor_xKeyGrid_sb->setValue(settings.value("diagrameditor/key_Xgrid", 10).toInt());
	ui->DiagramEditor_yKeyGrid_sb->setValue(settings.value("diagrameditor/key_Ygrid", 10).toInt());
	ui->DiagramEditor_xKeyGridFine_sb->setValue(settings.value("diagrameditor/key_fine_Xgrid", 1).toInt());
	ui->DiagramEditor_yKeyGridFine_sb->setValue(settings.value("diagrameditor/key_fine_Ygrid", 1).toInt());
	ui->DiagramEditor_Grid_PointSize_min_sb->setValue(settings.value("diagrameditor/grid_pointsize_min", 1).toInt());
	ui->DiagramEditor_Grid_PointSize_max_sb->setValue(settings.value("diagrameditor/grid_pointsize_max", 1).toInt());
	ui->m_use_system_color_cb->setChecked(settings.value("usesystemcolors", "true").toBool());
	bool sysColors = ui->m_use_system_color_cb->isChecked();
	ui->m_custom_app_color_kpb->setEnabled(!sysColors);
	if (settings.contains("customapplicationcolor"))
		ui->m_custom_app_color_kpb->setColor(QColor(settings.value("customapplicationcolor").toString()));
	else
		ui->m_custom_app_color_kpb->setColor(QApplication::palette().color(QPalette::Window));
	bool tabbed = settings.value("diagrameditor/viewmode", "tabbed") == "tabbed";
	if(tabbed)
		ui->m_use_tab_mode_rb->setChecked(true);
	else
		ui->m_use_windows_mode_rb->setChecked(true);
	ui->m_zoom_out_beyond_folio->setChecked(settings.value("diagrameditor/zoom-out-beyond-of-folio", false).toBool());
	ui->m_conductor_properties_panel->setChecked(settings.value("diagrameditor/conductor_properties_panel", false).toBool());
	ui->m_wiring_rules_cb->setChecked(WiringRules::masterEnabled());
	{
			//The rules every project follows unless it sets its own (#1158)
		const WiringRules::Settings rules = WiringRules::applicationSettings();
		ui->m_wiring_max_wires_sb->setValue(rules.max_wires);
		ui->m_wiring_one_wire_per_report_cb->setChecked(rules.one_wire_per_report);
		auto enable = [this](bool on) {
			ui->m_wiring_max_wires_label->setEnabled(on);
			ui->m_wiring_max_wires_sb->setEnabled(on);
			ui->m_wiring_one_wire_per_report_cb->setEnabled(on);
		};
		enable(ui->m_wiring_rules_cb->isChecked());
		connect(ui->m_wiring_rules_cb, &QCheckBox::toggled, this, enable);
	}
	ui->m_use_gesture_trackpad->setChecked(settings.value("diagramview/gestures", false).toBool());
	ui->m_save_label_paste->setChecked(settings.value("diagramcommands/erase-label-on-copy", true).toBool());
	ui->m_autonumber_pasted->setChecked(settings.value("diagramcommands/autonumber-pasted-elements", true).toBool());
	ui->m_enable_scripting->setChecked(QetSettings::scriptingEnabled());
#ifdef QET_HAS_SCRIPTING
	if (QetSettings::scriptingForcedByEnvironment()) {
			//QET_ENABLE_SCRIPTING wins over the stored value, so let the box
			//say so rather than offer a tick that changes nothing.
		ui->m_enable_scripting->setEnabled(false);
		ui->m_enable_scripting->setToolTip(
					tr("Enabled by the QET_ENABLE_SCRIPTING environment "
					   "variable; this setting has no effect while it is "
					   "set."));
	}
#else
		//Built without Qt Qml: there is no scripting to allow. Disabled as
		//well as hidden, so applyConf() leaves the stored value alone --
		//a hidden box still reports its state, and writing it here would
		//quietly clear a preference set on a build that does have Qml.
	ui->m_enable_scripting->setVisible(false);
	ui->m_enable_scripting->setEnabled(false);
#endif
	ui->m_live_assistant->setChecked(QetSettings::liveAssistantEnabled());
#ifndef QET_HAS_SCRIPTING
	ui->m_live_assistant->setVisible(false);
	ui->m_live_assistant->setEnabled(false);
#endif
	ui->m_use_folio_label->setChecked(settings.value("genericpanel/folio", true).toBool());
	ui->m_border_0->setChecked(settings.value("border-columns_0", false).toBool());
	ui->m_autosave_sb->setValue(settings.value("diagrameditor/autosave-interval", 0).toInt());
	
	QString fontInfos = settings.value("diagramitemfont", "Liberation Sans").toString() + " " +
			settings.value("diagramitemsize", "9").toString() + " (" +
			settings.value("diagramitemstyle", "Regular").toString() + ")";
	ui->m_font_pb->setText(fontInfos);


		//Dynamic element text item
	ui->m_dyn_text_rotation_sb->setValue(settings.value("diagrameditor/dynamic_text_rotation", 0).toInt());
	ui->m_dyn_text_width_sb->setValue(settings.value("diagrameditor/dynamic_text_width", -1).toInt());
	if (settings.contains("diagrameditor/dynamic_text_font"))
	{
		QFont font;
		QETUtils::fontFromString(font, settings.value("diagrameditor/dynamic_text_font").toString());

		QString fontInfos = font.family() + " " +
				QString::number(font.pointSize()) + " (" +
				font.styleName() + ")";
		ui->m_dyn_text_font_pb->setText(fontInfos);
	} else { ui->m_dyn_text_font_pb->setText("Liberation Sans 9 (Regular)"); }

		//Independent text item
	ui->m_indi_text_rotation_sb->setValue(settings.value("diagrameditor/independent_text_rotation",0).toInt());
	if (settings.contains("diagrameditor/independent_text_font"))
	{
		QFont font;
		QETUtils::fontFromString(font, settings.value("diagrameditor/independent_text_font").toString());

		QString fontInfos = font.family() + " " +
							QString::number(font.pointSize()) + " (" +
							font.styleName() + ")";
		ui->m_indi_text_font_pb->setText(fontInfos);
	} else { ui->m_indi_text_font_pb->setText("Liberation Sans 9 (Regular)"); }
	
	ui->m_highlight_integrated_elements->setChecked(settings.value("diagrameditor/highlight-integrated-elements", true).toBool());
	ui->m_default_elements_info->setPlainText(settings.value("elementeditor/default-informations", "").toString());
	/*
	  Nombre maximum de primitives affichees par la "liste des parties"
	  Au-dela, un petit message est affiche, indiquant que ce nombre a ete depasse
	  et que la liste ne sera donc pas mise a jour.
	*/
	ui->MaxPartsElementEditorList_sb->setValue(settings.value("elementeditor/max-parts-element-editor-list", 200).toInt());
	ui->ElementEditor_Grid_PointSize_min_sb->setValue(settings.value("elementeditor/grid_pointsize_min", 1).toInt());
	ui->ElementEditor_Grid_PointSize_max_sb->setValue(settings.value("elementeditor/grid_pointsize_max", 1).toInt());
	ui->m_check_terminal_names_cb->setChecked(settings.value(TerminalNameCheck::settings_key, true).toBool());

	QString path = settings.value("elements-collections/common-collection-path", "default").toString();
	if (path != "default")
	{
		ui->m_common_elmt_path_cb->blockSignals(true);
		ui->m_common_elmt_path_cb->setCurrentIndex(1);
		ui->m_common_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		ui->m_common_elmt_path_cb->blockSignals(false);
	}

	path = settings.value("elements-collections/company-collection-path", "default").toString();
	if (path != "default")
	{
		ui->m_company_elmt_path_cb->blockSignals(true);
		ui->m_company_elmt_path_cb->setCurrentIndex(1);
		ui->m_company_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		ui->m_company_elmt_path_cb->blockSignals(false);
	}

	path = settings.value("elements-collections/company-tbt-path", "default").toString();
	if (path != "default")
	{
		ui->m_company_tbt_path_cb->blockSignals(true);
		ui->m_company_tbt_path_cb->setCurrentIndex(1);
		ui->m_company_tbt_path_cb->setItemData(1, path, Qt::DisplayRole);
		ui->m_company_tbt_path_cb->blockSignals(false);
	}

	path = settings.value("elements-collections/custom-collection-path", "default").toString();
	if (path != "default")
	{
		ui->m_custom_elmt_path_cb->blockSignals(true);
		ui->m_custom_elmt_path_cb->setCurrentIndex(1);
		ui->m_custom_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		ui->m_custom_elmt_path_cb->blockSignals(false);
	}
	

	path = settings.value("elements-collections/custom-tbt-path", "default").toString();
	if (path != "default")
	{
		ui->m_custom_tbt_path_cb->blockSignals(true);
		ui->m_custom_tbt_path_cb->setCurrentIndex(1);
		ui->m_custom_tbt_path_cb->setItemData(1, path, Qt::DisplayRole);
		ui->m_custom_tbt_path_cb->blockSignals(false);
	}
	
	path = settings.value("elements-collections/macros-path", "default").toString();
	if (path != "default")
	{
		ui->m_user_macros_path_cb->blockSignals(true);
		ui->m_user_macros_path_cb->setCurrentIndex(1);
		ui->m_user_macros_path_cb->setItemData(1, path, Qt::DisplayRole);
		ui->m_user_macros_path_cb->blockSignals(false);
	}

		//MATERIAL FILE
	path = MaterialList::configuredPath();
	ui->m_material_list_path_le->setText(path);
	if (path.isEmpty())
	{
		ui->m_material_list_path_le->setPlaceholderText(
			tr("Not set (default: %1)",
			   "hint shown in the material file field when no file is configured yet")
				.arg(MaterialList::defaultPath()));
	}

	fillLang();	
}

GeneralConfigurationPage::~GeneralConfigurationPage()
{
	delete ui;
}

/**
	@brief GeneralConfigurationPage::applyConf
	Write all configuration in settings file
*/
void GeneralConfigurationPage::applyConf()
{
	QSettings settings;
	
		//GLOBAL
	bool was_using_system_colors = settings.value("usesystemcolors", "true").toBool();
	bool must_use_system_colors  = ui->m_use_system_color_cb->isChecked();
	settings.setValue("usesystemcolors", must_use_system_colors);
	if (was_using_system_colors != must_use_system_colors) {
		if (must_use_system_colors) {
			QETApp::instance()->useSystemPalette(true);
		} else {
			QColor custom_color = ui->m_custom_app_color_kpb->color();
			settings.setValue("customapplicationcolor", custom_color.name());
			QETApp::instance()->useCustomPalette(custom_color);
		}
	} else if (!must_use_system_colors) {
		QColor custom_color = ui->m_custom_app_color_kpb->color();
		settings.setValue("customapplicationcolor", custom_color.name());
		QETApp::instance()->useCustomPalette(custom_color);
	}
	settings.setValue("border-columns_0",ui->m_border_0->isChecked());
	settings.setValue("lang", ui->m_lang_cb->itemData(ui->m_lang_cb->currentIndex()).toString());

		//hdpi
	QetSettings::setHdpiScaleFactorRoundingPolicy(ui->m_hdpi_round_policy_cb->currentData().toString());
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(QetSettings::hdpiScaleFactorRoundingPolicy());

		//ELEMENT EDITOR
	settings.setValue("elementeditor/default-informations", ui->m_default_elements_info->toPlainText());
	settings.setValue("elementeditor/max-parts-element-editor-list", ui->MaxPartsElementEditorList_sb->value());
	settings.setValue("elementeditor/grid_pointsize_min", ui->ElementEditor_Grid_PointSize_min_sb->value());
	settings.setValue("elementeditor/grid_pointsize_max", ui->ElementEditor_Grid_PointSize_max_sb->value());
	settings.setValue(TerminalNameCheck::settings_key, ui->m_check_terminal_names_cb->isChecked());

		//DIAGRAM VIEW
	settings.setValue("diagramview/gestures", ui->m_use_gesture_trackpad->isChecked());

		//DIAGRAM COMMAND
	settings.setValue("diagramcommands/erase-label-on-copy", ui->m_save_label_paste->isChecked());
	settings.setValue("diagramcommands/autonumber-pasted-elements", ui->m_autonumber_pasted->isChecked());

		//SCRIPTING
		//Left alone while the environment forces it on: the box is disabled
		//in that case and writing its state would silently clear the user's
		//real preference the first time this dialog is accepted.
	if (ui->m_enable_scripting->isEnabled()) {
		QetSettings::setScriptingEnabled(ui->m_enable_scripting->isChecked());
	}
	if (ui->m_live_assistant->isEnabled()) {
		QetSettings::setLiveAssistantEnabled(ui->m_live_assistant->isChecked());
#ifdef QET_HAS_SCRIPTING
			//Switching it off closes the door now, not at the next start
		if (!ui->m_live_assistant->isChecked()) LiveServer::instance().stop();
#endif
	}
		//What an assistant reads about this QElectroTech follows the change
	AssistantInfo::write();

		//GENERIC PANEL
	settings.setValue("genericpanel/folio",ui->m_use_folio_label->isChecked());


		//DIAGRAM EDITOR
	QString view_mode = ui->m_use_tab_mode_rb->isChecked() ? "tabbed" : "windowed";
	settings.setValue("diagrameditor/viewmode", view_mode) ;
	settings.setValue("diagrameditor/highlight-integrated-elements", ui->m_highlight_integrated_elements->isChecked());
	settings.setValue("diagrameditor/zoom-out-beyond-of-folio", ui->m_zoom_out_beyond_folio->isChecked());
	settings.setValue("diagrameditor/conductor_properties_panel", ui->m_conductor_properties_panel->isChecked());
	WiringRules::setMasterEnabled(ui->m_wiring_rules_cb->isChecked());
	{
		const WiringRules::Settings before = WiringRules::applicationSettings();
		WiringRules::Settings rules = before;
		rules.max_wires = ui->m_wiring_max_wires_sb->value();
		rules.one_wire_per_report = ui->m_wiring_one_wire_per_report_cb->isChecked();
		WiringRules::setApplicationSettings(rules);
		if (WiringRules::masterEnabled() && WiringRules::turnsRuleOn(before, rules)) {
			WiringRulesWarning::show(this);
		}
	}
	settings.setValue("diagrameditor/autosave-interval", ui->m_autosave_sb->value());

	settings.setValue("diagrameditor/grid_display_startup", ui->grid_startup_cb->isChecked());
	settings.setValue("diagrameditor/guides_display_startup", ui->guides_startup_cb->isChecked());
	settings.setValue("elementscollection/double-click-inserts", !ui->m_collection_dblclick_edits->isChecked());
	settings.setValue("elementscollection/search-flat-list", ui->m_collection_search_flat_cb->isChecked());
	settings.setValue("diagrameditor/context_toolbar", ui->m_context_toolbar_cb->isChecked());
	settings.setValue("diagrameditor/mouse_gestures", ui->m_mouse_gestures_cb->isChecked());
		//Grid step and key navigation
	settings.setValue("diagrameditor/Xgrid", ui->DiagramEditor_xGrid_sb->value());
	settings.setValue("diagrameditor/Ygrid", ui->DiagramEditor_yGrid_sb->value());
	settings.setValue(TextGrid::settings_key, ui->DiagramEditor_textGrid_cb->currentData());
	settings.setValue("diagrameditor/key_Xgrid", ui->DiagramEditor_xKeyGrid_sb->value());
	settings.setValue("diagrameditor/key_Ygrid", ui->DiagramEditor_yKeyGrid_sb->value());
	settings.setValue("diagrameditor/key_fine_Xgrid", ui->DiagramEditor_xKeyGridFine_sb->value());
	settings.setValue("diagrameditor/key_fine_Ygrid", ui->DiagramEditor_yKeyGridFine_sb->value());
	settings.setValue("diagrameditor/grid_pointsize_min", ui->DiagramEditor_Grid_PointSize_min_sb->value());
	settings.setValue("diagrameditor/grid_pointsize_max", ui->DiagramEditor_Grid_PointSize_max_sb->value());
		//Dynamic text item
	settings.setValue("diagrameditor/dynamic_text_rotation", ui->m_dyn_text_rotation_sb->value());
	settings.setValue("diagrameditor/dynamic_text_width", ui->m_dyn_text_width_sb->value());
		//Independent text item
	settings.setValue("diagrameditor/independent_text_rotation", ui->m_indi_text_rotation_sb->value());

		//ELEMENTS COLLECTION
	QString path = settings.value("elements-collections/common-collection-path").toString();
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
	if (path != settings.value("elements-collections/common-collection-path").toString()) {
		QETApp::resetCollectionsPath();
	}

	path = settings.value("elements-collections/company-collection-path").toString();
	if (ui->m_company_elmt_path_cb->currentIndex() == 1)
	{
		QString path = ui->m_company_elmt_path_cb->currentText();
		QDir dir(path);
		settings.setValue("elements-collections/company-collection-path",
						  dir.exists() ? path : "default");
	}
	else {
		settings.setValue("elements-collections/company-collection-path", "default");
	}
	if (path != settings.value("elements-collections/company-collection-path").toString()) {
		QETApp::resetCollectionsPath();
	}

	path = settings.value("elements-collections/custom-collection-path").toString();
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
	if (path != settings.value("elements-collections/custom-collection-path").toString()) {
		QETApp::resetCollectionsPath();
	}
	
	path = settings.value("elements-collections/company-tbt-path").toString();
	if (ui->m_company_tbt_path_cb->currentIndex() == 1)
	{
		QString path = ui->m_company_tbt_path_cb->currentText();
		QDir dir(path);
		settings.setValue("elements-collections/company-tbt-path",
						  dir.exists() ? path : "default");
	}
	else {
		settings.setValue("elements-collections/company-tbt-path", "default");
	}
	if (path != settings.value("elements-collections/company-tbt-path").toString()) {
		QETApp::resetCollectionsPath();
	}

	path = settings.value("elements-collections/custom-tbt-path").toString();
	if (ui->m_custom_tbt_path_cb->currentIndex() == 1)
	{
		QString path = ui->m_custom_tbt_path_cb->currentText();
		QDir dir(path);
		settings.setValue("elements-collections/custom-tbt-path",
						  dir.exists() ? path : "default");
	}
	else {
		settings.setValue("elements-collections/custom-tbt-path", "default");
	}
	if (path != settings.value("elements-collections/custom-tbt-path").toString()) {
		QETApp::resetCollectionsPath();
	}

	path = settings.value("elements-collections/macros-path").toString();
	if (ui->m_user_macros_path_cb->currentIndex() == 1)
	{
		QString path = ui->m_user_macros_path_cb->currentText();
		QDir dir(path);
		settings.setValue("elements-collections/macros-path",
						  dir.exists() ? path : "default");
	}
	else {
		settings.setValue("elements-collections/macros-path", "default");
	}
	if (path != settings.value("elements-collections/macros-path").toString()) {
		QETApp::resetCollectionsPath();
	}

		//MATERIAL FILE
		//Unlike the collections, the material file is a plain file, it is
		//kept as chosen even when it doesn't exist yet : the user may well
		//point QElectroTech at a file he intends to write later.
	MaterialList::setConfiguredPath(ui->m_material_list_path_le->text().trimmed());
}

/**
	@brief GeneralConfigurationPage::title
	@return The title of this page
*/
QString GeneralConfigurationPage::title() const
{
	return(tr("General", "configuration page title"));
}

/**
	@brief GeneralConfigurationPage::icon
	@return The icon of this page
*/
QIcon GeneralConfigurationPage::icon() const
{
	return(QET::Icons::Settings);
}

/**
	@brief GeneralConfigurationPage::fillLang
	fill all available lang
*/
void GeneralConfigurationPage::fillLang()
{
	ui->m_lang_cb->addItem(QET::Icons::translation,	tr("System"), "system");
	ui->m_lang_cb->insertSeparator(1);

		// all lang available on lang directory
	ui->m_lang_cb->addItem(QET::Icons::sa,		tr("Arabic"), "ar");
	ui->m_lang_cb->addItem(QET::Icons::br,		tr("Brazilian"), "pt_BR");
	ui->m_lang_cb->addItem(QET::Icons::catalonia,	tr("Catalan"), "ca");
	ui->m_lang_cb->addItem(QET::Icons::cs,		tr("Czech"), "cs");
	ui->m_lang_cb->addItem(QET::Icons::de,		tr("German"), "de");
	ui->m_lang_cb->addItem(QET::Icons::da,		tr("Danish"), "da");
	ui->m_lang_cb->addItem(QET::Icons::gr,		tr("Greek"), "el");
	ui->m_lang_cb->addItem(QET::Icons::en,		tr("English"), "en");
	ui->m_lang_cb->addItem(QET::Icons::es,		tr("Spanish"), "es");
	ui->m_lang_cb->addItem(QET::Icons::fr,		tr("French"), "fr");
	ui->m_lang_cb->addItem(QET::Icons::hr,		tr("Croatian"), "hr");
	ui->m_lang_cb->addItem(QET::Icons::it,		tr("Italian"), "it");
	ui->m_lang_cb->addItem(QET::Icons::jp,		tr("Japanese"), "ja");
	ui->m_lang_cb->addItem(QET::Icons::ko,		tr("Korean"), "ko");
	ui->m_lang_cb->addItem(QET::Icons::pl,		tr("Polish"), "pl");
	ui->m_lang_cb->addItem(QET::Icons::pt,		tr("Portuguese"), "pt");
	ui->m_lang_cb->addItem(QET::Icons::ro,		tr("Romanian"), "ro");
	ui->m_lang_cb->addItem(QET::Icons::ru,		tr("Russian"), "ru");
	ui->m_lang_cb->addItem(QET::Icons::sl,		tr("Slovenian"), "sl");
	ui->m_lang_cb->addItem(QET::Icons::nl,		tr("Dutch"), "nl");
	ui->m_lang_cb->addItem(QET::Icons::no,		tr("Norwegian"), "nb");
	ui->m_lang_cb->addItem(QET::Icons::nl_BE,	tr("Belgium-Flemish"), "nl_BE");
	ui->m_lang_cb->addItem(QET::Icons::tr,		tr("Turkish"), "tr");
	ui->m_lang_cb->addItem(QET::Icons::hu,		tr("Hungarian"), "hu");
	ui->m_lang_cb->addItem(QET::Icons::mn,		tr("Mongolian"), "mn");
	ui->m_lang_cb->addItem(QET::Icons::uk,		tr("Ukrainian"), "uk");
	ui->m_lang_cb->addItem(QET::Icons::zh,		tr("Chinese"), "zh");
	ui->m_lang_cb->addItem(QET::Icons::se,		tr("Swedish"), "sv");
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
	@brief GeneralConfigurationPage::on_m_font_pb_clicked
	Apply font to config
*/
void GeneralConfigurationPage::on_m_font_pb_clicked()
{
	bool ok;
	QSettings settings;
	QFont curFont = QFont(settings.value("diagramitemfont", "Liberation Sans").toString());
	curFont.setPointSizeF(settings.value("diagramitemsize", "9").toInt());
	curFont.setStyleName (settings.value("diagramitemstyle", "Regular").toString());
	QFont font = QFontDialog::getFont(&ok, curFont, this);
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
	@brief GeneralConfigurationPage::m_dyn_text_font_pb_clicked
	 Apply font to config
*/
void GeneralConfigurationPage::on_m_dyn_text_font_pb_clicked()
{
	bool ok;
	QSettings settings;
	QFont curFont;
	QETUtils::fontFromString(curFont, settings.value("diagrameditor/dynamic_text_font", "Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular").toString());
	QFont font = QFontDialog::getFont(&ok, curFont, this);
	if (ok)
	{
		settings.setValue("diagrameditor/dynamic_text_font", QETUtils::fontToString(font));
		QString fontInfos = font.family() + " " +
							QString::number(font.pointSize()) + " (" +
							font.styleName() + ")";
		ui->m_dyn_text_font_pb->setText(fontInfos);
	}
}




void GeneralConfigurationPage::on_m_common_elmt_path_cb_currentIndexChanged(int index)
{
	if (index == 1)
	{
		QString path = QFileDialog::getExistingDirectory(this, tr("Path of the Common Collection"), QETApp::documentDir());
		if (!path.isEmpty()) {
			ui->m_common_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		}
		else {
			ui->m_common_elmt_path_cb->setCurrentIndex(0);
		}
	}
}

void GeneralConfigurationPage::on_m_company_elmt_path_cb_currentIndexChanged(int index)
{
	if (index == 1)
	{
		QString path = QFileDialog::getExistingDirectory(this, tr("Company collection path"), QETApp::documentDir());
		if (!path.isEmpty()) {
			ui->m_company_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		}
		else {
			ui->m_company_elmt_path_cb->setCurrentIndex(0);
		}
	}
}

void GeneralConfigurationPage::on_m_custom_elmt_path_cb_currentIndexChanged(int index)
{
	if (index == 1)
	{
		QString path = QFileDialog::getExistingDirectory(this, tr("User Collection Path"), QETApp::documentDir());
		if (!path.isEmpty()) {
			ui->m_custom_elmt_path_cb->setItemData(1, path, Qt::DisplayRole);
		}
		else {
			ui->m_custom_elmt_path_cb->setCurrentIndex(0);
		}
	}
}

void GeneralConfigurationPage::on_m_company_tbt_path_cb_currentIndexChanged(int index)
{
	if (index == 1)
	{
		QString path = QFileDialog::getExistingDirectory(this, tr("Company title-blocks"), QETApp::documentDir());
		if (!path.isEmpty()) {
			ui->m_company_tbt_path_cb->setItemData(1, path, Qt::DisplayRole);
		}
		else {
			ui->m_company_tbt_path_cb->setCurrentIndex(0);
		}
	}
}

void GeneralConfigurationPage::on_m_custom_tbt_path_cb_currentIndexChanged(int index)
{
	if (index == 1)
	{
		QString path = QFileDialog::getExistingDirectory(this, tr("User Title blocks Path"), QETApp::documentDir());
		if (!path.isEmpty()) {
			ui->m_custom_tbt_path_cb->setItemData(1, path, Qt::DisplayRole);
		}
		else {
			ui->m_custom_tbt_path_cb->setCurrentIndex(0);
		}
	}
}

void GeneralConfigurationPage::on_m_user_macros_path_cb_currentIndexChanged(int index)
{
	if (index == 1)
	{
		QString path = QFileDialog::getExistingDirectory(this, tr("User macro path"), QETApp::documentDir());
		if (!path.isEmpty()) {
			ui->m_user_macros_path_cb->setItemData(1, path, Qt::DisplayRole);
		}
		else {
			ui->m_user_macros_path_cb->setCurrentIndex(0);
		}
	}
}

/**
	@brief GeneralConfigurationPage::on_m_prefix_pb_clicked
	Open the dialog where the prefixes of the user collection folders are
	configured, creating the qet_labels.xml of that collection when it
	does not exist yet.
	Nothing is written until that dialog is validated : cancelling it
	leaves the collection exactly as it was.
*/
void GeneralConfigurationPage::on_m_prefix_pb_clicked()
{
		//The directory the page displays, even when the change has not
		//been applied yet : QETApp::customElementsDir() still answers with
		//the previously saved path, which is not what is shown when the
		//combo has been put back on "Par defaut".
	QString directory;
	switch (ui->m_custom_elmt_path_cb->currentIndex()) {
	case 1:			//"Parcourir..." : the item itself holds the chosen path
		directory = ui->m_custom_elmt_path_cb->itemData(1, Qt::DisplayRole).toString();
		break;
	case 0:			//"Par defaut" : where a default custom collection lives
		directory = QETApp::dataDir() + QStringLiteral("/elements/");
		break;
	default:
		break;
	}
	if (directory.isEmpty()) {
		directory = QETApp::customElementsDir();
	}
	directory = QDir::cleanPath(directory);

	if (!QDir(directory).exists() && !QDir().mkpath(directory)) {
		QMessageBox::warning(this,
							 tr("Folder not found"),
							 tr("The user collection folder:\n%1\ndoes not exist and could not be created.")
							 .arg(directory));
		return;
	}

	const QList<QStringList> folders = QetLabelsFile::scanFolders(directory);
	if (folders.isEmpty()) {
		QMessageBox::information(this,
								 tr("No subfolder"),
								 tr("The user collection:\n%1\nhas no subfolder, so there is no prefix to configure.")
								 .arg(directory));
		return;
	}

	QetLabelsFile labels;
	if (!labels.load(directory)) {
		QMessageBox::warning(this,
							 tr("Prefix file cannot be read"),
							 labels.errorString());
		return;
	}
	if (labels.isBroken()) {
			//A broken file may only be one forgotten tag away from being
			//perfectly valid : tell what is wrong and let the user decide,
			//rebuilding would drop every prefix the file still holds.
		QMessageBox box(QMessageBox::Warning,
						tr("Prefix file damaged"),
						tr("The file %1 is not a valid XML file:\n%2")
						.arg(labels.filePath(), labels.brokenReason()),
						QMessageBox::NoButton,
						this);
		box.addButton(tr("Fix the file"), QMessageBox::AcceptRole);
		auto *rebuild_button = box.addButton(tr("Rebuild"), QMessageBox::DestructiveRole);
		box.setInformativeText(tr("Nothing has been changed yet.\n"
								  "\n"
								  "“Fix the file”: this window closes without changing anything. Open "
								  "the file in a text editor at the place shown, fix it, then run this "
								  "command again.\n"
								  "\n"
								  "“Rebuild”: the folder tree is made again, but all the current "
								  "prefixes are lost. The current file is kept as qet_labels.xml.bak "
								  "before it is replaced."));
		box.setDetailedText(tr("File: %1").arg(labels.filePath()));
		box.exec();
		if (box.clickedButton() != rebuild_button) {
			return;
		}
	}

	PrefixConfigurationDialog dialog(labels, folders, this);
	dialog.exec();
}

/**
	@brief GeneralConfigurationPage::on_m_material_list_browse_pb_clicked
	Let the user pick an existing material file.
*/
void GeneralConfigurationPage::on_m_material_list_browse_pb_clicked()
{
	QString start_dir = ui->m_material_list_path_le->text();
	start_dir = start_dir.isEmpty()
			? QETApp::documentDir()
			: QFileInfo(start_dir).absolutePath();

	const QString path = QFileDialog::getOpenFileName(
		this,
		tr("Select the materials list file"),
		start_dir,
		tr("CSV files (*.csv)"));

	if (!path.isEmpty()) {
		ui->m_material_list_path_le->setText(path);
	}
}

/**
	@brief GeneralConfigurationPage::on_m_material_list_create_pb_clicked
	Create the material file with its header line, so the columns are
	known before the user fills them from his spreadsheet.
*/
void GeneralConfigurationPage::on_m_material_list_create_pb_clicked()
{
	QString path = ui->m_material_list_path_le->text();
	path = path.isEmpty()
			? MaterialList::defaultPath()
			: QFileInfo(path).absolutePath() + QLatin1Char('/') + MaterialList::defaultFileName();

	path = QFileDialog::getSaveFileName(
		this,
		tr("Create the materials list file"),
		path,
		tr("CSV files (*.csv)"));
	if (path.isEmpty()) {
		return;
	}
	if (QFileInfo(path).suffix().isEmpty()) {
		path += QStringLiteral(".csv");
	}

		//An existing file is kept as it is : this button creates the
		//header, it never overwrites a catalogue.
	if (MaterialList::isEmptyFile(path))
	{
		QString error;
		if (!MaterialList::createFile(path, &error))
		{
			QET::QetMessageBox::critical(this,
										 tr("Cannot create"),
										 tr("Cannot create the file:\n%1\n%2")
											.arg(path, error));
			return;
		}
	}

	ui->m_material_list_path_le->setText(path);
}

void GeneralConfigurationPage::on_m_indi_text_font_pb_clicked()
{
	bool ok;
	QSettings settings;
	QFont curFont;
	QETUtils::fontFromString(curFont, settings.value("diagrameditor/independent_text_font", "Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular").toString());
	QFont font = QFontDialog::getFont(&ok, curFont, this);
	if (ok)
	{
		settings.setValue("diagrameditor/independent_text_font", QETUtils::fontToString(font));
		QString fontInfos = font.family() + " " +
							QString::number(font.pointSize()) + " (" +
							font.styleName() + ")";
		ui->m_indi_text_font_pb->setText(fontInfos);
	}
}

void GeneralConfigurationPage::on_MaxPartsElementEditorList_sb_valueChanged(int value)
{
	if (value > 500) {
		ui->MaxPartsElementEditorList_sb->setToolTip(tr("Values that are too high might cause the application to crash"));
		ui->MaxPartsElementEditorList_sb->setStyleSheet("background-color: orange");
	} else {
		ui->MaxPartsElementEditorList_sb->setToolTip("");
		ui->MaxPartsElementEditorList_sb->setStyleSheet("");
	}
}

/**
	@brief GeneralConfigurationPage::on_DiagramEditor_Grid_PointSize_min_sb_valueChanged
	the min-value of the max-SpinBox has to be limited:
	may not be smaller than current value of min-SpinBox
	@param value - the new value of the min-SpinBox
 */
void GeneralConfigurationPage::on_DiagramEditor_Grid_PointSize_min_sb_valueChanged(int value)
{
	ui->DiagramEditor_Grid_PointSize_max_sb->setMinimum(std::max(1, value));
}

/**
	@brief GeneralConfigurationPage::on_ElementEditor_Grid_PointSize_min_sb_valueChanged
	the min-value of the max-SpinBox has to be limited:
	may not be smaller than current value of min-SpinBox
	@param value - the new value of the min-SpinBox
 */
void GeneralConfigurationPage::on_ElementEditor_Grid_PointSize_min_sb_valueChanged(int value)
{
	ui->ElementEditor_Grid_PointSize_max_sb->setMinimum(std::max(1, value));
}

void GeneralConfigurationPage::on_m_hdpi_round_cb_clicked(bool checked)
{
	if (checked) {
		if (QMessageBox::Cancel == QET::QetMessageBox::warning(
				this,
				tr("Experimental feature"),
				tr("WARNING:\n"
				   "Any setting other than “No rounding” may cause rendering errors in the project, depending on:\n"
				   "\n"
				   "1 - the selected setting \n"
				   "2 - the screen's dpi \n"
				   "3 - editing the project on another computer and/or screen that does not have the same settings as in points 1 "
				   "and 2."),
				QMessageBox::StandardButton::Cancel|QMessageBox::StandardButton::Ok,
				QMessageBox::StandardButton::Cancel
														   )) {
			ui->m_hdpi_round_cb->blockSignals(true);
			ui->m_hdpi_round_cb->setChecked(false);
			ui->m_hdpi_round_cb->blockSignals(false);
			return;
		}
	}
	ui->m_hdpi_round_label->setEnabled(checked);
	ui->m_hdpi_round_policy_cb->setEnabled(checked);
}

/**
	@brief GeneralConfigurationPage::on_m_use_system_color_cb_toggled
	Enable/disable the custom color picker when the system color
	checkbox is toggled.
	@param checked
*/
void GeneralConfigurationPage::on_m_use_system_color_cb_toggled(bool checked)
{
	ui->m_custom_app_color_kpb->setEnabled(!checked);
}


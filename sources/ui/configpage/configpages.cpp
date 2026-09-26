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

#include "configpages.h"

#include "../../NameList/nameslist.h"
#include "../../bordertitleblock.h"
#include "../../exportpropertieswidget.h"
#include "../../properties/reportproperties.h"
#include "../../qetapp.h"
#include "../../qeticons.h"
#include "../../qetproject.h"
#include "../borderpropertieswidget.h"
#include "../conductorpropertieswidget.h"
#include "../reportpropertiewidget.h"
#include "../titleblockpropertieswidget.h"
#include "../xrefpropertieswidget.h"
#include "guidespropertieswidget.h"
#include "../autoNum/numerotationcontext.h"
#include "../autoNum/ui/selectautonumw.h"
#include <QFont>
#include <QFontDialog>
#include <QSizePolicy>
#include <utility>

/**
	@brief NewDiagramPage::NewDiagramPage
	Default constructor
	@param project : QETProject
	@param parent : QWidget
	@param ppd : ProjectPropertiesDialog
*/
NewDiagramPage::NewDiagramPage(QETProject *project,
			       QWidget *parent,
			       ProjectPropertiesDialog *ppd) :
	ConfigPage (parent),
	ppd_ (ppd),
	m_project  (project)
{
	//By default we set the global default properties

	// dimensions by default for diagram
	bpw = new BorderPropertiesWidget(BorderProperties::defaultProperties());
	// default titleblock properties
	QList <TitleBlockTemplatesCollection *> c;
	c << QETApp::commonTitleBlockTemplatesCollection()
	  << QETApp::companyTitleBlockTemplatesCollection()
	  << QETApp::customTitleBlockTemplatesCollection();
	if (m_project) c << m_project->embeddedTitleBlockTemplatesCollection();
	ipw = new TitleBlockPropertiesWidget(
				c,
				TitleBlockProperties::defaultProperties(),
				true,
				project,
				parent);
	// default conductor properties
	m_cpw = new ConductorPropertiesWidget(ConductorProperties::defaultProperties());
	m_cpw->setHiddenAvailableAutonum(true);
	// default properties of report label
	rpw = new ReportPropertieWidget(ReportProperties::defaultProperties());
	// default properties of xref
	xrefpw = new XRefPropertiesWidget(XRefProperties::defaultProperties(), this);
	// default guides properties
	m_gpw = new GuidesPropertiesWidget(this);

	QSettings settings;
	QList<Diagram::Guide> loaded_guides;
	if (m_project) {
		for (const auto &pg : m_project->defaultGuides()) {
			Diagram::Guide g;
			g.orientation = static_cast<Diagram::Guide::Orientation>(pg.orientation);
			g.position = pg.position;
			g.color = pg.color;
			loaded_guides.append(g);
		}
	} else {
		QSettings settings;
		int size = settings.beginReadArray(QStringLiteral("diagrameditor/defaultguides"));
		for (int i = 0; i < size; ++i) {
			settings.setArrayIndex(i);
			Diagram::Guide g;
			g.orientation = static_cast<Diagram::Guide::Orientation>(settings.value(QStringLiteral("orientation"), 0).toInt());
			g.position = settings.value(QStringLiteral("position"), 0.0).toReal();
			g.color = QColor(settings.value(QStringLiteral("color"), QStringLiteral("#ff0000")).toString());
			loaded_guides.append(g);
		}
		settings.endArray();
	}
	m_gpw->setGuides(loaded_guides);

	// global auto-numbering defaults (only when editing global settings, not a project)
	if (!m_project) {
		auto saw_conductor = new SelectAutonumW(1);
		auto saw_element = new SelectAutonumW(0);
		auto saw_folio = new SelectAutonumW(2);

		initAutoNumTab(m_autonum_conductor, saw_conductor, QStringLiteral("autonum/conductor"));
		initAutoNumTab(m_autonum_element, saw_element, QStringLiteral("autonum/element"));
		initAutoNumTab(m_autonum_folio, saw_folio, QStringLiteral("autonum/folio"));

		QSettings autonum_settings;
		loadAutoNumTab(m_autonum_conductor, autonum_settings);
		loadAutoNumTab(m_autonum_element, autonum_settings);
		loadAutoNumTab(m_autonum_folio, autonum_settings);

		// Intercept Return key in the combo line edits so it doesn't
		// activate the dialog's default button (OK).
		for (auto *tab : {&m_autonum_conductor, &m_autonum_element, &m_autonum_folio}) {
			if (QComboBox *combo = tab->widget->contextComboBox()) {
				if (combo->lineEdit()) {
					combo->lineEdit()->installEventFilter(this);
				}
			}
		}
	}

	//If there is a project, we edit his properties
	if (m_project) {
		bpw -> setProperties (m_project -> defaultBorderProperties());
		m_cpw -> setProperties (m_project -> defaultConductorProperties());
		ipw -> setProperties (m_project -> defaultTitleBlockProperties());
		rpw -> setReportProperties (m_project -> defaultReportProperties());
		xrefpw -> setProperties (m_project -> defaultXRefProperties());
	}

	connect(ipw, &TitleBlockPropertiesWidget::openAutoNumFolioEditor, this, &NewDiagramPage::changeToAutoFolioTab);

	// main tab widget
	QTabWidget *tab_widget      = new QTabWidget(this);
	m_tab_widget = tab_widget;
	QWidget *diagram_widget     = new QWidget();
	QVBoxLayout *diagram_layout = new QVBoxLayout(diagram_widget);

	diagram_layout -> addWidget(bpw);
	diagram_layout -> addWidget(ipw);
	tab_widget->setMinimumSize(800, 650);

	tab_widget -> addTab (diagram_widget, tr("Folio"));
	tab_widget -> addTab (m_cpw,            tr("Conducteur"));
	tab_widget -> addTab (rpw,            tr("Reports de folio"));
	tab_widget -> addTab (xrefpw,         tr("Références croisées"));
	tab_widget -> addTab (m_gpw,          tr("Guides"));

	// add auto-numbering tab only for global settings (not per project)
	if (!m_project) {
		QWidget *autonum_widget = new QWidget();
		QVBoxLayout *autonum_layout = new QVBoxLayout(autonum_widget);
		autonum_layout->addWidget(new QLabel(tr("Définir les règles de numérotation automatique par défaut pour les nouveaux projets :")));
		QTabWidget *autonum_inner_tab = new QTabWidget();
		autonum_inner_tab->addTab(m_autonum_conductor.widget, tr("Conducteurs"));
		autonum_inner_tab->addTab(m_autonum_element.widget, tr("Eléments"));
		autonum_inner_tab->addTab(m_autonum_folio.widget, tr("Folios"));
		autonum_layout->addWidget(autonum_inner_tab);
		tab_widget -> addTab (autonum_widget, tr("Numérotation auto"));
	}

	QVBoxLayout *vlayout1 = new QVBoxLayout();
	vlayout1->addWidget(tab_widget);

	setLayout(vlayout1);
}

/**
	@brief NewDiagramPage::~NewDiagramPage
*/
NewDiagramPage::~NewDiagramPage()
{
	disconnect(ipw, &TitleBlockPropertiesWidget::openAutoNumFolioEditor, this, &NewDiagramPage::changeToAutoFolioTab);
}

/**
	@brief NewDiagramPage::applyConf
	Apply conf for this page.
	If there is a project, save in the project,
	else save to the default conf of QElectroTech
*/
void NewDiagramPage::applyConf()
{
	if (m_project) { //If project we save to the project
		if (m_project -> isReadOnly()) return;
		bool modified_project = false;

		BorderProperties new_border_prop = bpw -> properties();
		if (m_project -> defaultBorderProperties() != new_border_prop) {
			m_project -> setDefaultBorderProperties(bpw -> properties());
			modified_project = true;
		}

		TitleBlockProperties new_tbt_prop = ipw -> properties();
		if (m_project -> defaultTitleBlockProperties() != new_tbt_prop) {
			m_project -> setDefaultTitleBlockProperties(ipw -> properties());
			modified_project = true;
		}

		ConductorProperties new_conductor_prop = m_cpw -> properties();
		if (m_project -> defaultConductorProperties() != new_conductor_prop) {
			m_project -> setDefaultConductorProperties(m_cpw -> properties());
			modified_project = true;
		}

		QString new_report_prop = rpw -> ReportProperties();
		if (m_project -> defaultReportProperties() != new_report_prop) {
			m_project -> setDefaultReportProperties(new_report_prop);
			modified_project = true;
		}

		QHash<QString, XRefProperties> new_xref_properties = xrefpw -> properties();
		if (m_project -> defaultXRefProperties() != new_xref_properties) {
			m_project -> setDefaultXRefProperties(new_xref_properties);
			modified_project = true;
		}

		QList<GuideProperties> proj_guides;
		for (const auto &g : m_gpw->guides()) {
			GuideProperties pg;
			pg.orientation = static_cast<int>(g.orientation);
			pg.position = g.position;
			pg.color = g.color;
			proj_guides.append(pg);
		}
		m_project->setDefaultGuides(proj_guides);
		modified_project = true;

		if (modified_project) {
			m_project -> setModified(modified_project);
		}

	} else { //Else we save to the default value
		QSettings settings;

		// dimensions des nouveaux schemas
		bpw -> properties().toSettings(settings, "diagrameditor/default");

		// proprietes du cartouche
		ipw-> properties().toSettings(settings, "diagrameditor/default");

		// proprietes par defaut des conducteurs
		m_cpw -> properties().toSettings(settings, "diagrameditor/defaultconductor");

		// default report properties
		rpw->toSettings(settings, "diagrameditor/defaultreport");

		// default xref properties
		const QHash<QString, XRefProperties> hash_xrp = xrefpw->properties();
		for (auto it = hash_xrp.constBegin() ; it != hash_xrp.constEnd() ; ++it) {
			it.value().toSettings(settings,
						  QStringLiteral("diagrameditor/defaultxref") % it.key());
		}

		// Global in QSettings speichern
		QList<Diagram::Guide> current_guides = m_gpw->guides();
		settings.beginWriteArray(QStringLiteral("diagrameditor/defaultguides"));
		for (int i = 0; i < current_guides.size(); ++i) {
			settings.setArrayIndex(i);
			settings.setValue(QStringLiteral("orientation"), static_cast<int>(current_guides[i].orientation));
			settings.setValue(QStringLiteral("position"), current_guides[i].position);
			settings.setValue(QStringLiteral("color"), current_guides[i].color.name());
		}
		settings.endArray();

		// save global auto-numbering defaults
		persistAutonumSettings();
	}
}

/**
	@brief NewDiagramPage::icon
	@return  icon of this page
*/
QIcon NewDiagramPage::icon() const
{
	if (m_project) return(QET::Icons::NewDiagram);
	return(QET::Icons::Projects);
}

/**
	@brief NewDiagramPage::title
	@return title of this page
*/
QString NewDiagramPage::title() const
{
	if (m_project) return(tr("Nouveau folio", "configuration page title"));
	return(tr("Nouveau projet", "configuration page title"));
}

/**
	@brief NewDiagramPage::changeToAutoFolioTab
	Set the current tab to Autonum
*/
void NewDiagramPage::changeToAutoFolioTab()
{
	if (m_project){
		ppd_->setCurrentPage(ProjectPropertiesDialog::Autonum);
		ppd_->changeToFolio();
		ppd_->exec();
	}
}

/**
	@brief NewDiagramPage::setFolioAutonum
	Set temporary TBP to use in auto folio num
*/
void NewDiagramPage::setFolioAutonum(QString autoNum){
	TitleBlockProperties tbptemp = ipw->propertiesAutoNum(std::move(autoNum));
	ipw->setProperties(tbptemp);
	applyConf();
}

/**
	@brief NewDiagramPage::saveCurrentTbp
	Save current TBP to retrieve after auto folio	num
*/
void NewDiagramPage::saveCurrentTbp()
{
	savedTbp = ipw->properties();
}

/**
	@brief NewDiagramPage::loadSavedTbp
	Retrieve saved auto folio num
*/
void NewDiagramPage::loadSavedTbp()
{
	ipw->setProperties(savedTbp);
	applyConf();
}

/**
	@brief NewDiagramPage::isPlaceholder
	Return true if @a name matches the combo box's built-in placeholder text
	(first item). This is locale-independent because it reads the actual item text.
*/
bool NewDiagramPage::isPlaceholder(QComboBox *combo, const QString &name)
{
	return !combo->count() || name == combo->itemText(0);
}

/**
	@brief NewDiagramPage::initAutoNumTab
	Initialise an AutoNumTab struct and connect its signals.
*/
void NewDiagramPage::initAutoNumTab(AutoNumTab &tab, SelectAutonumW *w, const QString &prefix)
{
	tab.widget = w;
	tab.prefix = prefix;

	connect(w, &SelectAutonumW::applyPressed, this, [this, &tab]() { saveAutoNumContext(tab); });
	connect(w, &SelectAutonumW::removeClicked, this, [this, &tab]() { removeAutoNumContext(tab); });
	connect(w->contextComboBox(), &QComboBox::activated, this, [this, &tab](int index) {
		if (index >= 0) {
			QString name = tab.widget->contextComboBox()->itemText(index);
			if (tab.contexts.contains(name)) {
				tab.widget->setContext(tab.contexts.value(name));
			}
		}
	});
}

/**
	@brief NewDiagramPage::loadAutoNumTab
	Load saved rules from QSettings into an AutoNumTab.
*/
void NewDiagramPage::loadAutoNumTab(AutoNumTab &tab, QSettings &settings)
{
	auto data = NumerotationContext::loadFromSettings(settings, tab.prefix);
	tab.contexts = data.first;
	for (auto it = tab.contexts.constBegin(); it != tab.contexts.constEnd(); ++it) {
		tab.widget->contextComboBox()->addItem(it.key());
	}
	if (!tab.contexts.isEmpty() && !data.second.isEmpty()
	    && tab.contexts.contains(data.second)) {
		tab.widget->contextComboBox()->setCurrentText(data.second);
		tab.widget->setContext(tab.contexts.value(data.second));
	}
}

/**
	@brief NewDiagramPage::saveAutoNumContext
	Save the current context from an AutoNumTab's widget into its hash and
	persist to QSettings immediately.
*/
void NewDiagramPage::saveAutoNumContext(AutoNumTab &tab)
{
	QString name = tab.widget->contextComboBox()->currentText().trimmed();
	if (name.isEmpty() || isPlaceholder(tab.widget->contextComboBox(), name)) {
		return;
	}
	tab.contexts.insert(name, tab.widget->toNumContext());
	if (tab.widget->contextComboBox()->findText(name) == -1) {
		tab.widget->contextComboBox()->addItem(name);
	}
	persistAutonumSettings();
}

/**
	@brief NewDiagramPage::removeAutoNumContext
	Remove the current context from an AutoNumTab's hash and persist.
*/
void NewDiagramPage::removeAutoNumContext(AutoNumTab &tab)
{
	QString name = tab.widget->contextComboBox()->currentText().trimmed();
	if (name.isEmpty() || isPlaceholder(tab.widget->contextComboBox(), name)) {
		return;
	}
	int idx = tab.widget->contextComboBox()->findText(name);
	if (idx == -1) return;
	tab.contexts.remove(name);
	tab.widget->contextComboBox()->removeItem(idx);
	tab.widget->contextComboBox()->setCurrentText(QString());
	tab.widget->setContext(NumerotationContext());
	persistAutonumSettings();
}

/**
	@brief NewDiagramPage::persistAutonumSettings
	Save all autonum contexts to QSettings immediately.
*/
void NewDiagramPage::persistAutonumSettings()
{
	QSettings settings;
	for (auto *tab : {&m_autonum_conductor, &m_autonum_element, &m_autonum_folio}) {
		QString current;
		QComboBox *combo = tab->widget->contextComboBox();
		if (!isPlaceholder(combo, combo->currentText().trimmed())
		    && tab->contexts.contains(combo->currentText().trimmed())) {
			current = combo->currentText().trimmed();
		}
		NumerotationContext::saveToSettings(tab->contexts, current,
						   settings, tab->prefix);
	}
}

/**
	@brief NewDiagramPage::eventFilter
	Intercept Return/Enter in combo box line edits so it doesn't close the
	settings dialog.
*/
bool NewDiagramPage::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		auto *ke = static_cast<QKeyEvent *>(event);
		if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)) {
			// Check if this is a line edit inside one of our autonum combos
			for (auto *tab : {&m_autonum_conductor, &m_autonum_element, &m_autonum_folio}) {
				if (QComboBox *combo = tab->widget->contextComboBox()) {
					if (combo->lineEdit() && combo->lineEdit() == obj) {
						return true; // eat the event
					}
				}
			}
		}
	}
	return ConfigPage::eventFilter(obj, event);
}

/**
	Constructeur
	@param parent QWidget parent
*/
ExportConfigPage::ExportConfigPage(QWidget *parent) : ConfigPage(parent) {
	// epw contient les options d'export
	epw = new ExportPropertiesWidget(ExportProperties::defaultExportProperties());

	// layout vertical contenant le titre, une ligne horizontale et epw
	QVBoxLayout *vlayout1 = new QVBoxLayout();

	QLabel *title = new QLabel(this -> title());
	vlayout1 -> addWidget(title);

	QFrame *horiz_line = new QFrame();
	horiz_line -> setFrameShape(QFrame::HLine);
	vlayout1 -> addWidget(horiz_line);
	vlayout1 -> addWidget(epw);
	vlayout1 -> addStretch();

	// activation du layout
	setLayout(vlayout1);
}

/// Destructeur
ExportConfigPage::~ExportConfigPage()
{
}

/**
	Applique la configuration de cette page
*/
void ExportConfigPage::applyConf()
{
	QSettings settings;
	epw -> exportProperties().toSettings(settings, "export/default");
}

/// @return l'icone de cette page
QIcon ExportConfigPage::icon() const
{
	return(QET::Icons::DocumentExport);
}

/// @return le titre de cette page
QString ExportConfigPage::title() const
{
	return(tr("Export", "configuration page title"));
}

/**
	Constructeur
	@param parent QWidget parent
*/
PrintConfigPage::PrintConfigPage(QWidget *parent) : ConfigPage(parent) {
	// epw contient les options d'export
	epw = new ExportPropertiesWidget(ExportProperties::defaultPrintProperties());
	epw -> setPrintingMode(true);

	// layout vertical contenant le titre, une ligne horizontale et epw
	QVBoxLayout *vlayout1 = new QVBoxLayout();

	QLabel *title = new QLabel(this -> title());
	vlayout1 -> addWidget(title);

	QFrame *horiz_line = new QFrame();
	horiz_line -> setFrameShape(QFrame::HLine);
	vlayout1 -> addWidget(horiz_line);
	vlayout1 -> addWidget(epw);
	vlayout1 -> addStretch();

	// activation du layout
	setLayout(vlayout1);
}

/// Destructeur
PrintConfigPage::~PrintConfigPage()
{
}

/**
	@brief PrintConfigPage::applyConf
	Apply the config of this page
*/
void PrintConfigPage::applyConf()
{
	QString prefix = "print/default";

	QSettings settings;
	epw -> exportProperties().toSettings(settings, prefix);

	// annule l'enregistrement de certaines proprietes non pertinentes
	settings.remove(prefix % "path");
	settings.remove(prefix % "format");
	settings.remove(prefix % "area");
}

/// @return l'icone de cette page
QIcon PrintConfigPage::icon() const
{
	return(QET::Icons::Printer);
}

/// @return le titre de cette page
QString PrintConfigPage::title() const
{
	return(tr("Impression", "configuration page title"));
}


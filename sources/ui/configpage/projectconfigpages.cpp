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
#include "projectconfigpages.h"

#include "../autoNum/numerotationcontext.h"
#include "../autoNum/ui/autonumberingmanagementw.h"
#include "../autoNum/ui/folioautonumbering.h"
#include "../autoNum/ui/formulaautonumberingw.h"
#include "../autoNum/ui/selectautonumw.h"
#include "../project/projectpropertieshandler.h"
#include "../qet.h"
#include "../qeticons.h"
#include "../qetproject.h"
#include "../borderpropertieswidget.h"
#include "../conductorpropertieswidget.h"
#include "../diagramcontextwidget.h"
#include "../reportpropertiewidget.h"
#include "../titleblockpropertieswidget.h"
#include "../xrefpropertieswidget.h"

//#include "ui_autonumberingmanagementw.h"

#include <QtWidgets>

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectConfigPage::ProjectConfigPage(QETProject *project, QWidget *parent) :
	ConfigPage(parent),
	m_project(project)
{
}

/**
	Destructor
*/
ProjectConfigPage::~ProjectConfigPage()
{
}

/**
	@return the project being edited by this page
*/
QETProject *ProjectConfigPage::project() const
{
	return(m_project);
}

/**
	@brief ProjectConfigPage::setProject
	Set \a new_project as the project being edited by this page.
	@param new_project : True to read values from the project
	into widgets before setting them read only accordingly,
	false otherwise. Defaults to true.
	@param read_values
	@return the former project
*/
QETProject *ProjectConfigPage::setProject(QETProject *new_project,
					  bool read_values) {
	if (new_project == m_project) return(m_project);
	
	QETProject *former_project = m_project;
	m_project = new_project;
	if (m_project && read_values) {
		readValuesFromProject();
		adjustReadOnly();
	}
	return(former_project);
}

/**
	Apply the configuration after user input
*/
void ProjectConfigPage::applyConf()
{
	if (!m_project || m_project -> isReadOnly()) return;
	applyProjectConf();
}

/**
	Initialize the page by calling initWidgets() and initLayout(). Also call
	readValuesFromProject() and adjustReadOnly() if a non-zero project has been
	set. Typically, you should call this function in your subclass constructor.
*/
void ProjectConfigPage::init()
{
	initWidgets();
	initLayout();
	if (m_project) {
		readValuesFromProject();
		adjustReadOnly();
	}
}

//######################################################################################//

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectMainConfigPage::ProjectMainConfigPage(QETProject *project, QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	init();
}

/**
	Destructor
*/
ProjectMainConfigPage::~ProjectMainConfigPage()
{
}

/**
	@return the title for this page
*/
QString ProjectMainConfigPage::title() const
{
	return(tr("Général", "configuration page title"));
}

/**
	@return the icon for this page
*/
QIcon ProjectMainConfigPage::icon() const
{
	return(QET::Icons::Settings);
}

/**
	Apply the configuration after user input
*/
void ProjectMainConfigPage::applyProjectConf()
{
	bool modified_project = false;
	
	QString new_title = title_value_ -> text();
	if (m_project -> title() != new_title) {
		m_project -> setTitle(new_title);
		modified_project = true;
	}
	
	DiagramContext new_properties = project_variables_ -> context();
	if (m_project -> projectProperties() != new_properties) {
		m_project -> setProjectProperties(new_properties);
		modified_project = true;
	}

	ProjectUsageTracker &usage_tracker = m_project -> projectPropertiesHandler().usageTracker();
	if (usage_tracker.isEnabled() != usage_enabled_cb_ -> isChecked()) {
		usage_tracker.setEnabled(usage_enabled_cb_ -> isChecked());
		modified_project = true;
	}

	if (modified_project) {
		m_project -> setModified(true);
	}
}

/**
	@return the project title entered by the user
*/
QString ProjectMainConfigPage::projectTitle() const
{
	return(title_value_ -> text());
}

/**
	Initialize widgets displayed by the page.
*/
void ProjectMainConfigPage::initWidgets()
{
	title_label_ = new QLabel(tr("Titre du projet :", "label when configuring"));
	title_value_ = new QLineEdit();
	title_information_ = new QLabel(tr("Ce titre sera disponible pour tous les folios de ce projet en tant que %projecttitle.", "informative label"));
	project_variables_label_ = new QLabel(
		tr(
			"Vous pouvez définir ci-dessous des propriétés personnalisées qui seront disponibles pour tous les folios de ce projet (typiquement pour les cartouches).",
			 "informative label"
		)
	);
	project_variables_label_ -> setWordWrap(true);
	project_variables_ = new DiagramContextWidget();
	project_variables_ -> setContext(DiagramContext());

	usage_label_ = new QLabel(tr("Temps passé sur ce projet :", "label when configuring"));
	usage_value_ = new QLabel();
	usage_enabled_cb_ = new QCheckBox(tr("Suivre le temps passé sur ce projet (uniquement enregistré localement dans ce fichier)", "checkbox label"));
	usage_reset_pb_ = new QPushButton(tr("Réinitialiser", "button label"));
	connect(usage_reset_pb_, &QPushButton::clicked, this, &ProjectMainConfigPage::resetUsageTracker);
}

/**
	Initialize the layout of this page.
*/
void ProjectMainConfigPage::initLayout()
{
	QVBoxLayout *main_layout0 = new QVBoxLayout();
	QHBoxLayout *title_layout0 = new QHBoxLayout();
	title_layout0 -> addWidget(title_label_);
	title_layout0 -> addWidget(title_value_);
	main_layout0 -> addLayout(title_layout0);
	main_layout0 -> addWidget(title_information_);
	main_layout0 -> addSpacing(10);
	main_layout0 -> addWidget(project_variables_label_);
	main_layout0 -> addWidget(project_variables_);
	main_layout0 -> addSpacing(10);

	QHBoxLayout *usage_layout0 = new QHBoxLayout();
	usage_layout0 -> addWidget(usage_label_);
	usage_layout0 -> addWidget(usage_value_);
	usage_layout0 -> addStretch();
	usage_layout0 -> addWidget(usage_reset_pb_);
	main_layout0 -> addLayout(usage_layout0);
	main_layout0 -> addWidget(usage_enabled_cb_);

	setLayout(main_layout0);
	this -> setMinimumWidth(680);

}

/**
	Read properties from the edited project then fill widgets with them.
*/
void ProjectMainConfigPage::readValuesFromProject()
{
	title_value_ -> setText(m_project -> title());
	project_variables_ -> setContext(m_project -> projectProperties());

	const ProjectUsageTracker &usage_tracker = m_project -> projectPropertiesHandler().usageTracker();
	const qint64 total_seconds = usage_tracker.secondsSpent();
	usage_value_ -> setText(tr("%1 h %2 min", "hours and minutes of time spent on a project")
							 .arg(total_seconds / 3600)
							 .arg((total_seconds % 3600) / 60));
	usage_enabled_cb_ -> setChecked(usage_tracker.isEnabled());
}

/**
	@brief ProjectMainConfigPage::resetUsageTracker
	Reset the accumulated "time spent on this project" counter to zero and
	refresh its displayed value. Applies immediately (not staged behind
	OK/Cancel like the other fields on this page), since it isn't
	destructive to any actual project content.
*/
void ProjectMainConfigPage::resetUsageTracker()
{
	m_project -> projectPropertiesHandler().usageTracker().resetSecondsSpent();
	m_project -> setModified(true);
	usage_value_ -> setText(tr("%1 h %2 min", "hours and minutes of time spent on a project").arg(0).arg(0));
}

/**
	Set the content of this page read only if the project is read only,
	editable if the project is editable.
*/
void ProjectMainConfigPage::adjustReadOnly()
{
	bool is_read_only = m_project -> isReadOnly();
	title_value_ -> setReadOnly(is_read_only);
	usage_enabled_cb_ -> setDisabled(is_read_only);
	usage_reset_pb_ -> setDisabled(is_read_only);
}

//######################################################################################//

/**
	@brief ProjectAutoNumConfigPage::ProjectAutoNumConfigPage
	Default constructor
	@param project : project to edit
	@param parent : parent widget
*/
ProjectAutoNumConfigPage::ProjectAutoNumConfigPage (QETProject *project,
						    QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	// Follow the same contract ProjectMainConfigPage's constructor does --
	// init() is documented to be the thing a subclass constructor calls
	// (see ProjectConfigPage::init()'s doc comment). Calling the pieces
	// individually here used to skip both initLayout() and adjustReadOnly(),
	// and called readValuesFromProject() even when m_project was null.
	// buildConnections() itself is invoked from the end of initWidgets()
	// below, in the same relative position it held here.
	init();
}

/**
	@brief ProjectAutoNumConfigPage::title
	Title of this config page
	@return
*/
QString ProjectAutoNumConfigPage::title() const
{
	return tr("Numérotation auto");
}

/**
	@brief ProjectAutoNumConfigPage::icon
	Icon of this config pafe
	@return
*/
QIcon ProjectAutoNumConfigPage::icon() const
{
	return QIcon (QET::Icons::AutoNum);
}

/**
	@brief ProjectAutoNumConfigPage::applyProjectConf
*/
void ProjectAutoNumConfigPage::applyProjectConf()
{}

/**
	@brief ProjectAutoNumConfigPage::initWidgets
	Init some widget of this page
*/
void ProjectAutoNumConfigPage::initWidgets()
{
	QTabWidget *tab_widget = new QTabWidget(this);
	
		//Management tab
	m_amw = new AutoNumberingManagementW(project());
	tab_widget->addTab(m_amw, tr("Management"));
	
		//Conductor tab
	m_saw_conductor = new SelectAutonumW(1);
	tab_widget->addTab(m_saw_conductor, tr("Conducteurs"));
	
		//Element tab
	m_saw_element = new SelectAutonumW(0);
	tab_widget->addTab(m_saw_element, tr("Eléments"));
	
		//Folio Tab
	m_saw_folio = new SelectAutonumW(2);
	tab_widget->addTab(m_saw_folio, tr("Folios"));
	
		//AutoNumbering Tab
	m_faw = new FolioAutonumberingW(project());
	tab_widget->addTab(m_faw, tr("Numérotation auto des folios"));
	
	m_import_pb = new QPushButton(
				tr("Importer depuis un autre projet..."), this);
	m_import_pb->setToolTip(
				tr("Reprendre les numérotations automatiques "
				   "enregistrées dans un autre projet"));

	QHBoxLayout *button_layout = new QHBoxLayout();
	button_layout->addStretch();
	button_layout->addWidget(m_import_pb);

	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout->addWidget(tab_widget);
	main_layout->addLayout(button_layout);
	setLayout(main_layout);

	buildConnections();
}

/**
	@brief ProjectAutoNumConfigPage::readValuesFromProject
	Read value stored on project, and update display
*/
void ProjectAutoNumConfigPage::readValuesFromProject()
{
		// This is called again after an import, so start from an empty
		// combo box instead of appending a second copy of every name.
	m_saw_conductor->contextComboBox()->clear();
	m_saw_element->contextComboBox()->clear();
	m_saw_folio->contextComboBox()->clear();

		//Conductor Tab
	const QStringList strlc(m_project->conductorAutoNum().keys());
	m_saw_conductor->contextComboBox()->addItems(strlc);
	
		//Element Tab
	const QStringList strle(m_project->elementAutoNum().keys());
	m_saw_element->contextComboBox()->addItems(strle);
	
		//Folio Tab
	const QStringList strlf(m_project->folioAutoNum().keys());
	m_saw_folio->contextComboBox()->addItems(strlf);
	
		//Folio AutoNumbering Tab
	m_faw->setContext(m_project->folioAutoNum().keys());
}

/**
	@brief ProjectAutoNumConfigPage::adjustReadOnly
	set this config page disable if project is read only
*/
void ProjectAutoNumConfigPage::adjustReadOnly()
{
	if (m_import_pb && m_project) {
		m_import_pb->setDisabled(m_project->isReadOnly());
	}
}

/**
	@brief ProjectAutoNumConfigPage::buildConnections
	setup some connections
*/
void ProjectAutoNumConfigPage::buildConnections()
{
		//Management Tab
	connect(m_amw, &AutoNumberingManagementW::applyPressed, this, &ProjectAutoNumConfigPage::applyManagement);

		//Conductor Tab
	connect(m_saw_conductor, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextConductor);
	connect(m_saw_conductor, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextConductor);
	connect(m_saw_conductor->contextComboBox(), &QComboBox::textActivated, this, &ProjectAutoNumConfigPage::updateContextConductor);

		//Element Tab
	connect(m_saw_element, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextElement);
	connect(m_saw_element, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextElement);
	connect(m_saw_element->contextComboBox(), &QComboBox::textActivated, this, &ProjectAutoNumConfigPage::updateContextElement);

		//Folio Tab
	connect(m_saw_folio, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextFolio);
	connect(m_saw_folio, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextFolio);
	connect(m_saw_folio->contextComboBox(), &QComboBox::textActivated, this, &ProjectAutoNumConfigPage::updateContextFolio);

		//	Auto Folio Numbering
	connect(m_faw, &FolioAutonumberingW::applyPressed, this, &ProjectAutoNumConfigPage::applyAutoNum);

		//Import from another project
	connect(m_import_pb, &QPushButton::clicked, this, &ProjectAutoNumConfigPage::importFromProject);
}

/**
	@brief ProjectAutoNumConfigPage::updateContext_conductor
	Display the current selected context for conductor
	@param str : key of context stored in project
*/
void ProjectAutoNumConfigPage::updateContextConductor(const QString& str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw_conductor -> setContext(NumerotationContext());
	else m_saw_conductor ->setContext(m_project->conductorAutoNum(str));
}

/**
	@brief ProjectAutoNumConfigPage::updateContext_folio
	Display the current selected context for folio
	@param str : key of context stored in project
*/
void ProjectAutoNumConfigPage::updateContextFolio(const QString& str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw_folio -> setContext(NumerotationContext());
	else m_saw_folio ->setContext(m_project->folioAutoNum(str));
}

/**
	@brief ProjectAutoNumConfigPage::updateContextElement
	Display the current selected context for element
	@param str : key of context stored in project
*/
void ProjectAutoNumConfigPage::updateContextElement(const QString& str)
{
	if (str == tr("Nom de la nouvelle numérotation"))
	{
		m_saw_element->setContext(NumerotationContext());
	}
	else
	{
		m_saw_element->setContext(m_project->elementAutoNum(str));
	}
}

/**
	@brief ProjectAutoNumConfigPage::saveContextElement
	Save the current displayed Element formula in project
*/
void ProjectAutoNumConfigPage::saveContextElement()
{
		// If the text is the default text "Name of new numerotation" save the edited context
		// With the the name "No name"
	if (m_saw_element->contextComboBox()->currentText() == tr("Nom de la nouvelle numérotation"))
	{
		QString title(tr("Sans nom"));

		m_project->addElementAutoNum (title, m_saw_element -> toNumContext());
		m_project->setCurrrentElementAutonum(title);
		m_saw_element->contextComboBox()->addItem(tr("Sans nom"));
	}
		// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> elementAutoNum().contains( m_saw_element->contextComboBox()->currentText()))
	{
		m_project->addElementAutoNum(m_saw_element->contextComboBox()->currentText(), m_saw_element->toNumContext());
		m_project->setCurrrentElementAutonum(m_saw_element->contextComboBox()->currentText());
		m_saw_element->contextComboBox()->addItem(m_saw_element->contextComboBox()->currentText());
	}
		// Else, the text already exist in the autonum of the project, just update the context
	else
	{
		m_project->addElementAutoNum (m_saw_element->contextComboBox() -> currentText(), m_saw_element -> toNumContext());
		m_project->setCurrrentElementAutonum(m_saw_element->contextComboBox()->currentText());
	}
}

/**
	@brief ProjectAutoNumConfigPage::importFromProject
	Read the automatic numbering rules stored in another .qet project and
	copy the ones the user selects into this project.

	The source file is parsed as plain XML rather than opened as a
	QETProject: opening it would run the whole load path, including the
	modal dialog raised when the file was written by a different version
	of QElectroTech.
*/
void ProjectAutoNumConfigPage::importFromProject()
{
	if (!m_project || m_project->isReadOnly()) {
		return;
	}

	const QString path = QFileDialog::getOpenFileName(
				this,
				tr("Importer les numérotations d'un projet"),
				m_project->currentDir(),
				tr("Projet QElectroTech (*.qet)"));
	if (path.isEmpty()) {
		return;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(this, tr("Import impossible"),
					 tr("Impossible d'ouvrir %1").arg(path));
		return;
	}

	QDomDocument doc;
	if (!doc.setContent(&file)) {
		QMessageBox::warning(this, tr("Import impossible"),
					 tr("%1 n'est pas un projet QElectroTech valide.")
					 .arg(QFileInfo(path).fileName()));
		return;
	}
	file.close();

	const QDomNodeList newdiagrams =
			doc.elementsByTagName(QStringLiteral("newdiagrams"));
	if (newdiagrams.isEmpty()) {
		QMessageBox::information(this, tr("Aucune numérotation"),
					 tr("Ce projet ne contient aucune numérotation automatique."));
		return;
	}
	const QDomElement root = newdiagrams.at(0).toElement();

		// tag of the group, tag of one entry, label shown to the user
	struct Category {
		QString group_tag;
		QString item_tag;
		QString label;
	};
	const QList<Category> categories {
		{QStringLiteral("conductors_autonums"),
		 QStringLiteral("conductor_autonum"), tr("Conducteurs")},
		{QStringLiteral("element_autonums"),
		 QStringLiteral("element_autonum"), tr("Eléments")},
		{QStringLiteral("folio_autonums"),
		 QStringLiteral("folio_autonum"), tr("Folios")}
	};

	QDialog dialog(this);
	dialog.setWindowTitle(tr("Numérotations à importer"));
	QVBoxLayout *layout = new QVBoxLayout(&dialog);
	layout->addWidget(new QLabel(
				  tr("Numérotations trouvées dans %1 :")
				  .arg(QFileInfo(path).fileName()), &dialog));

	QListWidget *list = new QListWidget(&dialog);
	layout->addWidget(list);

		// NumerotationContext is not a QVariant type, so the list item
		// carries an index into this instead of the context itself.
	QList<NumerotationContext> contexts;
	for (int i = 0 ; i < categories.count() ; ++i)
	{
		const Category &category = categories.at(i);
		QDomElement group;
		for (QDomNode n = root.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
			if (n.toElement().tagName() == category.group_tag) {
				group = n.toElement();
				break;
			}
		}
		if (group.isNull()) {
			continue;
		}

		for (QDomElement entry : QET::findInDomElement(group, category.item_tag))
		{
			const QString title = entry.attribute(QStringLiteral("title"));
			if (title.isEmpty()) {
				continue;
			}

			bool exists = false;
			switch (i) {
				case 0: exists = m_project->conductorAutoNum().contains(title); break;
				case 1: exists = m_project->elementAutoNum().contains(title); break;
				default: exists = m_project->folioAutoNum().contains(title); break;
			}

			QListWidgetItem *item = new QListWidgetItem(
						exists ? tr("%1 : %2 (existe déjà)")
							 .arg(category.label, title)
					       : QStringLiteral("%1 : %2")
							 .arg(category.label, title),
						list);
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(exists ? Qt::Unchecked : Qt::Checked);
			item->setData(Qt::UserRole, i);
			item->setData(Qt::UserRole + 1, title);

			NumerotationContext nc;
			nc.fromXml(entry);
			item->setData(Qt::UserRole + 2, contexts.count());
			contexts << nc;
		}
	}

	if (contexts.isEmpty()) {
		QMessageBox::information(this, tr("Aucune numérotation"),
					 tr("Ce projet ne contient aucune numérotation automatique."));
		return;
	}

	QCheckBox *overwrite_cb = new QCheckBox(
				tr("Remplacer les numérotations de même nom"), &dialog);
	layout->addWidget(overwrite_cb);

	QDialogButtonBox *buttons = new QDialogButtonBox(
				QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	if (dialog.exec() != QDialog::Accepted) {
		return;
	}

	int imported = 0, skipped = 0, conductors = 0;
	for (int row = 0 ; row < list->count() ; ++row)
	{
		QListWidgetItem *item = list->item(row);
		if (item->checkState() != Qt::Checked) {
			continue;
		}

		const int category = item->data(Qt::UserRole).toInt();
		const QString title = item->data(Qt::UserRole + 1).toString();

		bool exists = false;
		switch (category) {
			case 0: exists = m_project->conductorAutoNum().contains(title); break;
			case 1: exists = m_project->elementAutoNum().contains(title); break;
			default: exists = m_project->folioAutoNum().contains(title); break;
		}
		if (exists && !overwrite_cb->isChecked()) {
			++skipped;
			continue;
		}

		const NumerotationContext &nc =
				contexts.at(item->data(Qt::UserRole + 2).toInt());
		switch (category) {
			case 0:
				m_project->addConductorAutoNum(title, nc);
				++conductors;
				break;
			case 1: m_project->addElementAutoNum(title, nc); break;
			default: m_project->addFolioAutoNum(title, nc); break;
		}
		++imported;
	}

	readValuesFromProject();
	if (conductors) {
		m_project->conductorAutoNumAdded();
	}

	QMessageBox::information(
				this, tr("Import terminé"),
				skipped ? tr("%1 numérotation(s) importée(s), "
						 "%2 conservée(s) telles quelles.")
					  .arg(imported).arg(skipped)
					: tr("%1 numérotation(s) importée(s).").arg(imported));
}

/**
	@brief ProjectAutoNumConfigPage::removeContextElement
	Remove from project the current element numerotation context
*/
void ProjectAutoNumConfigPage::removeContextElement()
{
		//if default text, return
	if (m_saw_element->contextComboBox()->currentText() == tr("Nom de la nouvelle numérotation"))
		return;
	m_project->removeElementAutoNum (m_saw_element->contextComboBox()->currentText());
	m_saw_element->contextComboBox()->removeItem (m_saw_element->contextComboBox()->currentIndex());
	// removeItem() removes the current selection programmatically but
	// textActivated() does not react to (by design, see buildConnections()).
	// Refresh the displayed pattern explicitly so it matches the new selection.
	updateContextElement(m_saw_element->contextComboBox()->currentText());
}

/**
	@brief ProjectAutoNumConfigPage::saveContext_conductor
	Save the current displayed conductor context in project
*/
void ProjectAutoNumConfigPage::saveContextConductor()
{
		// If the text is the default text "Name of new numerotation" save the edited context
		// With the the name "No name"
	if (m_saw_conductor->contextComboBox()-> currentText() == tr("Nom de la nouvelle numérotation"))
	{
		m_project->addConductorAutoNum (tr("Sans nom"), m_saw_conductor -> toNumContext());
		project()->setCurrentConductorAutoNum(tr("Sans nom"));
		m_saw_conductor->contextComboBox()-> addItem(tr("Sans nom"));
	}
	// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> conductorAutoNum().contains( m_saw_conductor->contextComboBox()->currentText()))
	{
		project()->addConductorAutoNum(m_saw_conductor->contextComboBox()->currentText(), m_saw_conductor->toNumContext());
		project()->setCurrentConductorAutoNum(m_saw_conductor->contextComboBox()->currentText());
		m_saw_conductor->contextComboBox()-> addItem(m_saw_conductor->contextComboBox()->currentText());
	}
	// Else, the text already exist in the autonum of the project, just update the context
	else
	{
		project()->setCurrentConductorAutoNum(m_saw_conductor->contextComboBox()->currentText());
		m_project->addConductorAutoNum (m_saw_conductor->contextComboBox()-> currentText(), m_saw_conductor -> toNumContext());
	}
	project()->conductorAutoNumAdded();
}

/**
	@brief ProjectAutoNumConfigPage::saveContext_folio
	Save the current displayed folio context in project
*/
void ProjectAutoNumConfigPage::saveContextFolio()
{
	// If the text is the default text "Name of new numerotation" save the edited context
	// With the the name "No name"
	if (m_saw_folio->contextComboBox() -> currentText() == tr("Nom de la nouvelle numérotation")) {
		m_project->addFolioAutoNum (tr("Sans nom"), m_saw_folio -> toNumContext());
		m_saw_folio->contextComboBox() -> addItem(tr("Sans nom"));
	}
	// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> folioAutoNum().contains( m_saw_folio->contextComboBox()->currentText())) {
		project()->addFolioAutoNum(m_saw_folio->contextComboBox()->currentText(), m_saw_folio->toNumContext());
		m_saw_folio->contextComboBox() -> addItem(m_saw_folio->contextComboBox()->currentText());
	}
	// Else, the text already exist in the autonum of the project, just update the context
	else {
		m_project->addFolioAutoNum (m_saw_folio->contextComboBox() -> currentText(), m_saw_folio -> toNumContext());
	}
	project()->folioAutoNumAdded();
}

/**
	@brief ProjectAutoNumConfigPage::applyAutoNum
	Apply auto folio numbering, New Folios or Selected Folios
*/
void ProjectAutoNumConfigPage::applyAutoNum()
{

	if (m_faw->newFolios){
		int foliosRemaining = m_faw->newFoliosNumber();
		emit (saveCurrentTbp());
		emit (setAutoNum(m_faw->autoNumSelected()));
		while (foliosRemaining > 0){
			project()->autoFolioNumberingNewFolios();
			foliosRemaining = foliosRemaining-1;
		}
		emit (loadSavedTbp());
	}
	else{
		QString autoNum   = m_faw->autoNumSelected();
		int fromFolio = m_faw->fromFolio();
		int toFolio   = m_faw->toFolio();
		m_project->autoFolioNumberingSelectedFolios(fromFolio,toFolio,autoNum);
	}
}

/**
	@brief ProjectAutoNumConfigPage::applyAutoManagement
	Apply Management Options in Selected Folios
*/
void ProjectAutoNumConfigPage::applyManagement()
{
	//	int from;
	//	int to;
	//	//Apply to Entire Project
	//	if (m_amw->ui->m_apply_project_rb->isChecked()) {
	//		from = 0;
	//		to = project()->diagrams().size() - 1;
	//	}
	//	//Apply to selected Folios
	//	else {
	//		from =
	//m_amw->ui->m_from_folios_cb->itemData(m_amw->ui->m_from_folios_cb->currentIndex()).toInt();
	//		to =
	//m_amw->ui->m_to_folios_cb->itemData(m_amw->ui->m_to_folios_cb->currentIndex()).toInt();
	//	}

	//	//Conductor Autonumbering Update Policy
	//	//Allow Both Existent and New Conductors
	//	if (m_amw->ui->m_both_conductor_rb->isChecked()) {
	//		//Unfreeze Existent and New Conductors
	//		project()->freezeExistentConductorLabel(false, from,to);
	//		project()->freezeNewConductorLabel(false, from,to);
	//		project()->setFreezeNewConductors(false);
	//	}
	//	//Allow Only New
	//	else if (m_amw->ui->m_new_conductor_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Conductors
	//		project()->freezeExistentConductorLabel(true, from,to);
	//		project()->freezeNewConductorLabel(false, from,to);
	//		project()->setFreezeNewConductors(false);
	//	}
	//	//Allow Only Existent
	//	else if (m_amw->ui->m_existent_conductor_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Conductors
	//		project()->freezeExistentConductorLabel(false, from,to);
	//		project()->freezeNewConductorLabel(true, from,to);
	//		project()->setFreezeNewConductors(true);
	//	}
	//	//Disable
	//	else if (m_amw->ui->m_disable_conductor_rb->isChecked()) {
	//		//Freeze Existent and New Elements, Set Freeze Element Project Wide
	//		project()->freezeExistentConductorLabel(true, from,to);
	//		project()->freezeNewConductorLabel(true, from,to);
	//		project()->setFreezeNewConductors(true);
	//	}

	//	//Element Autonumbering Update Policy
	//	//Allow Both Existent and New Elements
	//	if (m_amw->ui->m_both_element_rb->isChecked()) {
	//		//Unfreeze Existent and New Elements
	//		project()->freezeExistentElementLabel(false, from,to);
	//		project()->freezeNewElementLabel(false, from,to);
	//		project()->setFreezeNewElements(false);
	//	}
	//	//Allow Only New
	//	else if (m_amw->ui->m_new_element_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Elements
	//		project()->freezeExistentElementLabel(true, from,to);
	//		project()->freezeNewElementLabel(false, from,to);
	//		project()->setFreezeNewElements(false);
	//	}
	//	//Allow Only Existent
	//	else if (m_amw->ui->m_existent_element_rb->isChecked()) {
	//		//Freeze New and Unfreeze Existent Elements, Set Freeze Element
	//Project Wide 		project()->freezeExistentElementLabel(false, from,to);
	//		project()->freezeNewElementLabel(true, from,to);
	//		project()->setFreezeNewElements(true);
	//	}
	//	//Disable
	//	else if (m_amw->ui->m_disable_element_rb->isChecked()) {
	//		//Freeze Existent and New Elements, Set Freeze Element Project Wide
	//		project()->freezeExistentElementLabel(true, from,to);
	//		project()->freezeNewElementLabel(true, from,to);
	//		project()->setFreezeNewElements(true);
	//	}

	//	//Folio Autonumbering Status
	//	if (m_amw->ui->m_both_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_new_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_existent_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_disable_folio_rb->isChecked()) {

	//	}
}

/**
	@brief ProjectAutoNumConfigPage::removeContext
	Remove from project the current conductor numerotation context
*/
void ProjectAutoNumConfigPage::removeContextConductor()
{
	//if default text, return
	if ( m_saw_conductor->contextComboBox()-> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	m_project -> removeConductorAutoNum (m_saw_conductor->contextComboBox()-> currentText() );
	m_saw_conductor->contextComboBox()-> removeItem (m_saw_conductor->contextComboBox()-> currentIndex() );
	// removeItem() removes the current selection programmatically but
	// textActivated() does not react to (by design, see buildConnections()).
	// Refresh the displayed pattern explicitly so it matches the new selection.
	updateContextConductor(m_saw_conductor->contextComboBox()->currentText());
	project()->conductorAutoNumRemoved();
}

/**
	@brief ProjectAutoNumConfigPage::removeContext_folio
	Remove from project the current folio numerotation context
*/
void ProjectAutoNumConfigPage::removeContextFolio()
{
	//if default text, return
	if ( m_saw_folio->contextComboBox() -> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	m_project -> removeFolioAutoNum (m_saw_folio->contextComboBox() -> currentText() );
	m_saw_folio->contextComboBox() -> removeItem (m_saw_folio->contextComboBox() -> currentIndex() );
	// removeItem() removes the current selection programmatically but
	// textActivated() does not react to (by design, see buildConnections()).
	// Refresh the displayed pattern explicitly so it matches the new selection.
	updateContextFolio(m_saw_folio->contextComboBox()->currentText());
	project()->folioAutoNumRemoved();
}

/**
	@brief ProjectAutoNumConfigPage::changeToTab
	@param i index
	Change to Selected Tab
*/
void ProjectAutoNumConfigPage::changeToTab(int i)
{
	qDebug()<<"Q_UNUSED"<<i;
}

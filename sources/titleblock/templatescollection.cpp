/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "templatescollection.h"
#include "titleblocktemplate.h"
#include "qetapp.h"
#include "qetproject.h"

/**
	Constructor
	@param parent Parent QObject
*/
TitleBlockTemplatesCollection::TitleBlockTemplatesCollection(QObject *parent) :
	QObject(parent),
	protocol_("unknown")
{
}

/**
	Destructor
*/
TitleBlockTemplatesCollection::~TitleBlockTemplatesCollection() {
}

/**
	@return the title of this collection
*/
QString TitleBlockTemplatesCollection::title() const {
	return(title_);
}

/**
	@oaram title New title for this collection
*/
void TitleBlockTemplatesCollection::setTitle(const QString &title) {
	title_ = title;
}

/**
	@return the protocol used by this collection ; examples: commontbt,
	customtbt, embedtbt, ...
*/
QString TitleBlockTemplatesCollection::protocol() const {
	return(protocol_);
}

/**
	Define the protocol for this collection
	@param protocol New protocol for this collection
*/
void TitleBlockTemplatesCollection::setProtocol(const QString &protocol) {
	if (!protocol.isEmpty()) protocol_ = protocol;
}

/**
	@return the project this collection is affiliated to, or 0 if this
	collection is not related to any project.
*/
QETProject *TitleBlockTemplatesCollection::parentProject() {
	return(0);
}

/**
	@return the templates contained within this collection, as a list of location
	objects.
	@see templates()
*/
QList<TitleBlockTemplateLocation> TitleBlockTemplatesCollection::templatesLocations() {
	QList<TitleBlockTemplateLocation> locations;
	foreach (QString template_name, templates()) {
		locations << location(template_name);
	}
	return(locations);
}

/**
	Constructor
	@param project Parent project
	@param parent Parent QObject
*/
TitleBlockTemplatesProjectCollection::TitleBlockTemplatesProjectCollection(QETProject *project, QObject *parent) :
	TitleBlockTemplatesCollection(parent),
	project_(project)
{
}

/**
	Destructor
*/
TitleBlockTemplatesProjectCollection::~TitleBlockTemplatesProjectCollection() {
}

/**
	@return a human readable title for this collection
*/
QString TitleBlockTemplatesProjectCollection::title() const {
	if (!title_.isEmpty()) return(title_);
	
	// if the title attribute is empty, we generate a suitable one using the
	// parent project
	QString final_title;
	if (project_) {
		QString project_title = project_ -> title();
		if (project_title.isEmpty()) {
			final_title = QString(
				tr(
					"Cartouches du projet sans titre (id %1)",
					"collection title when the parent project has an empty title -- %1 is the project internal id"
				)
			);
			final_title = final_title.arg(QETApp::projectId(project_));
		} else {
			final_title = QString(
				tr(
					"Cartouches du projet \"%1\"",
					"collection title when the project has a suitable title -- %1 is the project title"
				)
			);
			final_title = final_title.arg(project_title);
		}
	}
	return(final_title);
}

/**
	@return the protocol used to mention this collection
*/
QString TitleBlockTemplatesProjectCollection::protocol() const {
	if (project_) {
		int project_id = QETApp::projectId(project_);
		if (project_id != -1) {
			return(QString("project%1+embedtbt").arg(project_id));
		}
	}
	// fall back on the parent method
	return(TitleBlockTemplatesCollection::protocol());
}

/**
	@return the parent project of this project collection
*/
QETProject *TitleBlockTemplatesProjectCollection::parentProject() {
	return(project_);
}

/**
	@return the list of title block templates embedded within the project.
*/
QStringList TitleBlockTemplatesProjectCollection::templates() {
	return(titleblock_templates_xml_.keys());
}

/**
	@param template_name Name of the requested template
	@return the requested template, or 0 if there is no valid template of this
	name within the project
*/
TitleBlockTemplate *TitleBlockTemplatesProjectCollection::getTemplate(const QString &template_name){
	// Do we have already loaded this template?
	if (titleblock_templates_.contains(template_name)) {
		return(titleblock_templates_[template_name]);
	}
	
	// No? Do we even know of it?
	if (!titleblock_templates_xml_.contains(template_name)) {
		return(0);
	}
	
	// Ok, we have its XML description, we have to generate a TitleBlockTemplate object
	TitleBlockTemplate *titleblock_template = new TitleBlockTemplate(this);
	if (titleblock_template -> loadFromXmlElement(titleblock_templates_xml_[template_name])) {
		titleblock_templates_.insert(template_name, titleblock_template);
		return(titleblock_template);
	} else {
		return(0);
	}
}

/**
	@param template_name Name of the requested template
	@return the XML description of the requested template, or a null QDomElement
	if the project does not have such an titleblock template
*/
QDomElement TitleBlockTemplatesProjectCollection::getTemplateXmlDescription(const QString &template_name)  {
	if (titleblock_templates_xml_.contains(template_name)) {
		return(titleblock_templates_xml_[template_name]);
	}
	return(QDomElement());
}

/**
	This methods allows adding or modifying a template embedded within the
	project. This method emits the signal changed() if
	necessary.
	@param template_name Name / Identifier of the template - will be used to
	determine whether the given description will be added or will replace an
	existing one.
	@param xml_elmt An \<titleblocktemplate\> XML element describing the
	template. Its "name" attribute must equal to template_name.
	@return false if a problem occured, true otherwise
*/
bool TitleBlockTemplatesProjectCollection::setTemplateXmlDescription(const QString &template_name, const QDomElement &xml_elmt) {
	// check basic stuff
	if (xml_elmt.tagName() != "titleblocktemplate") {
		return(false);
	}
	
	// we *require* a project (at least for the moment...)
	if (!project_) return(false);
	
	// we import the provided XML element in the project document
	QDomElement import = xml_document_.importNode(xml_elmt, true).toElement();
	
	// ensure the name stored in the XML description remains consistent with the provided template name
	import.setAttribute("name", template_name);
	
	// we either replace the previous description
	if (titleblock_templates_xml_.contains(template_name)) {
		QDomElement old_description = titleblock_templates_xml_[template_name];
		if (!old_description.parentNode().isNull()) {
			old_description.parentNode().replaceChild(import, old_description);
		}
	}
	titleblock_templates_xml_.insert(template_name, import);
	
	if (titleblock_templates_.contains(template_name)) {
		titleblock_templates_[template_name] -> loadFromXmlElement(titleblock_templates_xml_[template_name]);
	}
	emit(changed(this, template_name));
	
	return(true);
}

/**
	This methods allows removing a template embedded within the project. This
	method emits the signal changed() if necessary.
	@param template_name Name of the template to be removed
*/
void TitleBlockTemplatesProjectCollection::removeTemplate(const QString &template_name) {
	emit(aboutToRemove(this, template_name));
	
	// remove the template itself
	titleblock_templates_xml_.remove(template_name);
	titleblock_templates_.remove(template_name);
	
	// warn the rest of the world that the list of templates embedded within this project has changed
	emit(changed(this, template_name));
}

/**
	@param template_name Name of the requested template
	@return the location object for the requested template
*/
TitleBlockTemplateLocation TitleBlockTemplatesProjectCollection::location(const QString &template_name) {
	return(TitleBlockTemplateLocation(template_name, this));
}

/**
	@return always false since a project collection is not stored on any
	filesystem.
*/
bool TitleBlockTemplatesProjectCollection::hasFilePath() {
	return(false);
}

/**
	@return always an empty string since a project collection is not stored on
	any filesystem.
*/
QString TitleBlockTemplatesProjectCollection::filePath() {
	return(QString());
}

/**
	@param template_name Either an empty QString to know whether the collection
	itself is read only, or a specific template name.
	@return true if the specified template is read only, false otherwise
*/
bool TitleBlockTemplatesProjectCollection::isReadOnly(const QString &template_name) const {
	Q_UNUSED(template_name)
	if (project_) {
		return(project_ -> isReadOnly());
	}
	return(false);
}

/**
	@param xml_element XML element to be parsed to load title block templates
*/
void TitleBlockTemplatesProjectCollection::fromXml(const QDomElement &xml_element) {
	foreach (QDomElement e, QET::findInDomElement(xml_element, "titleblocktemplates", "titleblocktemplate")) {
		// each titleblock template must have a name
		if (!e.hasAttribute("name")) continue;
		QString titleblock_template_name = e.attribute("name");
		
		// if several templates have the same name, we keep the first one encountered
		if (titleblock_templates_xml_.contains(titleblock_template_name)) continue;
		
		// we simply store the XML element describing the titleblock template,
		// without any further analysis for the moment
		titleblock_templates_xml_.insert(titleblock_template_name, e);
	}
}

/**
	Delete all title block templates not used within the parent project
*/
void TitleBlockTemplatesProjectCollection::deleteUnusedTitleBlocKTemplates() {
	if (!project_) return;
	
	foreach (QString template_name, templates()) {
		if (!project_ -> usesTitleBlockTemplate(location(template_name))) {
			removeTemplate(template_name);
		}
	}
}

/**
	Constructor
	@param path Path of the directory containing the collection
	@param parent Parent QObject
*/
TitleBlockTemplatesFilesCollection::TitleBlockTemplatesFilesCollection(const QString &path, QObject *parent) :
	TitleBlockTemplatesCollection(parent),
	dir_(
		path,
		QString("*%1").arg(TITLEBLOCKS_FILE_EXTENSION),
		QDir::SortFlags(QDir::Name | QDir::IgnoreCase),
		QDir::Readable | QDir::Files
	)
{
	if (dir_.exists()) {
		watcher_.addPath(dir_.canonicalPath());
	}
	connect(&watcher_, SIGNAL(directoryChanged(const QString &)), this, SLOT(fileSystemChanged(const QString &)));
}

/**
	Destructor
*/
TitleBlockTemplatesFilesCollection::~TitleBlockTemplatesFilesCollection() {
}

/**
	@return the canonical path of the directory hosting this collection.
*/
QString TitleBlockTemplatesFilesCollection::path(const QString &template_name) const {
	if (template_name.isEmpty()) {
		return(dir_.canonicalPath());
	} else {
		return(dir_.absoluteFilePath(toFileName(template_name)));
	}
}

/**
	@return the list of templates contained in this collection
*/
QStringList TitleBlockTemplatesFilesCollection::templates() {
	QStringList templates_names;
	QRegExp replace_regexp(QString("%1$").arg(TITLEBLOCKS_FILE_EXTENSION));
	foreach(QString name, dir_.entryList()) {
		templates_names << name.replace(replace_regexp, "");
	}
	return(templates_names);
}

/**
	@return the template which name is \a template_name, or 0 if the template
	could not be loaded.
*/
TitleBlockTemplate *TitleBlockTemplatesFilesCollection::getTemplate(const QString &template_name) {
	if (!templates().contains(template_name)) return(0);
	
	TitleBlockTemplate *tbtemplate = new TitleBlockTemplate();
	QString tbt_file_path = path(template_name);
	
	bool loading = tbtemplate -> loadFromXmlFile(tbt_file_path);
	if (!loading) {
		delete tbtemplate;
		return(0);
	}
	return(tbtemplate);
}

/**
	@param template_name Name of a template (which has to already exist)
	@return the XML description for the \a template_name template, or a null QDomElement if no such template exists.
*/
QDomElement TitleBlockTemplatesFilesCollection::getTemplateXmlDescription(const QString &template_name) {
	QString xml_file_path = path(template_name);
	
	QFileInfo xml_file_info(xml_file_path);
	if (!xml_file_info.exists() || !xml_file_info.isReadable()) {
		return(QDomElement());
	}
	
	QFile xml_file(xml_file_path);
	if (!xml_file.open(QIODevice::ReadOnly)) {
		return(QDomElement());
	}
	
	QDomDocument *xml_document = new QDomDocument();
	bool xml_parsing = xml_document -> setContent(&xml_file);
	if (!xml_parsing) {
		delete xml_document;
		return(QDomElement());
	}
	return(xml_document -> documentElement());
}

/**
	Set the XML description of the \a template_name template to \a xml_element.
	@param template_name Name of a template (which does not have to already exist)
	@param xml_element XML element describing the template
*/
bool TitleBlockTemplatesFilesCollection::setTemplateXmlDescription(const QString &template_name, const QDomElement &xml_element) {
	if (template_name.isEmpty()) return(false);
	
	// prevent the watcher from emitting signals while we open and write to file
	blockSignals(true);
	
	QDomDocument doc;
	doc.appendChild(doc.importNode(xml_element, true));
	
	bool writing = QET::writeXmlFile(doc, path(template_name));
	if (!writing) return(false);
	
	// emit a single signal for the change
	blockSignals(false);
	emit(changed(this, template_name));
	return(true);
}

/**
	Remove the \a template_name template.
*/
void TitleBlockTemplatesFilesCollection::removeTemplate(const QString &template_name) {
	emit(aboutToRemove(this, template_name));
	// prevent the watcher from emitting signals while we open and write to file
	blockSignals(true);
	
	dir_.remove(toFileName(template_name));
	
	// emit a single signal for the removal
	blockSignals(false);
	emit(changed(this, template_name));
}

/**
	@param template_name Name of a template supposed to be contained within
	this collection.
	@return 
*/
TitleBlockTemplateLocation TitleBlockTemplatesFilesCollection::location(const QString &template_name) {
	return(TitleBlockTemplateLocation(template_name, this));
}

/**
	@return always true since a files collection is always stored on a
	filesystem.
*/
bool TitleBlockTemplatesFilesCollection::hasFilePath() {
	return(true);
}

/**
	@return The filesystem path where this files collection is actually stored.
*/
QString TitleBlockTemplatesFilesCollection::filePath() {
	return(dir_.canonicalPath());
}

/**
	@param template_name Either an empty QString to know whether the collection
	itself is read only, or a specific template name.
	@return true if the specified template is read only, false otherwise
*/
bool TitleBlockTemplatesFilesCollection::isReadOnly(const QString &template_name) const {
	if (template_name.isEmpty()) {
		QFileInfo info(dir_.canonicalPath());
		return(!info.isWritable());
	} else {
		QFileInfo info(dir_.absoluteFilePath(toFileName(template_name)));
		return(!info.isWritable());
	}
}

/**
	@param file_name A file name
	@return the template name for \a file_name
*/
QString TitleBlockTemplatesFilesCollection::toTemplateName(const QString &file_name) {
	static QRegExp replace_regexp(QString("%1$").arg(TITLEBLOCKS_FILE_EXTENSION));
	QString template_name(file_name);
	return(template_name.replace(replace_regexp, ""));
}

/**
	@param template_name A template name
	@return the file name for \a template_name
*/
QString TitleBlockTemplatesFilesCollection::toFileName(const QString &template_name) {
	return(QString("%1%2").arg(template_name).arg(TITLEBLOCKS_FILE_EXTENSION));
}

/**
	Handle the changes occuring on the file system.
	@param str Path of the directory that changed.
*/
void TitleBlockTemplatesFilesCollection::fileSystemChanged(const QString &str) {
	Q_UNUSED(str);
	dir_.refresh();
	emit(changed(this));
}

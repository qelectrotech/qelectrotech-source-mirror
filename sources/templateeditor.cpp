#include "templateeditor.h"
#include <QtXml>
#include "qetproject.h"
#include "qetapp.h"

/**
	Constructor
	@param parent parent QWidget for this editor
	@param f Windows flags for this editor
	@see QWidget()
*/
TemplateEditor::TemplateEditor(QWidget *parent, Qt::WindowFlags f) :
	QWidget(parent, f),
	parent_project_(0)
{
	build();
}

/**
	Destructor
*/
TemplateEditor::~TemplateEditor() {
}

/**
	Edit the given template.
	@param project Parent project of the template to edit.
	@param template_name Name of the template to edit within its parent project.
*/
bool TemplateEditor::edit(QETProject *project, const QString &template_name) {
	// we require a project we will rattach templates to
	if (!project) return(false);
	parent_project_ = project;
	updateProjectLabel();
	
	// the template name may be empty to create a new element
	if (template_name.isEmpty()) {
		template_name_edit_ -> setText(tr("Nouveau-modele"));
		template_name_edit_ -> setReadOnly(false);
		return(true);
	}
	
	QDomElement xml_tb_template = project -> getTemplateXmlDescriptionByName(template_name);
	if (!xml_tb_template.isNull()) {
		QDomDocument xml_doc;
		xml_doc.appendChild(xml_doc.importNode(xml_tb_template, true));
		template_name_edit_ -> setText(template_name);
		template_name_edit_ -> setReadOnly(true);
		
		QString xml_str = xml_doc.toString(4);
		xml_str.replace(QRegExp("^<titleblocktemplate[^>]*>"), "");
		xml_str.replace(QRegExp("</titleblocktemplate>"), "");
		template_xml_edit_ -> setPlainText("    " + xml_str.trimmed());
		
		// stores the parent project and template name, in order to write/save the template later
		template_name_ = template_name;
		return(true);
	}
	return(false);
}

/**
	Validates the content of the current text area. It has to be a valid XML
	description of a title block template for this method not to display a
	message to the user.
	@todo implement it.
*/
void TemplateEditor::validate() {
	QMessageBox::information(
		this,
		tr("Not implemented yet"),
		tr("Sorry, Not implemented yet")
	);
}

/**
	Saves the content of the current text area to a template within the project.
*/
void TemplateEditor::save() {
	if (!parent_project_) return;
	
	// are we creating a new template?
	if (!template_name_edit_ -> isReadOnly()) {
		// Yes, so we must ensure the new name will not clatch with an existing ine
		if (parent_project_ -> embeddedTitleBlockTemplates().contains(template_name_edit_ -> text())) {
			QMessageBox::critical(
				this,
				tr("Un mod\350le de ce nom existe d\351j\340"),
				tr("Un mod\350le de ce nom existe d\351j\340 au sein du projet - veuillez choisir un autre nom.")
			);
			/// TODO propose to overwrite the existing template?
			return;
		}
		
	}
	
	QDomDocument xml_doc;
	bool parsing = xml_doc.setContent(getXmlString());
	if (!parsing) {
		QMessageBox::critical(
			this,
			tr("Code XML non valide"),
			tr("Le code XML du mod\350le ne semble pas \320tre valide. Impossible d'enregistrer le mod\350le.")
		);
		return;
	}
	
	if (!template_name_edit_ -> isReadOnly()) {
		template_name_edit_ -> setReadOnly(true);
		template_name_ = template_name_edit_ -> text();
	}
	parent_project_ -> setTemplateXmlDescription(template_name_, xml_doc.documentElement());
}

/**
	Exits this editor.
*/
void TemplateEditor::quit() {
	/// TODO save if needed
	close();
}

/**
	Builds the user interface.
*/
void TemplateEditor::build() {
	parent_project_label_ = new QLabel();
	updateProjectLabel();
	static_xml_1_ = new QLabel("<titleblocktemplate name=\"");
	static_xml_2_ = new QLabel("\">");
	static_xml_3_ = new QLabel("</titleblocktemplate>");
	template_name_edit_ = new QLineEdit();
	template_xml_edit_ = new QTextEdit();
	template_xml_edit_ -> setAcceptRichText(false);
	template_xml_edit_ -> setFontFamily("monospace");
	template_xml_edit_ -> setWordWrapMode(QTextOption::NoWrap);
	
	validate_button_ = new QPushButton(tr("V\351rifier le mod\350le"));
	save_button_ = new QPushButton(tr("Enregistrer et appliquer"));
	quit_button_ = new QPushButton(tr("Quitter"));
	
	connect(validate_button_, SIGNAL(released()), this, SLOT(validate()));
	connect(save_button_,     SIGNAL(released()), this, SLOT(save()));
	connect(quit_button_,     SIGNAL(released()), this, SLOT(quit()));
	
	QHBoxLayout *h_layout0 = new QHBoxLayout();
	h_layout0 -> addWidget(validate_button_);
	h_layout0 -> addWidget(save_button_);
	h_layout0 -> addWidget(quit_button_);
	
	QHBoxLayout *h_layout1 = new QHBoxLayout();
	h_layout1 -> addWidget(static_xml_1_);
	h_layout1 -> addWidget(template_name_edit_);
	h_layout1 -> addWidget(static_xml_2_);
	
	QVBoxLayout *v_layout0 = new QVBoxLayout();
	v_layout0 -> addWidget(parent_project_label_);
	v_layout0 -> addLayout(h_layout1);
	v_layout0 -> addWidget(template_xml_edit_);
	v_layout0 -> addWidget(static_xml_3_);
	v_layout0 -> addLayout(h_layout0);
	
	setLayout(v_layout0);
	
	setWindowTitle(tr("QElectroTech - \311diteur de mod\350le de cartouche"));
	resize(700, 500);
}

/**
	Updates the "Parent project:" label.
*/
void TemplateEditor::updateProjectLabel() {
	QString parent_project_title;
	if (parent_project_) {
		parent_project_title = parent_project_ -> pathNameTitle();
	} else {
		parent_project_title = tr("Non d\351fini");
	}
	
	parent_project_label_ -> setText(
		QString(tr("Projet parent : %1")).arg(parent_project_title)
	);
}

/**
	@return the XML description provided by the user, as a string.
*/
QString TemplateEditor::getXmlString() const {
	QString xml_str = QString("<titleblocktemplate name=\"%1\">%2</titleblocktemplate>");
	xml_str = xml_str.arg(Qt::escape(template_name_edit_ -> text())).arg(template_xml_edit_ -> toPlainText());
	return(xml_str);
}

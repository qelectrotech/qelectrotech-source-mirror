/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "qetelementeditor.h"
#include "ui_qetelementeditor.h"
#include "../elementscene.h"
#include "../../qeticons.h"
#include "../elementview.h"
#include "../../qetmessagebox.h"
#include "../../qetapp.h"
#include "../../recentfiles.h"
#include "../graphicspart/customelementpart.h"
#include "../elementitemeditor.h"
#include "../styleeditor.h"
#include "../esevent/eseventaddline.h"
#include "../esevent/eseventaddrect.h"
#include "../esevent/eseventaddellipse.h"
#include "../esevent/eseventaddpolygon.h"
#include "../esevent/eseventaddarc.h"
#include "../esevent/eseventaddtext.h"
#include "../esevent/eseventaddterminal.h"
#include "../esevent/eseventadddynamictextfield.h"
#include "../../elementdialog.h"
#include "../graphicspart/partterminal.h"
#include "../arceditor.h"
#include "ellipseeditor.h"
#include "lineeditor.h"
#include "polygoneditor.h"
#include "rectangleeditor.h"
#include "terminaleditor.h"
#include "texteditor.h"
#include "dynamictextfieldeditor.h"
#include "../../newelementwizard.h"
#include "../editorcommands.h"
#include "../../dxf/dxftoelmt.h"
#include "../../qet_elementscaler/qet_elementscaler.h"
#include "../UndoCommand/openelmtcommand.h"

#include <QSettings>
#include <QActionGroup>

/**
 * @brief QETElementEditor::QETElementEditor
 * @param parent
 */
QETElementEditor::QETElementEditor(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::QETElementEditor)
{
	ui->setupUi(this);
	initGui();
	setupActions();
	setupConnection();

	auto menu = createPopupMenu();
	menu->setTearOffEnabled(true);
	menu->setTitle(tr("Afficher", "menu entry"));
	menu->setIcon(QET::Icons::ConfigureToolbars);
	ui->m_display_menu->addMenu(menu);
	//ui->m_display_menu->insertMenu(ui->m_zoom_in_action, menu);

	setWindowState(Qt::WindowMaximized);
	readSettings();
	show();
}

/**
 * @brief QETElementEditor::~QETElementEditor
 */
QETElementEditor::~QETElementEditor()
{
	delete ui;
	qDeleteAll(m_editors.begin(), m_editors.end());
}

/**
 * @brief QETElementEditor::contextMenu
 * Display a context menu, with all available action.
 * @param p : the pos of the menu, in screen coordinate
 * @param actions : a list of actions who can be prepended to the context menu.
 */
void QETElementEditor::contextMenu(QPoint p, QList<QAction *> actions)
{
	QMenu menu(this);
	menu.addActions(std::move(actions));
	menu.addSeparator();
	menu.addAction(m_undo_action);
	menu.addAction(m_redo_action);
	menu.addAction(ui->m_select_all_act);
	menu.addAction(ui->m_deselect_all_action);
	menu.addAction(ui->m_revert_selection_action);
	menu.addSeparator();
	menu.addAction(ui->m_delete_action);
	menu.addAction(ui->m_cut_action);
	menu.addAction(ui->m_copy_action);
	menu.addSeparator();
	menu.addAction((ui->m_mirror_action));
	menu.addAction((ui->m_flip_action));
	menu.addAction((ui->m_rotate_action));
	menu.addSeparator();
	menu.addAction(ui->m_paste_action);
	menu.addAction(ui->m_paste_in_area_action);
	menu.addMenu(ui->m_paste_from_menu);
	menu.addSeparator();
	menu.addActions(m_depth_action_group -> actions());

		//Remove from the context menu the actions which are disabled.
	const QList<QAction *>menu_actions = menu.actions();
	for(QAction *action : menu_actions) {
		if(!action -> isEnabled()) {
			menu.removeAction(action);
		}
	}
	menu.exec(p);
}

/**
 * @brief QETElementEditor::setNames
 * Set the names list of the edited element
 * @param name_list
 */
void QETElementEditor::setNames(const NamesList &name_list)
{
	auto data = m_elmt_scene->elementData();
	data.m_names_list = name_list;
	m_elmt_scene->setElementData(data);
}

/**
 * @brief QETElementEditor::setLocation
 * Set the location to edit
 * @param location
 */
void QETElementEditor::setLocation(const ElementsLocation &location)
{
	m_location = location;
	m_opened_from_file = false;
	setReadOnly(!location.isWritable());
	updateTitle();

}

/**
 * @brief QETElementEditor::location
 * @return The location of the edited element
 */
ElementsLocation QETElementEditor::location() const {
	return m_location;
}

/**
 * @brief QETElementEditor::setFileName
 * Set the file name of the edited element
 * @param file_name
 */
void QETElementEditor::setFileName(const QString &file_name)
{
	m_file_name = file_name;
	m_opened_from_file = true;
	bool must_be_read_only = !QFileInfo(file_name).isWritable();
	if (isReadOnly() != must_be_read_only) {
		setReadOnly(must_be_read_only);
	}
	updateTitle();
}

/**
 * @brief QETElementEditor::fileName
 * @return the file name of this element
 */
QString QETElementEditor::fileName() const {
	return m_file_name;
}

void QETElementEditor::setReadOnly(bool ro)
{
	m_read_only = ro;
	m_view->setInteractive(!ro);
	updateAction();
}

bool QETElementEditor::isReadOnly() const {
	return m_read_only;
}

/**
 * @brief QETElementEditor::fromFile
 * Open an element from @filepath
 * @param filepath
 */
void QETElementEditor::fromFile(const QString &filepath)
{
	bool state_ = true;
	QString error_message;


	QFileInfo infos_file(filepath);
	if (!infos_file.exists() || !infos_file.isFile())
	{
		state_ = false;
		error_message = QString(tr("Le fichier %1 n'existe pas.", "message box content")).arg(filepath);
	}

	QFile file(filepath);
	if (state_ && !file.open(QIODevice::ReadOnly)) {
		state_ = false;
		error_message = QString(tr("Impossible d'ouvrir le fichier %1.", "message box content")).arg(filepath);
	}

	QDomDocument document_xml;
	if (state_) {
		if (!document_xml.setContent(&file)) {
			state_ = false;
			error_message = tr("Ce fichier n'est pas un document XML valide", "message box content");
		}
		file.close();
	}

	if (!state_) {
		QET::QetMessageBox::critical(this, tr("Erreur", "toolbar title"), error_message);
		return;
	}

	m_elmt_scene -> fromXml(document_xml);
	fillPartsList();

	if (!infos_file.isWritable())
	{
		QET::QetMessageBox::warning(
			this,
			tr("Édition en lecture seule", "message box title"),
			tr("Vous n'avez pas les privilèges nécessaires pour modifier cet élement. Il sera donc ouvert en lecture seule.", "message box content")
		);
		setReadOnly(true);
	}
	else {
		setReadOnly(false);
	}

	// memorise le fichier
	setFileName(filepath);
	QETApp::elementsRecentFiles() -> fileWasOpened(filepath);
	updateAction();
}

/**
 * @brief QETElementEditor::toFile
 * Save to a file the current edited element
 * @param filepath : path of the file
 * @return true if success
 */
bool QETElementEditor::toFile(const QString &filepath)
{
	m_elmt_scene -> clearEventInterface();
	m_elmt_scene -> clearSelection();
	UncheckAddPrimitive();

	QDomDocument element_xml = m_elmt_scene -> toXml();
	bool writing = QET::writeXmlFile(element_xml, filepath);
	if (!writing) {
		QET::QetMessageBox::warning(
			this,
			tr("Erreur", "message box title"),
			tr("Impossible d'écrire dans ce fichier", "message box content")
		);
	}
	return(writing);
}

/**
 * @brief QETElementEditor::fromLocation
 * Location of an element to edit.
 */
void QETElementEditor::fromLocation(const ElementsLocation &location)
{
	if (!location.isElement()) {
		QET::QetMessageBox::critical(this,
									 tr("Élément inexistant.", "message box title"),
									 tr("Le chemin virtuel choisi ne correspond pas à un élément.", "message box content"));
		return;
	}
	if (!location.exist()) {
		QET::QetMessageBox::critical(this,
									 tr("Élément inexistant.", "message box title"),
									 tr("L'élément n'existe pas.", "message box content"));
		return;
	}

		//The file must be an xml document
	QDomDocument document_xml;
	QDomNode node = document_xml.importNode(location.xml(), true);
	document_xml.appendChild(node);

		//Load the element
	m_elmt_scene -> fromXml(document_xml);
	fillPartsList();

		//location is read only
	if (!location.isWritable()) {
		QET::QetMessageBox::warning(this,
									tr("Édition en lecture seule", "message box title"),
									tr("Vous n'avez pas les privilèges nécessaires pour modifier cet élement. Il sera donc ouvert en lecture seule.", "message box content"));
		setReadOnly(true);
	}
	else {
		setReadOnly(false);
	}

	setLocation(location);
	updateAction();
}

/**
 * @brief QETElementEditor::toLocation
 * Save the edited element into @location
 * @param location
 * @return true if successfully saved
 */
bool QETElementEditor::toLocation(const ElementsLocation &location)
{
	m_elmt_scene -> clearEventInterface();
	m_elmt_scene -> clearSelection();
	UncheckAddPrimitive();

	if (!location.setXml(m_elmt_scene -> toXml())) {
		QET::QetMessageBox::critical(this,
									 tr("Erreur", "message box title"),
									 tr("Impossible d'enregistrer l'élément", "message box content"));
		return(false);
	}
	return(true);
}

/**
 * @brief QETElementEditor::isEditing
 * @param location
 * @return true if this editor is currently
 * editing element of location
 */
bool QETElementEditor::isEditing(const ElementsLocation &location)
{
	if (m_opened_from_file)
	{
		auto r = QET::compareCanonicalFilePaths(
					 m_file_name,
					 QETApp::realPath(location.toString()));
		return r;
	}
	else {
		return(location == m_location);
	}
}

/**
 * @brief QETElementEditor::isEditing
 * @param filepath
 * @return true if this editor is currently
 * editing element at filepath.
 */
bool QETElementEditor::isEditing(const QString &filepath)
{
	QString current_filepath;
	if (m_opened_from_file) {
		current_filepath = m_file_name;
	} else {
		current_filepath = QETApp::realPath(m_location.toString());
	}

	auto r = QET::compareCanonicalFilePaths(
				 current_filepath,
				 filepath);
	return r;
}

/**
 * @brief QETElementEditor::elementScene
 * @return
 */
ElementScene *QETElementEditor::elementScene() const {
	return m_elmt_scene;
}

/**
 * @brief QETElementEditor::elementView
 * @return
 */
ElementView *QETElementEditor::elementView() const {
	return m_view;
}

/**
 * @brief QETElementEditor::pasteOffset
 * @return the vertical and horizontal paste offset
 */
QPointF QETElementEditor::pasteOffset()
{
	QPointF paste_offset(5.0, 0.0);
	return(paste_offset);
}

/**
 * @brief QETElementEditor::getOpenElementFileName
 * Ask user to open a file who must be an element.
 * @param parent
 * @param dir
 * @return
 */
QString QETElementEditor::getOpenElementFileName(QWidget *parent, const QString &dir)
{
	QString user_filename = QFileDialog::getOpenFileName(
		parent,
		tr("Ouvrir un fichier", "dialog title"),
		dir.isEmpty() ? QETApp::customElementsDir() : dir,
		tr(
			"Éléments QElectroTech (*.elmt);;"
			"Fichiers XML (*.xml);;"
			"Tous les fichiers (*)",
			"filetypes allowed when opening an element file"
		)
	);
	return(user_filename);
}

/**
 * @brief QETElementEditor::updateTitle
 * Update the title of the windows
 */
void QETElementEditor::updateTitle()
{
	QString title = m_min_title;
	title += " - " + m_elmt_scene->elementData().m_names_list.name() + " ";
	if (!m_file_name.isEmpty() || !m_location.isNull()) {
		if (!m_elmt_scene -> undoStack().isClean()) {
			title += tr("[Modifié]", "window title tag");
		}
	}
	if (isReadOnly()) {
		title += tr(" [lecture seule]", "window title tag");
	}
	setWindowTitle(title);
}

/**
 * @brief QETElementEditor::fillPartsList
 */
void QETElementEditor::fillPartsList()
{
	m_parts_list -> blockSignals(true);
	m_parts_list -> clear();
	QList<QGraphicsItem *> qgis = m_elmt_scene -> zItems();

	QSettings settings;
	int maxParts = settings.value("elementeditor/max-parts-element-editor-list", 200).toInt();
	if (qgis.count() <= maxParts)
	{
		for (int j = qgis.count() - 1 ; j >= 0 ; -- j)
		{
			QGraphicsItem *qgi = qgis[j];
			if (CustomElementPart *cep = dynamic_cast<CustomElementPart *>(qgi))
			{
				QString part_desc = cep -> name();
				if (PartTerminal *terminal = dynamic_cast<PartTerminal *>(qgi)) {
					const auto t_name { terminal->terminalName() } ;
					if (!t_name.isEmpty()) {
						part_desc += QLatin1String(" : ") + t_name;
					}
				}
				QListWidgetItem *qlwi = new QListWidgetItem(part_desc);
				QVariant v;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
				v.setValue<QGraphicsItem *>(qgi);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
				qDebug()<<"Help code for QT 6 or later";
#endif
				qlwi -> setData(42, v);
				m_parts_list -> addItem(qlwi);
				qlwi -> setSelected(qgi -> isSelected());
			}
		}
	}
	else {
		m_parts_list -> addItem(new QListWidgetItem(tr("Trop de primitives, liste non générée: %1").arg(qgis.count())));
	}
	m_parts_list -> blockSignals(false);
}

/**
 * @brief QETElementEditor::UncheckAddPrimitive
 */
void QETElementEditor::UncheckAddPrimitive() {
	for (auto action : m_add_part_action_grp->actions()) {
		action->setChecked(false);
	}
}

/**
 * @brief QETElementEditor::updateCurrentPartEditor
 */
void QETElementEditor::updateCurrentPartEditor()
{
	if (!m_tools_dock_stack -> currentIndex()) {
		return;
	}

	if (ElementItemEditor *current_editor = dynamic_cast<ElementItemEditor *>(m_tools_dock_stack -> widget(1))) {
		current_editor -> updateForm();
	}
}

/**
 * @brief QETElementEditor::updateInformations
 * Update the information and editor dock.
 */
void QETElementEditor::updateInformations()
{
	QList<QGraphicsItem *> selected_qgis = m_elmt_scene -> selectedItems();
	if (selected_qgis.isEmpty())
	{
		clearToolsDock();
		m_default_informations -> setText(tr("%n partie(s) sélectionnée(s).",
											 "",
											 selected_qgis.size()));
		m_default_informations -> setAlignment(Qt::AlignHCenter | Qt::AlignTop);
		m_tools_dock_stack -> setCurrentIndex(0);
		return;
	}

	QList<CustomElementPart *> cep_list;

	CustomElementPart* part = dynamic_cast<CustomElementPart *>(selected_qgis.first());
	QString selection_xml_name = part -> xmlName();
	bool same_xml_name = true;
	bool style_editable = true;
	for (QGraphicsItem *qgi: selected_qgis)
	{
		if (CustomElementPart *cep = dynamic_cast<CustomElementPart *>(qgi)) {
			cep_list << cep;
			if (cep -> xmlName() != selection_xml_name) {
				same_xml_name = false;
			}
		}
		else {
			style_editable = false;
			same_xml_name = false;
		}
	}
	if (style_editable) {
		style_editable = StyleEditor::isStyleEditable(cep_list);
	}

	if (same_xml_name)
	{
		if (   selection_xml_name == "text"
			|| selection_xml_name == "dynamic_text"
			|| selection_xml_name == "line"
			|| selection_xml_name == "rect"
			|| selection_xml_name == "ellipse"
			|| selection_xml_name == "arc")
		{
			clearToolsDock();
			//We add the editor widget
			ElementItemEditor *editor = static_cast<ElementItemEditor*>(m_editors[selection_xml_name]);

#if TODO_LIST
#pragma message("@TODO Check if it takes longer than setting the parts again to the editor.")
#endif
			// TODO: Check if it takes longer than setting the parts again to the editor.
			bool equal = true;
			QList<CustomElementPart*> parts = editor -> currentParts();
			if (parts.length() == cep_list.length()) {
				for (auto cep: cep_list) {
					bool part_found = false;
					for (auto part: parts) {
						if (part == cep) {
							part_found = true;
							break;
						}
					}
					if (!part_found) {
						equal = false;
						break;
					}
				}
			}
			else {
				equal = false;
			}

			if (editor) {
				bool success = true;
				if (equal == false) {
					success = editor -> setParts(cep_list);
				}
				if (success) {
					m_tools_dock_stack -> insertWidget(1, editor);
					m_tools_dock_stack -> setCurrentIndex(1);
				}
				else {
					qDebug() << "Editor refused part.";
				}
			}
			return;
		}
		else if (cep_list.length() == 1 &&
				 (selection_xml_name == "polygon" || selection_xml_name == "terminal"))
		{
#if TODO_LIST
#pragma message("@TODO maybe allowing multipart edit when number of points is the same?")
#endif
			// multi edit for polygons makes no sense
			// TODO: maybe allowing multipart edit when number of points is the same?
			//We add the editor widget
			clearToolsDock();
			ElementItemEditor *editor = static_cast<ElementItemEditor*>(m_editors[selection_xml_name]);
			CustomElementPart* part = editor -> currentPart();
			bool equal = part == cep_list.first();

			if (editor) {
				bool success = true;
				if (equal == false) {
					success = editor -> setPart(cep_list.first());
				}
				if (success) {
					m_tools_dock_stack -> insertWidget(1, editor);
					m_tools_dock_stack -> setCurrentIndex(1);
				}
				else {
					qDebug() << "Editor refused part.";
				}
			}
			return;

		}
		else {
			qDebug() << "Multiedit not supported for: " << cep_list.first() -> xmlName();
		}

	}

	//There's several parts selecteds and all can be edited by style editor.
	if (style_editable) {
		clearToolsDock();
		ElementItemEditor *selection_editor = m_editors["style"];
		if (selection_editor) {
			if (selection_editor -> setParts(cep_list)) {
				m_tools_dock_stack -> insertWidget(1, selection_editor);
				m_tools_dock_stack -> setCurrentIndex(1);
			}
			else {
				qDebug() << "Editor refused part.";
			}
		}
	}
	//Else we only display the number of selected items
	else {
		clearToolsDock();
		m_default_informations -> setText(tr("%n partie(s) sélectionnée(s).",
											 "",
											 selected_qgis.size()));
		m_default_informations -> setAlignment(Qt::AlignHCenter | Qt::AlignTop);
		m_tools_dock_stack -> setCurrentIndex(0);
	}
}

/**
 * @brief QETElementEditor::updatePartsList
 */
void QETElementEditor::updatePartsList()
{
	int items_count = m_elmt_scene -> items().count();
	QSettings settings;
  const int maxParts = settings.value("elementeditor/max-parts-element-editor-list", 200).toInt();
	if (m_parts_list -> count() != items_count) {
		fillPartsList();
	}
	else if (items_count <= maxParts) {
		m_parts_list -> blockSignals(true);
		int i = 0;
		QList<QGraphicsItem *> items = m_elmt_scene -> zItems();
		for (int j = items.count() - 1 ; j >= 0 ; -- j) {
			QGraphicsItem *qgi = items[j];
			QListWidgetItem *qlwi = m_parts_list -> item(i);
			if (qlwi) qlwi -> setSelected(qgi -> isSelected());
			++ i;
		}
		m_parts_list -> blockSignals(false);
	}
}

/**
 * @brief QETElementEditor::updateSelectionFromPartsList
 */
void QETElementEditor::updateSelectionFromPartsList()
{
	m_elmt_scene  -> blockSignals(true);
	m_parts_list -> blockSignals(true);
	for (int i = 0 ; i < m_parts_list -> count() ; ++ i) {
		QListWidgetItem *qlwi = m_parts_list -> item(i);
		QGraphicsItem *qgi = qlwi -> data(42).value<QGraphicsItem *>();
		if (qgi) {
			qgi -> setSelected(qlwi -> isSelected());
		}
	}
	m_parts_list -> blockSignals(false);
	m_elmt_scene -> blockSignals(false);
	updateInformations();
	updateAction();
}

/**
 * @brief QETElementEditor::checkElement
 * Do several check about element
 * @return true if there is no error
 */
bool QETElementEditor::checkElement()
{
		//List of warning and error
	typedef QPair<QString, QString> QETWarning;
	QList<QETWarning> warnings;
	QList<QETWarning> errors;

		// Warning #1: Element haven't got terminal
		// (except for report, because report must have one terminal and this checking is do below)
	if (!m_elmt_scene -> containsTerminals() &&
		!(m_elmt_scene->elementData().m_type & ElementData::AllReport)) {
		warnings << qMakePair(
						tr("Absence de borne", "warning title"),
						tr(
							"<br>En l'absence de borne, l'élément ne pourra être"
			" relié à d'autres éléments par l'intermédiaire de conducteurs.",
							"warning description"
		)
						);
	}

		// Check folio report element
	if (m_elmt_scene->elementData().m_type & ElementData::AllReport)
	{
		int terminal =0;

		for(auto qgi : m_elmt_scene -> items()) {
			if (qgraphicsitem_cast<PartTerminal *>(qgi)) {
				terminal ++;
			}
		}

			//Error folio report must have only one terminal
		if (terminal != 1) {
			errors << qMakePair (tr("Absence de borne"),
								 tr("<br><b>Erreur</b> :"
								"<br>Les reports de folio doivent posséder une seul borne."
								"<br><b>Solution</b> :"
								"<br>Verifier que l'élément ne possède qu'une seul borne"));
		}
	}

	if (!errors.count() && !warnings.count()) {
		return(true);
	}

		// Display warnings
	QString dialog_message = tr("La vérification de cet élément a généré", "message box content");

	if (errors.size()) {
		dialog_message += QString(tr(" %n erreur(s)", "errors", errors.size()));
	}

	if (warnings.size()) {
		if (errors.size()) {
			dialog_message += QString (tr(" et"));
		}
		dialog_message += QString (tr(" %n avertissement(s)", "warnings", warnings.size()));
	}
	dialog_message += " :";

	dialog_message += "<ol>";
	QList<QETWarning> total = warnings << errors;
	for(QETWarning warning : total)
	{
		dialog_message += "<li>";
		dialog_message += QString(
							  tr("<b>%1</b> : %2", "warning title: warning description")
							  ).arg(warning.first).arg(warning.second);
		dialog_message += "</li>";
	}
	dialog_message += "</ol>";

	if (errors.size()) {
		QMessageBox::critical(this, tr("Erreurs"), dialog_message);
	}
	else {
		QMessageBox::warning(this, tr("Avertissements"), dialog_message);
	}

		//if error == 0 that means they are only warning, we return true.
	if (errors.count() == 0) {
		return(true);
	}
	return false;
}

/**
 * @brief QETElementEditor::event
 * @param event
 * @return
 */
bool QETElementEditor::event(QEvent *event)
{
	if (m_first_activation && event->type() == QEvent::WindowActivate) {
		m_first_activation = false;
		QTimer::singleShot(250, m_view, SLOT(zoomFit()));
	}

	return QMainWindow::event(event);
}

/**
 * @brief QETElementEditor::openElement
 * OPen an element from @filepath
 * @param filepath
 */
void QETElementEditor::openElement(const QString &filepath)
{
	if (filepath.isEmpty()) {
		return;
	}
		// we have to test the file existence here because QETApp::openElementFiles()
		// will discard non-existent files through QFileInfo::canonicalFilePath()
	if (!QFile::exists(filepath)) {
		QET::QetMessageBox::critical(
			this,
			tr("Impossible d'ouvrir le fichier", "message box title"),
			QString(
				tr("Il semblerait que le fichier %1 que vous essayez d'ouvrir"
				" n'existe pas ou plus.")
			).arg(filepath)
		);
	}
	QETApp::instance() -> openElementFiles(QStringList() << filepath);
}

/**
 * @brief QETElementEditor::closeEvent
 * @param qce
 */
void QETElementEditor::closeEvent(QCloseEvent *qce)
{
	if (canClose()) {
		writeSettings();
		setAttribute(Qt::WA_DeleteOnClose);
		m_elmt_scene -> reset();
		qce -> accept();
	}
	else {
		qce -> ignore();
	}
}

/**
 * @brief QETElementEditor::canClose
 * @return
 */
bool QETElementEditor::canClose()
{
	if (m_elmt_scene -> undoStack().isClean()) {
		return(true);
	}
		//First ask user to save
	QMessageBox::StandardButton answer = QET::QetMessageBox::question(
		this,
		tr("Enregistrer l'élément en cours ?", "dialog title"),
		QString(
			tr(
				"Voulez-vous enregistrer l'élément %1 ?",
				"dialog content - %1 is an element name"
			)
		).arg(m_elmt_scene->elementData().m_names_list.name()),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
		QMessageBox::Cancel
	);

	bool result;
	switch(answer) {
		case QMessageBox::Cancel: {
			result = false;
			break;
		}
		case QMessageBox::Yes: {
			result = on_m_save_action_triggered();
			break;
		}
		default: {
			result = true;
		}
	}
	return(result);
}

/**
 * @brief QETElementEditor::readSettings
 * Read some setting about this widget in the QSetting
 * of qelectrotech
 */
void QETElementEditor::readSettings()
{
	QSettings settings;

	QVariant geometry = settings.value("elementeditor/geometry");
	if (geometry.isValid()) {
		restoreGeometry(geometry.toByteArray());
	}

	QVariant state = settings.value("elementeditor/state");
	if (state.isValid()) {
		restoreState(state.toByteArray());
	}

	auto data = m_elmt_scene->elementData();
	data.m_drawing_information = settings.value("elementeditor/default-informations", "").toString();
	m_elmt_scene->setElementData(data);
}

/**
 * @brief QETElementEditor::writeSettings
 * Write some setting of this widget in the
 * QSetting of qelectrotech
 */
void QETElementEditor::writeSettings() const
{
	QSettings settings;
	settings.setValue("elementeditor/geometry", saveGeometry());
	settings.setValue("elementeditor/state", saveState());
}

/**
 * @brief QETElementEditor::setupActions
 */
void QETElementEditor::setupActions()
{
	m_undo_action = m_elmt_scene -> undoStack().createUndoAction(this, tr("Annuler"));
	m_redo_action = m_elmt_scene -> undoStack().createRedoAction(this, tr("Refaire"));
	m_undo_action -> setIcon(QET::Icons::EditUndo);
	m_redo_action -> setIcon(QET::Icons::EditRedo);
	m_undo_action -> setShortcuts(QKeySequence::Undo);
	m_redo_action -> setShortcuts(QKeySequence::Redo);
	ui->m_undo_toolbar->addAction(m_undo_action);
	ui->m_undo_toolbar->addAction(m_redo_action);

	ui->m_new_action              -> setShortcut(QKeySequence::New);
	ui->m_open_action             -> setShortcut(QKeySequence::Open);
	ui->m_open_from_file_action   -> setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_O);
	ui->m_save_action             -> setShortcut(QKeySequence::Save);
	ui->m_save_as_file_action     -> setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
	ui->m_select_all_act          -> setShortcut(QKeySequence::SelectAll);
	ui->m_deselect_all_action     -> setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
	ui->m_revert_selection_action -> setShortcut(Qt::CTRL | Qt::Key_I);
	ui->m_cut_action              -> setShortcut(QKeySequence::Cut);
	ui->m_copy_action             -> setShortcut(QKeySequence::Copy);
	ui->m_paste_action            -> setShortcut(QKeySequence::Paste);
	ui->m_paste_in_area_action    -> setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_V);
	ui->m_edit_names_action       -> setShortcut(Qt::CTRL | Qt::Key_E);
	ui->m_edit_author_action      -> setShortcut(Qt::CTRL | Qt::Key_Y);

#ifdef Q_OS_MAC
	ui->m_delete_action -> setShortcut(Qt::Key_Backspace);
	ui->m_quit_action -> setShortcut(Qt::CTRL | Qt::Key_W);
#else
	ui->m_delete_action -> setShortcut(Qt::Key_Delete);
	ui->m_quit_action -> setShortcut(Qt::CTRL | Qt::Key_Q);
#endif

		//Depth action
	m_depth_action_group = QET::depthActionGroup(this);
	connect(m_depth_action_group, &QActionGroup::triggered, [this](QAction *action) {
		this -> elementScene() -> undoStack().push(
			new ChangeZValueCommand(this -> elementScene(), action -> data().value<QET::DepthOption>()));
		emit(this -> elementScene() -> partsZValueChanged());
	});
	auto depth_toolbar = addToolBar(tr("Profondeur", "toolbar title"));
	depth_toolbar -> setObjectName("depth_toolbar");
	depth_toolbar -> addActions(m_depth_action_group -> actions());
	addToolBar(Qt::TopToolBarArea, depth_toolbar);

		//Rotate action
	ui->m_rotate_action -> setShortcut(Qt::Key_Space);
	connect(ui->m_rotate_action, &QAction::triggered, [this]() {this -> elementScene() -> undoStack().push(new RotateElementsCommand(this->elementScene()));});

		//Rotate Fine action = rotate with smaller inkrement
	ui->m_rotateFine_action -> setShortcut(Qt::CTRL | Qt::Key_Space);
	connect(ui->m_rotateFine_action, &QAction::triggered, [this]() {this -> elementScene() -> undoStack().push(new RotateFineElementsCommand(this->elementScene()));});

		//Flip action
	ui->m_flip_action -> setShortcut(Qt::Key_F);
	connect(ui->m_flip_action, &QAction::triggered, [this]() {this -> elementScene() -> undoStack().push(new FlipElementsCommand(this->elementScene()));});

		//Mirror action
	ui->m_mirror_action -> setShortcut(Qt::Key_M);
	connect(ui->m_mirror_action, &QAction::triggered, [this]() {this -> elementScene() -> undoStack().push(new MirrorElementsCommand(this->elementScene()));});


		//Zoom action
	ui->m_zoom_in_action       -> setShortcut(QKeySequence::ZoomIn);
	ui->m_zoom_out_action      -> setShortcut(QKeySequence::ZoomOut);
	ui->m_zoom_fit_best_action -> setShortcut(Qt::CTRL | Qt::Key_9);
	ui->m_zoom_original_action -> setShortcut(Qt::CTRL | Qt::Key_0);

		//Add primitive actions
	m_add_part_action_grp = new QActionGroup(this);

	auto *add_line               = new QAction(QET::Icons::PartLine,      tr("Ajouter une ligne"),                m_add_part_action_grp);
	auto *add_rectangle          = new QAction(QET::Icons::PartRectangle, tr("Ajouter un rectangle"),             m_add_part_action_grp);
	auto *add_ellipse            = new QAction(QET::Icons::PartEllipse,   tr("Ajouter une ellipse"),              m_add_part_action_grp);
	auto *add_polygon            = new QAction(QET::Icons::PartPolygon,   tr("Ajouter un polygone"),              m_add_part_action_grp);
	auto *add_text               = new QAction(QET::Icons::PartText,      tr("Ajouter du texte"),                 m_add_part_action_grp);
	auto *add_arc                = new QAction(QET::Icons::PartArc,       tr("Ajouter un arc de cercle"),         m_add_part_action_grp);
	auto *add_terminal           = new QAction(QET::Icons::Terminal,      tr("Ajouter une borne"),                m_add_part_action_grp);
	auto *add_dynamic_text_field = new QAction(QET::Icons::PartTextField, tr("Ajouter un champ texte dynamique"), m_add_part_action_grp);

	for (auto action : m_add_part_action_grp->actions()) {
		action -> setCheckable(true);
	}

	connect(add_line,      &QAction::triggered, [this]() {m_elmt_scene->setEventInterface(new ESEventAddLine(m_elmt_scene));});
	connect(add_rectangle, &QAction::triggered, [this]() {m_elmt_scene->setEventInterface(new ESEventAddRect(m_elmt_scene));});
	connect(add_ellipse,   &QAction::triggered, [this]() {m_elmt_scene->setEventInterface(new ESEventAddEllipse(m_elmt_scene));});
	connect(add_polygon,   &QAction::triggered, [this]() {m_elmt_scene->setEventInterface(new ESEventAddPolygon(m_elmt_scene));});
	connect(add_text,      &QAction::triggered, [this]() {m_elmt_scene->setEventInterface(new ESEventAddText(m_elmt_scene));});
	connect(add_arc,       &QAction::triggered, [this]() {m_elmt_scene->setEventInterface(new ESEventAddArc(m_elmt_scene));});
	connect(add_terminal,  &QAction::triggered, [this]() {m_elmt_scene->setEventInterface(new ESEventAddTerminal(m_elmt_scene));});
	connect(add_dynamic_text_field, &QAction::triggered, [this]() {m_elmt_scene->setEventInterface(new ESEventAddDynamicTextField(m_elmt_scene));});

	add_polygon -> setStatusTip(tr("Double-click pour terminer la forme, Click droit pour annuler le dernier point"));
	add_text    -> setStatusTip(tr("Ajouter un texte d'élément non éditable dans les schémas"));
	add_dynamic_text_field -> setStatusTip(tr("Ajouter un texte d'élément pouvant être édité dans les schémas"));

	auto parts_toolbar =  addToolBar(tr("Parties", "toolbar title"));
	parts_toolbar -> setAllowedAreas(Qt::AllToolBarAreas);
	parts_toolbar -> setObjectName("parts");
	parts_toolbar -> addActions(m_add_part_action_grp -> actions());
	addToolBar(Qt::LeftToolBarArea, parts_toolbar);
}

/**
 * @brief QETElementEditor::updateAction
 * Update actions
 */
void QETElementEditor::updateAction()
{
		//Action disabled if read only
	auto ro_list = m_add_part_action_grp->actions();
	ro_list << ui->m_select_all_act
			<< ui->m_revert_selection_action
			<< ui->m_paste_from_file_action
			<< ui->m_paste_from_element_action;
	for (auto action : std::as_const(ro_list)) {
		action->setDisabled(m_read_only);
	}

		//Action enabled if a primitive is selected
	auto select_list = m_depth_action_group->actions();
	select_list << ui->m_deselect_all_action
				<< ui->m_cut_action
				<< ui->m_copy_action
				<< ui->m_delete_action
				<< ui->m_rotate_action
				<< ui->m_rotateFine_action
				<< ui->m_flip_action
				<< ui->m_mirror_action;
	auto items_selected = !m_read_only && m_elmt_scene->selectedItems().count();
	for (auto action : std::as_const(select_list)) {
		action->setEnabled(items_selected);
	}

		//Action about clipboard
	auto clipboard_contain_elmt = !m_read_only && ElementScene::clipboardMayContainElement();
	ui->m_paste_action->setEnabled(clipboard_contain_elmt);
	ui->m_paste_in_area_action->setEnabled(clipboard_contain_elmt);

		//Action about undo stack status
	ui->m_save_action-> setEnabled(!m_read_only && !m_elmt_scene->undoStack().isClean());
	m_undo_action    -> setEnabled(!m_read_only && m_elmt_scene -> undoStack().canUndo());
	m_redo_action    -> setEnabled(!m_read_only && m_elmt_scene -> undoStack().canRedo());

}

/**
 * @brief QETElementEditor::setupConnection
 */
void QETElementEditor::setupConnection()
{
	connect(m_elmt_scene, &ElementScene::partsAdded,          this, &QETElementEditor::UncheckAddPrimitive);
	connect(m_elmt_scene, &ElementScene::partsAdded,          this, &QETElementEditor::fillPartsList);
	connect(m_elmt_scene, &ElementScene::partsRemoved,        this, &QETElementEditor::fillPartsList);
	connect(m_elmt_scene, &ElementScene::partsZValueChanged,  this, &QETElementEditor::fillPartsList);
	connect(m_parts_list, &QListWidget::itemSelectionChanged, this, &QETElementEditor::updateSelectionFromPartsList);
	connect(QApplication::clipboard(),  &QClipboard::dataChanged, this, &QETElementEditor::updateAction);

	connect(m_elmt_scene, &ElementScene::selectionChanged, [this]() {
		this->updateInformations();
		this->updateAction();
		this->updatePartsList();
	});

	connect(&(m_elmt_scene -> undoStack()), &QUndoStack::cleanChanged, [this]() {
		this->updateAction();
		this->updateTitle();
	});

	connect(&(m_elmt_scene -> undoStack()), &QUndoStack::indexChanged, [this]() {
		this->updateInformations();
	});
}

/**
 * @brief QETElementEditor::initGui
 */
void QETElementEditor::initGui()
{
	m_editors["arc"]          = new ArcEditor(this);
	m_editors["ellipse"]      = new EllipseEditor(this);
	m_editors["line"]         = new LineEditor(this);
	m_editors["polygon"]      = new PolygonEditor(this);
	m_editors["rect"]         = new RectangleEditor(this);
	m_editors["terminal"]     = new TerminalEditor(this);
	m_editors["text"]         = new TextEditor(this);
	m_editors["style"]        = new StyleEditor(this);
	m_editors["dynamic_text"] = new DynamicTextFieldEditor(this);

	m_elmt_scene = new ElementScene(this, this);
	m_view = new ElementView(m_elmt_scene, this);
	m_view->setDragMode(QGraphicsView::RubberBandDrag);
	setCentralWidget(m_view);

		//Part dock editor
	m_default_informations = new QLabel(this);
	m_tools_dock_stack = new QStackedWidget(this);
	m_tools_dock_stack->insertWidget(0, m_default_informations);
	ui->m_tools_dock->setWidget(m_tools_dock_stack);

		//Undo dock
	auto undo_view = new QUndoView(&(m_elmt_scene->undoStack()), this);
	undo_view->setEmptyLabel(tr("Aucune modification"));
	ui->m_undo_dock->setWidget(undo_view);

		//parts list dock
	m_parts_list = new QListWidget(this);
	m_parts_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	tabifyDockWidget(ui->m_undo_dock, ui->m_parts_dock);
	ui->m_parts_dock->setWidget(m_parts_list);

	updateInformations();
	fillPartsList();

	statusBar()->showMessage(tr("Éditeur d'éléments", "status bar message"));
}

/**
 * @brief QETElementEditor::clearToolsDock
 * Remove and hide the widget displayed by
 * the dock used to edit primitives
 * @return Return the removed widget or nullptr if there is no widget to remove.
 */
QWidget *QETElementEditor::clearToolsDock()
{
	if (QWidget *previous_widget = m_tools_dock_stack -> widget(1)) {
		m_tools_dock_stack -> removeWidget(previous_widget);
		previous_widget -> hide();
		return(previous_widget);
	}
	return(nullptr);
}

/**
 * @brief QETElementEditor::copyAndPasteXml
 * Copy the content of @xml_document to the clipboard
 * and call pasteInArea method of elementView.
 * @param xml_document
 */
void QETElementEditor::copyAndPasteXml(const QDomDocument &xml_document)
{
	QClipboard *clipboard = QApplication::clipboard();
	QString clipboard_content = xml_document.toString(4);

	if (clipboard -> supportsSelection()) {
		clipboard -> setText(clipboard_content, QClipboard::Selection);
	}
	clipboard -> setText(clipboard_content);

	m_view -> pasteInArea();
}

/**
 * @brief QETElementEditor::on_m_save_action_triggered
 * @return
 */
bool QETElementEditor::on_m_save_action_triggered()
{
	if (checkElement())
	{
		if (m_opened_from_file)
		{
			if (m_file_name.isEmpty()) {
				return(on_m_save_as_action_triggered());
			}

			//Else wa save to the file at filename_ path
			bool result_save = toFile(m_file_name);
			if (result_save) m_elmt_scene -> undoStack().setClean();
			return(result_save);
		}
		else
		{
			if (m_location.isNull()) {
				return(on_m_save_as_action_triggered());
			}

			//Else save to the known location
			bool result_save = toLocation(m_location);
			if (result_save) {
				m_elmt_scene -> undoStack().setClean();
				emit saveToLocation(m_location);
			}
			return(result_save);
		}
	}

	QMessageBox::critical(this, tr("Echec de l'enregistrement"), tr("L'enregistrement à échoué,\nles conditions requises ne sont pas valides"));
	return false;
}

/**
 * @brief QETElementEditor::on_m_save_as_action_triggered
 * Ask a location to user and save the current edited element
 * to this location
 * @return true is success
 */
bool QETElementEditor::on_m_save_as_action_triggered()
{
	// Check element before writing
	if (checkElement()) {
		//Ask a location to user
		ElementsLocation location = ElementDialog::getSaveElementLocation(this);
		if (location.isNull()) {
			return(false);
		}

		bool result_save = toLocation(location);
		if (result_save) {
			setLocation(location);
			m_elmt_scene -> undoStack().setClean();
			emit saveToLocation(location);
		}

		return(result_save);
	}
	QMessageBox::critical(this, tr("Echec de l'enregistrement"), tr("L'enregistrement à échoué,\nles conditions requises ne sont pas valides"));
	return (false);
}

void QETElementEditor::on_m_select_all_act_triggered() {
	m_elmt_scene->slot_selectAll();
}

void QETElementEditor::on_m_edit_element_properties_action_triggered() { m_elmt_scene->slot_editProperties(); }

void QETElementEditor::on_m_new_action_triggered()
{
	NewElementWizard new_element_wizard(this);
	new_element_wizard.exec();
}

void QETElementEditor::on_m_open_action_triggered()
{
	ElementsLocation location = ElementDialog::getOpenElementLocation(this);
	if (location.isNull()) {
		return;
	}
	QETApp::instance() -> openElementLocations(QList<ElementsLocation>() << location);
}

void QETElementEditor::on_m_open_from_file_action_triggered()
{
	QString open_dir = m_file_name.isEmpty() ? QETApp::customElementsDir() : QDir(m_file_name).absolutePath();
	QString user_filename = QETElementEditor::getOpenElementFileName(this, open_dir);
	openElement(user_filename);
}

bool QETElementEditor::on_m_save_as_file_action_triggered()
{
	// Check element before writing
	if (checkElement()) {
		//Ask a filename to user, for save the element
		QString fn = QFileDialog::getSaveFileName(
						 this,
						 tr("Enregistrer sous", "dialog title"),
						 m_file_name.isEmpty() ? QETApp::customElementsDir() : QDir(m_file_name).absolutePath(),
						 tr(
							 "Éléments QElectroTech (*.elmt)",
							 "filetypes allowed when saving an element file"
		)
						 );

		if (fn.isEmpty()) {
			return(false);
		}

		//If the name doesn't end by .elmt, we add it
		if (!fn.endsWith(".elmt", Qt::CaseInsensitive)) {
			fn += ".elmt";
		}

		bool result_save = toFile(fn);
		//If the save success, the filename is keep
		if (result_save) {
			setFileName(fn);
			QETApp::elementsRecentFiles() -> fileWasOpened(fn);
			m_elmt_scene -> undoStack().setClean();
		}

		return(result_save);
	}
	QMessageBox::critical(this, tr("Echec de l'enregistrement"), tr("L'enregistrement à échoué,\nles conditions requises ne sont pas valides"));
	return false;
}

void QETElementEditor::on_m_reload_action_triggered()
{
	//If user already edit the element, ask confirmation to reload
	if (!m_elmt_scene -> undoStack().isClean()) {
		QMessageBox::StandardButton answer = QET::QetMessageBox::question(this,
																		  tr("Recharger l'élément", "dialog title"),
																		  tr("Vous avez efffectué des modifications sur cet élément. Si vous le rechargez, ces modifications seront perdues. Voulez-vous vraiment recharger l'élément ?", "dialog content"),
																		  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
																		  QMessageBox::Cancel);
		if (answer != QMessageBox::Yes){
			return;
		}
	}

	//Reload the element
	m_elmt_scene -> reset();
	if (m_opened_from_file) {
		fromFile(m_file_name);
	}
	else {
		fromLocation(m_location);
	}
}

void QETElementEditor::on_m_quit_action_triggered() { close(); }

void QETElementEditor::on_m_deselect_all_action_triggered() { m_elmt_scene->slot_deselectAll(); }

void QETElementEditor::on_m_cut_action_triggered() { m_view->cut(); }

void QETElementEditor::on_m_copy_action_triggered() { m_view->copy(); }

void QETElementEditor::on_m_paste_action_triggered() { m_view->paste(); }

void QETElementEditor::on_m_paste_in_area_action_triggered() { m_view->pasteInArea(); }

void QETElementEditor::on_m_paste_from_file_action_triggered()
{
	QString element_file_path = getOpenElementFileName(this);
	if (element_file_path.isEmpty()) {
		return;
	}

	QString error_message;
	QDomDocument xml_document;
	QFile element_file(element_file_path);
	// le fichier doit etre lisible
	if (!element_file.open(QIODevice::ReadOnly)) {
		error_message = QString(tr("Impossible d'ouvrir le fichier %1.", "message box content")).arg(element_file_path);
	}
	else {
		// le fichier doit etre un document XML
		if (!xml_document.setContent(&element_file)) {
			error_message = tr("Ce fichier n'est pas un document XML valide", "message box content");
		}
		element_file.close();
	}

	if (!error_message.isEmpty()) {
		QET::QetMessageBox::critical(this, tr("Erreur", "toolbar title"), error_message);
	}
	copyAndPasteXml(xml_document);
}

void QETElementEditor::on_m_paste_from_element_action_triggered()
{
		//Ask for a location
	ElementsLocation location = ElementDialog::getOpenElementLocation(this);
	if (location.isNull()) {
		return;
	}

	if (!location.isElement()) {
		QET::QetMessageBox::critical(this,
									 tr("Élément inexistant.", "message box title"),
									 tr("Le chemin virtuel choisi ne correspond pas à un élément.", "message box content"));
		return;
	}
	if (!location.exist()) {
		QET::QetMessageBox::critical(this,
									 tr("Élément inexistant.", "message box title"),
									 tr("L'élément n'existe pas.", "message box content"));
		return;
	}

		//Create an xml document from the location xml
	QDomDocument document_xml;
	QDomNode node = document_xml.importNode(location.xml(), true);
	document_xml.appendChild(node);

	copyAndPasteXml(document_xml);
}

void QETElementEditor::on_m_revert_selection_action_triggered() { m_elmt_scene->slot_invertSelection(); }

void QETElementEditor::on_m_delete_action_triggered() { m_elmt_scene->slot_delete(); }

void QETElementEditor::on_m_edit_names_action_triggered() { m_elmt_scene->slot_editNames(); }

void QETElementEditor::on_m_edit_author_action_triggered() { m_elmt_scene->slot_editAuthorInformations(); }

void QETElementEditor::on_m_zoom_in_action_triggered() { m_view->zoomIn(); }

void QETElementEditor::on_m_zoom_out_action_triggered() { m_view->zoomOut(); }

void QETElementEditor::on_m_zoom_fit_best_action_triggered() { m_view->zoomFit(); }

void QETElementEditor::on_m_zoom_original_action_triggered() { m_view->zoomReset(); }

void QETElementEditor::on_m_about_qet_action_triggered() { QETApp::instance()->aboutQET(); }

void QETElementEditor::on_m_online_manual_triggered() {
	QString link = "https://download.qelectrotech.org/qet/manual_0.7/build/index.html";
	QDesktopServices::openUrl(QUrl(link));
}

void QETElementEditor::on_m_youtube_action_triggered() {
	QString link = "https://www.youtube.com/user/scorpio8101/videos";
	QDesktopServices::openUrl(QUrl(link));
}

void QETElementEditor::on_m_donate_action_triggered() {
	QString link = "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=ZZHC9D7C3MDPC";
	QDesktopServices::openUrl(QUrl(link));
}

void QETElementEditor::on_m_about_qt_action_triggered() { qApp->aboutQt(); }

void QETElementEditor::on_m_import_dxf_triggered()
{
	if (dxf2ElmtIsPresent(true, this))
	{
		QString file_path{QFileDialog::getOpenFileName(this,
													   QObject::tr("Importer un fichier dxf"),
													   QETApp::documentDir(),
													   "DXF (*.dxf)")};
		if (file_path.isEmpty()) {
			return;
		}

		QMessageBox::information(this, tr("Avertissement"), tr("L'import d'un dxf volumineux peut prendre du temps \n"
															   "veuillez patienter durant l'import..."));

		const QByteArray array_{dxfToElmt(file_path)};
		if (array_.isEmpty()) {
			return;
		}
		QDomDocument xml_;
		xml_.setContent(array_);

		m_elmt_scene->undoStack().push(new OpenElmtCommand(xml_, m_elmt_scene));
	}
}

void QETElementEditor::on_m_import_scaled_element_triggered()
{
	if (ElementScalerIsPresent(true, this))
	{
		QString file_path{QFileDialog::getOpenFileName(this,
													   tr("Importer un élément à redimensionner"),
													   QETApp::documentDir(),
													   tr("Éléments QElectroTech (*.elmt)"))};
		if (file_path.isEmpty()) {
			return;
		}

		const QByteArray array_{ElementScaler(file_path, this)};
		if (array_.isEmpty()) {
			return;
		}
		QDomDocument xml_;
		xml_.setContent(array_);

		m_elmt_scene->undoStack().push(new OpenElmtCommand(xml_, m_elmt_scene));
	}
}


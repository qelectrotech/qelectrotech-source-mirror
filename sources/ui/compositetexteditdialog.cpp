#include "compositetexteditdialog.h"

#include <qetapp.h>
#include <qetgraphicsitem/conductor.h>
#include <qetgraphicsitem/dynamicelementtextitem.h>
#include <qetgraphicsitem/element.h>
#include <diagramcontext.h>
#include <qetinformation.h>
#include <qet.h>
#include "ui_compositetexteditdialog.h"

#include <utility>

CompositeTextEditDialog::CompositeTextEditDialog(DynamicElementTextItem *text, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CompositeTextEditDialog),
	m_text(text)
{
	ui->setupUi(this);
	QET::trackDialogGeometry(this);
	m_default_text = m_text->compositeText();
	ui->m_plain_text_edit->setPlainText(m_default_text);
	ui->m_plain_text_edit->setPlaceholderText(tr("Enter your composite text here, using the available variables"));
	bool report = false;
	if ((m_text) &&(m_text->parentElement()->linkType()) & (Element::AllReport))
	{
		report = true;
	}
	setUpComboBox(report);
	if (m_text && m_text->parentElement()->elementData().m_type == ElementData::Terminal) {
		addTerminalConductorInfo(m_text->parentElement()->terminals().size());
	}
	if (m_text && !report && m_text->elementUseForInfo()) {
		addCustomInfo(m_text->elementUseForInfo()->elementInformations());
	}
}

CompositeTextEditDialog::CompositeTextEditDialog(QString text, bool report, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CompositeTextEditDialog)
{
	ui->setupUi(this);
	QET::trackDialogGeometry(this);
	m_default_text = std::move(text);
	ui->m_plain_text_edit->setPlainText(m_default_text);
	ui->m_plain_text_edit->setPlaceholderText(tr("Enter your composite text here, using the available variables"));
	setUpComboBox(report);
}

CompositeTextEditDialog::~CompositeTextEditDialog()
{
	delete ui;
}

/**
	@brief CompositeTextEditDialog::plainText
	@return The edited text
*/
QString CompositeTextEditDialog::plainText() const
{
	return ui->m_plain_text_edit->toPlainText();
}

/**
	@brief CompositeTextEditDialog::setUpComboBox
	Add the available element information in the combo box
*/
void CompositeTextEditDialog::setUpComboBox(bool is_report)
{
	QStringList qstrl;

	if(is_report) //Special treatment for text owned by a folio report
	{
		qstrl = QETInformation::folioReportInfoKeys();
	}
	else
	{
		qstrl = QETInformation::elementInfoKeys();
		qstrl.removeAll("formula");
	}
	
	for (int i=0; i<qstrl.size();++i) {
		ui -> m_info_cb -> addItem(QETInformation::translatedInfoKey(qstrl[i]),
								   is_report ? QETInformation::folioReportInfoToVar(qstrl[i]) : QETInformation::elementInfoToVar(qstrl[i]));
	}

		//Synthetic tokens computed by AssignVariables::replaceVariable() rather
		//than stored directly in the element's info; not part of elementInfoKeys().
	if (!is_report)
	{
		ui -> m_info_cb -> addItem(tr("Prefix (-)"), QStringLiteral("%{prefix}"));
		ui -> m_info_cb -> addItem(tr("Reference designation, IEC 81346 (=+-), short on its own page"), QStringLiteral("%{structure_id}"));
		ui -> m_info_cb -> addItem(tr("Full reference designation, IEC 81346 (=+-)"), QStringLiteral("%{structure_id_full}"));
	}
}

/**
	@brief CompositeTextEditDialog::addTerminalConductorInfo
	Add to the combo box the information a terminal symbol with
	@p connection_count connection points reads from its wires
	(@sa QETInformation::terminalConductorInfoKeys)
*/
void CompositeTextEditDialog::addTerminalConductorInfo(int connection_count)
{
	for (const QString &key : QETInformation::terminalConductorInfoKeys(connection_count)) {
		ui->m_info_cb->addItem(QETInformation::translatedInfoKey(key),
							   QETInformation::infoToVar(key));
	}
}

void CompositeTextEditDialog::on_m_info_cb_activated(int index)
{
	Q_UNUSED(index)
	ui->m_plain_text_edit->insertPlainText(ui->m_info_cb->currentData().toString());
}

/**
	@brief CompositeTextEditDialog::focusInEvent
	Reimplemented from QWidget::focusInEvent
	@param event
*/
void CompositeTextEditDialog::focusInEvent(QFocusEvent *event)
{
	ui->m_plain_text_edit->setFocus();
	QDialog::focusInEvent(event);
}

/**
	@brief CompositeTextEditDialog::addCustomInfo
	Add to the combo box the user-defined custom properties of
	@p informations (@sa QETInformation::elementCustomInfoKeys)
*/
void CompositeTextEditDialog::addCustomInfo(const DiagramContext &informations)
{
	for (const QString &key : QETInformation::elementCustomInfoKeys(informations.keys())) {
		ui->m_info_cb->addItem(key, QETInformation::infoToVar(key));
	}
}

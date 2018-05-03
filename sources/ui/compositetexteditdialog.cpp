#include "compositetexteditdialog.h"
#include "ui_compositetexteditdialog.h"
#include "dynamicelementtextitem.h"
#include "element.h"
#include "qetapp.h"
#include "conductor.h"

CompositeTextEditDialog::CompositeTextEditDialog(DynamicElementTextItem *text, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CompositeTextEditDialog),
	m_text(text)
{
	ui->setupUi(this);
	m_default_text = m_text->compositeText();
	ui->m_plain_text_edit->setPlainText(m_default_text);
#if QT_VERSION >= 0x050300
	ui->m_plain_text_edit->setPlaceholderText(tr("Entrée votre texte composé ici, en vous aidant des variables disponible"));
#endif
	setUpComboBox();
}

CompositeTextEditDialog::CompositeTextEditDialog(QString text, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CompositeTextEditDialog)
{
	ui->setupUi(this);
	m_default_text = text;
	ui->m_plain_text_edit->setPlainText(m_default_text);
#if QT_VERSION >= 0x050300
	ui->m_plain_text_edit->setPlaceholderText(tr("Entrée votre texte composé ici, en vous aidant des variables disponible"));
#endif
	setUpComboBox();
}

CompositeTextEditDialog::~CompositeTextEditDialog() {
	delete ui;
}

/**
 * @brief CompositeTextEditDialog::plainText
 * @return The edited text
 */
QString CompositeTextEditDialog::plainText() const {
	return ui->m_plain_text_edit->toPlainText();
}

/**
 * @brief CompositeTextEditDialog::setUpComboBox
 * Add the available element information in the combo box
 */
void CompositeTextEditDialog::setUpComboBox()
{
	QStringList qstrl;
	
	if(m_text && (m_text->parentElement()->linkType() & Element::AllReport)) //Special treatment for text owned by a folio report
	{
		qstrl << "label" << "function" << "tension-protocol";
	}
	else
	{
		qstrl = QETApp::elementInfoKeys();
		qstrl.removeAll("formula");
	}
	
		//We use a QMap because the keys of the map are sorted, then no matter the curent local,
		//the value of the combo box are always alphabetically sorted
	QMap <QString, QString> info_map;
	for(QString str : qstrl) {
		info_map.insert(QETApp::elementTranslatedInfoKey(str), QETApp::elementInfoToVar(str));
	}
	for(QString key : info_map.keys()) {
		ui->m_info_cb->addItem(key, info_map.value(key));
	}
}

void CompositeTextEditDialog::on_m_info_cb_activated(const QString &arg1)
{
	Q_UNUSED(arg1)
	ui->m_plain_text_edit->insertPlainText(ui->m_info_cb->currentData().toString());
}

/**
 * @brief CompositeTextEditDialog::focusInEvent
 * Reimplemented from QWidget::focusInEvent
 * @param event
 */
void CompositeTextEditDialog::focusInEvent(QFocusEvent *event)
{
	ui->m_plain_text_edit->setFocus();
	QDialog::focusInEvent(event);
}

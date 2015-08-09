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
#include "conductor.h"
#include "potentialtextsdialog.h"
#include "ui_potentialtextsdialog.h"
#include <QSignalMapper>
#include <QRadioButton>

/**
 * @brief PotentialTextsDialog::PotentialTextsDialog
 * Constructor
 * @param conductor : A Conductor of the potential to check
 * @param parent : parent widget
 */
PotentialTextsDialog::PotentialTextsDialog(Conductor *conductor, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PotentialTextsDialog),
	m_conductor (conductor)
{
	ui->setupUi(this);
	conductorsTextToMap();
	buildRadioList();
}

/**
 * @brief PotentialTextsDialog::~PotentialTextsDialog
 * Destructor
 */
PotentialTextsDialog::~PotentialTextsDialog()
{
	delete ui;
}

/**
 * @brief PotentialTextsDialog::selectedText
 * @return the selected text
 */
QString PotentialTextsDialog::selectedText() const {
	return m_selected_text;
}

/**
 * @brief PotentialTextsDialog::buildRadioList
 * Build the radio list of this dialog, for selected a text
 */
void PotentialTextsDialog::buildRadioList() {
	//map the signal for each radio button create in buildRadioList
	m_signal_mapper = new QSignalMapper(this);
	connect(m_signal_mapper, SIGNAL(mapped(QString)), this, SLOT(setSelectedText(QString)));

	//create a new radio button for each text of @conductorList
	for (QMultiMap<int, QString>::ConstIterator it = m_texts.constEnd()-1; it != m_texts.constBegin()-1; --it) {
		QRadioButton *rb= new QRadioButton(it.value() + tr("  : est présent ") + QString::number(it.key()) + tr(" fois."), this);
		if (it == m_texts.constEnd()-1) {
			rb -> setChecked(true);
			m_selected_text = it.value();
		}
		//connect the button to mapper @m_signal_mapper
		connect(rb, SIGNAL(clicked()), m_signal_mapper, SLOT(map()));
		m_signal_mapper -> setMapping(rb, it.value());
		ui -> m_buttons_layout -> addWidget(rb);
	}
}

/**
 * @brief PotentialTextsDialog::conductorsTextToMap
 * Fill the multimap @m_text with all different text found in the same potentil of @m_conductor
 * The key "int" of multimap is the number of conductors with the same text.
 * The value "QString" of multimap is the text.
 */
void PotentialTextsDialog::conductorsTextToMap() {
	QStringList textList;
	textList << m_conductor -> text();
	foreach(Conductor *c, m_conductor->relatedPotentialConductors()) textList << c -> text();

	while (!textList.size() == 0) {
		QString t = textList.at(0);
		int n = textList.count(t);
		textList.removeAll(t);
		m_texts.insert(n, t);
	}
}

/**
 * @brief PotentialTextsDialog::setSelectedText
 * @param text
 */
void PotentialTextsDialog::setSelectedText(QString text) {
	m_selected_text = text;
}

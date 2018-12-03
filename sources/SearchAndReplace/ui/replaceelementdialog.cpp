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
#include "replaceelementdialog.h"
#include "ui_replaceelementdialog.h"
#include "replaceelementdialog.h"
#include "elementinfopartwidget.h"
#include "qetapp.h"
#include "searchandreplaceworker.h"

#include <QAbstractButton>

ReplaceElementDialog::ReplaceElementDialog(DiagramContext context, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ReplaceElementDialog)
{
	ui->setupUi(this);
	buildWidget();
	setContext(context);
}

ReplaceElementDialog::~ReplaceElementDialog()
{
	delete ui;
}

/**
 * @brief ReplaceElementDialog::setContext
 * Set the current diagram context to be edited
 * @param context
 */
void ReplaceElementDialog::setContext(DiagramContext context)
{
	m_context = context;
	
	for (ElementInfoPartWidget *eipw : m_eipw_list)
	{
		eipw->setText(m_context[eipw->key()].toString());
		eipw->setEraseTextChecked(false);
	}
}

/**
 * @brief ReplaceElementDialog::context
 * @return The edited diagram context
 */
DiagramContext ReplaceElementDialog::context() const
{
	DiagramContext context;
	for (ElementInfoPartWidget *eipw : m_eipw_list) {		
		context.addValue(eipw->key(), eipw->text());
	}
	
	return context;
}

void ReplaceElementDialog::buildWidget()
{
	ui->m_button_box->disconnect();
	connect(ui->m_button_box, &QDialogButtonBox::clicked, [this](QAbstractButton *button_)
	{
		this->done(ui->m_button_box->buttonRole(button_));
	});
	
	for (QString str : QETApp::elementInfoKeys())
	{
		ElementInfoPartWidget *eipw = new ElementInfoPartWidget(str, QETApp::elementTranslatedInfoKey(str), this);
		eipw->setEraseTextVisible(true);
        eipw->setPlaceHolderText(tr("Ne pas modifier"));
		ui->m_scroll_layout->addWidget(eipw);
		m_eipw_list << eipw;
	}
}

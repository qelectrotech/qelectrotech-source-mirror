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
#include "customelementinfopartwidget.h"

#include "../diagramcontext.h"
#include "../qeticons.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QToolButton>

/**
	@brief CustomElementInfoPartWidget::CustomElementInfoPartWidget
	Constructor
	@param key initial key name (empty for a freshly added row)
	@param value initial value
	@param parent parent widget
*/
CustomElementInfoPartWidget::CustomElementInfoPartWidget(
		const QString &key,
		const QString &value,
		QWidget *parent) :
	QWidget(parent),
	m_key_edit(new QLineEdit(key, this)),
	m_value_edit(new QLineEdit(value, this)),
	m_remove_button(new QToolButton(this))
{
	m_key_edit->setPlaceholderText(tr("nom_de_la_propriete"));
	m_key_edit->setToolTip(tr("Lettres minuscules, chiffres, tiret et underscore uniquement"));
	m_value_edit->setClearButtonEnabled(true);

	m_remove_button->setIcon(QET::Icons::Remove);
	m_remove_button->setToolTip(tr("Supprimer cette propriété"));
	m_remove_button->setAutoRaise(true);

	auto *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 2, 0, 2);
	layout->setVerticalSpacing(2);
	layout->setHorizontalSpacing(0);
	layout->addWidget(m_key_edit,   0, 0);
	layout->addWidget(m_value_edit, 1, 0);
	layout->addWidget(m_remove_button, 0, 1, 2, 1);

	connect(m_key_edit, &QLineEdit::textChanged, this, &CustomElementInfoPartWidget::validateKey);
	connect(m_key_edit, &QLineEdit::textChanged, this, &CustomElementInfoPartWidget::changed);
	connect(m_value_edit, &QLineEdit::textChanged, this, &CustomElementInfoPartWidget::changed);
	connect(m_remove_button, &QToolButton::clicked, this, [this]() {
		emit removeRequested(this);
	});

	setFocusProxy(m_key_edit);
	validateKey();
}

CustomElementInfoPartWidget::~CustomElementInfoPartWidget()
{
}

/**
	@return the key name currently typed in this row
*/
QString CustomElementInfoPartWidget::key() const
{
	return m_key_edit->text().trimmed();
}

/**
	@return the value currently typed in this row
*/
QString CustomElementInfoPartWidget::value() const
{
	return m_value_edit->text();
}

/**
	@return true if the typed key is non-empty and matches
	DiagramContext::isKeyAcceptable()
*/
bool CustomElementInfoPartWidget::hasValidKey() const
{
	const QString k = key();
	return !k.isEmpty() && DiagramContext::isKeyAcceptable(k);
}

/**
	@brief CustomElementInfoPartWidget::validateKey
	Flag the key field when it doesn't match the accepted format,
	instead of silently dropping it later.
*/
void CustomElementInfoPartWidget::validateKey()
{
	const QString k = key();
	if (k.isEmpty() || DiagramContext::isKeyAcceptable(k)) {
		m_key_edit->setStyleSheet(QString());
	} else {
		m_key_edit->setStyleSheet(QStringLiteral("border: 1px solid red;"));
	}
}

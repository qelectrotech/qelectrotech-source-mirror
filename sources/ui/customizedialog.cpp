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
#include "customizedialog.h"

#include "configpage/configpage.h"

#include <QDialogButtonBox>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>

CustomizeDialog::CustomizeDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Personnaliser", "window title"));

	m_tabs = new QTabWidget(this);
	m_tabs->setObjectName(QStringLiteral("customizeTabs"));
	m_tabs->setDocumentMode(true);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
		applyConf();
		accept();
	});
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(m_tabs);
	layout->addWidget(buttons);

	resize(1000, 700);
}

/**
	@brief CustomizeDialog::addPage
	Add @a page as a tab, named after its title. The dialog takes it.
*/
void CustomizeDialog::addPage(ConfigPage *page)
{
	if (!page || m_pages.contains(page)) {
		return;
	}
	m_pages << page;
	m_tabs->addTab(page, page->icon(), page->title());
}

QList<ConfigPage *> CustomizeDialog::pages() const
{
	return m_pages;
}

/**
	@brief CustomizeDialog::applyConf
	Apply every page, in tab order.
*/
void CustomizeDialog::applyConf()
{
	for (ConfigPage *page : std::as_const(m_pages)) {
		page->applyConf();
	}
}

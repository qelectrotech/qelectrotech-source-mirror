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
#include "devicetypechangedialog.h"

#include "../devicetype/devicetypechange.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

/**
	@brief DeviceTypeChangeDialog::DeviceTypeChangeDialog
	@param device : the device changing its type
	@param type : its new type
	@param parts : the part of @p type each of its symbols draws to begin with
	@param parent : parent widget
*/
DeviceTypeChangeDialog::DeviceTypeChangeDialog(const DeviceTypeCheck::Device &device,
											   const DeviceType &type, const QList<int> &parts,
											   QWidget *parent) :
	QDialog(parent),
	m_count(int(device.symbols.size()))
{
	//: %1 is a device type's name, such as "CONTR10"
	setWindowTitle(tr("Change the device type to %1").arg(type.name));
	auto *layout = new QVBoxLayout(this);
	auto *help = new QLabel(tr("Choose, for each symbol of the device, the part of the new device "
							   "type it draws. It takes that part's terminals. A symbol left beyond "
							   "the type stays as it is."), this);
	help->setWordWrap(true);
	layout->addWidget(help);

	m_table = new QTableWidget(m_count, 2, this);
	m_table->setObjectName(QStringLiteral("deviceTypeChangeTable"));
	m_table->setHorizontalHeaderLabels({tr("Symbol"), tr("Part of the new type")});
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_table->verticalHeader()->hide();
	for (int i = 0 ; i < m_count ; ++i) {
		auto *item = new QTableWidgetItem(DeviceTypeChange::symbolText(device, i));
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(i, 0, item);
		auto *box = new QComboBox(m_table);
		box->addItem(tr("Beyond the type"), int(DeviceTypeChange::Beyond));
		for (int part = 0 ; part < type.parts.size() ; ++part)
			//: %1 is a part's number in the type, %2 how it reads ("contact NO 13 14")
			box->addItem(tr("%1: %2").arg(part + 1).arg(DeviceTypeCheck::partText(type.parts.at(part))), part);
		m_table->setCellWidget(i, 1, box);
	}
	for (int i = 0 ; i < m_count ; ++i) setPart(i, parts.value(i, DeviceTypeChange::Beyond));
	layout->addWidget(m_table, 1);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &DeviceTypeChangeDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &DeviceTypeChangeDialog::reject);
	layout->addWidget(buttons);
	resize(640, 360);
}

QList<int> DeviceTypeChangeDialog::parts() const
{
	QList<int> list;
	for (int i = 0 ; i < m_count ; ++i) {
		const auto *box = qobject_cast<QComboBox *>(m_table->cellWidget(i, 1));
		list << (box ? box->currentData().toInt() : int(DeviceTypeChange::Beyond));
	}
	return list;
}

void DeviceTypeChangeDialog::setPart(int symbol, int part)
{
	auto *box = qobject_cast<QComboBox *>(m_table->cellWidget(symbol, 1));
	if (!box) return;
	const int index = box->findData(part);
	box->setCurrentIndex(index < 0 ? 0 : index);
}

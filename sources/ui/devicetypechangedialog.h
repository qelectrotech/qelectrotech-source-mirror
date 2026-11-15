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
#ifndef DEVICETYPECHANGEDIALOG_H
#define DEVICETYPECHANGEDIALOG_H

#include "../devicetype/devicetypecheck.h"

#include <QDialog>
#include <QList>

class QTableWidget;

/**
	@brief The DeviceTypeChangeDialog class
	A device takes another device type, its symbols moved one by one: each
	symbol of the device on the left, the part of the new type it draws on
	the right (or none: beyond the type), the automatic choice first.
*/
class DeviceTypeChangeDialog : public QDialog
{
		Q_OBJECT

	public:
		DeviceTypeChangeDialog(const DeviceTypeCheck::Device &device, const DeviceType &type,
							   const QList<int> &parts, QWidget *parent = nullptr);

		QList<int> parts() const;
		void setPart(int symbol, int part);

	private:
		QTableWidget *m_table = nullptr;
		int m_count = 0;
};

#endif // DEVICETYPECHANGEDIALOG_H

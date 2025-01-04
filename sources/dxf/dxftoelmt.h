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
#ifndef DXFTOELMT_H
#define DXFTOELMT_H

#include <QByteArray>

class QWidget;

QByteArray dxfToElmt(const QString &file_path);
QString dxf2ElmtDirPath();
QString dxf2ElmtBinaryPath();
bool dxf2ElmtIsPresent(bool install_dialog = true, QWidget *parent = nullptr);

#endif // DXFTOELMT_H

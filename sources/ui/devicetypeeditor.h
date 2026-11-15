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
#ifndef DEVICETYPEEDITOR_H
#define DEVICETYPEEDITOR_H

#include "../devicetype/devicetype.h"

#include <QDialog>
#include <QPointer>
#include <QStringList>

class QETProject;
class QLabel;
class QLineEdit;
class QTableWidget;

/**
	@brief The DeviceTypeEditor class
	Make or change one device type: its name, its description, what it
	gives the symbols of its devices, and its parts. Each row of the parts
	table is one way to draw a part: rows of the same part number are the
	ways one part may be drawn (a contact as NO, NC or changeover). The
	first master part is the device's head.

	An existing type keeps its name: a type is known by it, and the
	devices of a project name it. Duplicate a type to give it another.
*/
class DeviceTypeEditor : public QDialog
{
		Q_OBJECT

	public:
		DeviceTypeEditor(const DeviceType &type, QETProject *project,
						 const QStringList &taken_names, QWidget *parent = nullptr);

		DeviceType type() const;
		void setType(const DeviceType &type);
		void setNameEditable(bool editable);

		static QStringList problems(const DeviceType &type, QETProject *project,
									const QStringList &taken_names);
		static QStringList terminalNames(const QString &location, QETProject *project,
										 int *count = nullptr);

	public slots:
		void accept() override;

	private:
		void addRow(int part, const QString &role, const QString &kind,
					const QString &state, const QString &location, const QString &pins);
		void addPart();
		void addDrawing();
		void removeRow();
		void moveRow(int by);
		void chooseSymbol();
		void addInformation();
		void removeInformation();

		QPointer<QETProject> m_project;
		QStringList m_taken;
		bool m_new = true;
		QString m_lang;
		DeviceType m_original;
		QLineEdit *m_name = nullptr;
		QLineEdit *m_description = nullptr;
		QTableWidget *m_informations = nullptr;
		QTableWidget *m_parts = nullptr;
		QLabel *m_problems = nullptr;
};

#endif // DEVICETYPEEDITOR_H

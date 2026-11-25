/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef LABELLISTEXPORT_H
#define LABELLISTEXPORT_H

#include <QByteArray>
#include <QDialog>
#include <QStringList>

class QETProject;
class QButtonGroup;
class QCheckBox;
class QLabel;

/**
	@brief The LabelListExport namespace
	The labels of a project as lists for a label printer (markers for
	devices, terminals, cables and wires): one label per line, read from
	the project database.
*/
namespace LabelListExport
{
	enum Kind {
		Devices,	///< one tag per device but terminals, in folio order
		Terminals,	///< terminal markers, strip by strip in strip order
		Cables,		///< one number per cable, in folio order
		Wires		///< wire numbers, as "Export the list of names of wires"
	};

	bool kindFromString(const QString &name, Kind *kind);
	QString kindName(Kind kind);

	QStringList labels(QETProject *project, Kind kind, bool full_designation);
	QByteArray toCsv(const QStringList &labels);
	bool write(const QStringList &labels, const QString &file_name, QString *error = nullptr);
}

/**
	@brief The LabelListExportDialog class
	Project > Export labels for printing: pick a list, see how many
	labels it has, save it as CSV.
*/
class LabelListExportDialog : public QDialog
{
	Q_OBJECT

	public:
		LabelListExportDialog(QETProject *project, QWidget *parent = nullptr);

	private:
		LabelListExport::Kind kind() const;
		void refresh();
		void save();

		QETProject *m_project = nullptr;
		QButtonGroup *m_kinds = nullptr;
		QCheckBox *m_full = nullptr;
		QLabel *m_count = nullptr;
		QStringList m_labels;
};

#endif // LABELLISTEXPORT_H

/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CORECOLOREDITOR_H
#define CORECOLOREDITOR_H

#include <QComboBox>
#include <QWidget>
#include <QVector>
#include <QStringList>

class QGridLayout;
class QLabel;

/**
	@brief Editable combo box pre-filled with IEC 60757 colours + swatch icons.
	Includes a leading "<No color>" entry so a core's tracer slots can be empty.
	Still editable so a user can type a non-standard colour name.
*/
class WireColorComboBox : public QComboBox
{
	Q_OBJECT
	public:
		explicit WireColorComboBox(QWidget *parent = nullptr);

		/// Empty string when "<No color>" is selected.
		QString colorName() const;
		void setColorName(const QString &name);
};

/**
	@brief The CoreColorEditor — the "Cable cores" tab content.
	A table of cores (one row each) modelled on the SolidWorks Electrical cable
	cores editor: each core has Colour 1 (base) + Colour 2/3 (optional tracers).
	Cores are added/removed with the Add / Remove buttons; the row count is the
	cable's core count.
*/
class CoreColorEditor : public QWidget
{
	Q_OBJECT
	public:
		explicit CoreColorEditor(QWidget *parent = nullptr);

		int coreCount() const { return m_rows.size(); }

		/// Per-core colours: each entry is {base, tracer1, tracer2} with empties
		/// trimmed, so a single-colour core returns a 1-element list.
		QVector<QStringList> colors() const;
		void setColors(const QVector<QStringList> &cores);

	signals:
		void coresChanged();

	public slots:
		void addCore();
		void removeSelectedCore();

	private:
		struct Row {
			QLabel *number;
			WireColorComboBox *c1;
			WireColorComboBox *c2;
			WireColorComboBox *c3;
		};
		void renumber();

		QGridLayout *m_grid = nullptr;
		QVector<Row> m_rows;
};

#endif // CORECOLOREDITOR_H

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

class QVBoxLayout;

/**
	@brief Editable combo box pre-filled with IEC 60757 colours + swatch icons.
	Editable so a user can still type a non-standard colour name.
*/
class WireColorComboBox : public QComboBox
{
	Q_OBJECT
	public:
		explicit WireColorComboBox(QWidget *parent = nullptr);

		QString colorName() const;
		void setColorName(const QString &name);
};

/**
	@brief The CoreColorEditor — multi-core cable builder.
	Shows one WireColorComboBox per core ("Core 1 … Core N"), so a multi-core
	cable's per-core colours can be picked visually. The number of rows tracks
	the core count set via setCoreCount(); existing selections are preserved
	when the count grows or shrinks. Lives in a fixed-height scroll-friendly
	column inside the wire dialog.
*/
class CoreColorEditor : public QWidget
{
	Q_OBJECT
	public:
		explicit CoreColorEditor(QWidget *parent = nullptr);

		void setCoreCount(int count);
		int coreCount() const { return m_combos.size(); }

		QStringList colors() const;
		void setColors(const QStringList &colors);

	signals:
		void colorsChanged();

	private:
		QVBoxLayout *m_layout = nullptr;
		QVector<WireColorComboBox*> m_combos;
};

#endif // CORECOLOREDITOR_H

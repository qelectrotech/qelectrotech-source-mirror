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
#ifndef QET_KCOLORBUTTON_H
#define QET_KCOLORBUTTON_H

#include <QColor>
#include <QPushButton>

class KColorButton : public QPushButton
{
		Q_OBJECT

	public:
		explicit KColorButton(QWidget *parent = nullptr);

		QColor color() const;

	public slots:
		void setColor(const QColor &color);

	signals:
		void changed(const QColor &color);

	private slots:
		void chooseColor();

	private:
		void updateButton();

		QColor m_color;
};

#endif // QET_KCOLORBUTTON_H

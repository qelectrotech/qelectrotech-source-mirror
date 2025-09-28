/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QColor>
#include <QObject>
#include <QPushButton>
#include <Qt>

/**
	@brief The ColorButton class
*/
class ColorButton : public QPushButton
{
	    Q_OBJECT

	public:
    	ColorButton(QWidget * = nullptr);

		const QColor color();
		void setColor(const QColor &);

	public slots:
		void clicked(bool = false);

	protected:
		void paintEvent(QPaintEvent *) override;

	signals:
		void changed(const QColor &);

	private:
    	/// @brief Current selected color
    	QColor m_color = Qt::black;
};

#endif

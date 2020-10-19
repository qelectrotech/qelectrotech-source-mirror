/*
	Copyright 2006-2020 The QElectroTech Team
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
#ifndef QWIDGETANIMATION_H
#define QWIDGETANIMATION_H

#include <QPropertyAnimation>
#include <QSize>
#include <QRect>

/**
	@brief The QWidgetAnimation class
	This class animate the show and hide function of a QWidget.

	The role of behavior is to calcul as best the animation process
	when widget is show.
	Because this class don't change the current and final size
	of the widget but her maximum size during the animation process,
	we must to know in advance the final size of the widget.
	Behavior minimumSizeHint : the final size of the widget
	will be his minimum size hint.
	Behavior availableSpace : the final size of widget will be
	the available size of her parent.
	Since parent can have other widgets you can add a QVector of widget
	to subtract of the final size.
	Because we suppose the animated widget will take the maximum
	available space, we subtract the minimum size hint of widgets in QVector.
	Behavior lastSize :
	The widget will have the same size as the last time he was showed.
*/
class QWidgetAnimation : public QPropertyAnimation
{
	Q_OBJECT

	public:
		enum Behavior {
			minimumSizeHint,
			availableSpace,
			lastSize,
		};

		QWidgetAnimation(
				QWidget *widget,
				Qt::Orientation orientation,
				QWidgetAnimation::Behavior
				  behavior = QWidgetAnimation::minimumSizeHint,
				int duration = 250);
		void widgetToSubtract (QVector<QWidget *> widgets);

		void show();
		void hide();
		void setHidden(bool hidden);
		void setLastShowSize(int size);

	private:
		enum currentState {
			Showing,
			Hidding,
			Finish
		};

		Qt::Orientation m_orientation = Qt::Horizontal;
		QVector <QWidget *> m_widget_to_substract;
		QWidget *m_widget;
		QSize m_maximum;
		QRect m_last_rect;
		QWidgetAnimation::Behavior m_behavior = Behavior::minimumSizeHint;
		QWidgetAnimation::currentState m_state = QWidgetAnimation::Finish;
};

#endif // QWIDGETANIMATION_H

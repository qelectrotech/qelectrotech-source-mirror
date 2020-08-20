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
#include "qwidgetanimation.h"
#include <QWidget>

/**
	@brief QWidgetAnimation::QWidgetAnimation
	@param widget : widget to animate
	@param orientation : animate widget horizontally or vertically
	@param behavior :
	@param duration : the duration of animation
	@see void QVariantAnimation::setDuration(int msecs)
*/
QWidgetAnimation::QWidgetAnimation(QWidget *widget,
				   Qt::Orientation orientation,
				   QWidgetAnimation::Behavior behavior,
				   int duration) :
	QPropertyAnimation(widget),
	m_orientation(orientation),
	m_widget(widget),
	m_maximum(widget->maximumSize()),
	m_last_rect(widget->geometry()),
	m_behavior(behavior)
{
	setTargetObject(widget);
	setPropertyName( m_orientation == Qt::Vertical
			 ? "maximumHeight"
			 : "maximumWidth");
	setDuration(duration);
	setEasingCurve(QEasingCurve::OutCubic);

	connect(this, &QPropertyAnimation::finished, [this]()
	{
		m_state = QWidgetAnimation::Finish;

			if (	(this->m_orientation == Qt::Vertical
				 && m_widget->geometry().height() == 0) ||
					(this->m_orientation == Qt::Horizontal
					 && m_widget->geometry().width() == 0))
			{
				m_widget->hide();
			} else {
				m_widget->setMaximumSize(m_maximum);
			}
	});
}

/**
	@brief QWidgetAnimation::widgetToSubtract
	Widget to subtract the size when the behavior is availableSpace
	@param widgets
*/
void QWidgetAnimation::widgetToSubtract(QVector<QWidget *> widgets)
{
	m_widget_to_substract.clear();
	m_widget_to_substract = widgets;
}

/**
	@brief QWidgetAnimation::show
	show the widget
*/
void QWidgetAnimation::show()
{
	if (m_state == QWidgetAnimation::Showing)
		return;

	stop();

	m_widget->show();

	int end_value = 10000;
	if (m_behavior == QWidgetAnimation::minimumSizeHint)
	{
		end_value = m_orientation == Qt::Horizontal
				? m_widget->minimumSizeHint().width()
				: m_widget->minimumSizeHint().height();
	}
	else if (m_behavior == QWidgetAnimation::availableSpace
		 && m_widget->parentWidget())
	{
		m_widget->parentWidget()->layout();
		int available_ = m_orientation == Qt::Horizontal
				? m_widget->parentWidget()->width()
				: m_widget->parentWidget()->height();
		for (auto w : m_widget_to_substract) {
			available_ -= m_orientation == Qt::Horizontal
					? w->minimumSizeHint().width()
					: w->minimumSizeHint().height();
		}

		int mini_ = m_orientation == Qt::Horizontal
				? m_widget->minimumSizeHint().width()
				: m_widget->minimumSizeHint().height();

		end_value = available_ > mini_ ? available_ : mini_;
	}
	else
	{
		if (m_last_rect.isValid()) {
			end_value = m_orientation == Qt::Horizontal
					? m_last_rect.width()
					: m_last_rect.height();
		} else {
			end_value = m_orientation == Qt::Horizontal
					? m_maximum.width()
					: m_maximum.height();
		}
	}

	setStartValue(0);
	setEndValue(end_value);
	m_state = QWidgetAnimation::Showing;
	start();
}

/**
	@brief QWidgetAnimation::hide
	Hide the widget
*/
void QWidgetAnimation::hide()
{
	if (m_state == QWidgetAnimation::Hidding)
		return;

	if (m_state == Finish && m_widget->isVisible()) {
		m_last_rect = m_widget->geometry();
	}

	stop();
	int start_value = m_orientation == Qt::Horizontal
			? m_widget->width()
			: m_widget->height();
	setStartValue(start_value);
	setEndValue(0);
	m_state = QWidgetAnimation::Hidding;
	start();
}

/**
	@brief QWidgetAnimation::setHidden
	true hide, false show
	@param hidden
*/
void QWidgetAnimation::setHidden(bool hidden)
{
	if (hidden)
		hide();
	else
		show();
}

/**
	@brief QWidgetAnimation::setLastShowSize
	Force the last show size value to size
	@param size
*/
void QWidgetAnimation::setLastShowSize(int size)
{
	if (m_orientation == Qt::Vertical) {
		m_last_rect.setHeight(size);
	} else {
		m_last_rect.setWidth(size);
	}
}

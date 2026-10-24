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
#include "docktextwrap.h"

#include <QAbstractButton>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QEvent>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QRadioButton>

namespace {
	const char *const handled = "qetDockTextWrapped";

	/**
		The text of a check box or a radio button set beside it, so that it
		can wrap; a click on it toggles the button, as on its own text.
	*/
	class ButtonText : public QLabel
	{
		public:
			ButtonText(QAbstractButton *button, QWidget *parent) :
				QLabel(button->text(), parent), m_button(button)
			{
				setWordWrap(true);
				setBuddy(button);
				setToolTip(button->toolTip());
				setStatusTip(button->statusTip());
				setEnabled(button->isEnabled());
				setVisible(!button->isHidden());
				button->installEventFilter(this);
			}
		protected:
				/// The text follows the button: enabled, shown, its tip
			bool eventFilter(QObject *watched, QEvent *event) override
			{
				if (watched == m_button) {
					switch (event->type()) {
						case QEvent::EnabledChange: setEnabled(m_button->isEnabled()); break;
						case QEvent::ShowToParent:  show(); break;
						case QEvent::HideToParent:  hide(); break;
						case QEvent::ToolTipChange: setToolTip(m_button->toolTip()); break;
						default: break;
					}
				}
				return QLabel::eventFilter(watched, event);
			}
			void mouseReleaseEvent(QMouseEvent *event) override
			{
				if (event->button() == Qt::LeftButton && isEnabled()
				    && rect().contains(event->position().toPoint()))
					m_button->click();
				QLabel::mouseReleaseEvent(event);
			}
		private:
			QAbstractButton *m_button;
	};

	/// @return true when @a widget lies inside a dock
	bool inDock(const QWidget *widget)
	{
		for (const QWidget *w = widget->parentWidget(); w; w = w->parentWidget())
			if (qobject_cast<const QDockWidget *>(w))
				return true;
		return false;
	}

	/// Put @a button's text in a wrapping label beside it, where it stands
	/// in its parent's layout; nothing when it stands in none
	void wrapButton(QAbstractButton *button)
	{
		if (button->text().isEmpty() || !button->parentWidget())
			return;
		QLayout *layout = button->parentWidget()->layout();
		if (!layout || layout->indexOf(button) < 0)
			return;

		const bool hidden = button->isHidden();
		auto *row = new QWidget(button->parentWidget());
		auto *row_layout = new QHBoxLayout(row);
		row_layout->setContentsMargins(0, 0, 0, 0);
		auto *text = new ButtonText(button, row);
		delete layout->replaceWidget(button, row);
		button->setText(QString());
		button->setParent(row);
		row_layout->addWidget(button, 0, Qt::AlignTop);
		row_layout->addWidget(text, 1);
		button->setVisible(!hidden);
		row->setProperty(handled, true);
	}

	class Filter : public QObject
	{
		public:
			using QObject::QObject;
		protected:
			bool eventFilter(QObject *watched, QEvent *event) override
			{
				if (event->type() == QEvent::Polish && watched->isWidgetType()) {
					auto *widget = static_cast<QWidget *>(watched);
					if (inDock(widget))
						DockTextWrap::wrap(widget);
				}
				return QObject::eventFilter(watched, event);
			}
	};
}

void DockTextWrap::install()
{
	static Filter *filter = nullptr;
	if (filter || !qApp)
		return;
	filter = new Filter(qApp);
	qApp->installEventFilter(filter);
}

void DockTextWrap::wrap(QWidget *widget)
{
	if (widget->property(handled).toBool())
		return;
	widget->setProperty(handled, true);

	if (auto *label = qobject_cast<QLabel *>(widget)) {
		if (!label->pixmap().isNull() || label->text().isEmpty())
			return;
		label->setWordWrap(true);
	} else if (qobject_cast<QCheckBox *>(widget) || qobject_cast<QRadioButton *>(widget)) {
		wrapButton(static_cast<QAbstractButton *>(widget));
	} else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
		if (combo->sizeAdjustPolicy() != QComboBox::AdjustToMinimumContentsLengthWithIcon) {
			combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
			combo->setMinimumContentsLength(4);
		}
	}
}

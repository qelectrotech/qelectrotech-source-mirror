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
#include "dockfit.h"

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
#include <QScrollArea>
#include <QScrollBar>
#include <QHeaderView>
#include <QTreeView>

namespace {
	const char *const handled = "qetDockFitHandled";

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

	/**
		A scroll area that scrolls sideways only: its content takes its
		whole width when there is room and keeps its smallest width when
		there is not, with a horizontal scroll bar. Upwards it is as tall
		as the content allows, its smallest and largest height included,
		so a panel whose content shrinks still shrinks with it.
	*/
	class SideScroll : public QScrollArea
	{
		public:
			SideScroll(QWidget *content, QWidget *parent) : QScrollArea(parent)
			{
				setObjectName(QStringLiteral("qet_dock_side_scroll"));
				setFrameShape(QFrame::NoFrame);
				setWidgetResizable(true);
				setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
				setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
				setWidget(content);
				content->installEventFilter(this);
				follow();
			}

			QSize minimumSizeHint() const override
			{
				const QWidget *content = widget();
				if (!content)
					return QScrollArea::minimumSizeHint();
				const int height = qMax(content->minimumSizeHint().height(), content->minimumHeight());
				return QSize(qMin(content->minimumSizeHint().width(), 48), height + barHeight());
			}

			QSize sizeHint() const override
			{
				const QWidget *content = widget();
				if (!content)
					return QScrollArea::sizeHint();
				return content->sizeHint() + QSize(0, barHeight());
			}

		protected:
			bool eventFilter(QObject *watched, QEvent *event) override
			{
				if (watched == widget() && event->type() == QEvent::LayoutRequest) {
					if (QLayout *layout = widget()->layout())
						layout->activate();
					follow();
				}
				return QScrollArea::eventFilter(watched, event);
			}

			void resizeEvent(QResizeEvent *event) override
			{
				QScrollArea::resizeEvent(event);
					//The bar comes and goes with the width: the height
					//asked for changes with it
				follow();
			}

		private:
			int barHeight() const
			{
				return horizontalScrollBar()->isVisible()
						? horizontalScrollBar()->sizeHint().height() : 0;
			}

			void follow()
			{
				const QWidget *content = widget();
				if (!content)
					return;
				const int largest = content->maximumHeight();
				const int wanted = largest >= QWIDGETSIZE_MAX ? QWIDGETSIZE_MAX
									      : largest + barHeight();
				if (maximumHeight() != wanted)
					setMaximumHeight(wanted);
				updateGeometry();
			}
	};

	class Filter : public QObject
	{
		public:
			using QObject::QObject;
		protected:
			bool eventFilter(QObject *watched, QEvent *event) override
			{
				if (event->type() == QEvent::Polish && watched->isWidgetType()) {
					auto *widget = static_cast<QWidget *>(watched);
					if (auto *dock = qobject_cast<QDockWidget *>(widget))
						DockFit::scrollSideways(dock);
					else if (inDock(widget))
						DockFit::wrap(widget);
				}
				return QObject::eventFilter(watched, event);
			}
	};
}

void DockFit::install()
{
	static Filter *filter = nullptr;
	if (filter || !qApp)
		return;
	filter = new Filter(qApp);
	qApp->installEventFilter(filter);
}

void DockFit::wrap(QWidget *widget)
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
	} else if (auto *tree = qobject_cast<QTreeView *>(widget)) {
			//A tree of one column (the collections, the project's
			//folios): the column takes the width of its longest entry
			//rather than the tree's, so names are no longer cut with "…"
			//and a narrow tree scrolls sideways with its own scroll bar
		if (tree->isHeaderHidden()) {
			tree->header()->setStretchLastSection(false);
			tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
			tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		}
	} else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
		if (combo->sizeAdjustPolicy() != QComboBox::AdjustToMinimumContentsLengthWithIcon) {
			combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
			combo->setMinimumContentsLength(4);
		}
	}
}

void DockFit::scrollSideways(QDockWidget *dock)
{
	QWidget *content = dock->widget();
	if (!content || qobject_cast<QAbstractScrollArea *>(content)
	    || dock->property(handled).toBool())
		return;
	dock->setProperty(handled, true);
	dock->setWidget(new SideScroll(content, dock));
}

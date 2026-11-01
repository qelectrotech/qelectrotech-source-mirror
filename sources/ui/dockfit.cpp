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
#include <QGroupBox>
#include <QPointer>
#include <QLabel>
#include <QMouseEvent>
#include <QRadioButton>
#include <QAbstractScrollArea>
#include <QHash>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QHeaderView>
#include <QTabWidget>
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
				/// The text of a button set later, with setText(), comes
				/// here too: before the button paints it
			void takeText()
			{
				if (m_button->text().isEmpty())
					return;
				setText(m_button->text());
				m_button->setText(QString());
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
						case QEvent::Paint:         takeText(); break;
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

	/// @return true when @a widget stands in @a layout or in one nested in it
	bool holds(QLayout *layout, QWidget *widget)
	{
		if (layout->indexOf(widget) >= 0)
			return true;
		for (int i = 0; i < layout->count(); ++i)
			if (QLayout *inner = layout->itemAt(i)->layout())
				if (holds(inner, widget))
					return true;
		return false;
	}

	/// Put @a button's text in a wrapping label beside it, where it stands
	/// in its parent's layout or a layout nested in it; nothing when it
	/// stands in none
	void wrapButton(QAbstractButton *button)
	{
		if (button->text().isEmpty() || !button->parentWidget())
			return;
		QLayout *layout = button->parentWidget()->layout();
		if (!layout || !holds(layout, button))
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

	/**
		A scroll area keeps the size hint its content had first, so it
		stays as high as the highest page it ever showed. Each scroll area
		around @a widget, up to its dock, is held to its content's height
		now; one that scrolls sideways only (SideScroll) already follows
		its content.
	*/
	void updateScrollAreasAround(QWidget *widget)
	{
		for (QWidget *w = widget->parentWidget(); w && !qobject_cast<QDockWidget *>(w);
		     w = w->parentWidget()) {
			auto *scroll = qobject_cast<QScrollArea *>(w);
			if (!scroll || !scroll->widget() || !scroll->widgetResizable()
			    || scroll->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff)
				continue;
			const int frame = 2 * scroll->frameWidth();
			scroll->setMaximumHeight(scroll->widget()->sizeHint().height() + frame
						 + (scroll->horizontalScrollBar()->isVisible()
						    ? scroll->horizontalScrollBar()->sizeHint().height() : 0));
			scroll->updateGeometry();
		}
	}

	/**
		A tab widget as high as its current page, not as its highest one,
		so a short page leaves no empty space below it: its other pages are
		ignored by its stack, and its height is held to the current page's,
		since QTabWidget::sizeHint() still asks every page. A page that
		scrolls, or holds what scrolls (a list, a tree), keeps its own size
		policy and is not held: it takes the height the dock gives it.
	*/
	class TabFit : public QObject
	{
		public:
			explicit TabFit(QTabWidget *tabs) : QObject(tabs), m_tabs(tabs)
			{
				connect(tabs, &QTabWidget::currentChanged, this, &TabFit::fit);
				tabs->installEventFilter(this);
				fit();
			}

			void fit()
			{
				int highest = 0;
				for (int i = 0; i < m_tabs->count(); ++i) {
					QWidget *page = m_tabs->widget(i);
					if (!m_policies.contains(page))
						m_policies.insert(page, page->sizePolicy());
					highest = qMax(highest, page->sizeHint().height());
					if (i != m_tabs->currentIndex())
						page->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
					else if (scrolls(page))
						page->setSizePolicy(m_policies.value(page, page->sizePolicy()));
					else
						page->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
				}
				QWidget *current = m_tabs->currentWidget();
				if (!current)
					return;
				if (scrolls(current)) {
					if (m_tabs->maximumHeight() != QWIDGETSIZE_MAX)
						m_tabs->setMaximumHeight(QWIDGETSIZE_MAX);
					m_tabs->updateGeometry();
					updateScrollAreasAround(m_tabs);
					return;
				}
					//The tab bar and the frame, above and around the page
				const int around = m_tabs->sizeHint().height() - highest;
				const int height = qMax(current->sizeHint().height() + around,
							m_tabs->minimumSizeHint().height());
				if (m_tabs->maximumHeight() != height)
					m_tabs->setMaximumHeight(height);
				m_tabs->updateGeometry();
				updateScrollAreasAround(m_tabs);
			}

		protected:
			bool eventFilter(QObject *watched, QEvent *event) override
			{
				if (watched == m_tabs && event->type() == QEvent::LayoutRequest)
					fit();
				return QObject::eventFilter(watched, event);
			}

		private:
				/// A page that scrolls, or shows what scrolls (not the
				/// hidden list of a combo box)
			static bool scrolls(QWidget *page)
			{
				if (qobject_cast<QAbstractScrollArea *>(page))
					return true;
				const auto areas = page->findChildren<QAbstractScrollArea *>();
				for (QAbstractScrollArea *area : areas)
					if (area->window() == page->window() && area->isVisibleTo(page))
						return true;
				return false;
			}

			QTabWidget *m_tabs;
				/// Each page's own size policy, as it was first seen
			QHash<QWidget *, QSizePolicy> m_policies;
	};

	/**
		A grid of label-field pairs side by side (label, field, label,
		field) that is wider than its dock: its right half goes below its
		left half, each of its rows on a row of its own, so the dock gets
		narrower without cutting its fields. Back side by side once the dock
		is wide enough again.
	*/
	class GridReflow : public QObject
	{
		public:
			GridReflow(QGridLayout *grid, QWidget *owner) :
				QObject(owner), m_grid(grid), m_owner(owner)
			{
				learnPlaces();
			}

				/// Side by side when @a width is enough for it, else one half
				/// below the other
			void fit(int width)
			{
				if (!m_grid)
					return;
				if (!m_narrow)
					m_wide_width = m_owner->minimumSizeHint().width();
				const bool narrow = m_wide_width > 0 && width < m_wide_width;
				if (narrow == m_narrow)
					return;
					//An editor may have taken items out of the grid, or put
					//new ones in, since the places were learned: a widget
					//deleted takes its layout item with it, which must not
					//be put back. Looked up by address only, never used
				bool changed = m_grid->count() != m_places.size();
				for (int i = m_places.size() - 1; i >= 0; --i)
					if (m_grid->indexOf(m_places.at(i).item) < 0) {
						m_places.removeAt(i);
						changed = true;
					}
				if (changed) {
					if (m_narrow)
						return; // leave it as it is rather than guess
					learnPlaces();
				}
				m_narrow = narrow;
				for (const Place &place : std::as_const(m_places))
					m_grid->removeItem(place.item);
				for (const Place &place : std::as_const(m_places)) {
					if (narrow)
						m_grid->addItem(place.item, place.narrow_row, place.narrow_column,
								place.row_span, place.narrow_span);
					else
						m_grid->addItem(place.item, place.row, place.column,
								place.row_span, place.column_span);
				}
				m_grid->invalidate();
				m_owner->updateGeometry();
				updateScrollAreasAround(m_owner);
			}

		private:
				/// Where each item of the grid stands, wide and narrow
			void learnPlaces()
			{
				m_places.clear();
				QGridLayout *grid = m_grid;
				const int split = (grid->columnCount() + 1) / 2;
				for (int i = 0; i < grid->count(); ++i) {
					Place place;
					place.item = grid->itemAt(i);
					grid->getItemPosition(i, &place.row, &place.column,
							      &place.row_span, &place.column_span);
					place.narrow_row = place.row * 2 + (place.column >= split ? 1 : 0);
					place.narrow_column = place.column >= split ? place.column - split
										    : place.column;
					place.narrow_span = qMax(1, qMin(place.column_span,
							split - place.narrow_column));
					m_places << place;
				}
			}

			struct Place {
				QLayoutItem *item = nullptr;
				int row = 0, column = 0, row_span = 1, column_span = 1;
				int narrow_row = 0, narrow_column = 0, narrow_span = 1;
			};
			QPointer<QGridLayout> m_grid;
			QWidget *m_owner;
			QList<Place> m_places;
			bool m_narrow = false;
			int m_wide_width = 0;
	};

	/// The dock @a widget lies in, or nullptr
	QDockWidget *dockOf(QWidget *widget)
	{
		for (QWidget *w = widget->parentWidget(); w; w = w->parentWidget())
			if (auto *dock = qobject_cast<QDockWidget *>(w))
				return dock;
		return nullptr;
	}

	/**
		Watches the docks whose grids reflow: on a new width, each grid
		goes side by side or one half below the other. The width a grid
		has is the dock's, less the margins around the grid's widget.
	*/
	class ReflowWatch : public QObject
	{
		public:
			using QObject::QObject;
			void add(QDockWidget *dock, GridReflow *reflow, QWidget *owner)
			{
				m_reflows.append({dock, reflow, owner});
				if (!m_docks.contains(dock)) {
					m_docks << dock;
					dock->installEventFilter(this);
				}
			}
		protected:
			bool eventFilter(QObject *watched, QEvent *event) override
			{
				if (event->type() == QEvent::Resize || event->type() == QEvent::Show
				    || event->type() == QEvent::LayoutRequest)
					fitAll(static_cast<QDockWidget *>(watched));
				return QObject::eventFilter(watched, event);
			}
		private:
			void fitAll(QDockWidget *dock)
			{
				for (int i = m_reflows.size() - 1; i >= 0; --i) {
					const Entry &entry = m_reflows.at(i);
					if (!entry.reflow || !entry.owner) {
						m_reflows.removeAt(i);
						continue;
					}
					if (entry.dock != dock || !entry.owner->isVisible())
						continue;
						//The margins on both sides of the grid's widget, as
						//on its left, without what is scrolled away
					const int left = qMax(0, entry.owner->mapTo(dock, QPoint(0, 0)).x());
					entry.reflow->fit(dock->width() - 2 * left);
				}
			}
			struct Entry {
				QDockWidget *dock;
				QPointer<GridReflow> reflow;
				QPointer<QWidget> owner;
			};
			QList<Entry> m_reflows;
			QList<QDockWidget *> m_docks;
	};

	ReflowWatch *reflowWatch()
	{
		static ReflowWatch *watch = new ReflowWatch(qApp);
		return watch;
	}

	/// Let every grid of two pairs or more in @a layout, @a layout
	/// included, reflow in @a dock
	void reflowGrids(QLayout *layout, QWidget *owner, QDockWidget *dock)
	{
		for (int i = 0; i < layout->count(); ++i)
			if (QLayout *inner = layout->itemAt(i)->layout())
				reflowGrids(inner, owner, dock);
		auto *grid = qobject_cast<QGridLayout *>(layout);
		if (grid && grid->columnCount() >= 2)
			reflowWatch()->add(dock, new GridReflow(grid, owner), owner);
	}

	class Filter : public QObject
	{
		public:
			using QObject::QObject;
		protected:
			bool eventFilter(QObject *watched, QEvent *event) override
			{
					//Polished before it stood in a dock, a widget is handled
					//when it is first shown there
				if ((event->type() == QEvent::Polish || event->type() == QEvent::Show)
				    && watched->isWidgetType()) {
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

bool DockFit::enabled()
{
	return QSettings().value(QStringLiteral("diagrameditor/narrow_docks"), false).toBool();
}

void DockFit::setEnabled(bool on)
{
	QSettings().setValue(QStringLiteral("diagrameditor/narrow_docks"), on);
}

void DockFit::install()
{
	static Filter *filter = nullptr;
	if (filter || !qApp || !enabled())
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
	} else if (auto *tabs = qobject_cast<QTabWidget *>(widget)) {
		new TabFit(tabs);
	} else if (auto *group = qobject_cast<QGroupBox *>(widget)) {
			//A width a form gave the group in the designer
		group->setMinimumWidth(0);
	}

		//The grids of a group box, label-field pairs side by side, go one
		//half below the other in a narrow dock; other grids, tables like
		//the Auto numbering panel's, stay as they are
	if (qobject_cast<QGroupBox *>(widget))
		if (QLayout *layout = widget->layout())
			if (QDockWidget *dock = dockOf(widget))
				reflowGrids(layout, widget, dock);
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

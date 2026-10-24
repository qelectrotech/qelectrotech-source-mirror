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
#ifndef BRIDGECREATEDIALOG_H
#define BRIDGECREATEDIALOG_H

#include "bridgemanager.h"

#include <QColor>
#include <QDialog>
#include <QList>

class Bridge;
class Diagram;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QSpinBox;
class QTabWidget;
class KColorCombo;

/**
	@brief What to do with the bridge over the terminals just crossed:
	build a new one, or land it on a bridge which is already there.

	One window, two pages -- the way the cable does it. The new page
	says what the bridge may hold and which colour it is drawn in; the
	other lists the bridges already there to land on.

	A bridge which cannot take the terminals just crossed is listed
	struck through and cannot be chosen at all -- not with a warning,
	not at all. A bridge which cannot be built in the cabinet is not
	drawn either: the user takes what is free or draws a new bridge, and
	that choice is the very test of what can really be built.
*/
class BridgeCreateDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit BridgeCreateDialog(Diagram *diagram,
									const QList<BridgeCrossing> &crossings,
									QWidget *parent = nullptr);
			/**
				The window of a bridge already drawn, opened by double
				clicking one of its lines: the colour and how many
				terminals it may hold, both the bridge's own -- nothing
				here lands on anything.
			*/
		explicit BridgeCreateDialog(Bridge *bridge, QWidget *parent = nullptr);
		~BridgeCreateDialog() override = default;

			///The bridge to land on, or nullptr when a new one is built
		Bridge *bridge() const;
			///The colour of a new bridge
		QColor newColor() const;
			///How thick the lines of the bridge are drawn, in the same
			///way a conductor takes its size
		qreal newLineWidth() const;
			///How many terminals a new bridge may hold,
			///Bridge::unlimited_connections when there is no limit
		int newMaxConnections() const;

			/**
				Why what is chosen now cannot be taken -- a bridge which
				cannot be built in the cabinet is refused, not warned
				about -- or an empty string when it can. This is the very
				question accept() asks before it takes anything.
			*/
		QString problem() const;

			/**
				When the terminals a bridge would land on do not agree
				about the function and the voltage they carry, the user is
				asked which the bridge takes -- the way sheet references
				are linked: the candidates with what they carry, one to
				choose, and cancelling aborts whatever was being done.
				@param sets the distinct sets the terminals carry
				@param ok set to false when he cancelled
				@return the set the bridge takes
			*/
		static BridgeInformation askForInformation(const QList<BridgeInformation> &sets,
												   bool *ok,
												   QWidget *parent = nullptr);

	public slots:
		void accept() override;

	private slots:
		void rebuildList();
		void selectionChanged();

	private:
		bool isFull(Bridge *bridge) const;
		bool hasTooFewPlaces(Bridge *bridge) const;
		static QIcon swatch(const QColor &color);

	private:
		Diagram *m_diagram = nullptr;
		QList<BridgeCrossing> m_crossings;
		Bridge *m_bridge = nullptr;
			///The bridge being edited, when this is its window rather
			///than the one opening after a line was drawn
		Bridge *m_editing = nullptr;

		QTabWidget *m_tabs = nullptr;
		QSpinBox *m_max_sb = nullptr;
		QDoubleSpinBox *m_width_sb = nullptr;
		KColorCombo *m_color_cb = nullptr;
		QCheckBox *m_hide_full = nullptr;
		QCheckBox *m_hide_small = nullptr;
		QListWidget *m_list = nullptr;
		QLabel *m_note = nullptr;
};

#endif // BRIDGECREATEDIALOG_H

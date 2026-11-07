// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef POTENTIALSOURCESWIDGET_H
#define POTENTIALSOURCESWIDGET_H

#include "abstractelementpropertieseditorwidget.h"

#include <QHash>
#include <QList>
#include <QUuid>

class QComboBox;
class QTableWidget;

/**
	@brief The PotentialSourcesWidget class
	The "Potential sources" tab of a placed symbol's properties: a potential
	type for each of its terminals, for a project that takes the types from
	where the potential starts (PotentialTypes::Mode::AtSource). The type
	reaches every conductor at that terminal's potential that has no type
	of its own.
*/
class PotentialSourcesWidget : public AbstractElementPropertiesEditorWidget
{
		Q_OBJECT
	public:
		explicit PotentialSourcesWidget(Element *element, QWidget *parent = nullptr);

			/// Whether @p element gets this tab: its project takes the
			/// types from the sources, and it has terminals
		static bool isWanted(const Element *element);

		void setElement(Element *element) override;
		void apply() override;
		void reset() override {updateUi();}
		QUndoCommand *associatedUndo() const override;
		QString title() const override;
		void updateUi() override;
		bool setLiveEdit(bool live_edit) override;

	private:
		QHash<QUuid, QString> currentSources() const;

		QTableWidget *m_table = nullptr;
		QList<QComboBox *> m_combos;
		QList<QUuid> m_terminals;
};

#endif // POTENTIALSOURCESWIDGET_H

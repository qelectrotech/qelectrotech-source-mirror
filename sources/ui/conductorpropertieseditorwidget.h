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
#ifndef CONDUCTORPROPERTIESEDITORWIDGET_H
#define CONDUCTORPROPERTIESEDITORWIDGET_H

#include "../PropertiesEditor/propertieseditorwidget.h"
#include "../conductorproperties.h"

#include <QList>
#include <QMetaObject>
#include <QPointer>

class Conductor;
class ConductorPropertiesWidget;
class QCheckBox;
class QLabel;

/**
	@brief The ConductorPropertiesEditorWidget class
	Hosts the existing ConductorPropertiesWidget in the dockable selection-
	properties panel, so selected conductors can be edited in place like the
	other item types, instead of only through the modal dialog (issue #500).
	A pinned "apply to all conductors of the potential" checkbox (persisted)
	mirrors the modal dialog's option to propagate edits to the whole potential.
	When several conductors are selected, the panel shows the first one and
	applies to each only the fields the user changed.
*/
class ConductorPropertiesEditorWidget : public PropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit ConductorPropertiesEditorWidget(
			const QList<Conductor *> &conductors = {},
			QWidget *parent = nullptr);
		~ConductorPropertiesEditorWidget() override;

		void setConductors(const QList<Conductor *> &conductors);

		void apply() override;
		void reset() override;
		void updateUi() override;
		QUndoCommand *associatedUndo() const override;
		QString title() const override;
		bool setLiveEdit(bool live_edit) override;

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;

	private:
		void connectChangeSignals();
		void disconnectChangeSignals();
		void scheduleUpdateUi();
		Conductor *firstConductor() const;

	private:
		ConductorPropertiesWidget *m_cpw = nullptr;
		QCheckBox *m_apply_all_cb = nullptr;
		QLabel *m_count_label = nullptr;
		QList<QPointer<Conductor>> m_conductors;
			//What the widget showed before the edit: the fields that
			//differ from it are the ones the user changed.
		ConductorProperties m_shown;
		QList<QMetaObject::Connection> m_live_connections;
		bool m_updating = false;
		bool m_update_pending = false;
};

#endif // CONDUCTORPROPERTIESEDITORWIDGET_H

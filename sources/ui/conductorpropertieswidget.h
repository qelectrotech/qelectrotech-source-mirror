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
#ifndef CONDUCTORPROPERTIESWIDGET_H
#define CONDUCTORPROPERTIESWIDGET_H

#include "../conductorproperties.h"
#include "../custom/wirecatalogue/wirespec.h"

#include <QWidget>
class QTextOrientationSpinBoxWidget;
class QComboBox;
class QPushButton;
class QLabel;
class QTableView;
class WireCatalogueDb;
class WireCatalogueModel;
class WireFilterProxyModel;

namespace Ui {
	class ConductorPropertiesWidget;
}

class ConductorPropertiesWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit ConductorPropertiesWidget(QWidget *parent = nullptr);
		explicit ConductorPropertiesWidget(
			const ConductorProperties &properties,
			QWidget *parent = nullptr);
		~ConductorPropertiesWidget() override;

	//METHODS
		void setProperties(const ConductorProperties &properties);
		ConductorProperties properties() const;
		void setReadOnly(const bool &ro);

		void addAutonumWidget (QWidget *widget);
		void setHiddenOneTextPerFolio   (const bool &hide);
		void setDisabledShowText        (const bool &disable = true);
		void setHiddenAvailableAutonum (const bool &hide);
		QComboBox *autonumComboBox() const;
		QPushButton *editAutonumPushButton() const;

	signals:
		/// Emitted when a catalogue wire is assigned from the "Assign wires"
		/// tab, so the dialog can scope the change to this conductor only.
		void wireAssigned();

	private:
		void initWidget();
		void setConductorType(ConductorProperties::ConductorType type);
		void initAssignWiresTab();
		void populateFilters();
		WireSpec selectedWire() const;
		void applyWireAppearance(const QString &section,
								 const QString &colour,
								 const QString &cableId,
								 const QString &lineLabel);

	protected:
		bool event(QEvent *event) override;

	//SLOTS
	public slots:
		void updatePreview(bool b = true);

	private slots:
		void on_m_earth_cb_toggled(bool checked);
		void on_m_neutral_cb_toggled(bool checked);
		void on_m_update_preview_pb_clicked();
		void filtersChanged();
		void wireSelectionChanged();
		void assignSelectedWire();

	private:
		Ui::ConductorPropertiesWidget *ui;
		ConductorProperties m_properties;
		QTextOrientationSpinBoxWidget *m_verti_select, *m_horiz_select;
		// Custom (Trovo Tech): "Assign wires" tab.
		WireCatalogueDb      *m_wire_db       = nullptr;
		WireCatalogueModel   *m_wire_model    = nullptr;
		WireFilterProxyModel *m_wire_proxy    = nullptr;
		QTableView           *m_wire_table    = nullptr;
		QComboBox            *m_colour_filter = nullptr;
		QComboBox            *m_section_filter= nullptr;
		QComboBox            *m_core_cb       = nullptr;
		QLabel               *m_core_label    = nullptr;
		QPushButton          *m_assign_btn    = nullptr;
};

#endif // CONDUCTORPROPERTIESWIDGET_H

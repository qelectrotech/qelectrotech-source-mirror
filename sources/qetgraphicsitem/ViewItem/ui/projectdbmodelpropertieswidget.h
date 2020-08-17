/*
	Copyright 2006-2020 The QElectroTech Team
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
#ifndef PROJECTDBMODELPROPERTIESWIDGET_H
#define PROJECTDBMODELPROPERTIESWIDGET_H

#include "PropertiesEditor/propertieseditorwidget.h"

class ProjectDBModel;

namespace Ui {
class ProjectDBModelPropertiesWidget;
}

/**
	@brief The projectDBModelPropertiesWidget class
	This class is an editor for a NomenclatureModel
*/
class ProjectDBModelPropertiesWidget : public PropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit ProjectDBModelPropertiesWidget(ProjectDBModel *model = nullptr, QWidget *parent = nullptr);
		~ProjectDBModelPropertiesWidget();

		void setModel(ProjectDBModel *model);

	private slots:
		void on_m_edit_query_pb_clicked();
		void on_m_refresh_pb_clicked();

		private:
		Ui::ProjectDBModelPropertiesWidget *ui;
		ProjectDBModel *m_model = nullptr;
};

#endif // PROJECTDBMODELPROPERTIESWIDGET_H


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
#ifndef NOMENCLATUREMODELPROPERTIESWIDGET_H
#define NOMENCLATUREMODELPROPERTIESWIDGET_H

#include "PropertiesEditor/propertieseditorwidget.h"

class NomenclatureModel;

namespace Ui {
class NomenclatureModelPropertiesWidget;
}

/**
 * @brief The NomenclatureModelPropertiesWidget class
 * This class is an editor for a NomenclatureModel
 */
class NomenclatureModelPropertiesWidget : public PropertiesEditorWidget
{
    Q_OBJECT

    public:
        explicit NomenclatureModelPropertiesWidget(NomenclatureModel *model = nullptr, QWidget *parent = nullptr);
        ~NomenclatureModelPropertiesWidget();

        void setModel(NomenclatureModel *model);

	private slots:
		void on_m_edit_query_pb_clicked();
		void on_m_refresh_pb_clicked();

		private:
        Ui::NomenclatureModelPropertiesWidget *ui;
        NomenclatureModel *m_model = nullptr;
};

#endif // NOMENCLATUREMODELPROPERTIESWIDGET_H

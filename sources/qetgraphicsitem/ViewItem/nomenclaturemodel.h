/*
        Copyright 2006-2020 QElectroTech Team
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
#ifndef NOMENCLATUREMODEL_H
#define NOMENCLATUREMODEL_H

#include <QAbstractTableModel>
#include <QPointer>
#include <QDomElement>

class QETProject;

/**
 * @brief The NomenclatureModel class
 * An element nomenclature Model.
 * This model represent a 2D data.
 */
class NomenclatureModel : public QAbstractTableModel
{
    Q_OBJECT

	public:
		explicit NomenclatureModel(QETProject *project, QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		int columnCount(const QModelIndex &parent = QModelIndex()) const override;
		bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
		bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		void query(const QString &query);
		QETProject *project() const;
		void autoHeaders();

		QDomElement toXml(QDomDocument &document) const;
		void fromXml(const QDomElement &element);
		static QString xmlTagName() {return QString("nomenclature_model");}

	private:
		void dataBaseUpdated();

	private:
		QPointer<QETProject> m_project;
		QString m_query;
		QVector<QStringList> m_record;
		QHash<int, QHash<int, QVariant>> m_header_data;
		QHash<int, QVariant> m_index_0_0_data;
};

#endif // NOMENCLATUREMODEL_H

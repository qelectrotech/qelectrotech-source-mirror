/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef TITLEBLOCKPROPERTIESWIDGET_H
#define TITLEBLOCKPROPERTIESWIDGET_H

#include <QWidget>
#include "titleblockproperties.h"
#include "diagramcontextwidget.h"

class QMenu;
class TitleBlockTemplatesCollection;

namespace Ui {
	class TitleBlockPropertiesWidget;
}

class TitleBlockPropertiesWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit TitleBlockPropertiesWidget(const TitleBlockProperties &titleblock = TitleBlockProperties(), bool current_date = false, QWidget *parent = 0);
		explicit TitleBlockPropertiesWidget(TitleBlockTemplatesCollection *tbt_collection, const TitleBlockProperties &titleblock = TitleBlockProperties(), bool current_date = false, QWidget *parent = 0);
		~TitleBlockPropertiesWidget();

		void setProperties(const TitleBlockProperties &properties);
		TitleBlockProperties properties() const;

		void setTitleBlockTemplatesVisible(const bool &visible);
		void setReadOnly (const bool &ro);
		void setTitleBlockTemplatesCollection(TitleBlockTemplatesCollection *tbt_collection);

	private:
		QString currentTitleBlockTemplateName () const;
		void setCurrentTitleBlockTemplateName (const QString &name);
		void setTitleBlockTemplatesList(const QStringList &tbt);
		void initDialog(const bool &current_date);

	private slots:
		void editCurrentTitleBlockTemplate();
		void duplicateCurrentTitleBlockTemplate();
		void updateTemplateList();
		void changeCurrentTitleBlockTemplate(QString name);
		void on_m_date_now_pb_clicked();

	signals:
		void editTitleBlockTemplate(const QString &, bool);

	private:
		Ui::TitleBlockPropertiesWidget *ui;
		DiagramContextWidget *m_dcw;
		TitleBlockTemplatesCollection *m_tbt_collection;
		QAction *m_tbt_edit, *m_tbt_duplicate;
		QMenu *m_tbt_menu;
};

#endif // TITLEBLOCKPROPERTIESWIDGET_H

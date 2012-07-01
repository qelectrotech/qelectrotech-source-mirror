/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef TITLEBLOCK_PROPERTIES_WIDGET_H
#define TITLEBLOCK_PROPERTIES_WIDGET_H
#include <QtGui>
#include "titleblockproperties.h"
class DiagramContextWidget;
class TitleBlockTemplatesCollection;
/**
	Ce widget permet d'editer un objet TitleBlockProperties, c'est-a-dire les
	valeurs affichees par le cartouche d'un schema.
*/
class TitleBlockPropertiesWidget : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	TitleBlockPropertiesWidget(const TitleBlockProperties &titleblock = TitleBlockProperties(), bool = false, QWidget * = 0);
	virtual ~TitleBlockPropertiesWidget();
	private:
	TitleBlockPropertiesWidget(const TitleBlockPropertiesWidget &);
	
	// methodes
	public:
	TitleBlockProperties titleBlockProperties() const;
	void setTitleBlockProperties(const TitleBlockProperties &);
	void clearDiagramContext();
	void setDiagramContext(const DiagramContext &);
	bool displayCurrentDate() const;
	bool isReadOnly() const;
	void setReadOnly(bool);
	void setTitleBlockTemplatesList(const QList<QString> &);
	void setTitleBlockTemplatesCollection(TitleBlockTemplatesCollection *);
	void setTitleBlockTemplatesVisible(bool);
	QString currentTitleBlockTemplateName() const;
	void setCurrentTitleBlockTemplateName(const QString &);
	
	// slots:
	private slots:
	void updateTemplateList();
	void editCurrentTitleBlockTemplate();
	void duplicateCurrentTitleBlockTemplate();
	
	// private methods
	private:
	void initWidgets(const TitleBlockProperties &);
	void initLayouts();
	
	signals:
	void editTitleBlockTemplate(const QString &, bool);
	
	// attributs
	private:
	QStackedLayout *stack_layout;
	QLabel       *titleblock_template_label;
	QComboBox    *titleblock_template_name;
	QPushButton  *titleblock_template_button_;
	QMenu        *titleblock_template_menu_;
	QAction      *titleblock_template_edit_;
	QAction      *titleblock_template_duplicate_;
	QLineEdit    *titleblock_title;
	QLineEdit    *titleblock_author;
	QDateEdit    *titleblock_date;
	QLineEdit    *titleblock_filename;
	QLineEdit    *titleblock_folio;
	QLabel       *folio_tip;
	QRadioButton *titleblock_no_date;
	QRadioButton *titleblock_current_date;
	QRadioButton *titleblock_fixed_date;
	bool display_current_date;
	QLabel       *additional_fields_label;
	DiagramContextWidget *additional_fields_;
	QTabBar      *tabbar;
	TitleBlockTemplatesCollection *tbt_collection_;
};
#endif

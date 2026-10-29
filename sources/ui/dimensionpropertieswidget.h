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
#ifndef DIMENSIONPROPERTIESWIDGET_H
#define DIMENSIONPROPERTIESWIDGET_H

#include "../PropertiesEditor/propertieseditorwidget.h"
#include <QPointer>

namespace Ui {
	class DimensionPropertiesWidget;
}

class DimensionItem;

/**
	@brief The DimensionPropertiesWidget class
	Properties editor panel in the selection dock for DimensionItem primitives.
*/
class DimensionPropertiesWidget : public PropertiesEditorWidget
{
	Q_OBJECT

public:
	explicit DimensionPropertiesWidget(DimensionItem *item = nullptr, QWidget *parent = nullptr);
	DimensionPropertiesWidget(const QList<DimensionItem *> &items, QWidget *parent = nullptr);
	~DimensionPropertiesWidget() override;

	void setItem(DimensionItem *item);
	void setItems(const QList<DimensionItem *> &items);

	void apply() override;
	void reset() override;
	void updateUi() override;
	QString title() const override { return tr("Dimension properties"); }

private slots:
	void on_m_auto_cb_toggled(bool checked);
	void on_m_custom_text_le_editingFinished();
	void on_m_offset_sb_valueChanged(double val);
	void on_m_style_cb_currentIndexChanged(int index);
	void on_m_edit_dialog_btn_clicked();

private:
	void setupConnections();
	void clearConnections();

	Ui::DimensionPropertiesWidget *ui;
	QPointer<DimensionItem> m_item;
	QList<QPointer<DimensionItem>> m_items;
	QList<QMetaObject::Connection> m_connections;
	bool m_updating = false;
};

#endif // DIMENSIONPROPERTIESWIDGET_H


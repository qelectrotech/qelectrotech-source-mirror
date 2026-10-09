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

#ifndef CABINETLAYOUTREFERENCEPROPERTIESWIDGET_H
#define CABINETLAYOUTREFERENCEPROPERTIESWIDGET_H

#include "../PropertiesEditor/propertieseditorwidget.h"

#include <QPointer>

class CabinetLayoutReferenceItem;

namespace Ui {
	class CabinetLayoutReferencePropertiesWidget;
}

/**
	@brief The CabinetLayoutReferencePropertiesWidget class
	Properties editor for a single CabinetLayoutReferenceItem, shown
	in the "Auswahleigenschaften" dock like every other diagram item
	type. Single-selection only (mirrors Element/DiagramImageItem's
	own factory handling -- see PropertiesEditorFactory -- rather
	than QetShapeItem's multi-select support: a reference always
	points at exactly one real device, editing several at once has
	no clear meaning here).

	Line and fill fields mirror ShapeGraphicsItemPropertiesWidget
	exactly (same widget names/behavior, minus multi-select).
	Rotation, font, and info-source fields mirror the element
	editor's DynamicTextFieldEditor (m_rotation_sb, m_font_pb ->
	QFontDialog::getFont(), m_elmt_info_cb populated via
	QETInformation::elementInfoKeys()/translatedInfoKey()).
*/
class CabinetLayoutReferencePropertiesWidget : public PropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit CabinetLayoutReferencePropertiesWidget(CabinetLayoutReferenceItem *item, QWidget *parent = nullptr);
		~CabinetLayoutReferencePropertiesWidget() override;

		void setItem(CabinetLayoutReferenceItem *item);

	public slots:
		void apply() override;
		void reset() override;

	public:
		QUndoCommand* associatedUndo() const override;
		QString title() const override { return tr("Éditer les propriétés d'une référence de disposition"); }
		void updateUi() override;
		bool setLiveEdit(bool live_edit) override;

	private:
		void setUpEditConnection();
		void clearEditConnection();
		void populateInfoCombo();

	private slots:
		void on_m_font_pb_clicked();

	private:
		Ui::CabinetLayoutReferencePropertiesWidget *ui;
		QPointer<CabinetLayoutReferenceItem> m_item;
		QFont m_current_font;
		QList<QMetaObject::Connection> m_edit_connection;
};

#endif // CABINETLAYOUTREFERENCEPROPERTIESWIDGET_H
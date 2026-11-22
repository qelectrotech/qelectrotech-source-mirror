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
#ifndef DEFINITIONMARKSTYLEEDITOR_H
#define DEFINITIONMARKSTYLEEDITOR_H

#include "properties/definitionmarkstyle.h"

#include <QDialog>
#include <QList>

class PreviewItem;
class QGraphicsItem;
class QGraphicsScene;
class QETProject;

namespace Ui {
	class DefinitionMarkStyleEditor;
}

/**
	@brief The DefinitionMarkStyleEditor class
	The dialog behind the "Customize..." button of a conductor's
	properties: how the conductor's stroke with colour and section is
	drawn. A live preview shows the shapes and texts around the stroke on
	a sample conductor; they are dragged in it and their properties are
	edited beside it. The drawing the dialog leaves is saved with the
	project (QETProject::setDefinitionMarkStyle()), so the project looks
	the same wherever it is opened, and -- asked to -- becomes the
	drawing new projects start with.
*/
class DefinitionMarkStyleEditor : public QDialog
{
	Q_OBJECT

	public:
		explicit DefinitionMarkStyleEditor(QETProject *project,
						   QWidget *parent = nullptr);
		~DefinitionMarkStyleEditor() override;

		DefinitionMarkStyle style() const { return m_style; }

	protected:
		void accept() override;
		void resizeEvent(QResizeEvent *event) override;
		void showEvent(QShowEvent *event) override;

	private slots:
		void toggleEnabled(bool on);
		void applyVerticalTurn(int index);
		void addShape();
		void addText();
		void removeItem();
		void currentRowChanged();
		void fillForm();
		void applyShapeForm();
		void applyTextForm();
		void resetToStandard();
		void previewSelectionChanged();

	private:
		void rebuildItems();
		void updatePreviews();
		void updateList();
		void selectRow(int row);
		int rowOf(bool text_item, int index) const;
		bool rowIsText(int row) const;
		int rowIndexOf(int row) const;

		Ui::DefinitionMarkStyleEditor *ui;
		QETProject *m_project;
		DefinitionMarkStyle m_style;
		QGraphicsScene *m_scene;
		QList<PreviewItem *> m_preview_items;
			/// What the preview's sample conductors are drawn as,
			/// with the shapes and texts of the style on each
		QList<QGraphicsItem *> m_anchors;
		bool m_filling = false;
};

#endif // DEFINITIONMARKSTYLEEDITOR_H

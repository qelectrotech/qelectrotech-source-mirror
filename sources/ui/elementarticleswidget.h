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
#ifndef ELEMENTARTICLESWIDGET_H
#define ELEMENTARTICLESWIDGET_H

#include "abstractelementpropertieseditorwidget.h"
#include <diagramcontext.h>

#include <QList>
#include <QMap>
#include <QStringList>
#include <array>

class Element;
class ElementInfoPartWidget;
class QLabel;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QUndoCommand;
struct MaterialRecord;

/**
	@brief The ElementArticlesWidget class
	Provides a master-detail editor tab for the five native article slots
	of an element (main article + four auxiliary articles).
*/
class ElementArticlesWidget : public AbstractElementPropertiesEditorWidget
{
	Q_OBJECT

	public:
		static constexpr int kBlockCount = 5;

		explicit ElementArticlesWidget(Element *elmt, QWidget *parent = nullptr);
		~ElementArticlesWidget() override;

		static QStringList managedKeys();

		void setElement(Element *element) override;
		void apply() override;
		void reset() override;
		QUndoCommand *associatedUndo() const override;
		QString title() const override { return tr("Parts"); }
		bool setLiveEdit(bool live_edit) override;
		void updateUi() override;
		DiagramContext currentInfo() const;
		QSize sizeHint() const override;
		QSize minimumSizeHint() const override;

	protected:
		void enableLiveEdit() override;
		void disableLiveEdit() override;

	private slots:
		void elementInfoChange();
		void addArticle();
		void removeCurrentArticle();
		void currentArticleChanged(int row);
		void articleItemChanged(QTreeWidgetItem *item, int column);

	private:
		void buildInterface();
		QWidget *buildBlockPage(int block);
		void refreshArticleList(int preferred_block = -1);
		void updateArticleListRow(int block);
		void updateActionButtons();
		bool blockHasData(int block) const;
		QString articleSubtitle(int block) const;
		QString articleQuantity(int block) const;
		void materialFromFile(int block);
		void applyMaterialRecord(const MaterialRecord &record, int block);
		ElementInfoPartWidget *partForKey(const QString &key) const;
		int currentBlock() const;
		bool isHostedInDock() const;

	private:
		QSplitter *m_splitter = nullptr;
		QWidget *m_left_panel = nullptr;
		QWidget *m_right_panel = nullptr;
		QTreeWidget *m_article_list = nullptr;
		QPushButton *m_add_btn = nullptr;
		QPushButton *m_remove_btn = nullptr;
		QLabel *m_page_header_label = nullptr;
		QPushButton *m_pick_material_btn = nullptr;
		QStackedWidget *m_stack = nullptr;
		QList<ElementInfoPartWidget *> m_parts;
		std::array<QList<ElementInfoPartWidget *>, kBlockCount> m_block_parts{};
		std::array<QWidget *, kBlockCount> m_block_pages{};
		std::array<bool, kBlockCount> m_explicitly_added{};
		QMap<QString, QString> m_loaded_values;
		QMap<QString, QString> m_pending_extra_info;
		bool m_ui_built = false;

		friend class tst_elementarticleswidget;
};

#endif // ELEMENTARTICLESWIDGET_H

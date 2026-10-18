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
#ifndef MULTIELEMENTINFOWIDGET_H
#define MULTIELEMENTINFOWIDGET_H

#include "../PropertiesEditor/propertieseditorwidget.h"
#include "../diagramcontext.h"

#include <QHash>
#include <QPointer>

class Element;
class QCheckBox;
class QGraphicsItem;
class QLabel;
class QLineEdit;
class QToolButton;

/**
	@brief The MultiElementInfoWidget class
	Edit the information (manufacturer, supplier, references...) of
	several elements at once (issue #1439).

	Each field shows the value when every element has the same one, or
	"Multiple values" when they differ. A field left alone keeps each
	element's own value, a typed text is given to every element, and the
	erase box clears the field on every element. The restore button of a
	field, and reset(), bring back the values the elements had.
	All the changes of one apply are a single undo step.
*/
class MultiElementInfoWidget : public PropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit MultiElementInfoWidget(const QList<Element *> &elements,
										int other_items = 0,
										QWidget *parent = nullptr);
		~MultiElementInfoWidget() override;

		void setElements(const QList<Element *> &elements, int other_items = 0);
		QList<Element *> elements() const;

		void apply() override;
		void reset() override;
		QUndoCommand *associatedUndo() const override;
		QString title() const override {return tr("Information");}
		void updateUi() override;
		bool setLiveEdit(bool live_edit) override;

		static bool takesInformations(const Element *element);
		static QList<Element *> informationElements(const QList<QGraphicsItem *> &items);
		static QList<QGraphicsItem *> withoutOwnTexts(const QList<QGraphicsItem *> &items);

			//What a field will do to the elements on apply
		enum FieldState {Keep, Set, Clear};
		FieldState fieldState(const QString &key) const;
		void setFieldText(const QString &key, const QString &text);
		void setFieldCleared(const QString &key, bool cleared);
		void restoreField(const QString &key);
		bool fieldIsMixed(const QString &key) const;

	private:
		struct Row {
			QString      key;
			QLabel      *label   = nullptr;
			QLineEdit   *edit    = nullptr;
			QCheckBox   *erase   = nullptr;
			QToolButton *restore = nullptr;
			QString      initial;
			bool         mixed   = false;
			FieldState   state   = Keep;
		};
		struct Flag {
			QString      key;
			QCheckBox   *check   = nullptr;
			QToolButton *restore = nullptr;
			Qt::CheckState initial = Qt::Unchecked;
			bool         changed = false;
		};

		void buildInterface();
		void takeSnapshot();
		void fillRows();
		void updateHeader();
		void updateSuggestions();
		void rowEdited(Row *row);
		void rowErased(Row *row);
		void restoreRow(Row *row);
		void restoreFlag(Flag *flag);
		void flagClicked(Flag *flag);
		void updateRowLook(Row *row);
		void updateLabelRow();
		void changed();
		void elementInfoChanged();
		Row *rowForKey(const QString &key) const;

		QList<QPointer<Element>> m_elements;
		QHash<Element *, DiagramContext> m_snapshot;
		int m_other_items = 0;
		QLabel *m_header = nullptr;
		QList<Row *> m_rows;
		QList<Flag *> m_flags;
		bool m_updating = false;
		bool m_applying = false;
		bool m_refresh_pending = false;
		quint64 m_session = 0;
};

#endif // MULTIELEMENTINFOWIDGET_H

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
#include "placesuggestions.h"

#include "dataBase/projectdatabase.h"
#include "qetproject.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCompleter>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPointer>
#include <QSqlQuery>
#include <QStringListModel>

namespace
{
	/**
		A click outside the open list closes it, and Qt drops that click: the
		OK or Apply button clicked to finish the edit would do nothing. The
		click is passed on to the widget under it, as the symbol information
		list does (ElementInfoPartWidget::eventFilter).
	*/
	class PopupClickFilter : public QObject
	{
		public:
			PopupClickFilter(QAbstractItemView *popup, QObject *parent) :
				QObject(parent), m_popup(popup) {}

		protected:
			bool eventFilter(QObject *watched, QEvent *event) override
			{
				if (!m_popup || watched != m_popup
						|| event->type() != QEvent::MouseButtonPress)
					return QObject::eventFilter(watched, event);
				auto *mouse = static_cast<QMouseEvent *>(event);
				const QPoint global = mouse->globalPosition().toPoint();
				if (m_popup->rect().contains(m_popup->mapFromGlobal(global)))
					return false;
				m_popup->hide();
				if (QWidget *target = QApplication::widgetAt(global)) {
					const QPointF local = target->mapFromGlobal(global);
					QCoreApplication::postEvent(target, new QMouseEvent(
							QEvent::MouseButtonPress, local, global, mouse->button(),
							mouse->buttons(), mouse->modifiers()));
					QCoreApplication::postEvent(target, new QMouseEvent(
							QEvent::MouseButtonRelease, local, global, mouse->button(),
							Qt::NoButton, mouse->modifiers()));
				}
				return true;
			}

		private:
			QPointer<QAbstractItemView> m_popup;
	};

		/// One column of qet_locations: distinct, not empty, one spelling of
		/// names which differ only in case
	QStringList column(QETProject *project, const QString &where)
	{
		if (!project || !project->dataBase()) return {};
		project->dataBase()->updateDB();
		QSqlQuery query = project->dataBase()->newQuery(where);
		QStringList list;
		while (query.next()) {
			const QString name = query.value(0).toString().trimmed();
			if (!name.isEmpty()) list << name;
		}
		return PlaceSuggestions::merged(list, {});
	}
}

namespace PlaceSuggestions
{
	/**
		@return @p first then what @p second adds, each name once, any case,
		in order
	*/
	QStringList merged(const QStringList &first, const QStringList &second)
	{
		QStringList list;
		for (const QStringList *source : {&first, &second})
			for (const QString &name : *source)
				if (!name.isEmpty() && !list.contains(name, Qt::CaseInsensitive))
					list << name;
		list.sort(Qt::CaseInsensitive);
		return list;
	}

	/**
		@return the plants of @p project, sorted
	*/
	QStringList plants(QETProject *project)
	{
		return column(project, QStringLiteral(
				"SELECT DISTINCT plant FROM qet_locations ORDER BY plant COLLATE NOCASE"));
	}

	/**
		@return the locations of plant @p plant in @p project, sorted; of
		every plant when @p plant is empty
	*/
	QStringList locations(QETProject *project, const QString &plant)
	{
		if (plant.trimmed().isEmpty())
			return column(project, QStringLiteral(
					"SELECT DISTINCT location FROM qet_locations ORDER BY location COLLATE NOCASE"));
		if (!project || !project->dataBase()) return {};
		project->dataBase()->updateDB();
		QSqlQuery query(project->dataBase()->database());
		query.prepare(QStringLiteral(
				"SELECT DISTINCT location FROM qet_locations "
				"WHERE plant = :plant COLLATE NOCASE ORDER BY location COLLATE NOCASE"));
		query.bindValue(QStringLiteral(":plant"), plant.trimmed());
		QStringList list;
		if (query.exec())
			while (query.next()) list << query.value(0).toString().trimmed();
		return merged(list, {});
	}

	/**
		Give @p edit a drop-down of @p suggestions, matching anywhere in the
		text and ignoring case. An empty list removes it.
	*/
	void setSuggestions(QLineEdit *edit, const QStringList &suggestions)
	{
		if (!edit) return;
		if (suggestions.isEmpty()) {
			edit->setCompleter(nullptr);
			return;
		}
		QCompleter *completer = edit->completer();
		auto *model = completer ? qobject_cast<QStringListModel *>(completer->model()) : nullptr;
		if (!model) {
			model = new QStringListModel(edit);
			completer = new QCompleter(model, edit);
			completer->setCaseSensitivity(Qt::CaseInsensitive);
			completer->setFilterMode(Qt::MatchContains);
			completer->popup()->installEventFilter(new PopupClickFilter(completer->popup(), completer));
			edit->setCompleter(completer);
		}
		model->setStringList(suggestions);
	}

	/**
		Offer the plants of @p project in @p plant_edit and, in
		@p location_edit, the locations of the plant typed in the first,
		refilled as it changes. Nothing happens without a project.
	*/
	void attach(QLineEdit *plant_edit, QLineEdit *location_edit, QETProject *project)
	{
		if (!project || !plant_edit || !location_edit) return;
		setSuggestions(plant_edit, plants(project));
		QPointer<QETProject> guard(project);
		const auto refill = [guard, plant_edit, location_edit]() {
			if (!guard) return;
			setSuggestions(location_edit, locations(guard, plant_edit->text()));
		};
		refill();
		QObject::connect(plant_edit, &QLineEdit::textChanged, location_edit, refill);
	}
}

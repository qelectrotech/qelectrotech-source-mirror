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

#include <QDebug>
#include <QAbstractItemView>
#include <QApplication>
#include <QCompleter>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPointer>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringListModel>

#include <memory>

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
		@return the places of @p project: where its symbols and folios are,
		as the database views work them out (what qet_locations counts),
		without a symbol of @p except_element
	*/
	QList<QPair<QString, QString>> places(QETProject *project,
										  const QString &except_element)
	{
		QList<QPair<QString, QString>> list;
		if (!project || !project->dataBase()) return list;
		project->dataBase()->updateDB();
		QSqlQuery query(project->dataBase()->database());
		query.prepare(QStringLiteral(
				"SELECT COALESCE(plant, ''), COALESCE(location, '') FROM qet_devices "
				"WHERE COALESCE(kind, '') NOT IN ('next_report', 'previous_report') "
				"AND device_uuid != :except "
				"UNION SELECT COALESCE(plant, ''), COALESCE(location, '') FROM qet_folios"));
		//A null string would bind as NULL, and "!= NULL" matches nothing
		query.bindValue(QStringLiteral(":except"),
						except_element.isEmpty() ? QStringLiteral("") : except_element);
		if (!query.exec()) {
			qWarning() << "PlaceSuggestions::places" << query.lastError().text();
			return list;
		}
		while (query.next())
			list.append({query.value(0).toString().trimmed(),
						 query.value(1).toString().trimmed()});
		return list;
	}

	/**
		@return the plants among @p places, sorted, each once
	*/
	QStringList plants(const QList<QPair<QString, QString>> &places)
	{
		QStringList list;
		for (const auto &place : places) list << place.first;
		return merged(list, {});
	}

	/**
		@return the locations of plant @p plant among @p places, sorted; of
		every plant when @p plant is empty
	*/
	QStringList locations(const QList<QPair<QString, QString>> &places,
						  const QString &plant)
	{
		const QString wanted = plant.trimmed();
		QStringList list;
		for (const auto &place : places)
			if (wanted.isEmpty() || place.first.compare(wanted, Qt::CaseInsensitive) == 0)
				list << place.second;
		return merged(list, {});
	}

	QStringList plants(QETProject *project, const QString &except_element)
	{
		return plants(places(project, except_element));
	}

	QStringList locations(QETProject *project, const QString &plant,
						  const QString &except_element)
	{
		return locations(places(project, except_element), plant);
	}

	/**
		Give @p edit a drop-down of @p suggestions, matching anywhere in the
		text and ignoring case. An empty list removes it.
	*/
	void setSuggestions(QLineEdit *edit, const QStringList &suggestions)
	{
		if (!edit) return;
		QCompleter *completer = edit->completer();
		auto *model = completer ? qobject_cast<QStringListModel *>(completer->model()) : nullptr;
			//An emptied list keeps its completer, with nothing to offer
		if (!model && suggestions.isEmpty()) return;
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
			//One read of the database: typing only filters it
		const auto all = std::make_shared<const QList<QPair<QString, QString>>>(places(project));
		setSuggestions(plant_edit, plants(*all));
		const auto refill = [all, plant_edit, location_edit]() {
			setSuggestions(location_edit, locations(*all, plant_edit->text()));
		};
		refill();
		QObject::connect(plant_edit, &QLineEdit::textChanged, location_edit, refill);
	}
}

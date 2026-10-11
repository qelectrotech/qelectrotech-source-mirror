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
#include "autonumschemecommand.h"

#include "../diagram.h"
#include "../qetproject.h"

#include <algorithm>

AutoNumSchemeCommand::AutoNumSchemeCommand(QETProject *project, Kind kind) :
	m_project(project),
	m_kind(kind)
{}

/// @return the titles of the schemes of @p kind, sorted
QStringList AutoNumSchemeCommand::titles(const QETProject *project, Kind kind)
{
	if (!project) {
		return {};
	}
	QStringList list = kind == Kind::Conductor ? project->conductorAutoNum().keys()
											   : project->folioAutoNum().keys();
	list.sort(Qt::CaseInsensitive);
	return list;
}

bool AutoNumSchemeCommand::contains(const QETProject *project, Kind kind, const QString &title)
{
	if (!project) {
		return false;
	}
	return kind == Kind::Conductor ? project->conductorAutoNum().contains(title)
								   : project->folioAutoNum().contains(title);
}

NumerotationContext AutoNumSchemeCommand::contextOf(const QETProject *project, Kind kind, const QString &title)
{
	if (!project) {
		return NumerotationContext();
	}
	return kind == Kind::Conductor ? project->conductorAutoNum(title)
								   : project->folioAutoNum(title);
}

/**
	@return the title of the scheme @p name clashes with, empty if none:
	the same name, whatever the case or the white space
	@param ignored_title : the scheme being renamed
*/
QString AutoNumSchemeCommand::nameClash(const QETProject *project, Kind kind,
										const QString &name, const QString &ignored_title)
{
	const QString wanted = QETProject::normalizedAutoNumName(name);
	for (const QString &title : titles(project, kind)) {
		if (title == ignored_title) {
			continue;
		}
		if (QString::compare(QETProject::normalizedAutoNumName(title), wanted,
							 Qt::CaseInsensitive) == 0) {
			return title;
		}
	}
	return QString();
}

/// @return why @p name cannot be the name of a scheme, translated, empty if it can
QString AutoNumSchemeCommand::nameProblem(const QETProject *project, Kind kind,
										  const QString &name, const QString &ignored_title)
{
	if (QETProject::normalizedAutoNumName(name).isEmpty()) {
		return tr("The numbering name cannot be empty.");
	}
	const QString clash = nameClash(project, kind, name, ignored_title);
	if (!clash.isEmpty()) {
		return tr("A numbering named “%1” already exists.").arg(clash);
	}
	return QString();
}

/**
	@return the folios which follow the scheme @p title
	@param references : every folio which names it, for a folio numbering,
	even if its number has been written into the folio field already (see
	the class description)
*/
QList<Diagram *> AutoNumSchemeCommand::usersOf(const QETProject *project, Kind kind,
											   const QString &title, bool references)
{
	QList<Diagram *> users;
	if (!project || title.isEmpty()) {
		return users;
	}
		//The schemes a folio uses are its own data, read on opening:
		//folios() builds nothing
	for (Diagram *diagram : project->folios()) {
		if (kind == Kind::Conductor) {
			if (diagram->conductorsAutonumName() == title) {
				users << diagram;
			}
		} else if (diagram->border_and_titleblock.autoPageNum() == title
				   && (references || diagram->border_and_titleblock.folio().contains(QLatin1String("%autonum")))) {
			users << diagram;
		}
	}
	return users;
}

AutoNumSchemeCommand *AutoNumSchemeCommand::create(QETProject *project, Kind kind,
												   const QString &title,
												   const NumerotationContext &context,
												   bool make_current)
{
	if (!project || !nameProblem(project, kind, title).isEmpty()) {
		return nullptr;
	}
	auto *cmd = new AutoNumSchemeCommand(project, kind);
	cmd->m_after = Scheme{QETProject::normalizedAutoNumName(title), context};
	cmd->m_current_before = project->conductorCurrentAutoNum();
	cmd->m_current_after = (kind == Kind::Conductor && make_current)
						   ? cmd->m_after->title
						   : cmd->m_current_before;
	cmd->setText(tr("Create numbering %1").arg(cmd->m_after->title));
	return cmd;
}

/**
	Rename the scheme @p old_title to @p new_title and give it @p context.
	@param make_current : for conductors, make it the scheme new conductors
	take; otherwise it stays so if it was
	@return nullptr if there is no such scheme, the new name is not
	acceptable, or nothing changes
*/
AutoNumSchemeCommand *AutoNumSchemeCommand::edit(QETProject *project, Kind kind,
												 const QString &old_title,
												 const QString &new_title,
												 const NumerotationContext &context,
												 bool make_current)
{
	if (!project || !contains(project, kind, old_title)) {
		return nullptr;
	}
	if (!nameProblem(project, kind, new_title, old_title).isEmpty()) {
		return nullptr;
	}

	auto *cmd = new AutoNumSchemeCommand(project, kind);
	cmd->m_before = Scheme{old_title, contextOf(project, kind, old_title)};
	cmd->m_after = Scheme{QETProject::normalizedAutoNumName(new_title), context};
	cmd->m_current_before = project->conductorCurrentAutoNum();
	cmd->m_current_after = kind == Kind::Conductor
						   ? ((make_current || cmd->m_current_before == old_title)
							  ? cmd->m_after->title : cmd->m_current_before)
						   : cmd->m_current_before;
	if (cmd->m_before->title != cmd->m_after->title) {
		cmd->m_users = [&] {
			QList<QPointer<Diagram>> list;
			for (Diagram *d : usersOf(project, kind, old_title, true)) list << d;
			return list;
		}();
	}

	bool same_definition = cmd->m_before->context.size() == cmd->m_after->context.size();
	for (int i = 0 ; same_definition && i < cmd->m_before->context.size() ; ++i) {
		same_definition = cmd->m_before->context[i] == cmd->m_after->context[i];
	}
	if (cmd->m_before->title == cmd->m_after->title && same_definition
			&& cmd->m_current_before == cmd->m_current_after) {
		delete cmd;
		return nullptr;
	}
	if (cmd->m_before->title != cmd->m_after->title && same_definition) {
		cmd->setText(tr("Rename numbering %1 to %2")
					 .arg(cmd->m_before->title, cmd->m_after->title));
	} else {
		cmd->setText(tr("Modify numbering %1").arg(cmd->m_after->title));
	}
	return cmd;
}

/**
	Remove the scheme @p title.
	@return nullptr if there is no such scheme or a folio still follows it
*/
AutoNumSchemeCommand *AutoNumSchemeCommand::remove(QETProject *project, Kind kind,
												   const QString &title)
{
	if (!project || !contains(project, kind, title)) {
		return nullptr;
	}
	if (!usersOf(project, kind, title).isEmpty()) {
		return nullptr;
	}
	auto *cmd = new AutoNumSchemeCommand(project, kind);
	cmd->m_before = Scheme{title, contextOf(project, kind, title)};
	cmd->m_current_before = project->conductorCurrentAutoNum();
	cmd->m_current_after = (kind == Kind::Conductor && cmd->m_current_before == title)
						   ? QString() : cmd->m_current_before;
	cmd->setText(tr("Delete numbering %1").arg(title));
	return cmd;
}

/// Let the windows which list the schemes read them again
void AutoNumSchemeCommand::announce(bool added, bool removed)
{
	if (m_kind == Kind::Conductor) {
		if (removed) m_project->conductorAutoNumRemoved();
		if (added)   m_project->conductorAutoNumAdded();
	} else {
		if (removed) m_project->folioAutoNumRemoved();
		if (added)   m_project->folioAutoNumAdded();
	}
}

/// Give the scheme @p from the title @p to, and the folios which follow it
void AutoNumSchemeCommand::rename(const QString &from, const QString &to)
{
	const NumerotationContext context = contextOf(m_project, m_kind, from);
	if (m_kind == Kind::Conductor) {
		m_project->removeConductorAutoNum(from);
		m_project->addConductorAutoNum(to, context);
		if (m_project->conductorCurrentAutoNum() == from) {
			m_project->setCurrentConductorAutoNum(to);
		}
			//Folios' own data, read on opening: folios() builds nothing
		for (Diagram *d : m_project->folios()) {
				//The highest folio number given, kept by scheme title
			for (auto *hash : {&d->m_cnd_unitfolio_max, &d->m_cnd_tenfolio_max, &d->m_cnd_hundredfolio_max}) {
				if (hash->contains(from)) {
					hash->insert(to, hash->take(from));
				}
			}
		}
	} else {
		m_project->removeFolioAutoNum(from);
		m_project->addFolioAutoNum(to, context);
	}
	for (const QPointer<Diagram> &d : std::as_const(m_users)) {
		if (!d) continue;
		if (m_kind == Kind::Conductor) {
			d->setConductorsAutonumName(to);
		} else {
			d->border_and_titleblock.setAutoPageNum(to);
		}
	}
}

void AutoNumSchemeCommand::redo()
{
	if (!m_project) return;

	if (m_before && m_after && m_before->title != m_after->title) {
		rename(m_before->title, m_after->title);
	} else if (m_before && !m_after) {
		if (m_kind == Kind::Conductor) m_project->removeConductorAutoNum(m_before->title);
		else                           m_project->removeFolioAutoNum(m_before->title);
	}
	if (m_after) {
		if (m_kind == Kind::Conductor) m_project->addConductorAutoNum(m_after->title, m_after->context);
		else                           m_project->addFolioAutoNum(m_after->title, m_after->context);
	}
	if (m_kind == Kind::Conductor) {
		m_project->setCurrentConductorAutoNum(m_current_after);
	}
	announce(m_after.has_value(), m_before.has_value() && (!m_after || m_after->title != m_before->title));
}

void AutoNumSchemeCommand::undo()
{
	if (!m_project) return;

	if (m_before && m_after && m_before->title != m_after->title) {
		rename(m_after->title, m_before->title);
	} else if (!m_before && m_after) {
		if (m_kind == Kind::Conductor) m_project->removeConductorAutoNum(m_after->title);
		else                           m_project->removeFolioAutoNum(m_after->title);
	}
	if (m_before) {
		if (m_kind == Kind::Conductor) m_project->addConductorAutoNum(m_before->title, m_before->context);
		else                           m_project->addFolioAutoNum(m_before->title, m_before->context);
	}
	if (m_kind == Kind::Conductor) {
		m_project->setCurrentConductorAutoNum(m_current_before);
	}
	announce(m_before.has_value(), m_after.has_value() && (!m_before || m_after->title != m_before->title));
}

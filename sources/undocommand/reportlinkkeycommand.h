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
#ifndef REPORTLINKKEYCOMMAND_H
#define REPORTLINKKEYCOMMAND_H

#include <qetproject.h>

#include <QList>
#include <QPair>
#include <QPointer>
#include <QUndoCommand>

class Element;

/**
	@brief The ChangeReportLinkKeyCommand class
	Changing what two folio reports must share to be chained together
	(their label, or the function of their wire) takes the chains this
	project has away and makes them again from the new key. The choice,
	the links it took away and the links it made are one step: undoing
	puts the project back with the key and the chains it had before.
*/
class ChangeReportLinkKeyCommand : public QUndoCommand
{
	public:
		ChangeReportLinkKeyCommand(QETProject *project,
								   QETProject::ReportLinkKey key,
								   QUndoCommand *parent = nullptr);
		void redo() override;
		void undo() override;

	private:
		QPointer<QETProject> m_project;
		QETProject::ReportLinkKey m_old_key = QETProject::ReportLinkKey::Label;
		QETProject::ReportLinkKey m_new_key = QETProject::ReportLinkKey::Label;
			/// The links this project had when this command was made
		QList<QPair<QPointer<Element>, QPointer<Element>>> m_links;
};

#endif // REPORTLINKKEYCOMMAND_H

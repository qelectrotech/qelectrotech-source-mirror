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
#ifndef ELEMENTPROVIDER_H
#define ELEMENTPROVIDER_H

#include <QUuid>
#include <QList>
#include <QAbstractTableModel>

#include "properties/elementdata.h"

class QETProject;
class Diagram;
class Element;
class QetGraphicsTableItem;
class TerminalElement;

/**
  this class can search in the given diagram or project some kind of element
  like 'folio report' or 'master' and return it.
  We can get element element with specific status like 'free'.
*/

class ElementProvider
{
	public:
		ElementProvider(QETProject *prj, Diagram *diagram=nullptr);
		ElementProvider(Diagram *diag);
		explicit ElementProvider(const QList<Diagram *> &diagrams);
		QVector <QPointer<Element>> freeElement(ElementData::Types filter) const;
		QList <Element *> fromUuids(QList <QUuid>) const;
		QVector<QPointer<Element> > find(ElementData::Types elmt_type) const;
		QVector <QetGraphicsTableItem *> table(QetGraphicsTableItem *table = nullptr, QAbstractItemModel *model = nullptr);
		QetGraphicsTableItem *tableFromUuid(const QUuid &uuid);
		QVector<TerminalElement *> freeTerminal() const;

	private:
		QList<Diagram *> folios(ElementData::Types kinds, bool free_only) const;
		QList<Diagram *> allFolios() const;

		QList <Diagram *> m_diagram_list;
			//Searching a project: its folios are asked for when searched,
			//so that a project opened with QET_LAZY_FOLIOS builds only
			//those a search can find something on
		QETProject *m_project = nullptr;
		Diagram *m_excluded = nullptr;
};

#endif // ELEMENTPROVIDER_H

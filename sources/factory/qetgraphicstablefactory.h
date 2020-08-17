/*
	Copyright 2006-2020 The QElectroTech Team
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
#ifndef QETGRAPHICSTABLEFACTORY_H
#define QETGRAPHICSTABLEFACTORY_H

class Diagram;
class QetGraphicsTableItem;
class AddTableDialog;

/**
	@brief The QetGraphicsTableFactory class
*/
class QetGraphicsTableFactory
{
	public:
		QetGraphicsTableFactory();

		static void createAndAddNomenclature(Diagram *diagram);
		static void createAndAddSummary(Diagram *diagram);
	private:
		static void create(Diagram *diagram, AddTableDialog *dialog);
		static QetGraphicsTableItem *newTable(
				Diagram *diagram,
				AddTableDialog *dialog,
				QetGraphicsTableItem *previous_table = nullptr);
};

#endif // QETGRAPHICSTABLEFACTORY_H

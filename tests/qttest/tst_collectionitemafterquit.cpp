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
#include "ElementsCollection/fileelementcollectionitem.h"

#include <QCoreApplication>
#include <cstdio>

/*
	The elements collection sets up its items on worker threads
	(ElementsCollectionModel::loadCollections()), and a worker can still
	be at it while the program quits, after the application object was
	destroyed. An item's setData() and data() asked which thread is the
	application's through QCoreApplication::instance(), which is null by
	then: the program crashed on quitting now and then (seen on the CI in
	tst_conductordefinitionmark). Without an application object, an item
	keeps what is set on it, as on a worker thread, and reads it back.

	Run without any application object, as at that point of quitting.
*/
int main()
{
	if (QCoreApplication::instance()) {
		std::puts("FAIL: an application object exists");
		return 1;
	}
	FileElementCollectionItem item;
	item.setText(QStringLiteral("Contactor"));
	if (item.text() != QLatin1String("Contactor")) {
		std::puts("FAIL: the text set without an application is not read back");
		return 1;
	}
	std::puts("PASS: setData() and data() work without an application object");
	return 0;
}

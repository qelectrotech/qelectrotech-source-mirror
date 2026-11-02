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
#include "ElementsCollection/elementscollectionmodel.h"

#include <QApplication>
#include <QThreadPool>
#include <QTimer>
#include <QtTest>

/*
	The elements collection loads on worker threads. When the application
	quits while it is still loading, the model stops and waits for them on
	aboutToQuit, so none is left running once the application object goes
	(#1689).
*/
class tst_CollectionModelQuit : public QObject
{
	Q_OBJECT
	private slots:
		void loadingStopsOnAboutToQuit()
		{
			ElementsCollectionModel model;
			model.loadCollections(true, true, true, {});
			QVERIFY(QThreadPool::globalInstance()->activeThreadCount() > 0);

			//Quit through an event loop: aboutToQuit can't be emitted by hand.
			QTimer::singleShot(0, qApp, &QCoreApplication::quit);
			qApp->exec();

			QCOMPARE(QThreadPool::globalInstance()->activeThreadCount(), 0);
		}
};

QTEST_MAIN(tst_CollectionModelQuit)
#include "tst_collectionmodelquit.moc"

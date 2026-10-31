// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "ElementsCollection/elementscollectionmodel.h"
#include "ElementsCollection/elementscollectionwidget.h"
#include "qetproject.h"

#include <QTreeView>

// A project closed while the element collection panel has not loaded yet, or
// while it is loading, stayed in the collection: the next reload read the
// freed project (crash in XmlElementCollection::root() from
// ElementsCollectionWidget::reload()).
class tst_collectionclosedproject : public QObject
{
	Q_OBJECT

		/// The collection's tree (not the macros one): reload() disables it
		/// until the loading has finished.
	static QTreeView *loadingView(ElementsCollectionWidget &w)
	{
		for (QTreeView *v : w.findChildren<QTreeView *>())
			if (!v->isEnabled()) return v;
		return nullptr;
	}

		/// Reload and wait for the loading to finish.
	static ElementsCollectionModel *reloadAndWait(ElementsCollectionWidget &w)
	{
		w.reload();
		QTreeView *main_view = loadingView(w);
		if (!main_view) return nullptr;
		if (!QTest::qWaitFor([main_view]() { return main_view->isEnabled(); }, 300000))
			return nullptr;
		return qobject_cast<ElementsCollectionModel *>(main_view->model());
	}

	private slots:
		void closedBeforeFirstLoad()
		{
			ElementsCollectionWidget w;
			QETProject kept;
			QETProject closed;
			w.addProject(&kept);
			w.addProject(&closed);
			w.removeProject(&closed);

			ElementsCollectionModel *model = reloadAndWait(w);
			QVERIFY(model);
			QVERIFY(model->project().contains(&kept));
			QVERIFY2(!model->project().contains(&closed),
				"a project closed before the first load is still in the collection");
		}

		void closedWhileLoading()
		{
			ElementsCollectionWidget w;
			QETProject kept;
			QETProject closed;
			QVERIFY(reloadAndWait(w));
			w.addProject(&kept);
			w.addProject(&closed);

				// A second load starts; the project is closed before it ends.
			w.reload();
			QTreeView *main_view = loadingView(w);
			QVERIFY(main_view);
			w.removeProject(&closed);
			QTRY_VERIFY_WITH_TIMEOUT(main_view->isEnabled(), 300000);

			auto *model = qobject_cast<ElementsCollectionModel *>(main_view->model());
			QVERIFY(model);
			QVERIFY(model->project().contains(&kept));
			QVERIFY2(!model->project().contains(&closed),
				"a project closed during a load is still in the collection");
		}
};

QET_APP_TEST_MAIN(tst_collectionclosedproject)

#include "tst_collectionclosedproject.moc"

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
/*
	The element editor has the collections in a dock, hidden until it is
	shown: an element dragged from it onto the drawing pastes its parts,
	their middle near where it is dropped, moved by whole steps of 10 so
	its terminals stay on a folio's grid; double-clicked, the
	user then chooses where. Nothing is pasted from a macro, nor into a
	read-only element.
*/
#include "qetapptest.h"
#include "ElementsCollection/elementscollectionwidget.h"
#include "editor/elementscene.h"
#include "editor/elementview.h"
#include "editor/graphicspart/partterminal.h"
#include "editor/ui/qetelementeditor.h"

#include <QDockWidget>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>

#include <cmath>

class tst_elementeditorcollection : public QObject
{
	Q_OBJECT

	private:
		QTemporaryDir m_collection;
		QETElementEditor *m_editor = nullptr;

		QList<PartTerminal *> terminals() const
		{
			QList<PartTerminal *> list;
			for (QGraphicsItem *item : m_editor->elementScene()->items())
				if (auto *terminal = qgraphicsitem_cast<PartTerminal *>(item))
					list << terminal;
			return list;
		}

			/// Drop @p uri on the view at the scene point @p scene;
			/// tell whether the drag was accepted
		bool drop(const QPointF &scene, const QByteArray &uri = QByteArrayLiteral("custom://t.elmt"))
		{
			ElementView *view = m_editor->elementView();
			QMimeData mime;
			mime.setData(QStringLiteral("application/x-qet-element-uri"), uri);
			const QPoint pos = view->mapFromScene(scene);
			QDragEnterEvent enter(pos, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
			QApplication::sendEvent(view->viewport(), &enter);
			if (!enter.isAccepted())
				return false;
			QDropEvent drop_event(QPointF(pos), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
			QApplication::sendEvent(view->viewport(), &drop_event);
			return true;
		}

	private slots:
		void initTestCase()
		{
			QVERIFY(m_collection.isValid());
			QFile file(m_collection.filePath(QStringLiteral("t.elmt")));
			QVERIFY(file.open(QIODevice::WriteOnly));
				//A line from (0, -10) to (0, 10), a terminal at its top
			file.write(R"(<definition type="element" link_type="simple" width="20" height="30" hotspot_x="10" hotspot_y="15">
<uuid uuid="{6f1d3c2a-5b4e-4a9c-8d7f-1e2a3b4c5d6e}"/>
<names><name lang="en">t</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="-10" orientation="n"/></description></definition>)");
			file.close();
			QVERIFY(QFile::copy(m_collection.filePath(QStringLiteral("t.elmt")),
								m_collection.filePath(QStringLiteral("m.qetmak"))));
			QSettings().setValue(QStringLiteral("elements-collections/custom-collection-path"), m_collection.path());
			QETApp::resetCollectionsPath();

			m_editor = new QETElementEditor();
			m_editor->resize(1000, 700);
			QCoreApplication::processEvents();
		}

		void cleanupTestCase()
		{
				//As closeEvent() does, before the editor goes
			m_editor->elementScene()->reset();
			delete m_editor;
		}

			//Hidden until the user shows it, from the Paste from menu too
		void dockHiddenUntilShown()
		{
			auto *dock = m_editor->findChild<QDockWidget *>(QStringLiteral("element_editor_collections"));
			QVERIFY(dock);
			QVERIFY(!dock->isVisible());
			QVERIFY(dock->findChild<ElementsCollectionWidget *>());

			QAction *show = nullptr;
			for (QMenu *menu : m_editor->findChildren<QMenu *>())
				for (QAction *action : menu->actions())
					if (action->text() == QStringLiteral("The collections panel"))
						show = action;
			QVERIFY(show);
			show->trigger();
			QVERIFY(dock->isVisible());
			dock->hide();
		}

			//Dropped: the parts pasted, their middle near the drop, moved by whole steps of 10
		void dropPastes()
		{
			QVERIFY(terminals().isEmpty());
			QVERIFY(drop(QPointF(52, 33)));
			const QList<PartTerminal *> pasted = terminals();
			QCOMPARE(pasted.size(), 1);
				//On the grid of 10, as it was in its element, whatever the zoom
			const QPointF terminal = pasted.first()->scenePos();
			QCOMPARE(std::fmod(terminal.x(), 10.0), 0.0);
			QCOMPARE(std::fmod(terminal.y(), 10.0), 0.0);
				//The middle of the parts within half a step of the drop
			const QPointF middle = m_editor->elementScene()->elementSceneGeometricRect().center();
			QVERIFY2(qAbs(middle.x() - 52) <= 5 && qAbs(middle.y() - 33) <= 5,
					 qPrintable(QStringLiteral("middle %1, %2").arg(middle.x()).arg(middle.y())));
			QCOMPARE(m_editor->elementScene()->undoStack().count(), 1);
			m_editor->elementScene()->undoStack().undo();
			QVERIFY(terminals().isEmpty());
		}

			//Double-clicked in the dock: the user then chooses where
		void insertRequestedChoosesArea()
		{
			auto *widget = m_editor->findChild<ElementsCollectionWidget *>();
			QVERIFY(widget);
			emit widget->insertElementRequested(ElementsLocation(QStringLiteral("custom://t.elmt")));
			QCOMPARE(m_editor->elementScene()->behavior(), ElementScene::PasteArea);
			m_editor->elementScene()->setBehavior(ElementScene::Normal);
			QVERIFY(terminals().isEmpty());
		}

			//A macro has no parts to paste, whatever its file holds
		void macroNotPasted()
		{
			drop(QPointF(50, 30), QByteArrayLiteral("custom://m.qetmak"));
			QVERIFY(terminals().isEmpty());
			auto *widget = m_editor->findChild<ElementsCollectionWidget *>();
			emit widget->insertElementRequested(ElementsLocation(QStringLiteral("custom://m.qetmak")));
			QCOMPARE(m_editor->elementScene()->behavior(), ElementScene::Normal);
		}

			//Read-only: the drag is refused
		void readOnlyRefuses()
		{
			m_editor->setReadOnly(true);
			QVERIFY(!drop(QPointF(50, 30)));
			QVERIFY(terminals().isEmpty());
			m_editor->setReadOnly(false);
		}
};

QET_APP_TEST_MAIN(tst_elementeditorcollection)
#include "tst_elementeditorcollection.moc"

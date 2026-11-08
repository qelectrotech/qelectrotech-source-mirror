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
	The symbol editor drawing like the folio: the frame it writes lies on
	the grid around the origin, and "Set the insertion point" makes a
	terminal (or a grid point) the origin, as one undo step. Off, the
	editor writes the frame it always did.
*/
#include "qetapptest.h"
#include "editor/elementscene.h"
#include "editor/esevent/eseventsetorigin.h"
#include "editor/graphicspart/partterminal.h"
#include "editor/ui/qetelementeditor.h"

#include <QTemporaryDir>

class tst_editorlikefolio : public QObject
{
	Q_OBJECT

	private:
		QTemporaryDir m_dir;
		QETElementEditor *m_editor = nullptr;

			/// The socket as the editor saved it before: origin 10 below the
			/// first terminal, frame off the grid
		QString write() const
		{
			const QString path = m_dir.filePath(QStringLiteral("s.elmt"));
			QFile file(path);
			if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
				return QString();
			file.write(R"(<definition type="element" link_type="simple" width="70" height="40" hotspot_x="15" hotspot_y="20">
<uuid uuid="{6f1d3c2a-5b4e-4a9c-8d7f-1e2a3b4c5d61}"/>
<names><name lang="en">s</name></names>
<description>
<line x1="0" y1="-10" x2="0" y2="2" style="line-style:normal;line-weight:thin;filling:none;color:black"/>
<line x1="40" y1="-10" x2="40" y2="12" style="line-style:normal;line-weight:thin;filling:none;color:black"/>
<terminal x="0" y="-10" orientation="n" uuid="{b067b350-7625-5e33-b681-334340442d50}"/>
<terminal x="40" y="-10" orientation="n" uuid="{d96d436f-67b7-5d21-b192-344d0e4d520c}"/>
</description></definition>)");
			return path;
		}

		static QDomElement definition(ElementScene *scene)
		{
			return scene->toXml().documentElement();
		}

		QList<PartTerminal *> terminals() const
		{
			QList<PartTerminal *> list;
			for (QGraphicsItem *item : m_editor->elementScene()->items())
				if (auto *t = qgraphicsitem_cast<PartTerminal *>(item))
					list << t;
			return list;
		}

	private slots:
		void initTestCase()
		{
			QVERIFY(m_dir.isValid());
			m_editor = new QETElementEditor();
			m_editor->resize(1000, 700);
			QCoreApplication::processEvents();
		}

		void cleanupTestCase()
		{
			ElementScene::setLikeFolio(false);
			m_editor->elementScene()->reset();
			delete m_editor;
		}

		void init()
		{
			m_editor->elementScene()->reset();
			m_editor->fromFile(write());
		}

		void gridFrame()
		{
			QCOMPARE(ElementScene::gridFrame(QRectF(-5, -10, 50, 22)), QRect(-10, -10, 60, 30));
				//The origin is taken in even outside the drawing
			QCOMPARE(ElementScene::gridFrame(QRectF(5, 5, 10, 10)), QRect(0, 0, 20, 20));
		}

			//Off: the frame the editor always wrote
		void offKeepsOldFrame()
		{
			ElementScene::setLikeFolio(false);
			const QDomElement d = definition(m_editor->elementScene());
			QVERIFY(d.attribute(QStringLiteral("hotspot_y")).toInt() % 10 != 0
				|| d.attribute(QStringLiteral("hotspot_x")).toInt() % 10 != 0
				|| d.attribute(QStringLiteral("width")) != QStringLiteral("40"));
		}

			//On: on the grid around the origin
		void onFrameOnGrid()
		{
			ElementScene::setLikeFolio(true);
			const QDomElement d = definition(m_editor->elementScene());
			for (const char *key : {"width", "height", "hotspot_x", "hotspot_y"})
				QCOMPARE(d.attribute(QLatin1String(key)).toInt() % 10, 0);
		}

			//A click near the first terminal makes it the origin; undo
			//puts it back
		void setOriginOnTerminal()
		{
			ElementScene::setLikeFolio(true);
			ElementScene *scene = m_editor->elementScene();
			const QPointF target = ESEventSetOrigin::target(scene, QPointF(2, -9));
			QCOMPARE(target, QPointF(0, -10));
			QVERIFY(ESEventSetOrigin::moveOriginTo(scene, target));

			QList<QPointF> positions;
			for (PartTerminal *t : terminals())
				positions << t->scenePos();
			QVERIFY(positions.contains(QPointF(0, 0)));
			QVERIFY(positions.contains(QPointF(40, 0)));
			const QDomElement d = definition(scene);
			QCOMPARE(d.attribute(QStringLiteral("hotspot_y")), QStringLiteral("0"));
			QCOMPARE(d.attribute(QStringLiteral("height")), QStringLiteral("30"));

			scene->undoStack().undo();
			positions.clear();
			for (PartTerminal *t : terminals())
				positions << t->scenePos();
			QVERIFY(positions.contains(QPointF(0, -10)));
				//Already the origin: nothing to do
			QVERIFY(!ESEventSetOrigin::moveOriginTo(scene, QPointF()));
		}

			//Away from any terminal: the nearest folio grid point
		void setOriginOnGrid()
		{
			QCOMPARE(ESEventSetOrigin::target(m_editor->elementScene(), QPointF(23, 14)), QPointF(20, 10));
		}
};

QET_APP_TEST_MAIN(tst_editorlikefolio)
#include "tst_editorlikefolio.moc"

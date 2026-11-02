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
#include "qetapptest.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "projectview.h"

/**
	Dragging a symbol's text with the mouse, on a real folio view: a plain
	drag moves the symbol and Shift+drag the text alone, or the other way
	round with Preferences > "Drag symbol texts without holding Shift"
	(issue #1576). With it on, a plain click selects the text, so Space
	turns the text rather than its symbol.
*/
class tst_dragtextswithoutshift : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	DiagramView *m_view = nullptr;
	DynamicElementTextItem *m_text = nullptr;
	Element *m_element = nullptr;

		/// Drag m_text by three grid steps to the right, through the view.
	void dragText(Qt::KeyboardModifiers modifiers)
	{
		m_view->diagram()->clearSelection();
		m_view->centerOn(m_text);
		QTest::qWait(50);
		const QPoint from = m_view->mapFromScene(m_text->mapToScene(m_text->boundingRect().center()));
		const QPoint step = m_view->mapFromScene(QPointF(10, 0)) - m_view->mapFromScene(QPointF(0, 0));
		QTest::mousePress(m_view->viewport(), Qt::LeftButton, modifiers, from);
		QTest::mouseMove(m_view->viewport(), from + step);
		QTest::mouseMove(m_view->viewport(), from + step * 3);
		QTest::mouseRelease(m_view->viewport(), Qt::LeftButton, modifiers, from + step * 3);
		QTest::qWait(50);
	}

		/// What a drag with @p modifiers moved: "text", "element", or "".
	QString whatMoved(Qt::KeyboardModifiers modifiers)
	{
		const QPointF text_pos = m_text->pos();
		const QPointF element_pos = m_element->pos();
		dragText(modifiers);
		const bool text_moved = m_text->pos() != text_pos;
		const bool element_moved = m_element->pos() != element_pos;
		if (text_moved && !element_moved)
			return QStringLiteral("text");
		if (element_moved && !text_moved)
			return QStringLiteral("element");
		return QString();
	}

	private slots:
		void initTestCase()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->resize(1400, 1000);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));
			const QString copy = m_dir.filePath("p.qet");
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), copy));
			QVERIFY(editor->openAndAddProject(copy, false));
			m_view = editor->currentProjectView()->currentDiagram();
			QVERIFY(m_view);
			m_view->zoomReset();

				//A text straight on its symbol, not in a group of texts
			for (QGraphicsItem *item : m_view->diagram()->items())
				if (auto text = qgraphicsitem_cast<DynamicElementTextItem *>(item))
					if (!m_text && text->parentElement() && text->parentItem() == text->parentElement()
							&& text->isVisible() && !text->toPlainText().isEmpty())
						m_text = text;
			QVERIFY(m_text);
			m_element = m_text->parentElement();
		}

		void plainDragMovesTheSymbolByDefault()
		{
			DynamicElementTextItem::setDragMovesTextAlone(false);
			QCOMPARE(whatMoved(Qt::NoModifier), QStringLiteral("element"));
			QCOMPARE(whatMoved(Qt::ShiftModifier), QStringLiteral("text"));
		}

		void settingSwapsTheTwo()
		{
			DynamicElementTextItem::setDragMovesTextAlone(true);
			QCOMPARE(QSettings().value("diagrameditor/drag_texts_without_shift").toBool(), true);
			QCOMPARE(whatMoved(Qt::NoModifier), QStringLiteral("text"));
			QCOMPARE(whatMoved(Qt::ShiftModifier), QStringLiteral("element"));
			DynamicElementTextItem::setDragMovesTextAlone(false);
		}

			/// A plain click selects the text alone, and Space turns it
		void spaceTurnsTheClickedText()
		{
			DynamicElementTextItem::setDragMovesTextAlone(true);
			m_view->diagram()->clearSelection();
			m_view->centerOn(m_text);
			QTest::qWait(50);
			const QPoint at = m_view->mapFromScene(m_text->mapToScene(m_text->boundingRect().center()));
			QTest::mouseClick(m_view->viewport(), Qt::LeftButton, Qt::NoModifier, at);
			QTest::qWait(50);
			QVERIFY(m_text->isSelected());
			QVERIFY(!m_element->isSelected());

			const qreal text_rotation = m_text->rotation();
			const qreal element_rotation = m_element->rotation();
			QAction *rotate = QetAppTest::action(QStringLiteral("rotate_selection"));
			QVERIFY(rotate);
			rotate->trigger();
			QTRY_VERIFY(m_text->rotation() != text_rotation);
			QCOMPARE(m_element->rotation(), element_rotation);
			DynamicElementTextItem::setDragMovesTextAlone(false);
		}
};

QET_APP_TEST_MAIN(tst_dragtextswithoutshift)

#include "tst_dragtextswithoutshift.moc"

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
#include "editor/elementscene.h"
#include "editor/graphicspart/partdynamictextfield.h"
#include "editor/graphicspart/parttext.h"
#include "editor/ui/dynamictextfieldeditor.h"
#include "editor/ui/qetelementeditor.h"
#include "editor/ui/texteditor.h"

#include <QScopeGuard>

#ifdef BUILD_WITHOUT_KF
#	include "kcolorbutton.h"
#else
#	include <KColorButton>
#endif

/*
	The symbol editor's editors of a text and of a dynamic text field
	have a colour button, with KDE Frameworks and without them: built
	without them, QElectroTech has a KColorButton of its own
	(sources/ui/nokde). A colour chosen there changes the selected
	texts through the undo stack.
*/
class tst_textcolourbutton : public QObject
{
	Q_OBJECT

	private slots:
		void textEditorChangesTheColour()
		{
			QETElementEditor window;
			ElementScene *scene = window.elementScene();
				//As closing the window does: a scene destroyed with its
				//undo history still in it updates the half-destroyed window
			const auto reset = qScopeGuard([scene] { scene->reset(); });
			auto *text = new PartText(&window);
			text->setPlainText(QStringLiteral("Text"));
			text->setDefaultTextColor(Qt::red);
			scene->addItems({text});

			TextEditor editor(&window);
			QVERIFY(editor.setParts({text}));
			auto *button = editor.findChild<KColorButton *>();
			QVERIFY(button);
			QCOMPARE(button->color(), QColor(Qt::red));

			const int commands = scene->undoStack().count();
			emit button->changed(QColor(Qt::blue));
			QCOMPARE(text->defaultTextColor(), QColor(Qt::blue));
			QCOMPARE(scene->undoStack().count(), commands + 1);

			scene->undoStack().undo();
			QCOMPARE(text->defaultTextColor(), QColor(Qt::red));
		}

		void dynamicTextFieldEditorChangesTheColour()
		{
			QETElementEditor window;
			ElementScene *scene = window.elementScene();
			const auto reset = qScopeGuard([scene] { scene->reset(); });
			auto *field = new PartDynamicTextField(&window);
			field->setText(QStringLiteral("Field"));
			field->setColor(Qt::red);
			scene->addItems({field});

			DynamicTextFieldEditor editor(&window);
			QVERIFY(editor.setParts({field}));
			auto *button = editor.findChild<KColorButton *>();
			QVERIFY(button);
			QCOMPARE(button->color(), QColor(Qt::red));

			const int commands = scene->undoStack().count();
			emit button->changed(QColor(Qt::blue));
			QCOMPARE(field->color(), QColor(Qt::blue));
			QCOMPARE(scene->undoStack().count(), commands + 1);

			scene->undoStack().undo();
			QCOMPARE(field->color(), QColor(Qt::red));
		}
};

QET_APP_TEST_MAIN(tst_textcolourbutton)

#include "tst_textcolourbutton.moc"

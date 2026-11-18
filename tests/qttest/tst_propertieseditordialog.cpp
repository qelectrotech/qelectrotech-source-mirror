// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "PropertiesEditor/propertieseditordialog.h"
#include "PropertiesEditor/propertieseditorwidget.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QApplication>
#include <QDomDocument>
#include <QSettings>
#include <QTimer>
#include <QtTest>

/*
	PropertiesEditorDialog remembers its size per kind of editor. A caller
	can give it a key of its own (a PLC master's element dialog opens much
	wider than the others), so that size is not the one every other
	dialog of the same editor reopens at.

	The dialog can be moved off the drawing it edits, on every system
	(bugtracker #275).
*/
class tst_propertieseditordialog : public QObject
{
	Q_OBJECT

	class Editor : public PropertiesEditorWidget
	{
	  public:
		QString title() const override { return QStringLiteral("Test editor"); }
	};

	static QString saved(const QString &key)
	{
		return QSettings().contains(QStringLiteral("dialoggeometry/") + key)
				? QStringLiteral("saved") : QStringLiteral("none");
	}

		/// On macOS, Qt shows a window modal dialog that has a parent, and
		/// any Qt::Sheet, as a sheet fixed over its window
		/// (QCocoaWindow::setVisible()). The parent is not checked, so the
		/// result does not depend on which window is active.
	static bool movable(const QDialog *dialog)
	{
		return dialog->windowModality() == Qt::ApplicationModal
				&& dialog->windowType() != Qt::Sheet;
	}

	bool m_dialog_shown = false;
	bool m_dialog_movable = false;

  private slots:
	void dialogCanBeMoved()
	{
		QWidget parent;
		PropertiesEditorDialog dialog(new Editor, &parent);
		QVERIFY(movable(&dialog));
	}

		//Double-clicking a symbol opens its dialog through
		//Element::editProperty(), which set the dialog window modal again
	void symbolDialogCanBeMoved()
	{
		Diagram *diagram = QetAppTest::diagram();
		QVERIFY(diagram);
		QDomDocument definition;
		definition.setContent(QStringLiteral(
			R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">box</name></names>
<description><line x1="0" y1="-5" x2="0" y2="5" style="line-style:normal;line-weight:normal;filling:none;color:black"/></description></definition>)"));
		QVERIFY(diagram->project()->embeddedElementCollection()->addElementDefinition(
			QStringLiteral("import"), QStringLiteral("box"), definition.documentElement()));
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/box.elmt"), diagram->project()),
			nullptr, &state);
		QVERIFY(!state && element);
		diagram->addItem(element);

		QTimer::singleShot(0, this, [this]() {
			auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
			if (!dialog)
				return;
			m_dialog_shown = true;
			m_dialog_movable = movable(dialog);
			dialog->reject();
		});
		element->editProperty();
		QVERIFY(m_dialog_shown);
		QVERIFY(m_dialog_movable);
	}

	void ownKeyKeepsItsSizeApart()
	{
		const QString shared = QString::fromLatin1(Editor().metaObject()->className());
		QSettings().remove(QStringLiteral("dialoggeometry"));
		{
			PropertiesEditorDialog wide(new Editor, nullptr, QStringLiteral("TestEditorWide"));
			wide.resize(1500, 500);
			wide.done(QDialog::Rejected);
		}
		QCOMPARE(saved(QStringLiteral("TestEditorWide")), QStringLiteral("saved"));
		QCOMPARE(saved(shared), QStringLiteral("none"));

		{
			PropertiesEditorDialog usual(new Editor);
			usual.resize(700, 500);
			usual.done(QDialog::Rejected);
		}
		QCOMPARE(saved(shared), QStringLiteral("saved"));

			//The usual dialog reopens at its own size, not at the wide one
		PropertiesEditorDialog again(new Editor);
		QCOMPARE(again.width(), 700);
	}
};

QET_APP_TEST_MAIN(tst_propertieseditordialog)
#include "tst_propertieseditordialog.moc"

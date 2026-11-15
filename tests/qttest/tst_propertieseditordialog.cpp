// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "PropertiesEditor/propertieseditordialog.h"
#include "PropertiesEditor/propertieseditorwidget.h"

#include <QSettings>
#include <QtTest>

/*
	PropertiesEditorDialog remembers its size per kind of editor. A caller
	can give it a key of its own (a PLC master's element dialog opens much
	wider than the others), so that size is not the one every other
	dialog of the same editor reopens at.
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

  private slots:
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

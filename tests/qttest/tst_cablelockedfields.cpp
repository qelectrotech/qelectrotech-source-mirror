// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "conductorproperties.h"
#include "ui/conductorpropertieswidget.h"

#include <QLabel>
#include <QLineEdit>

	// A core of a cable: section, colour and type come from the cable,
	// so the dialog locks them and shows a small sign saying why --
	// an edit on one side of a terminal must not write over what the
	// cable says.
class tst_cablelockedfields : public QObject
{
	Q_OBJECT

	static ConductorProperties coreOfACable()
	{
		ConductorProperties properties;
		properties.m_wire_section = QStringLiteral("1.5");
		properties.m_wire_color = QStringLiteral("BN");
		properties.m_conductor_type = QStringLiteral("H05V-K");
		properties.m_cable_uuid = QUuid::createUuid();
		return properties;
	}

	static bool locked(const ConductorPropertiesWidget &widget, const char *name)
	{
		return !widget.findChild<QLineEdit *>(QLatin1String(name))->isEnabled();
	}

	static bool signShown(const ConductorPropertiesWidget &widget, const char *name)
	{
		return !widget.findChild<QLabel *>(QLatin1String(name))->isHidden();
	}

	private slots:
		void aCableCoreLocksItsThreeFields()
		{
			ConductorPropertiesWidget widget(coreOfACable());
			widget.setProperties(coreOfACable());
			QVERIFY(locked(widget, "m_wire_section_le"));
			QVERIFY(locked(widget, "m_wire_color_le"));
			QVERIFY(locked(widget, "m_conductor_type_le"));

			QVERIFY(signShown(widget, "m_wire_section_locked_info"));
			QVERIFY(signShown(widget, "m_wire_color_locked_info"));
			QVERIFY(signShown(widget, "m_conductor_type_locked_info"));
			QVERIFY2(!widget.findChild<QLabel *>("m_wire_section_locked_info")
							  ->toolTip().isEmpty(),
					 "the sign says why the field is locked");
		}

		void anOrdinaryWireKeepsItsFields()
		{
			ConductorProperties properties;
			properties.m_wire_section = QStringLiteral("1.5");
			ConductorPropertiesWidget widget(properties);
			widget.setProperties(properties);
			QVERIFY(!locked(widget, "m_wire_section_le"));
			QVERIFY(!locked(widget, "m_wire_color_le"));
			QVERIFY(!locked(widget, "m_conductor_type_le"));
			QVERIFY(!signShown(widget, "m_wire_section_locked_info"));
			QVERIFY(!signShown(widget, "m_wire_color_locked_info"));
			QVERIFY(!signShown(widget, "m_conductor_type_locked_info"));
		}

		void theLockFollowsThePropertiesShown()
		{
				//The same widget showing other properties: what was
				//locked opens again, what was open locks
			ConductorPropertiesWidget widget(coreOfACable());
			widget.setProperties(coreOfACable());
			QVERIFY(locked(widget, "m_wire_section_le"));
			widget.setProperties(ConductorProperties());
			QVERIFY(!locked(widget, "m_wire_section_le"));
			QVERIFY(!signShown(widget, "m_wire_section_locked_info"));
			widget.setProperties(coreOfACable());
			QVERIFY(locked(widget, "m_wire_section_le"));
			QVERIFY(signShown(widget, "m_wire_section_locked_info"));
		}
};

QET_APP_TEST_MAIN(tst_cablelockedfields)
#include "tst_cablelockedfields.moc"

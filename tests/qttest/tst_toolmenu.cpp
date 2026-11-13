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
#include <QtTest>
#include <QActionGroup>

#include "ui/toolmenu.h"

/**
	A menu of drawing tools draws no radio button in front of its
	checkable, exclusive actions; a plain menu still does.
*/
class tst_toolmenu : public QObject
{
	Q_OBJECT

		//Gives the test the style option the menu hands to the style
	template <class Menu>
	class Probe : public Menu
	{
		public:
			using Menu::Menu;
			QStyleOptionMenuItem option(QAction *action) const
			{
				QStyleOptionMenuItem opt;
				this->initStyleOption(&opt, action);
				return opt;
			}
	};

	private slots:
		void noRadioButton()
		{
			QActionGroup group(nullptr);
			group.setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
			QAction *line = group.addAction(QStringLiteral("line"));
			QAction *box  = group.addAction(QStringLiteral("box"));
			line->setCheckable(true);
			box->setCheckable(true);
			box->setChecked(true);

			Probe<QMenu> plain;
			plain.addActions(group.actions());
			QCOMPARE(plain.option(line).checkType, QStyleOptionMenuItem::Exclusive);

			Probe<ToolMenu> tools;
			tools.addActions(group.actions());
			for (QAction *action : group.actions()) {
				const QStyleOptionMenuItem opt = tools.option(action);
				QCOMPARE(opt.checkType, QStyleOptionMenuItem::NotCheckable);
				QVERIFY(!opt.checked);
			}
				//The action itself stays checked, for its toolbar button
			QVERIFY(box->isChecked());
		}
};

QTEST_MAIN(tst_toolmenu)
#include "tst_toolmenu.moc"

// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include <QDockWidget>
#include <QProxyStyle>
#include <QSpinBox>

namespace {

// Spin boxes as wide as the Windows 11 style draws them: its up and down
// buttons sit side by side, so a spin box needs far more room than with
// the buttons stacked.
class WideSpinBoxStyle : public QProxyStyle
{
	public:
		QSize sizeFromContents(ContentsType type, const QStyleOption *option,
				       const QSize &size, const QWidget *widget) const override
		{
			QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
			if (type == CT_SpinBox)
				s.rwidth() += 40;
			return s;
		}
};

}

// The Increment spin boxes of the Auto numbering panel get the width their
// style asks for (#1566). A fixed 55 px maximum cut off the number on
// Windows.
class tst_autonumincrementwidth : public QObject
{
	Q_OBJECT

	private slots:
		void incrementFitsItsStyle()
		{
			QApplication::setStyle(new WideSpinBoxStyle);

			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->resize(1600, 1000);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));

			const QStringList names{"m_conductor_increase_sb",
						"m_element_increase_sb",
						"m_folio_increase_sb"};
			for (const QString &name : names) {
				auto spin_box = editor->findChild<QSpinBox *>(name);
				QVERIFY2(spin_box, qPrintable(name));
				QVERIFY2(spin_box->isVisible(), qPrintable(name));
				QVERIFY2(spin_box->width() >= spin_box->minimumSizeHint().width(),
					 qPrintable(QString("%1 is %2 px, its style needs %3 px")
						    .arg(name)
						    .arg(spin_box->width())
						    .arg(spin_box->minimumSizeHint().width())));
			}
		}
};

QET_APP_TEST_MAIN(tst_autonumincrementwidth)

#include "tst_autonumincrementwidth.moc"

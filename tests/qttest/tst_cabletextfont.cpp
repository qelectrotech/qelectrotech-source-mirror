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
#include "ui/configpage/generalconfigurationpage.h"
#include "utils/qetutils.h"

#include <QPushButton>
#include <QSettings>
#include <QTest>
#include <QToolButton>
#include <QtTest>

/*
	QETApp::cableTextsFont(key) -- the default font of one text of a
	cable, each of the five having one of its own on the "Texte" page
	since they were set apart.

	Three things are pinned here, and all three are about not moving
	what an existing installation already sees:

	* With nothing chosen, every one of the five follows the font all
	  of them shared before, which itself is the font the drawing
	  writes its own texts with, a shade smaller. Opening the
	  preferences must change nothing until he picks something.
	* The single setting an older version stored under
	  "cable-management/cable-text-font" still reaches all five, so
	  what he picked before this was split apart goes on being drawn
	  the way he chose it.
	* A font set for one text reaches that text alone. The others go
	  on following the shared one, and taking the choice back puts
	  just that text back to it -- which is what the reset button
	  beside each row in the preferences does.

	Names outside those five (the colours of the cores, the reference
	to another folio) are left alone: the preferences offer no
	default of their own for them, so there is nothing to set apart
	and nothing to take back.
*/
class tst_cabletextfont : public QObject
{
	Q_OBJECT

		///The five texts of a cable, the way the settings and the
		///cables themselves spell them
	static QStringList cableTexts()
	{
		return {QStringLiteral("designation"), QStringLiteral("type"),
				QStringLiteral("installation"), QStringLiteral("location"),
				QStringLiteral("length")};
	}

		///A font told apart from another by what he reads off it
		///on the sheet: family, size and style
	static bool sameFont(const QFont &a, const QFont &b)
	{
		return a.family() == b.family()
				&& qFuzzyCompare(a.pointSizeF(), b.pointSizeF())
				&& a.bold() == b.bold()
				&& a.italic() == b.italic();
	}

		///A font of his own for that text, well away from the rest
	static QFont ownFont(qreal size)
	{
		QFont font(QStringLiteral("DejaVu Sans"));
		font.setPointSizeF(size);
		font.setBold(true);
		return font;
	}

	private slots:
		void initTestCase()
		{
			// A scope of this test's own: whatever this writes must not
			// land in the configuration of the account running it.
			QCoreApplication::setOrganizationName(
						QStringLiteral("QElectroTech-tst_cabletextfont"));
			QCoreApplication::setApplicationName(
						QStringLiteral("tst_cabletextfont"));
		}

		void init()
		{
			QSettings settings;
			settings.remove(QStringLiteral("cable-management"));
			settings.sync();
		}

		void nothingChosenSharesOneFont()
		{
				//The font every text falls back to: what the drawing
				//writes its own texts with, a shade smaller
			QFont shared = QETApp::diagramTextsItemFont();
			shared.setPointSizeF(shared.pointSizeF() * 0.9);
			QVERIFY(sameFont(QETApp::cableTextsFont(), shared));

			for (const QString &key : cableTexts()) {
				QVERIFY2(sameFont(QETApp::cableTextsFont(key), shared),
						 qPrintable(key));
			}
		}

		void theOldOneSettingStillReachesAllFive()
		{
				//What an installation which picked a font before the
				//five were set apart holds, and must go on holding
			const QFont chosen = ownFont(14.0);
			QSettings settings;
			settings.setValue(QStringLiteral("cable-management/cable-text-font"),
							  QETUtils::fontToString(chosen));
			settings.sync();

			QVERIFY(sameFont(QETApp::cableTextsFont(), chosen));
			for (const QString &key : cableTexts()) {
				QVERIFY2(sameFont(QETApp::cableTextsFont(key), chosen),
						 qPrintable(key));
			}
		}

		void oneTextSetApartLeavesTheOthersAlone()
		{
			const QFont shared = ownFont(11.0);
			const QFont apart = ownFont(17.0);
			QSettings settings;
			settings.setValue(QStringLiteral("cable-management/cable-text-font"),
							  QETUtils::fontToString(shared));
			settings.sync();
			QETApp::setCableTextFont(QStringLiteral("length"), apart);

			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("length")),
							 apart));
				//The four others go on following the shared one, and
				//so does the shared one itself
			QVERIFY(sameFont(QETApp::cableTextsFont(), shared));
			for (const QString &key : {QStringLiteral("designation"),
									   QStringLiteral("type"),
									   QStringLiteral("installation"),
									   QStringLiteral("location")}) {
				QVERIFY2(sameFont(QETApp::cableTextsFont(key), shared),
						 qPrintable(key));
			}
		}

		void hasSaysWhatThereIsToTakeBack()
		{
				//Nothing of his own yet: the reset beside that row in
				//the preferences has nothing to give back
			QVERIFY(!QETApp::hasCableTextFont(QStringLiteral("designation")));

			QETApp::setCableTextFont(QStringLiteral("designation"),
									 ownFont(13.0));
			QVERIFY(QETApp::hasCableTextFont(QStringLiteral("designation")));
				//Only that row: the reset of another one stays off
			QVERIFY(!QETApp::hasCableTextFont(QStringLiteral("type")));

			QETApp::clearCableTextFont(QStringLiteral("designation"));
			QVERIFY(!QETApp::hasCableTextFont(QStringLiteral("designation")));
		}

		void takingTheChoiceBackKeepsTheSharedOne()
		{
			const QFont shared = ownFont(11.0);
			QSettings settings;
			settings.setValue(QStringLiteral("cable-management/cable-text-font"),
							  QETUtils::fontToString(shared));
			settings.sync();

			QETApp::setCableTextFont(QStringLiteral("type"), ownFont(19.0));
			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("type")),
							 ownFont(19.0)));

				//Back to what the other four are written with, and the
				//font all of them shared before is still there
			QETApp::clearCableTextFont(QStringLiteral("type"));
			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("type")),
							 shared));
			QVERIFY(sameFont(QETApp::cableTextsFont(), shared));
		}

		void clearingTakesAwayOnlyWhatItNames()
		{
			QETApp::setCableTextFont(QStringLiteral("designation"),
									 ownFont(13.0));
			QETApp::setCableTextFont(QStringLiteral("location"),
									 ownFont(15.0));

			QETApp::clearCableTextFont(QStringLiteral("designation"));
			QVERIFY(!QETApp::hasCableTextFont(QStringLiteral("designation")));
			QVERIFY(QETApp::hasCableTextFont(QStringLiteral("location")));
			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("location")),
							 ownFont(15.0)));
		}

		void namesOutsideTheFiveAreLeftAlone()
		{
			const QFont shared = ownFont(11.0);
			QSettings settings;
			settings.setValue(QStringLiteral("cable-management/cable-text-font"),
							  QETUtils::fontToString(shared));
			settings.sync();

				//The colours of the cores and the reference to another
				//folio follow the shared font: no default of their own
				//to set apart
			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("cores")),
							 shared));
			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("xref")),
							 shared));
			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("nothing")),
							 shared));

				//And there is nothing to write or take back for them,
				//whatever he asks for
			QETApp::setCableTextFont(QStringLiteral("cores"), ownFont(22.0));
			QETApp::setCableTextFont(QStringLiteral("bogus"), ownFont(22.0));
			QVERIFY(!QETApp::hasCableTextFont(QStringLiteral("cores")));
			QVERIFY(!QETApp::hasCableTextFont(QStringLiteral("bogus")));
			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("cores")),
							 shared));
		}

		void thereButHoldingNoFontIsPassedBy()
		{
				//A setting which is there and holds no font at all --
				//an empty value, the way an interrupted write or a hand
				//edited file leaves it: the text follows the shared one
				//rather than being written with nothing
			QSettings settings;
			settings.setValue(QStringLiteral("cable-management/cable-text-font"),
							  QString());
			settings.setValue(
						QStringLiteral("cable-management/cable-text-font-length"),
						QString());
			settings.sync();

			QFont shared = QETApp::diagramTextsItemFont();
			shared.setPointSizeF(shared.pointSizeF() * 0.9);
			QVERIFY(sameFont(QETApp::cableTextsFont(), shared));
			QVERIFY(sameFont(QETApp::cableTextsFont(QStringLiteral("length")),
							 shared));
				//It is there to be taken back, even so
			QVERIFY(QETApp::hasCableTextFont(QStringLiteral("length")));
		}

		// The "Texte" page offers one row per text, each with its own
		// font button and a reset which is offered only while there is
		// a choice of his own to take back.
		void thePageOffersARowForEachText()
		{
			QETApp::setCableTextFont(QStringLiteral("location"),
									 ownFont(15.0));

			GeneralConfigurationPage page;
			const struct {
				QString key;
				QString font_button;
				QString reset_button;
			} rows[] = {
				{QStringLiteral("designation"),
				 QStringLiteral("m_cable_designation_font_pb"),
				 QStringLiteral("m_cable_designation_reset_tb")},
				{QStringLiteral("type"),
				 QStringLiteral("m_cable_type_font_pb"),
				 QStringLiteral("m_cable_type_reset_tb")},
				{QStringLiteral("installation"),
				 QStringLiteral("m_cable_installation_font_pb"),
				 QStringLiteral("m_cable_installation_reset_tb")},
				{QStringLiteral("location"),
				 QStringLiteral("m_cable_location_font_pb"),
				 QStringLiteral("m_cable_location_reset_tb")},
				{QStringLiteral("length"),
				 QStringLiteral("m_cable_length_font_pb"),
				 QStringLiteral("m_cable_length_reset_tb")},
			};

			for (const auto &row : rows)
			{
				auto *font_button = page.findChild<QPushButton *>(row.font_button);
				auto *reset_button = page.findChild<QToolButton *>(row.reset_button);
				QVERIFY2(font_button, qPrintable(row.font_button));
				QVERIFY2(reset_button, qPrintable(row.reset_button));

					//The button says the font that text really is
					//written with: his own where he picked one, the
					//shared one where he did not
				const QFont expected = QETApp::cableTextsFont(row.key);
				QVERIFY2(font_button->text().contains(expected.family()),
						 qPrintable(row.key));
				QCOMPARE(reset_button->isEnabled(),
						 QETApp::hasCableTextFont(row.key));
			}

				//Only the row he set apart offers something to undo
			QVERIFY(page.findChild<QToolButton *>(
						QStringLiteral("m_cable_location_reset_tb"))->isEnabled());
			QVERIFY(!page.findChild<QToolButton *>(
						 QStringLiteral("m_cable_length_reset_tb"))->isEnabled());
				//Taking it back puts that row back to the shared font
				//and takes the offer away again
			auto *reset = page.findChild<QToolButton *>(
						QStringLiteral("m_cable_location_reset_tb"));
			QTest::mouseClick(reset, Qt::LeftButton);
			QVERIFY(!QETApp::hasCableTextFont(QStringLiteral("location")));
			QVERIFY(!reset->isEnabled());
		}
};

QET_APP_TEST_MAIN(tst_cabletextfont)
#include "tst_cabletextfont.moc"

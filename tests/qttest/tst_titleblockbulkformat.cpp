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
#include "titleblock/titleblockbulkformat.h"
#include "titleblockcell.h"
#include "titleblocktemplate.h"

#include <QDomDocument>
#include <QImage>
#include <QPainter>
#include <QUndoStack>

/**
	Formatting the text of several title block cells at once: only the
	checked properties change, the change is one undo step, it is saved
	with the cell and drawn, and a template without it loads and saves as
	before.
*/
class tst_titleblockbulkformat : public QObject
{
	Q_OBJECT

	static const char *templateXml()
	{
		return "<titleblocktemplate name=\"t\">"
		       "<information/><logos/>"
		       "<grid cols=\"100;100;100;\" rows=\"30;\">"
		       "<field row=\"0\" col=\"0\" name=\"x\" fontsize=\"9\""
		       " displaylabel=\"true\" align=\"left\" valign=\"center\">"
		       "<value><translation lang=\"en\">Value</translation></value>"
		       "<label><translation lang=\"en\">Label</translation></label>"
		       "</field>"
		       "<field row=\"0\" col=\"1\" name=\"y\" fontsize=\"9\""
		       " displaylabel=\"false\" align=\"left\" valign=\"center\">"
		       "<value><translation lang=\"en\">Other</translation></value>"
		       "<label/></field>"
		       "<empty row=\"0\" col=\"2\" name=\"z\"/>"
		       "</grid></titleblocktemplate>";
	}

	static bool load(TitleBlockTemplate &tpl, const char *xml = nullptr)
	{
		QDomDocument doc;
		if (!doc.setContent(QByteArray(xml ? xml : templateXml()))) return false;
		return tpl.loadFromXmlElement(doc.documentElement());
	}

	static int lastInkRow(const QImage &image)
	{
		// inside the border of the first cell, 60 by 30
		for (int y = 27; y >= 2; --y)
			for (int x = 2; x < 58; ++x)
				if (image.pixelColor(x, y) != QColor(Qt::white)) return y;
		return -1;
	}

	static QImage draw(const TitleBlockTemplate &tpl)
	{
		QImage image(300, 30, QImage::Format_ARGB32);
		image.fill(Qt::white);
		QPainter painter(&image);
		tpl.render(painter, DiagramContext(), 300);
		painter.end();
		return image;
	}

private slots:
	void onlyCheckedPropertiesChange()
	{
		TitleBlockBulkFormat format;
		format.properties = TitleBlockBulkFormat::Family;
		format.font = QFont(QStringLiteral("Courier"), 20, QFont::Bold, true);

		QFont start(QStringLiteral("Arial"), 7);
		start.setItalic(true);
		const QFont result = [&] {
			QFont f; f.fromString(format.mergedFont(start.toString(), QFont()));
			return f;
		}();
		QCOMPARE(result.family(), QStringLiteral("Courier"));
		QCOMPARE(result.pointSizeF(), 7.0);
		QVERIFY(result.italic());
		QVERIFY(!result.bold());

		// nothing font related checked: the description is left alone
		format.properties = TitleBlockBulkFormat::Color;
		QCOMPARE(format.mergedFont(QStringLiteral("x"), QFont()),
			 QStringLiteral("x"));
	}

	void colourAndOpacityKeepEachOther()
	{
		TitleBlockBulkFormat format;
		format.color = Qt::red;
		format.opacity = 40;

		QColor semi_blue(Qt::blue);
		semi_blue.setAlpha(100);

		format.properties = TitleBlockBulkFormat::Color;
		QColor c = format.mergedColor(semi_blue);
		QCOMPARE(c.red(), 255);
		QCOMPARE(c.alpha(), 100);

		format.properties = TitleBlockBulkFormat::Opacity;
		c = format.mergedColor(semi_blue);
		QCOMPARE(c.blue(), 255);
		QCOMPARE(c.alpha(), qRound(255 * 0.4));

		format.properties = TitleBlockBulkFormat::Family;
		QVERIFY(!format.mergedColor(QColor()).isValid());
	}

	void oneUndoStepAndEmptyCellsSkipped()
	{
		TitleBlockTemplate tpl;
		QVERIFY(load(tpl));
		TitleBlockCell *a = tpl.cell(0, 0);
		TitleBlockCell *b = tpl.cell(0, 1);
		TitleBlockCell *empty = tpl.cell(0, 2);
		QCOMPARE(empty->cell_type, TitleBlockCell::EmptyCell);
		QVERIFY(!a->hasTextStyle());

		TitleBlockBulkFormat format;
		format.properties = TitleBlockBulkFormat::Family
				  | TitleBlockBulkFormat::Color
				  | TitleBlockBulkFormat::Alignment;
		format.font = QFont(QStringLiteral("Courier"));
		format.color = Qt::red;
		format.alignment = Qt::AlignRight;
		format.part = TitleBlockBulkFormat::ValueOnly;

		QUndoStack stack;
		QUndoCommand *command = format.createCommand({a, b, empty, a});
		QVERIFY(command);
		stack.push(command);
		QCOMPARE(stack.count(), 1);

		QVERIFY(a->value_font.contains(QStringLiteral("Courier")));
		QCOMPARE(a->value_color.red(), 255);
		QVERIFY(a->label_font.isEmpty());   // value only
		QVERIFY(!a->label_color.isValid());
		QVERIFY(a->alignment & Qt::AlignRight);
		QVERIFY(a->alignment & Qt::AlignVCenter);   // vertical part kept
		QVERIFY(!empty->hasTextStyle());

		stack.undo();
		QVERIFY(!a->hasTextStyle());
		QVERIFY(!b->hasTextStyle());
		QVERIFY(a->alignment & Qt::AlignLeft);
		stack.redo();
		QVERIFY(a->hasTextStyle() && b->hasTextStyle());
	}

	void nothingCheckedNothingDone()
	{
		TitleBlockTemplate tpl;
		QVERIFY(load(tpl));
		TitleBlockBulkFormat format;
		QVERIFY(!format.createCommand({tpl.cell(0, 0)}));
		format.properties = TitleBlockBulkFormat::Bold;
		QVERIFY(!format.createCommand({tpl.cell(0, 2)}));   // only an empty cell
	}

	void onlyChangedValuesAreWritten()
	{
		TitleBlockTemplate tpl;
		QVERIFY(load(tpl));
		TitleBlockBulkFormat format;
		format.properties = TitleBlockBulkFormat::Alignment;
		format.alignment = Qt::AlignLeft;   // the cells are left aligned already
		QVERIFY(!format.createCommand({tpl.cell(0, 0), tpl.cell(0, 1)}));

		// a change that touches one cell only gives that cell an undo step
		format.alignment = Qt::AlignRight;
		tpl.cell(0, 1)->alignment = Qt::AlignRight | Qt::AlignVCenter;
		QUndoCommand *command = format.createCommand(
			{tpl.cell(0, 0), tpl.cell(0, 1)});
		QVERIFY(command);
		QCOMPARE(command->childCount(), 1);
		delete command;
	}

	void pixelSizedFontKeepsItsSize()
	{
		TitleBlockBulkFormat format;
		format.properties = TitleBlockBulkFormat::Size;
		format.font.setPixelSize(12);
		QFont result;
		result.fromString(format.mergedFont(QString(), QFont(QStringLiteral("Arial"), 9)));
		QCOMPARE(result.pixelSize(), 12);
	}

	void styledTextNeverWraps()
	{
		const char *xml =
			"<titleblocktemplate name=\"t\"><information/><logos/>"
			"<grid cols=\"60;\" rows=\"30;\">"
			"<field row=\"0\" col=\"0\" name=\"x\" fontsize=\"9\" hadjust=\"false\""
			" displaylabel=\"false\" align=\"left\" valign=\"top\">"
			"<value><translation lang=\"en\">a very long text that does not fit the cell</translation></value>"
			"<label/></field></grid></titleblocktemplate>";
		TitleBlockTemplate tpl;
		QVERIFY(load(tpl, xml));
		const int plain_bottom = lastInkRow(draw(tpl));
		QVERIFY(plain_bottom > 2);   // the text is drawn at all

		tpl.cell(0, 0)->value_color = Qt::black;
		// one line, as the plain path draws it, not several
		QVERIFY(lastInkRow(draw(tpl)) <= plain_bottom + 1);
	}

	void styleIsSavedAndLoaded()
	{
		TitleBlockTemplate tpl;
		QVERIFY(load(tpl));
		TitleBlockCell *a = tpl.cell(0, 0);
		a->label_font = QFont(QStringLiteral("Courier"), 11).toString();
		a->value_color = QColor(QStringLiteral("#80ff0000"));

		QDomDocument doc;
		QDomElement root = doc.createElement(QStringLiteral("titleblocktemplate"));
		doc.appendChild(root);
		QVERIFY(tpl.saveToXmlElement(root));

		TitleBlockTemplate again;
		QVERIFY(again.loadFromXmlElement(root));
		TitleBlockCell *a2 = again.cell(0, 0);
		QCOMPARE(a2->label_font, a->label_font);
		QCOMPARE(a2->value_color, a->value_color);
		QCOMPARE(a2->value_color.alpha(), 0x80);
		QVERIFY(a2->value_font.isEmpty());
		QVERIFY(!a2->label_color.isValid());
		QVERIFY(!again.cell(0, 1)->hasTextStyle());
	}

	void unstyledTemplateSavesNoStyle()
	{
		TitleBlockTemplate tpl;
		QVERIFY(load(tpl));
		QDomDocument doc;
		QDomElement root = doc.createElement(QStringLiteral("titleblocktemplate"));
		doc.appendChild(root);
		QVERIFY(tpl.saveToXmlElement(root));
		const QString xml = doc.toString();
		QVERIFY(!xml.contains(QStringLiteral("-font")));
		QVERIFY(!xml.contains(QStringLiteral("-color")));
	}

	void styledCellIsDrawnDifferently()
	{
		TitleBlockTemplate tpl;
		QVERIFY(load(tpl));
		const QImage plain = draw(tpl);

		// same drawing path, no style: identical to the plain one
		QCOMPARE(draw(tpl), plain);

		tpl.cell(0, 0)->value_color = Qt::red;
		const QImage styled = draw(tpl);
		QVERIFY(styled != plain);

		// the change stays inside the styled cell
		const QImage plain_rest = plain.copy(100, 0, 200, 30);
		const QImage styled_rest = styled.copy(100, 0, 200, 30);
		QCOMPARE(styled_rest, plain_rest);
	}
};

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	return QetAppTest::run<tst_titleblockbulkformat>(argc, argv);
}

#include "tst_titleblockbulkformat.moc"

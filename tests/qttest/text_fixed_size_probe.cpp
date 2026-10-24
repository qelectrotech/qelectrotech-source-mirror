// SPDX-License-Identifier: GPL-2.0-or-later
// Width and height of a symbol's text, each fixed or not, linked against
// the application's objects.
//
// In the text's properties, width and height each take two rows: the
// value, and "fixed" with a tick box. Not fixed, the field follows the
// text, and the value shows the size it has now and cannot be edited;
// fixed, the field keeps the value, which can then be edited. A fixed
// height cuts off the lines that do not fit, to shorten a text where
// room is short: they are neither drawn nor picked. Both are saved with
// the text and undone as any other change.
#include <QApplication>
#include <QDomDocument>
#include <QFontDatabase>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/dynamicelementtextitem.h"
#include "../../sources/ui/dynamicelementtextmodel.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

/// The value cell of a text's row with the given type
static QStandardItem *cell(DynamicElementTextModel &model, DynamicElementTextItem *text, int type)
{
	QStandardItem *row = model.itemFromIndex(model.indexFromText(text));
	for (int r = 0; r < row->rowCount(); ++r)
		if (QStandardItem *c = row->child(r, 1))
			if (c->data(Qt::UserRole + 1).toInt() == type) return c;
	throw std::runtime_error("row missing");
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETTextFixedSizeRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *scene = project.diagrams().first();

		DynamicElementTextItem *text = nullptr;
		Element *element = nullptr;
		for (Element *e : scene->elements())
			for (DynamicElementTextItem *t : e->dynamicTextItems())
				if (!text) { text = t; element = e; }
		check(text, "the fixture has a symbol with a text");
		text->setTextFrom(DynamicElementTextItem::UserText);
		text->setText(QStringLiteral("Sockets bathroom first floor, 5"));
		text->setTextWidth(-1);
		text->setTextHeight(-1);

		// The properties: not fixed, the size the field has now, read only.
		DynamicElementTextModel model(element);
		QStandardItem *width = cell(model, text, DynamicElementTextModel::textWidth);
		QStandardItem *width_fixed = cell(model, text, DynamicElementTextModel::textWidthFixed);
		QStandardItem *height = cell(model, text, DynamicElementTextModel::textHeight);
		QStandardItem *height_fixed = cell(model, text, DynamicElementTextModel::textHeightFixed);
		check(width_fixed->isCheckable() && width_fixed->checkState() == Qt::Unchecked, "width not fixed: not ticked");
		check(height_fixed->isCheckable() && height_fixed->checkState() == Qt::Unchecked, "height not fixed: not ticked");
		check(!width->isCheckable() && !height->isCheckable(), "the values are rows of their own, without a tick box");
		check(qAbs(width->data(Qt::EditRole).toDouble() - text->boundingRect().width()) <= 1,
			  "the width shows the size the field has now");
		check(!(width->flags() & Qt::ItemIsEditable) && !(height->flags() & Qt::ItemIsEditable),
			  "and cannot be edited while not fixed");

		// Fix the width: the value can be edited at once; at 50 the text wraps.
		const qreal one_line = text->boundingRect().height();
		width_fixed->setCheckState(Qt::Checked);
		check(width->flags() & Qt::ItemIsEditable, "ticking fixed makes the width editable at once");
		width->setData(50, Qt::EditRole);
		scene->undoStack().push(model.undoForEditedText(text));
		check(qFuzzyCompare(text->textWidth(), 50), "fixed, the width is the value above");
		const qreal wrapped = text->boundingRect().height();
		check(wrapped > one_line * 2, "the text wraps inside it");

		// Fix the height at one line: the other lines are cut off.
		height_fixed->setCheckState(Qt::Checked);
		height->setData(qRound(one_line), Qt::EditRole);
		scene->undoStack().push(model.undoForEditedText(text));
		check(qFuzzyCompare(text->textHeight(), qRound(one_line)), "fixed, the height is the value above");
		check(qAbs(text->boundingRect().height() - qRound(one_line)) < 0.01, "the field keeps that height");
		const QPointF below(text->boundingRect().center().x(), wrapped - 2);
		check(!text->contains(below) && !text->shape().contains(below), "the lines cut off are not picked");

		// Kept in the file.
		QDomDocument doc;
		const QDomElement xml = text->toXml(doc);
		check(xml.attribute("text_height").toDouble() == qRound(one_line), "the height is saved with the text");
		text->setTextHeight(-1);
		QDomElement copy = xml;
		text->fromXml(copy);
		check(qFuzzyCompare(text->textHeight(), qRound(one_line)), "and read back");

		// Undo, untick.
		scene->undoStack().undo();
		check(text->textHeight() < 0 && qAbs(text->boundingRect().height() - wrapped) < 0.01,
			  "undo gives the text its full height back");
		check(cell(model, text, DynamicElementTextModel::textHeightFixed)->checkState() == Qt::Unchecked
			  && !(cell(model, text, DynamicElementTextModel::textHeight)->flags() & Qt::ItemIsEditable),
			  "and the properties show it not fixed");
		width_fixed->setCheckState(Qt::Unchecked);
		scene->undoStack().push(model.undoForEditedText(text));
		check(text->textWidth() < 0, "not ticked, the width follows the text again");
		check(!text->toXml(doc).hasAttribute("text_height"), "a height that follows the text is not written");
		std::puts("PASS");
	}
	catch (const std::exception &error)
	{
		std::printf("FAIL: %s\n", error.what());
		std::fflush(stdout);
		return 1;
	}
	std::fflush(stdout);
	return 0;
}

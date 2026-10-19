// SPDX-License-Identifier: GPL-2.0-or-later
// Width and height of a symbol's text, each fixed or not, linked against
// the application's objects.
//
// In the text's properties, "Width (fixed)" and "Height (fixed)" each
// carry a tick box. Not ticked, the field follows the text and shows the
// size it has now; ticked, it keeps the size beside the box. A fixed
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

		// The properties: not ticked, the size the field has now.
		DynamicElementTextModel model(element);
		QStandardItem *width = cell(model, text, DynamicElementTextModel::textWidth);
		QStandardItem *height = cell(model, text, DynamicElementTextModel::textHeight);
		check(width->isCheckable() && width->checkState() == Qt::Unchecked, "width not fixed: not ticked");
		check(height->isCheckable() && height->checkState() == Qt::Unchecked, "height not fixed: not ticked");
		check(qAbs(width->data(Qt::EditRole).toDouble() - text->boundingRect().width()) <= 1,
			  "it shows the width the field has now");

		// Fix the width at 50: the text wraps onto more lines.
		const qreal one_line = text->boundingRect().height();
		width->setData(50, Qt::EditRole);
		width->setCheckState(Qt::Checked);
		scene->undoStack().push(model.undoForEditedText(text));
		check(qFuzzyCompare(text->textWidth(), 50), "ticked, the width is fixed at the value beside it");
		const qreal wrapped = text->boundingRect().height();
		check(wrapped > one_line * 2, "the text wraps inside it");

		// Fix the height at one line: the other lines are cut off.
		height->setData(qRound(one_line), Qt::EditRole);
		height->setCheckState(Qt::Checked);
		scene->undoStack().push(model.undoForEditedText(text));
		check(qFuzzyCompare(text->textHeight(), qRound(one_line)), "ticked, the height is fixed");
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
		check(cell(model, text, DynamicElementTextModel::textHeight)->checkState() == Qt::Unchecked,
			  "and the properties show it not fixed");
		width->setCheckState(Qt::Unchecked);
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

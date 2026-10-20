// SPDX-License-Identifier: GPL-2.0-or-later
// A group of a symbol's texts keeps the point it is aligned on, linked
// against the application's objects.
//
// A group aligned right is a reference point at its right edge: when one
// of its texts grows or shrinks, the right edge stays where it is and the
// group grows to the left, so that texts placed left of a fuse do not run
// into it. Centred, the middle stays; aligned left, the left edge stays,
// as before. Also on a turned symbol, when the text shrinks back, and for a group read
// back from the file: reading it moves nothing, and the next change keeps
// its alignment point.
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
#include "../../sources/qetgraphicsitem/elementtextitemgroup.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

static bool near(const QPointF &a, const QPointF &b)
{
	return qAbs(a.x() - b.x()) < 0.01 && qAbs(a.y() - b.y()) < 0.01;
}

/// The point at height 0 of the group, at x along it, in the folio
static QPointF at(ElementTextItemGroup *group, qreal x)
{
	return group->mapToScene(QPointF(x, 0));
}

static qreal widthOf(ElementTextItemGroup *group)
{
	qreal w = 0;
	for (DynamicElementTextItem *t : group->texts())
		w = qMax(w, t->boundingRect().width());
	return w;
}

/// The right edge of each text, in the group's coordinates
static bool rightAligned(ElementTextItemGroup *group)
{
	const qreal w = widthOf(group);
	for (DynamicElementTextItem *t : group->texts())
		if (qAbs(t->pos().x() + t->boundingRect().width() - w) > 0.01) return false;
	return true;
}

static ElementTextItemGroup *makeGroup(Element *element, const QStringList &lines)
{
	QList<DynamicElementTextItem *> texts;
	qreal y = 0;
	for (const QString &line : lines) {
		auto *t = new DynamicElementTextItem(element);
		t->setTextFrom(DynamicElementTextItem::UserText);
		t->setText(line);
		t->setPos(0, y);
		y += 12;
		element->addDynamicTextItem(t);
		texts << t;
	}
	ElementTextItemGroup *group = element->addTextGroup(QStringLiteral("g"));
	for (DynamicElementTextItem *t : texts)
		element->addTextToGroup(t, group);
	return group;
}

static void checkAlignment(Diagram *scene, Element *element, Qt::Alignment alignment,
						   const char *keeps, const char *undone)
{
	ElementTextItemGroup *group = makeGroup(element, {"-FI1", "40 A / 30 mA", "FI UG"});
	group->setAlignment(alignment);
	DynamicElementTextItem *longest = group->texts().at(1);
	const qreal w = widthOf(group);
	const qreal x = alignment == Qt::AlignRight ? w : alignment == Qt::AlignVCenter ? w / 2 : 0;
	const QPointF fixed = at(group, x);
	const QPointF pos = group->pos();

	longest->setText(QStringLiteral("40 A / 30 mA, type A, 4-pole"));
	const qreal w2 = widthOf(group);
	check(w2 > w + 10, "the longest text grows");
	const qreal x2 = alignment == Qt::AlignRight ? w2 : alignment == Qt::AlignVCenter ? w2 / 2 : 0;
	check(near(at(group, x2), fixed), keeps);
	if (alignment == Qt::AlignRight)
		check(rightAligned(group), "the texts stay right-aligned to each other");

	longest->setText(QStringLiteral("40 A / 30 mA"));
	check(near(group->pos(), pos), undone);

	for (DynamicElementTextItem *t : group->texts())
		element->removeTextFromGroup(t, group);
	element->removeTextGroup(group);
	delete group;
	(void)scene;
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETTextGroupAnchorRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		project.buildFolios(project.diagrams());
		Diagram *scene = project.diagrams().first();
		Element *element = scene->elements().first();
		check(element, "the fixture has a symbol");

		checkAlignment(scene, element, Qt::AlignRight,
					   "aligned right, the right edge stays where it is", "shrinking it back puts the group back");
		checkAlignment(scene, element, Qt::AlignVCenter,
					   "centred, the middle stays where it is", "shrinking it back puts the centred group back");
		checkAlignment(scene, element, Qt::AlignLeft,
					   "aligned left, the left edge stays, as before", "and the left-aligned group is where it was");

		// On a turned symbol, the right edge is found along the group.
		element->setRotation(90);
		checkAlignment(scene, element, Qt::AlignRight,
					   "on a turned symbol, the right edge stays too", "and goes back");
		element->setRotation(0);

		// Read back from the file: nothing moves, the next change keeps
		// the right edge.
		ElementTextItemGroup *group = makeGroup(element, {"-FI2", "40 A / 30 mA", "FI OG"});
		group->setAlignment(Qt::AlignRight);
		group->setPos(-120, 30);
		QDomDocument doc;
		QDomElement xml = group->toXml(doc);
		const QList<DynamicElementTextItem *> texts = group->texts();
		QList<QPointF> text_pos;
			//Where the file has them: in the symbol, as Element::toXml() writes them
		for (DynamicElementTextItem *t : texts) text_pos << element->mapFromItem(group, t->pos());
		for (DynamicElementTextItem *t : texts) element->removeTextFromGroup(t, group);
		element->removeTextGroup(group);
		delete group;
		for (int i = 0; i < texts.size(); ++i) texts[i]->setPos(text_pos[i]);

			//As Element::fromXml() does: the group first, then its texts
		ElementTextItemGroup *read = element->addTextGroup(QStringLiteral("read"));
		read->fromXml(xml);
		check(read->texts().size() == 3, "the group is read back with its three texts");
		check(near(read->pos(), QPointF(-120, 30)), "a group read from the file stays where it was saved");
		const QPointF right = at(read, widthOf(read));
		read->texts().at(1)->setText(QStringLiteral("40 A / 30 mA, type A, 4-pole"));
		check(near(at(read, widthOf(read)), right), "and its first change after reading keeps the right edge");

		// A text whose content is filled in only after the group was read,
		// as Element::fromXml() sets an element's label from its
		// information last: the group stays where it was saved. Each save
		// and reload moved it before.
		for (Qt::Alignment alignment : {Qt::AlignRight, Qt::AlignVCenter}) {
			ElementTextItemGroup *g = makeGroup(element, {"-FI3", "40 A / 30 mA, type A", "FI EG"});
			g->setAlignment(alignment);
			g->setPos(-200, 60);
			QDomDocument d;
			QDomElement x = g->toXml(d);
			const QList<DynamicElementTextItem *> ts = g->texts();
			QList<QPointF> ps;
			for (DynamicElementTextItem *t : ts) ps << element->mapFromItem(g, t->pos());
			for (DynamicElementTextItem *t : ts) element->removeTextFromGroup(t, g);
			element->removeTextGroup(g);
			delete g;
			for (int i = 0; i < ts.size(); ++i) ts[i]->setPos(ps[i]);
			const QString longest = ts[1]->text();
			ts[1]->setText(QString());

			ElementTextItemGroup *r = element->addTextGroup(QStringLiteral("late"));
			r->fromXml(x);
			ts[1]->setText(longest);
			check(near(r->pos(), QPointF(-200, 60)),
				  "a group whose longest text is filled in after reading stays where it was saved");
		}
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

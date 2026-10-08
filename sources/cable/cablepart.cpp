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
#include "cablepart.h"

#include "cable.h"
#include "cablemanager.h"
#include "cablepropertiesdialog.h"
#include "editcablecommand.h"
#include "../autoNum/assignvariables.h"
#include "../diagram.h"
#include "../properties/xrefproperties.h"
#include "../qetapp.h"
#include "../qetproject.h"
#include "../qetgraphicsitem/conductor.h"
#include "../utils/qetutils.h"

#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHash>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include <QUndoStack>

#include <cmath>

#include <utility>

namespace {

	///Half size of the slash drawn where a core crosses a conductor
const qreal slash_half = 3.5;
	///How far from a slash or its colour the mouse still counts as holding it
const qreal slash_grab = 6.0;
	///How far the colour of a core may stand from the wire it names:
	///close enough to read as one thing, far enough away that the tail of
	///a letter like "g" does not run into the wire. The text is turned by
	///a quarter, so that tail is its right hand side, which is the side
	///facing the wire.
const qreal core_label_side = 2.5;
	///How far above the line the colour of a core is written, so
	///the line passes beside the letters instead of through them
const qreal core_label_gap = 4.0;
	///Radius of the blue point drawn at each end of a picked line, and
	///how far from such a point the mouse still counts as holding it.
	///Ten across, which is the size QET's own handles use.
const qreal grip_radius = 5.0;
const qreal grip_grab = 6.0;
	///How far from the trunk line the mouse still counts as aiming at
	///it while a core waits to be put down there: the core follows the
	///line only while the mouse is near enough for the click to read as
	///belonging to it, and the slash itself is drawn on the line, at a
	///grid step, so what is shown is always what would be kept.
const qreal place_grab = 30.0;

} // namespace

	///The one line which waits for the click putting a core down, if any
QPointer<CablePart> CablePart::s_placing;

/**
	@brief CablePart::CablePart
	@param data the section as the cable holds it
	@param parent
*/
CablePart::CablePart(const CablePartData &data, QGraphicsItem *parent) :
	CablePart(parent)
{
	setLine(data.p1, data.p2);
	m_part_uuid = data.uuid;
	m_pending_cable = QUuid();
}

/**
	@brief CablePart::CablePart
	An empty part, waiting for its geometry: used while reading a project.
	@param parent
*/
CablePart::CablePart(QGraphicsItem *parent) :
	QetGraphicsItem(parent)
{
		//ItemSendsGeometryChanges is what makes itemChange() hear about
		//position changes at all -- without it Qt silently never calls
		//it for a setPos(), and then this item is moved only in the
		//screen's opinion: the line and its colour labels are drawn at
		//the new place because the whole item is transformed, while
		//m_p1/m_p2 and CableCore::position go on naming the old one.
		//A paste would therefore wire its labels against the coordinates
		//the line was copied from, and the next copy would write those
		//stale coordinates into the clipboard -- which is exactly the
		//paste landing away from the group it belongs to.
	setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
	setAcceptHoverEvents(true);
	m_part_uuid = QUuid::createUuid();
}

CablePart::~CablePart()
{
}

/**
	@brief CablePart::setCable
	Bind this section to its cable. The item follows every change of the
	cable, because the number and the type it draws live there.
	@param cable
*/
void CablePart::setCable(Cable *cable)
{
	if (m_cable == cable) {
		return;
	}
	if (m_cable) {
		disconnect(m_cable, nullptr, this, nullptr);
	}
	m_cable = cable;
	m_pending_cable = QUuid();
	if (m_cable)
	{
		connect(m_cable, &Cable::changed, this, [this]() {
			prepareGeometryChange();
			update();
		});
	}
	setUpXrefHooks();
	prepareGeometryChange();
	update();
}

/**
	@brief CablePart::cable
	@return the cable this section belongs to, or nullptr
*/
Cable *CablePart::cable() const
{
	return m_cable.data();
}

/**
	@brief CablePart::partUuid
	@return which section of the cable this item draws
*/
QUuid CablePart::partUuid() const
{
	return m_part_uuid;
}

/**
	@brief CablePart::refreshPending
	Bind the item to its cable once the whole project is read: a folio is
	loaded before the cables it holds sections of, so at that moment the
	cable is only known by its identity.
*/
void CablePart::refreshPending()
{
	if (m_cable) return;
	if (!diagram() || !diagram()->project()) return;

	if (Cable *cable = CableManager::cableByUuid(diagram()->project(), m_pending_cable)) {
		setCable(cable);
	}
}

/**
	@brief CablePart::setLine
	Change where the trunk line runs. Purely optical: no core is added,
	none is dropped, nothing is reassigned -- a cable keeps its cores
	wherever its line happens to run.
	@param p1 scene coordinates of one end
	@param p2 scene coordinates of the other end
*/
void CablePart::setLine(const QPointF &p1, const QPointF &p2)
{
	if (m_p1 == p1 && m_p2 == p2) return;
	prepareGeometryChange();
	m_p1 = p1;
	m_p2 = p2;
	syncGeometry();
	update();
}

/**
	@brief CablePart::line
	@return the trunk line, in scene coordinates
*/
QLineF CablePart::line() const
{
	return QLineF(m_p1, m_p2);
}

/**
	@brief CablePart::textFont
	@return the font a text of the cable is written with when the cable
	itself asks for none: what the preferences offer on the "Textes"
	page, and -- until he has picked something there -- the font the
	folio writes its own texts with, a shade smaller: the way every
	cable has always been drawn, so that merely opening the
	preferences changes nothing.
*/
QFont CablePart::textFont() const
{
	return QETApp::cableTextsFont();
}

/**
	@brief CablePart::coreFont
	@return the font the colours of the cores are written with when the
	cable itself asks for none -- all of them together, since the
	colours of one cable are read as one and are never set apart.
*/
QFont CablePart::coreFont() const
{
	return fontOf(QStringLiteral("cores"));
}

/**
	@brief CablePart::fontOf
	@param key the name of a text of the cable: "designation", "type",
	"installation", "location", "length" or "cores"
	@return the font that text is really written with: the one this
	cable asks for, or -- when it asks for none -- the one the
	preferences offer
*/
QFont CablePart::fontOf(const QString &key) const
{
	if (m_cable)
	{
		const CableTextFormat format = m_cable->properties().texts.value(key);
		if (!format.font.isEmpty())
		{
			QFont font;
			if (QETUtils::fontFromString(font, format.font)) {
				return font;
			}
		}
	}

		//The reference to the other folios this cable runs on has a
		//font of its own, set with the button next to its text under
		//Querverweise -- he sets it there and not in the cable itself.
		//When he has left that alone, the reference follows the texts
		//of the cable like every other line of the label does.
	if (key == QLatin1String("xref") && m_cable && diagram() && diagram()->project())
	{
		const QString description = diagram()->project()
				->defaultXRefProperties(QStringLiteral("cable")).font();
		if (!description.isEmpty())
		{
			QFont font;
			if (QETUtils::fontFromString(font, description)) {
				return font;
			}
		}
	}

	return key == QLatin1String("cores") ? QETApp::cableCoreFont() : textFont();
}

/**
	@brief CablePart::alignOf
	How one text lines up at the left end of the trunk line. A cable
	which says nothing about it follows the preferences, which is where
	-- until he has said otherwise there either -- every text ends where
	the line begins, the way it always did.
	@param key the name of a text of the cable
	@return Qt::AlignLeft, Qt::AlignHCenter or Qt::AlignRight
*/
Qt::Alignment CablePart::alignOf(const QString &key) const
{
	int alignment = 0;
	if (m_cable) {
		alignment = m_cable->properties().texts.value(key).alignment;
	}
	if (!alignment) {
		return QETApp::cableTextAlignment();
	}
	switch (alignment)
	{
	case int(Qt::AlignLeft): return Qt::AlignLeft;
	case int(Qt::AlignHCenter): return Qt::AlignHCenter;
	case int(Qt::AlignRight): return Qt::AlignRight;
	default: return Qt::AlignRight;
	}
}

/**
	@brief CablePart::textOffset
	@return how far the whole label at the left of the line stands from
	where it has always stood: what the form offers as "Position du
	texte", and zero for a cable which has never been nudged -- which
	is where every cable starts, so that merely opening a project
	moves nothing.
*/
QPointF CablePart::textOffset() const
{
	return m_cable ? m_cable->properties().text_offset : QPointF();
}

/**
	@brief CablePart::coreOffset
	@return how far the row of colours stands from the slashes it names:
	the same kind of nudge, kept apart from the label's because the two
	are set apart in the form and are moved one without the other.
*/
QPointF CablePart::coreOffset() const
{
	return m_cable ? m_cable->properties().core_offset : QPointF();
}

/**
	@brief CablePart::textLeft
	Where a text of this width starts, at the left end of the trunk
	line -- the same place for every text, so that a cable whose
	fields are set apart still reads as one label rather than as five.
	@param key the name of a text of the cable
	@param width how wide that text is, in scene units
	@return the x of the left of the text, in scene coordinates
*/
qreal CablePart::textLeft(const QString &key, qreal width) const
{
	const qreal line_left = qMin(m_p1.x(), m_p2.x());
	if (alignOf(key) == Qt::AlignLeft) {
		return line_left + 6.0;
	}
	if (alignOf(key) == Qt::AlignHCenter) {
		return line_left - width * 0.5;
	}
	return line_left - 6.0 - width;
}

/**
	@brief CablePart::trunkIsHorizontal
	@return true when the trunk line runs along the sheet rather than up
	and down it: which way it runs is what tells the label where it may
	sit, and where the colour of a core may be written.
*/
bool CablePart::trunkIsHorizontal() const
{
	return qAbs(m_p2.y() - m_p1.y()) <= qAbs(m_p2.x() - m_p1.x());
}

/**
	@brief CablePart::labelLines
	@return what of the cable the label at the left of the trunk line
	writes, in the order the lines are stacked: where the cable runs
	first, then what it is called. Every field the user switched off,
	and every field he left empty, is left out.

	Neither the type of the cable, nor its length, nor the references to
	the other folios this cable runs on is among them: all three are
	written under the line rather than over it -- the type first, the
	length under it and the references at the very bottom -- so what is
	above the line stays the description of where the cable belongs and
	nothing else. See refLines() for those underneath.

	Each line comes with the rectangle it is written in, worked out with
	its own font and its own way of lining up: those lines may differ
	from each other as much as he likes, and they are stacked one
	under the other by their own height rather than by one common size.
*/
QList<CablePart::LabelLine> CablePart::labelLines() const
{
	if (!m_cable) return QList<LabelLine>();

	const CableProperties properties = m_cable->properties();
	QList<LabelLine> lines;
	const auto write = [this, &lines](const QString &text, const QString &key)
	{
		if (text.isEmpty()) return;
		LabelLine line;
		line.text = text;
		line.key = key;
		line.font = fontOf(key);
		line.align = alignOf(key);
		const QFontMetricsF metrics(line.font);
		line.width = metrics.horizontalAdvance(text);
		line.height = metrics.height();
		lines.append(line);
	};

	if (properties.show_installation) {
		write(properties.installation, QStringLiteral("installation"));
	}
	if (properties.show_location) {
		write(properties.location, QStringLiteral("location"));
	}
	if (properties.show_designation) {
		write(properties.designation, QStringLiteral("designation"));
	}
	if (lines.isEmpty()) return lines;

	qreal total_height = 0.0;
	for (const LabelLine &line : lines) {
		total_height += line.height;
	}
	const qreal top = trunkIsHorizontal()
			? (qMin(m_p1.y(), m_p2.y()) - 2.0 - total_height)
			: qMin(m_p1.y(), m_p2.y());

	qreal y = top;
	for (LabelLine &line : lines)
	{
		line.rect = QRectF(textLeft(line.key, line.width), y,
						   line.width, line.height);
		y += line.height;
	}

		//Where the label as a whole has been nudged to: every line of it
		//goes with the rest, so those three lines keep reading as one
		//label rather than as three which have drifted apart.
	const QPointF nudge = textOffset();
	if (!nudge.isNull())
	{
		for (LabelLine &line : lines) {
			line.rect.translate(nudge);
		}
	}
	return lines;
}

/**
	@brief CablePart::refLines
	@return one line per other folio this cable runs on, stacked under
	the length of the cable -- under the type when the length is not
	drawn, straight under the trunk line when neither is.

	That is the bottom of everything the cable says about itself: above
	the line stands where it belongs, under the line what it is and how
	long it runs, and last comes where it goes on. Each line names the
	section standing there, and each of them is the line he double
	clicks to be taken to it.

	Every entry comes in the order crossRefs() lists them, so the index
	of a line here is the index of that reference there.
*/
QList<CablePart::LabelLine> CablePart::refLines() const
{
	if (!m_cable) return QList<LabelLine>();

	QList<LabelLine> lines;
	for (const CrossRef &ref : crossRefs())
	{
		if (ref.text.isEmpty()) continue;
		LabelLine line;
		line.text = ref.text;
		line.key = QStringLiteral("xref");
		line.font = fontOf(line.key);
		line.align = alignOf(line.key);
		const QFontMetricsF metrics(line.font);
		line.width = metrics.horizontalAdvance(line.text);
		line.height = metrics.height();
		lines.append(line);
	}
	if (lines.isEmpty()) return lines;

		//The same stack as typeRect() and lengthRect() build, only
		//continued: what those two write between themselves decides how
		//far down the references start, so no two of them can overlap
		//whatever fonts he has chosen for them.
	qreal y = underTop();
	if (!typeText().isEmpty()) {
		y += QFontMetricsF(fontOf(QStringLiteral("type"))).height();
	}
	if (!lengthText().isEmpty()) {
		y += QFontMetricsF(fontOf(QStringLiteral("length"))).height();
	}

	for (LabelLine &line : lines)
	{
		line.rect = QRectF(textLeft(line.key, line.width), y,
						   line.width, line.height);
		y += line.height;
	}

	const QPointF nudge = textOffset();
	if (!nudge.isNull())
	{
		for (LabelLine &line : lines) {
			line.rect.translate(nudge);
		}
	}
	return lines;
}

/**
	@brief CablePart::crossRefs
	@return every other folio this cable runs on, one entry each, in the
	order the folios turn -- empty when the cable runs on this folio
	alone, which is what a cable normally does.

	The reference needs no data of its own: a cable standing on two
	folios is one cable drawn as several lines, so every one of those
	lines knows its siblings and they all get the same list, whichever
	way they were put together -- a line drawn onto a cable which was
	already there, or a cable drawn first and joined later.
*/
QList<CablePart::CrossRef> CablePart::crossRefs() const
{
	QList<CrossRef> refs;
	if (!m_cable || !diagram() || !diagram()->project()) return refs;

	QETProject *project = diagram()->project();

		//Which section stands on which other folio. The first section of
		//a folio is the one %l and %c name, when the format asks for
		//them: it is the line he has to look for over there.
	QHash<Diagram *, CablePartData> elsewhere;
	const QList<CablePartData> parts = m_cable->parts();
	for (const CablePartData &part : parts)
	{
		Diagram *other = project->diagramByUuid(part.diagram);
		if (!other || other == diagram()) continue;
		if (!elsewhere.contains(other)) {
			elsewhere.insert(other, part);
		}
	}
	if (elsewhere.isEmpty()) return refs;

		//The format this project asks for such a reference, chosen under
		//Querverweise for the type "Câble": one text per folio, written
		//out with that folio's own numbers and fields.
	const XRefProperties xref = project->defaultXRefProperties(QStringLiteral("cable"));
	const QString format = xref.masterLabel();
	if (format.isEmpty()) return refs;

		//The folios are walked over in the order they turn, so the lines
		//read the way the pages do.
	const QList<Diagram *> folios = project->diagrams();
	for (Diagram *other : folios)
	{
		if (!elsewhere.contains(other)) continue;

		autonum::FormulaContext context;
		const BorderTitleBlock &border = other->border_and_titleblock;
		context.folio = border.folio();
		context.folio_index = other->folioIndex();
		context.folio_total = border.folioTotal();
		context.plant = border.plant();
		context.locmach = border.locmach();
		context.title_block_fields = border.additionalFields();
		context.project_properties = project->projectProperties();
		const CablePartData part = elsewhere.value(other);
		context.has_element = true;
		context.element_position = other->convertPosition((part.p1 + part.p2) * 0.5);

			//A fresh set of numbers for every line: two references of
			//the same cable must not count one after the other.
		autonum::sequentialNumbers seq;
		CrossRef ref;
		ref.diagram = other;
		ref.part = part.uuid;
		ref.text = autonum::AssignVariables::formulaToLabel(format, seq, context);
		if (ref.text.isEmpty()) continue;
		refs.append(ref);
	}
	return refs;
}

/**
	@brief CablePart::crossRefAt
	@param scene_pos a point of the sheet, in scene coordinates
	@return the index of the reference standing at that very place, -1
	when none does -- measured on the lines which are really drawn, so
	the hand of the mouse, the double click and the drawing can never
	disagree about which line is which.
*/
int CablePart::crossRefAt(const QPointF &scene_pos) const
{
	int index = 0;
	for (const LabelLine &line : refLines())
	{
		if (line.rect.contains(scene_pos)) return index;
		++index;
	}
	return -1;
}

/**
	@brief CablePart::goToCrossRef
	Send the mouse to the section of the cable which stands on the folio
	the reference at that place points at, the way a cross reference of
	an element does: the folio is shown and the line is picked.
	@param scene_pos a point of the sheet, in scene coordinates
	@return true when the click stood on a reference
*/
bool CablePart::goToCrossRef(const QPointF &scene_pos)
{
	const int index = crossRefAt(scene_pos);
	if (index < 0) return false;

	const QList<CrossRef> refs = crossRefs();
	if (index >= refs.size()) return false;

	const CrossRef ref = refs.at(index);
	if (!ref.diagram) return false;

	const QList<QGraphicsItem *> items = ref.diagram->items();
	for (QGraphicsItem *item : items)
	{
		auto *section = qgraphicsitem_cast<CablePart *>(item);
		if (section && section->partUuid() == ref.part) {
			QetGraphicsItem::showItem(section);
			return true;
		}
	}
	return false;
}

/**
	@brief CablePart::pdfRefs
	Where the PDF export has to put a clickable link for every reference
	this section writes: the rectangle of the text on this folio, and on
	the folio it points at the rectangle of the section it names -- the
	very section a double click on the drawing sends him to (see
	goToCrossRef), so that the export and the drawing can never disagree
	about where the link leads.

	One entry per reference drawn, in the order it is drawn, so that the
	export and the drawing count the same way (see crossRefAt).
	@return those entries, empty when this cable runs on this folio alone
*/
QList<CablePart::PdfRef> CablePart::pdfRefs() const
{
	QList<PdfRef> out;
	if (!m_cable) return out;

	const QList<LabelLine> lines = refLines();
	const QList<CrossRef> refs = crossRefs();
	const int count = qMin(lines.size(), refs.size());

	for (int i = 0; i < count; ++i)
	{
		const CrossRef &ref = refs.at(i);
		if (!ref.diagram || lines.at(i).rect.isEmpty()) continue;

		PdfRef link;
		link.diagram = ref.diagram;
		link.here = lines.at(i).rect;

			//The section standing over there, which is what the reader
			//has to be brought to. It is looked for the same way a click
			//on the drawing looks for it, so link and click agree.
		const QList<QGraphicsItem *> items = ref.diagram->items();
		for (QGraphicsItem *item : items)
		{
			auto *section = qgraphicsitem_cast<CablePart *>(item);
			if (section && section->partUuid() == ref.part) {
				link.there = section->mapRectToScene(section->boundingRect());
				break;
			}
		}
		out.append(link);
	}
	return out;
}

/**
	@brief CablePart::setUpXrefHooks
	Keep the reference lines up to date with what the project does: a
	folio which is added, removed or moved changes which lines there are
	and what they say, and so does the format chosen under Querverweise.

	Only this line is redrawn, which is enough: the references of a
	cable are worked out again from the cable itself every time its
	label is drawn.
*/
void CablePart::setUpXrefHooks()
{
	for (const QMetaObject::Connection &connection : std::as_const(m_xref_connections)) {
		disconnect(connection);
	}
	m_xref_connections.clear();

	Diagram *folio = diagram();
	if (!m_cable || !folio || !folio->project()) return;

	QETProject *project = folio->project();
	const auto rebuild = [this]() {
		prepareGeometryChange();
		setUpXrefHooks();
		update();
	};

	m_xref_connections << connect(project, &QETProject::projectDiagramsOrderChanged, this, rebuild);
	m_xref_connections << connect(project, &QETProject::diagramAdded, this, rebuild);
	m_xref_connections << connect(project, &QETProject::diagramRemoved, this, rebuild);
	m_xref_connections << connect(project, &QETProject::XRefPropertiesChanged, this, rebuild);

		//%F names the other folio by its label, which is read off that
		//folio itself: only when the format really asks for it does that
		//folio have to be listened to as well.
	const XRefProperties xref = project->defaultXRefProperties(QStringLiteral("cable"));
	if (xref.masterLabel().contains(QLatin1String("%F")))
	{
		for (Diagram *other : project->diagrams()) {
			m_xref_connections << connect(other, &Diagram::diagramInformationChanged, this, rebuild);
		}
	}
}

/**
	@brief CablePart::typeText
	@return the type of the cable, an empty string when the drawing is
	not meant to show it or the cable says nothing about it
*/
QString CablePart::typeText() const
{
	if (!m_cable) return QString();

	const CableProperties properties = m_cable->properties();
	if (!properties.show_type || properties.type.isEmpty()) {
		return QString();
	}
	return properties.type;
}

/**
	@brief CablePart::lengthText
	@return how long the cable is, an empty string when the drawing is
	not meant to show it or the cable says nothing about it
*/
QString CablePart::lengthText() const
{
	if (!m_cable) return QString();

	const CableProperties properties = m_cable->properties();
	if (!properties.show_length || properties.length.isEmpty()) {
		return QString();
	}
	return properties.length;
}

/**
	@brief CablePart::labelRect
	The rectangle holding the label of the cable, drawn at the left of
	the trunk line -- in scene coordinates, like the line itself, so the
	item never has to be moved for its text to follow.

	Across a horizontal line the whole label stands above the line: text
	straddling the line is what the eyes have to fight with. The type
	and the length of the cable stand under the line, one under the
	other, and are therefore not measured here.
	@return an empty rectangle when the cable says nothing to show
*/
QRectF CablePart::labelRect() const
{
	QRectF rect;
	for (const LabelLine &line : labelLines())
	{
		rect = rect.isEmpty() ? line.rect : rect.united(line.rect);
	}
	return rect;
}

/**
	@brief CablePart::underTop
	Where the first line written under the trunk line starts.

	Across a horizontal line that is just under the line itself; along a
	line which runs up and down the sheet "under" has no meaning, so it
	is just under the rest of the label instead -- which keeps those
	lines at the left of the line either way.
	@return the y of the top of that first line, in scene coordinates
*/
qreal CablePart::underTop() const
{
	if (trunkIsHorizontal()) {
		return qMax(m_p1.y(), m_p2.y()) + 2.0;
	}

	const QRectF label = labelRect();
		//labelRect() already carries the nudge of the label, and the
		//lines under it are nudged once more further down: what is asked
		//for here is where they would stand without it, so that they go
		//with the label exactly once rather than twice.
	return label.isEmpty() ? qMin(m_p1.y(), m_p2.y())
						   : label.bottom() + 2.0 - textOffset().y();
}

/**
	@brief CablePart::typeRect
	The rectangle the type of the cable is written in. It sits under the
	trunk line rather than with the rest of the label, so the type reads
	as what the line carries while everything above the line says where
	the cable belongs -- and it is the first of the lines written under
	the line, so the length can stand under it.
	@return an empty rectangle when the type is not drawn
*/
QRectF CablePart::typeRect() const
{
	const QString type = typeText();
	if (type.isEmpty()) {
		return QRectF();
	}

	const QFontMetricsF metrics(fontOf(QStringLiteral("type")));
	const qreal width = metrics.horizontalAdvance(type);
	const qreal height = metrics.height();

	return QRectF(textLeft(QStringLiteral("type"), width), underTop(),
				  width, height).translated(textOffset());
}

/**
	@brief CablePart::lengthRect
	The rectangle the length of the cable is written in. It stands under
	the type of the cable, which itself stands straight under the trunk
	line: the type is what the cable is, the length is one more thing
	said about it. Where the type is not drawn, the length takes its
	place and sits straight under the line.
	@return an empty rectangle when the length is not drawn
*/
QRectF CablePart::lengthRect() const
{
	const QString length = lengthText();
	if (length.isEmpty()) {
		return QRectF();
	}

	const QFontMetricsF metrics(fontOf(QStringLiteral("length")));
	const qreal width = metrics.horizontalAdvance(length);
	const qreal height = metrics.height();

		//The type may well be written in another font than this, so how
		//far down the length hangs has to be measured from the type and
		//not from itself.
	qreal top = underTop();
	if (!typeText().isEmpty()) {
		top += QFontMetricsF(fontOf(QStringLiteral("type"))).height();
	}

	return QRectF(textLeft(QStringLiteral("length"), width), top,
				  width, height).translated(textOffset());
}

/**
	@brief CablePart::boundingRect
*/
QRectF CablePart::boundingRect() const
{
	QRectF rect(QPointF(qMin(m_p1.x(), m_p2.x()), qMin(m_p1.y(), m_p2.y())),
				QPointF(qMax(m_p1.x(), m_p2.x()), qMax(m_p1.y(), m_p2.y())));
	rect.adjust(-14.0, -14.0, 14.0, 14.0);

	const QRectF label = labelRect();
	if (!label.isEmpty()) {
		rect = rect.united(label);
	}
	const QRectF type = typeRect();
	if (!type.isEmpty()) {
		rect = rect.united(type);
	}
	const QRectF length = lengthRect();
	if (!length.isEmpty()) {
		rect = rect.united(length);
	}
	for (const LabelLine &line : refLines()) {
		rect = rect.united(line.rect);
	}

	if (m_cable)
	{
		for (const CableCore &core : m_cable->usedCores())
		{
			if (core.part != m_part_uuid || !coreDrawn(core)) continue;
			const QPointF at = (core.core == m_dragged_core) ? m_drag_scene : coreDrawPosition(core);
			rect = rect.united(coreLabelRect(at, m_cable->colorOfCore(core.core))
							   .adjusted(-3.0, -3.0, 3.0, 3.0));
		}
			//The core which is only waiting to be put down is drawn
			//too, so its colour has to be inside what is repainted --
			//right from the moment he asks for it, whether the mouse
			//already stands at the line or still has to come there
		if (m_pending_core >= 0)
		{
			rect = rect.united(coreLabelRect(m_drag_scene,
											 m_cable->colorOfCore(m_pending_core))
							   .adjusted(-3.0, -3.0, 3.0, 3.0));
		}
	}
	return rect;
}

/**
	@brief CablePart::coreLabelPivot
	The point the colour of a core is written around before being turned
	by a quarter: at the left of its slash -- or rather close enough to
	the wire that it reads as one thing with it -- and lifted, so its
	lower end stops just above the line instead of being cut through by
	it.

	Being turned by a quarter, the tail of a letter like "g" points to
	the right, which is the side facing the wire: that is what the small
	distance kept here has to clear.
	@param at where the slash stands, in scene coordinates
	@param color the colour being written, e.g. "br"
	@return the centre of the text, in scene coordinates -- nudged by
	whatever the form offers as "Position des couleurs", which moves the
	colours and never the slash: the slash marks where the wire starts,
	and that is not for a distance to decide.
*/
QPointF CablePart::coreLabelPivot(const QPointF &at, const QString &color) const
{
	const QFontMetricsF metrics(coreFont());
	const qreal length = metrics.horizontalAdvance(color);
	const qreal thickness = metrics.height();

	return QPointF(at.x() - core_label_side - thickness * 0.5,
				   at.y() - core_label_gap - length * 0.5) + coreOffset();
}

/**
	@brief CablePart::coreLabelRect
	The rectangle the colour of a core is written in: the text stands at
	the left of its slash and is turned by a quarter, so it runs along
	the line instead of across it.
	@param at where the slash stands, in scene coordinates
	@param color the colour being written, e.g. "br"
	@return the rectangle in scene coordinates, empty when there is
	nothing to write
*/
QRectF CablePart::coreLabelRect(const QPointF &at, const QString &color) const
{
	if (color.isEmpty()) return QRectF();

	const QFontMetricsF metrics(coreFont());
	const qreal width = metrics.horizontalAdvance(color);
	const qreal height = metrics.height();

		//The text is drawn around this point and then turned by -90°, so
		//its screen rectangle is height wide and text length tall.
	const QPointF pivot = coreLabelPivot(at, color);
	return QRectF(pivot.x() - height * 0.5, pivot.y() - width * 0.5, height, width);
}

/**
	@brief CablePart::coreDrawPosition
	Where a stored core is drawn right now. While the trunk line is
	being dragged the label stands where it stood, carried along by
	however far the line has come: the line, the description at its left
	and the colours of its cores are one thing and move as one thing,
	so no label ever jumps, ever lands on a neighbour's wire or ever
	disappears -- and letting go of the line needs no correction.
	@param core the stored core
	@return where it is drawn at this moment
*/
QPointF CablePart::coreDrawPosition(const CableCore &core) const
{
	if (m_line_moved)
	{
		for (const CableCore &dragged : std::as_const(m_line_drag_cores))
		{
			if (dragged.core == core.core && dragged.part == core.part) {
				return dragged.position;
			}
		}
	}
	return core.position;
}

/**
	@brief CablePart::coreDrawn
	Says whether a core is drawn at all. While the line is dragged, the
	labels are drawn as they stand during that drag -- which is every
	one of them: carrying the line along never leaves any of them out.
	@param core the stored core
	@return true when there is something to draw
*/
bool CablePart::coreDrawn(const CableCore &core) const
{
	if (!m_line_moved) return true;

	for (const CableCore &dragged : std::as_const(m_line_drag_cores))
	{
		if (dragged.core == core.core && dragged.part == core.part) {
			return true;
		}
	}
	return false;
}

/**
	@brief CablePart::dragPosition
	Where the label of a core being dragged is drawn: still exactly
	where it stood, taken along by however far the mouse has come since
	it was picked up -- and only by whole grid steps, counted from the
	place it stood, the same as anything else drawn on the sheet moves.

	So picking a label up never shifts it by a hair, even when the
	mouse closes on it a little to one side, and letting go can never
	take it back: what is drawn while he drags is exactly what is kept
	when he lets go. A pull shorter than half a grid step moves nothing
	at all, which is how every other item on the folio behaves.

	The label follows the line and nothing else: pulling it upwards,
	downwards, sideways or diagonally slides it along its own line,
	never takes it off it, and never past either end of it -- a colour
	label belongs to the line it is written on. Holding Ctrl lets go of
	the grid steps, as everywhere else on the sheet.
	@param scene_pos where the mouse stands, in scene coordinates
	@return where the label is drawn at this moment
*/
QPointF CablePart::dragPosition(const QPointF &scene_pos) const
{
		//Where it was picked up: still the place the cable says it
		//stands, since nothing is written while he drags.
	QPointF anchor = m_drag_scene;
	if (m_cable && m_dragged_core >= 0) {
		const CableCore held = m_cable->core(m_dragged_core);
		if (held.core == m_dragged_core) anchor = held.position;
	}

	const QPointF axis = m_p2 - m_p1;
	const qreal length_squared = axis.x() * axis.x() + axis.y() * axis.y();
	if (qFuzzyIsNull(length_squared)) return anchor;

	const qreal length = std::sqrt(length_squared);
	const QPointF shift = scene_pos - m_press_scene;
	const qreal along = (shift.x() * axis.x() + shift.y() * axis.y()) / length;

	const qreal stepped = stepAlong(along);

	const QPointF carried = anchor + (axis / length) * stepped;

		//And no further than the line itself: a colour label belongs to
		//the line it is written on and stops at either end of it, so it
		//goes on standing over a place the line really runs across and
		//an entry can always follow it. Where it was picked up lay on
		//the line, so this never moves a label which behaves -- it only
		//stops one being pulled past the end.
	const qreal share = ((carried.x() - m_p1.x()) * axis.x()
						 + (carried.y() - m_p1.y()) * axis.y())
						/ length_squared;
	if (share <= 0.0) return m_p1;
	if (share >= 1.0) return m_p2;
	return m_p1 + axis * share;
}

/**
	@brief CablePart::stepAlong
	Snap a distance measured along the line to a grid step, in the
	coordinates of the sheet: a horizontal line steps along x, a
	vertical one along y, and holding Ctrl lets go of the grid the same
	way Diagram::snapToGrid does everywhere else on the sheet.
	@param along how far from the first end of the line, in scene units
	@return that distance at a grid step
*/
qreal CablePart::stepAlong(qreal along) const
{
	const QPointF probe = trunkIsHorizontal() ? QPointF(along, 0.0)
											   : QPointF(0.0, along);
	const QPointF stepped_probe = Diagram::snapToGrid(probe);
	return trunkIsHorizontal() ? stepped_probe.x()
							   : stepped_probe.y();
}

/**
	@brief CablePart::placePosition
	Where the mouse would put down a core which is waiting to be put
	down: the mouse itself projected onto the line, snapped to a grid
	step and stopped at either end of the line, so what the drawing
	shows while he aims is exactly what the click leaves behind.

	Unlike dragPosition this asks where the mouse *is* rather than how
	far it has come, since nothing is held yet to measure that from.
	@param scene_pos where the mouse stands, in scene coordinates
	@return where the slash would be drawn
*/
QPointF CablePart::placePosition(const QPointF &scene_pos) const
{
	const QPointF axis = m_p2 - m_p1;
	const qreal length_squared = axis.x() * axis.x() + axis.y() * axis.y();
	if (qFuzzyIsNull(length_squared)) return m_p1;

	const qreal length = std::sqrt(length_squared);
	const QPointF shift = scene_pos - m_p1;
	const qreal along = (shift.x() * axis.x() + shift.y() * axis.y()) / length;

		//At a grid step like anything else drawn, and never beyond
		//either end: a colour label belongs to the line it is written
		//on and stops where the line stops.
	const qreal stepped = qBound<qreal>(0.0, stepAlong(along), length);
	return m_p1 + (axis / length) * stepped;
}

/**
	@brief CablePart::shape
	Only the trunk line, its text and its slashes are clickable, not the
	whole rectangle around them: a cable drawn across a dense folio must
	not steal the clicks meant for what it passes over.
*/
QPainterPath CablePart::shape() const
{
	QPainterPath trunk;
	trunk.moveTo(m_p1);
	trunk.lineTo(m_p2);

	QPainterPathStroker stroker;
	stroker.setWidth(10.0);
	stroker.setCapStyle(Qt::FlatCap);

	QPainterPath result = stroker.createStroke(trunk);

	const QRectF label = labelRect();
	if (!label.isEmpty()) {
		result.addRect(label);
	}
	const QRectF type = typeRect();
	if (!type.isEmpty()) {
		result.addRect(type);
	}
	const QRectF length = lengthRect();
	if (!length.isEmpty()) {
		result.addRect(length);
	}
		//The references to the other folios stand under those and are
		//clicked the same way.
	for (const LabelLine &line : refLines()) {
		result.addRect(line.rect);
	}

	if (m_cable)
	{
		for (const CableCore &core : m_cable->usedCores())
		{
			if (core.part != m_part_uuid || !coreDrawn(core)) continue;
			const QPointF at = (core.core == m_dragged_core) ? m_drag_scene : coreDrawPosition(core);
				//The colour beside the slash is held by the same grab as
				//the slash: it is read far more often than it is moved,
				//and clicking the letters has to do what clicking the
				//slash does. The label stands to the left of the slash,
				//so the grab reaches that way rather than the other.
			result.addRect(QRectF(at - QPointF(18.0, slash_grab),
								  QSizeF(24.0, slash_grab * 2.0)));
			const QRectF text = coreLabelRect(at, m_cable->colorOfCore(core.core));
			if (!text.isEmpty()) {
				result.addRect(text.adjusted(-3.0, -3.0, 3.0, 3.0));
			}
		}
	}

		//While the line is picked, its two end points are part of what
		//may be clicked: they reach out over the one unit of stroke the
		//trunk itself is, and a finger aiming at a blue point never
		//lands that exactly.
	if (isSelected())
	{
		result.addEllipse(m_p1, grip_grab, grip_grab);
		result.addEllipse(m_p2, grip_grab, grip_grab);
	}
	return result;
}

/**
	@brief CablePart::labelAt
	Whether the colour label of that core is standing at that point:
	either its slash, or the text beside it, both of which read as one
	thing with the wire the core names.

	One core at a time, so that the test can never be answered by a
	neighbour standing closer to the point -- dropping one colour
	straight onto another has to be seen as exactly that.
	@param core which core is being asked about
	@param local a point of this item, scene coordinates
	@return true when the point is on that label
*/
bool CablePart::labelAt(int core, const QPointF &local) const
{
	if (!m_cable) return false;

	const CableCore at_core = m_cable->core(core);
	if (at_core.core != core || at_core.part != m_part_uuid || !coreDrawn(at_core)) {
		return false;
	}

	const QPointF at = coreDrawPosition(at_core);
	if (QLineF(at, local).length() <= slash_grab) return true;

	const QRectF text = coreLabelRect(at, m_cable->colorOfCore(core));
	return !text.isEmpty() && text.adjusted(-3.0, -3.0, 3.0, 3.0).contains(local);
}

/**
	@brief CablePart::coreAt
	@param local a point of this item, scene coordinates
	@return the core whose slash or colour stands there, -1 when none does
*/
int CablePart::coreAt(const QPointF &local) const
{
	if (!m_cable) return -1;

	for (const CableCore &core : m_cable->usedCores())
	{
		if (core.part != m_part_uuid || !coreDrawn(core)) continue;
		if (core.core != m_dragged_core)
		{
			if (labelAt(core.core, local)) return core.core;
			continue;
		}

			//The one being dragged is drawn where the mouse holds it
			//right now, not where the cable still says it stands
		const QPointF at = m_drag_scene;
		if (QLineF(at, local).length() <= slash_grab) {
			return core.core;
		}
		const QRectF text = coreLabelRect(at, m_cable->colorOfCore(core.core));
		if (!text.isEmpty() && text.adjusted(-3.0, -3.0, 3.0, 3.0).contains(local)) {
			return core.core;
		}
	}
	return -1;
}

/**
	@brief CablePart::drawLabel
	@param painter
*/
void CablePart::drawLabel(QPainter *painter, const QColor &ink) const
{
	if (!m_cable) return;

	const QList<LabelLine> lines = labelLines();
	const QRectF type_rect = typeRect();
	const QRectF length_rect = lengthRect();
	const QList<LabelLine> ref_lines = refLines();
	if (lines.isEmpty() && type_rect.isEmpty() && length_rect.isEmpty()
		&& ref_lines.isEmpty()) return;

	painter->save();
	painter->setPen(ink);

		//Every line brings the font it is written with: they are not
		//measured with the same yard, so they may not be written with
		//one either.
	for (const LabelLine &line : lines)
	{
		painter->setFont(line.font);
		painter->drawText(line.rect, line.align | Qt::AlignVCenter, line.text);
	}
	if (!type_rect.isEmpty()) {
		painter->setFont(fontOf(QStringLiteral("type")));
		painter->drawText(type_rect, alignOf(QStringLiteral("type")) | Qt::AlignVCenter,
						  typeText());
	}
	if (!length_rect.isEmpty()) {
		painter->setFont(fontOf(QStringLiteral("length")));
		painter->drawText(length_rect, alignOf(QStringLiteral("length")) | Qt::AlignVCenter,
						  lengthText());
	}
		//Last of all, at the very bottom: which other folios this cable
		//runs on, one line each.
	for (const LabelLine &line : ref_lines)
	{
		painter->setFont(line.font);
		painter->drawText(line.rect, line.align | Qt::AlignVCenter, line.text);
	}
	painter->restore();
}

/**
	@brief CablePart::drawCores
	One slash per core the cable really wires, with the colour of that
	core beside it -- the same colour which is written into the cable
	field of the conductor underneath. Plus, while a core is waiting to
	be put down, a paler copy of the very same marks wherever the mouse
	stands near the line.
	@param painter
*/
void CablePart::drawCores(QPainter *painter, const QColor &ink) const
{
	if (!m_cable) return;

	for (const CableCore &core : m_cable->usedCores())
	{
		if (core.part != m_part_uuid || !coreDrawn(core)) continue;

		const bool dragged = (core.core == m_dragged_core);
		const QPointF at = dragged ? m_drag_scene : coreDrawPosition(core);

		drawCoreMark(painter, ink, at, m_cable->colorOfCore(core.core));
	}

		//A core which is waiting to be put down shows where it would
		//land: the same slash and the same colour as any other, only
		//paler, since nothing of it is written yet. It shows as soon as
		//he asks for the core, and not only once the mouse has come
		//near enough -- he picks it out of a list, and the line may well
		//be across the screen from where that list stands.
	if (m_pending_core >= 0)
	{
		painter->save();
		painter->setOpacity(painter->opacity() * 0.6);
		drawCoreMark(painter, ink, m_drag_scene,
					 m_cable->colorOfCore(m_pending_core));
		painter->restore();
	}
}

/**
	@brief CablePart::drawCoreMark
	One slash, with the colour of the core it marks written beside it:
	the colour stands at the left of its slash and is turned by a
	quarter, the way a measurement is written along the line it belongs
	to. The same drawing serves a core which is already on the sheet and
	one which is only being aimed at.
	@param painter
	@param ink the colour everything of this line is drawn with
	@param at where the slash stands, in scene coordinates
	@param color the colour of that core, e.g. "br"
*/
void CablePart::drawCoreMark(QPainter *painter, const QColor &ink,
							 const QPointF &at, const QString &color) const
{
	const QFont core_font = coreFont();
	const QFontMetricsF metrics(core_font);

	painter->save();
	painter->setFont(core_font);
	painter->setPen(QPen(ink, 1.0));
	painter->drawLine(QPointF(at.x() - slash_half, at.y() + slash_half),
					  QPointF(at.x() + slash_half, at.y() - slash_half));

	if (!color.isEmpty())
	{
		const qreal width = metrics.horizontalAdvance(color);
		const qreal height = metrics.height();
		const QPointF pivot = coreLabelPivot(at, color);

		painter->save();
		painter->translate(pivot);
		painter->rotate(-90.0);
		painter->drawText(QRectF(-width * 0.5, -height * 0.5, width, height),
						  Qt::AlignCenter, color);
		painter->restore();
	}
	painter->restore();
}

/**
	@brief CablePart::paint
	@param painter
	@param option
	@param widget
*/
void CablePart::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setFont(textFont());

	// The folio is painted into a buffer whose lightness is inverted to the
	// palette afterwards (PaletteGraphicsView): what is black in the buffer
	// comes out as the Text color, what is white comes out as the sheet.
	// The palette of the option, on the other hand, is the palette of the
	// view's widgets, so it names colors which are already final and land
	// on the sheet color -- an ink taken from it vanishes. Conductors are
	// drawn the same way: black when they stand, red when they are picked.
	const QColor ink = isSelected() ? Qt::red : Qt::black;

		//One unit, like every other line of the drawing: a cable trunk is
		//a line, not a highlight, and it has to line up with the wires it
		//runs across rather than stand out over them.
	QPen pen(ink, 1.0);
	pen.setCapStyle(Qt::FlatCap);
	pen.setJoinStyle(Qt::MiterJoin);
	painter->setPen(pen);
	painter->drawLine(m_p1, m_p2);

	drawLabel(painter, ink);

	drawCores(painter, ink);

		//The two points every line of the drawing shows once it is
		//picked: one at each end, in the same blue QET's own handles
		//use. Pulling one of them changes how long the line is, the
		//same way an end of a conductor is pulled (mousePressEvent
		//and mouseMoveEvent). They belong to the line and are painted
		//by it, so nothing else has to be put into the folio for them.
	if (isSelected())
	{
		painter->save();
		painter->setPen(Qt::NoPen);
		painter->setBrush(Qt::blue);
		painter->drawEllipse(m_p1, grip_radius, grip_radius);
		painter->drawEllipse(m_p2, grip_radius, grip_radius);
		painter->restore();
	}
}

/**
	@brief CablePart::name
	@return the cable number, so the cable shows under its own name in
	every list which names items
*/
QString CablePart::name() const
{
	return m_cable ? m_cable->designation() : QString();
}

/**
	@brief CablePart::syncGeometry
	Tell the cable where this section runs. The cable is the one place
	the drawing is read back from, so nothing has to be collected from
	the folios at saving time.
*/
void CablePart::syncGeometry()
{
	if (!m_cable) return;
	m_cable->setPartGeometry(m_part_uuid, m_p1, m_p2);
}

/**
	@brief CablePart::mousePressEvent
	While a core waits to be put down, the click belongs to that: on the
	line -- or near enough for the mouse to be pointing at it -- the core
	is left standing there, right click calls it off, and a click anywhere
	else calls it off as well and goes on doing what it does for everybody
	(left clicking on a slash or on its colour holds that core, on one of
	the two blue points at the ends it holds that end, anywhere else on
	the line it moves the line).

	Otherwise: left clicking on a slash or on its colour holds that core:
	it can be dragged onto another conductor to give it that conductor.

	On one of the two blue points at the ends of a picked line it holds
	that end instead, so the line can be made longer or shorter the same
	way an end of a conductor is pulled (mouseMoveEvent).

	Left clicking anywhere else on the line moves the line.
	@param event
*/
void CablePart::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
		//A core which waits to be put down takes the whole click: on
		//the line it is left standing there, anywhere else -- and on
		//the right button, which cancels everywhere on the sheet --
		//nothing is put down. The click itself goes on doing whatever
		//it does for everybody else.
	if (m_pending_core >= 0)
	{
		if (!m_cable)
		{
			cancelPlacement();
			event->ignore();
			return;
		}
			//Where he aims is read off the click itself as well, so a
			//dialog closing with the mouse already over the line needs
			//no move first and a stale position is never taken for a
			//place he never pointed at.
		m_drag_scene = placePosition(event->scenePos());
		m_pending_valid = QLineF(event->scenePos(), m_drag_scene).length()
						  <= place_grab;

		if (event->button() == Qt::LeftButton && m_pending_valid)
		{
			const int core = m_pending_core;
			const QPointF at = m_drag_scene;
			cancelPlacement();
			m_just_placed = placeCore(core, at);
			update();
			event->accept();
			return;
		}
		const bool cancel = (event->button() == Qt::RightButton);
		cancelPlacement();
		if (cancel) {
			event->accept();
		} else {
			event->ignore();
		}
		return;
	}

		//The second of two quick clicks arrives as a double click: by
		//then the first one has already put a core down, and that must
		//not read as a request to open the dialog a second time.
	m_just_placed = false;

	if (event->button() != Qt::LeftButton) {
		QetGraphicsItem::mousePressEvent(event);
		return;
	}

	m_press_scene = event->scenePos();
	m_move_p1 = m_p1;
	m_move_p2 = m_p2;
	m_line_moved = false;
	m_line_drag_cores.clear();
		//Nothing is carried along until he picks up a whole line: a
		//press on a colour or on one of the blue points works on that
		//one thing and leaves every other line standing.
	m_carried.clear();
		//Where every colour label stands when the gesture starts: a
		//line which is moved carries all of them along with it, and
		//undo has to know where they were before that. A line which is
		//stretched does not touch them, and records the same set
		//unchanged.
	m_cores_before = m_cable ? m_cable->usedCores() : QList<CableCore>();
		//Whether the sheet has already been told that the rest of the
		//selection comes along with this line (see carrySelection): every
		//press starts from nothing, so the next gesture tells it again.
	m_mover_started = false;
	m_told_mover = QPointF();

		//One of the two blue points at the ends, when the line is
		//picked and only one of the two is under the finger: pulling
		//it changes how long the line is. The line comes before
		//whatever stands on it: a colour label parked right at the end
		//may not take that point away, since the line could not be
		//made longer from that end any more -- the labels would have
		//to be pushed aside first, and there is nothing to push them
		//with. The label is still picked up beside the point, by its
		//letters or by its slash a little further along the line.
		//When both ends are that close, which only happens on a very
		//short line, nothing is stretched: it is moved like any other
		//line instead.
	int grip = 0;
	if (isSelected())
	{
		const bool first = QLineF(event->scenePos(), m_p1).length() <= grip_grab;
		const bool second = QLineF(event->scenePos(), m_p2).length() <= grip_grab;
		if (first != second) grip = first ? 1 : 2;
	}
	m_stretch_end = grip;

	if (grip > 0)
	{
		update();
		event->accept();
		return;
	}

	const int core = coreAt(event->scenePos());
	if (core >= 0)
	{
		m_dragged_core = core;
			//The label stays exactly where it stands until the mouse
			//really moves: picking it up must not shift it by a hair.
		m_drag_scene = m_cable->core(core).position;
		if (!isSelected()) {
			setSelected(true);
		}
		update();
		event->accept();
		return;
	}

	QetGraphicsItem::mousePressEvent(event);

		//Every other line he has picked up at the same time: dragging
		//one line takes the whole selection with it, and all of them go
		//back to where they stood if he calls the gesture off. The
		//selection is read after the click has been dealt with, since
		//clicking an unselected line leaves that one alone selected.
	m_carried.clear();
	if (isSelected() && scene())
	{
		const QList<QGraphicsItem *> selected = scene()->selectedItems();
		for (QGraphicsItem *item : selected)
		{
			auto *other = qgraphicsitem_cast<CablePart *>(item);
			if (!other || other == this || !other->m_cable) continue;

			CarriedLine carried;
			carried.part = other;
			carried.p1 = other->m_p1;
			carried.p2 = other->m_p2;
			carried.cores = other->m_cable->usedCores();
			m_carried.append(carried);
		}
	}
}

/**
	@brief CablePart::mouseMoveEvent
	Three things the mouse can carry: the colour of a core (which stays
	on its line and never past either end of it), one of the two blue
	points at the ends (which makes the line longer or shorter -- that
	changes nothing else: every colour label goes on standing exactly
	where the user put it, and the line may not be brought in past the
	outermost of those labels, which would otherwise be left standing
	beyond the end of their own line), and the whole line (which is
	translated as a piece, taking its colour labels with it). The line
	in each case is snapped to the grid.
	@param event
*/
void CablePart::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
		//Where a core waiting to be put down would stand: on the line
		//itself, at a grid step, and drawn from the very first moment
		//it is asked for -- how near the mouse stands decides nothing
		//but whether a click keeps it or sends it away (see
		//mousePressEvent), never whether he can see it.
	if (m_pending_core >= 0)
	{
		if (!m_cable) {
			cancelPlacement();
		} else {
			m_drag_scene = placePosition(event->scenePos());
			update();
		}
		event->accept();
		return;
	}

	if (m_dragged_core >= 0)
	{
		m_drag_scene = dragPosition(event->scenePos());
		update();
		event->accept();
		return;
	}

	if (m_stretch_end > 0 && (event->buttons() & Qt::LeftButton))
	{
			//The end under the finger follows the mouse, snapped to the
			//grid like anything else drawn on the folio, and the line
			//keeps its right angle: the direction the mouse covers the
			//most decides which way the line runs, the same way the
			//drawing tool decides it -- so pulling the end of a
			//horizontal line far enough upwards turns the line
			//vertical instead of hanging it into the air at an angle.
		const QPointF fixed = (m_stretch_end == 1) ? m_move_p2 : m_move_p1;
		QPointF moving = Diagram::snapToGrid(event->scenePos());
		const QPointF to = moving - fixed;

		bool horizontal;
		if (qAbs(to.x()) > qAbs(to.y())) horizontal = true;
		else if (qAbs(to.y()) > qAbs(to.x())) horizontal = false;
		else horizontal = trunkIsHorizontal();
		moving = horizontal ? QPointF(moving.x(), fixed.y())
							: QPointF(fixed.x(), moving.y());

			//Never shorter than his own colour labels: every one of
			//them which stood on this part of the line when the
			//gesture began has to go on standing on it, or it would be
			//left beyond the end of the line it belongs to and could
			//no longer be wired. He may pull the line out as far as he
			//likes, and he may bring either end in as far as he likes
			//up to the outermost of those labels -- which are found
			//along the line as it runs now, since the direction can
			//change from one move to the next.
		const QPointF started_from = (m_stretch_end == 1) ? m_move_p1 : m_move_p2;
		const qreal at_fixed = horizontal ? fixed.x() : fixed.y();
		const qreal at_start = horizontal ? started_from.x() : started_from.y();
		const qreal first = qMin(at_fixed, at_start);
		const qreal last = qMax(at_fixed, at_start);

		bool any_label = false;
		qreal low = at_fixed, high = at_fixed;
		for (const CableCore &core : m_cores_before)
		{
			if (core.part != m_part_uuid) continue;

			const qreal at = horizontal ? core.position.x() : core.position.y();
			if (at < first || at > last) continue;

			if (!any_label) { low = high = at; any_label = true; }
			else { low = qMin(low, at); high = qMax(high, at); }
		}

		if (any_label)
		{
				//The end sits on the grid, so the limit it stops
				//against is taken from it on the far side of the
				//label, never in front of it.
			QSettings settings;
			const int step = horizontal
				? settings.value(QStringLiteral("diagrameditor/Xgrid"),
								 Diagram::xGrid).toInt()
				: settings.value(QStringLiteral("diagrameditor/Ygrid"),
								 Diagram::yGrid).toInt();
			if (step > 0) {
				high = std::ceil(high / step) * step;
				low = std::floor(low / step) * step;
			}

			qreal at_end = horizontal ? moving.x() : moving.y();
			at_end = at_end > at_fixed ? qMax(at_end, high)
									   : qMin(at_end, low);
			moving = horizontal ? QPointF(at_end, moving.y())
								: QPointF(moving.x(), at_end);
		}

		const QPointF p1 = (m_stretch_end == 1) ? moving : fixed;
		const QPointF p2 = (m_stretch_end == 1) ? fixed : moving;

		if (p1 != m_p1 || p2 != m_p2)
		{
			setLine(p1, p2);
			m_line_moved = true;
				//Only the line is stretched: every colour label goes on
				//standing exactly where the user left it, none is
				//carried anywhere and none is worked out again. What
				//the drawing showed of them before the gesture goes on
				//being what the cable holds -- which is what the
				//release below records, along with the line.
			m_line_drag_cores = m_cores_before;
			update();
		}
		event->accept();
		return;
	}

	if (isSelected() && event->buttons() & Qt::LeftButton)
	{
			//The line follows the mouse, snapped to the grid like anything
			//else drawn on the folio, and keeps its length: it is purely
			//optical, so nothing else has to follow it.
		const QPointF target = Diagram::snapToGrid(
			m_move_p1 + (event->scenePos() - m_press_scene));
		const QPointF delta = target - m_move_p1;

		if (target != m_p1 || m_move_p2 + delta != m_p2)
		{
			setLine(target, m_move_p2 + delta);
			m_line_moved = true;
				//Where every colour label stands on the line at this
				//moment: carried along by however far the line has
				//come, so line, description and colours move as one
				//piece and no label ever jumps or disappears.
			m_line_drag_cores = m_cores_before;
			reconcileCores(m_line_drag_cores, delta);
			update();
		}
			//Every other line he picked up with this one comes along,
			//on every step of the gesture and with the same step: a
			//selection of lines is one thing to move. It happens even
			//when the line under his finger itself stands still -- a
			//pull back to where it began -- or those would be left
			//standing out where the gesture reached.
		carryCompanions(delta);
			//Everything else he marked at the same time comes along too
			//-- the elements and the texts of the same selection -- so
			//that one gesture moves all of it rather than two halves of
			//the selection going different ways.
		carrySelection(delta);
		event->accept();
		return;
	}

	QetGraphicsItem::mouseMoveEvent(event);
}

/**
	@brief CablePart::mouseReleaseEvent
	@param event
*/
void CablePart::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
		//Nothing has been put down by this gesture, so nothing has to
		//be settled either: the core is placed by the press itself.
	if (m_pending_core >= 0)
	{
		event->accept();
		return;
	}


	if (m_dragged_core >= 0 && event->button() == Qt::LeftButton)
	{
		const int core = m_dragged_core;
		m_dragged_core = -1;

			//What was drawn while he dragged is what is kept: the label
			//goes on standing where he left it, and letting go can never
			//take it back to where it was picked up. A press which never
			//went anywhere is a click: it keeps the label exactly where
			//it already stands and never makes it name some other wire
			//for having been touched.
		const CableCore held = m_cable ? m_cable->core(core) : CableCore();
		const bool moved_label = (held.core == core) && (m_drag_scene != held.position);

		if (moved_label) {
			rebindCore(core, m_drag_scene);
		}
		update();
		event->accept();
		return;
	}

		//The steps of this line, held back until the gesture is over:
		//when the sheet is moving elements together with the lines, its
		//own undo waits for them so that one single undo step takes all
		//of it back.
	QList<QUndoCommand *> pending;
	bool cancelled = false;

	if (event->button() == Qt::LeftButton && m_line_moved && diagram())
	{
		const QPointF after_p1 = m_p1;
		const QPointF after_p2 = m_p2;
			//One undo step for one gesture: taking the drag back puts
			//every line of it exactly where it started -- a line which
			//moved takes its colour labels back with it, since they are
			//written on that line, and one which was stretched leaves
			//them exactly where he put them.
		if (after_p1 != m_move_p1 || after_p2 != m_move_p2)
		{
			QList<CableCore> after = m_line_drag_cores;

				//Stretching only changes how long the line is: no
				//colour label has been carried anywhere, so nothing of
				//the cable has to be worked out again, no wire changes
				//hands and nothing at all is asked. One step which puts
				//the line -- and only the line -- back where it was.
			if (m_stretch_end > 0)
			{
				pending.append(
					new MoveCablePartCommand(m_cable, this,
											 m_move_p1, m_move_p2,
											 after_p1, after_p2,
											 m_cores_before, after));
			}
			else
			{
					//The lines he picked up with this one settle first,
					//one after the other: a wire one of them lets go of
					//is then already free for the next one instead of
					//being asked about as if it were still taken. Each
					//of them asks before taking anything away from a
					//cable which is not part of this gesture.
				QList<QUndoCommand *> steps;
				cancelled = !settleCompanions(steps);

					//What the drawing already showed during the drag goes
					//into the cable now -- and which wire each core names
					//is worked out at this moment, and only at it, from
					//where its colour label has come to rest: that is
					//what makes the entries of the wires right again
					//after a move, instead of leaving them standing on
					//the wires the line has gone away from. No label is
					//moved for it.
				if (!cancelled)
				{
					readyToSettle(after);
					cancelled = !settleCores(after);
				}

				if (cancelled)
				{
						//He called the whole gesture off: every line of
						//it goes back to where he picked it up, nothing
						//at all is written, and there is no step to undo
						//-- the drawing is as it was before he started.
					putCompanionsBack();
					if (m_cable) {
						m_cable->setCores(m_cores_before);
					}
					setLine(m_move_p1, m_move_p2);
					for (QUndoCommand *step : steps) {
						delete step;
					}
				}
				else
				{
					if (m_cable) {
						m_cable->setCores(after);
					}
					steps.append(new MoveCablePartCommand(m_cable, this,
														 m_move_p1, m_move_p2,
														 after_p1, after_p2,
														 m_cores_before, after));
						//Given to the whole gesture rather than pushed
						//here, since the sheet may still have the rest
						//of the selection to write down with it.
					pending = steps;
					steps.clear();
				}
			}
		}
	}

		//The lines he picked up with this one go on standing where the
		//gesture left them, and none of them is carried any more --
		//whether it was written down or called off.
	for (const CarriedLine &carried : std::as_const(m_carried))
	{
		if (!carried.part) continue;
		carried.part->m_line_moved = false;
		carried.part->m_line_drag_cores.clear();
		carried.part->update();
	}
	m_carried.clear();

	m_line_moved = false;
	m_stretch_end = 0;
	m_line_drag_cores.clear();

		//One step for one gesture, however mixed the selection is: when
		//the sheet is moving the elements he marked together with these
		//lines, its own undo waits for the steps above and pushes
		//everything at once, so a single undo takes the whole selection
		//back. When nothing else was marked they are pushed here exactly
		//as they always were -- and he called the gesture off when
		//nothing at all is written, the elements included.
	if (diagram())
	{
		ElementsMover &mover = diagram()->elementsMover();
		if (cancelled)
		{
			qDeleteAll(pending);
			pending.clear();
			mover.cancelMovement();
		}
		else if (!pending.isEmpty() && !mover.isReady())
		{
			mover.addExtraCommands(pending);
			pending.clear();
		}
		else if (pending.size() == 1)
		{
			diagram()->undoStack().push(pending.takeFirst());
		}
		else if (!pending.isEmpty())
		{
				//One step however many lines he moved: taking the
				//gesture back has to take all of it back, or half the
				//selection would stay where the drag left it.
			diagram()->undoStack().push(new BatchCommand(
				pending,
				QCoreApplication::translate(
					"BatchCommand", "Déplacer %n câble(s)",
					"Un geste déplace plusieurs lignes à la fois.",
					pending.size())));
			pending.clear();
		}
	}
	else
	{
		qDeleteAll(pending);
		pending.clear();
	}

	QetGraphicsItem::mouseReleaseEvent(event);

		//The sheet has been told everything this gesture had to tell it:
		//the next press starts from nothing again (see carrySelection).
	m_mover_started = false;
	m_told_mover = QPointF();
}

/**
	@brief CablePart::mouseDoubleClickEvent
	Opening the cable is what a double click on the line is for. On one
	of the colour labels it does nothing: they are part of a core, and
	the user asking for the whole cable by clicking twice in the same
	place never said so. On a line which points at another folio it goes
	to that folio instead: the reference leads somewhere, and asking
	twice in it is asking to be taken there.
	@param event
*/
void CablePart::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
		//The click which put a core down is reported once more as a
		//double click when he clicks twice fast: it was there to place
		//something and must not go on to open the dialog.
	if (m_just_placed)
	{
		m_just_placed = false;
		event->accept();
		return;
	}

	if (event->button() == Qt::LeftButton && coreAt(event->scenePos()) >= 0)
	{
		event->accept();
		return;
	}

		//A line which points at another folio leads there: asking twice
		//in that very line is asking to be taken to the section standing
		//over there, the way a cross reference of an element does, rather
		//than to open this cable.
	if (event->button() == Qt::LeftButton && goToCrossRef(event->scenePos()))
	{
		event->accept();
		return;
	}
	QetGraphicsItem::mouseDoubleClickEvent(event);
}

/**
	@brief CablePart::hoverMoveEvent
	The hand over a line which points at another folio: it says the line
	leads somewhere before he clicks and finds out. While a core waits to
	be put down the cross of that gesture is showing, and it stays.
	@param event
*/
void CablePart::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	QetGraphicsItem::hoverMoveEvent(event);

	if (m_pending_core >= 0) return;

	if (crossRefAt(event->scenePos()) >= 0) {
		setCursor(Qt::PointingHandCursor);
	} else {
		unsetCursor();
	}
}

/**
	@brief CablePart::rebindCore
	Let go of a core the user dragged: it lands where he let it go, and
	whether it takes the wire standing there is decided by what he did
	rather than by anything being looked up for him.

	Nothing here ever moves the label to reach a wire. It goes on
	standing at the place he dropped it -- on the line, at the grid,
	the very place it has been drawn for every moment of the drag -- so
	that a label never crawls back to the wire he pulled it away from
	and never goes off to look for one of its own. What changes, if
	anything, is which wire names it:

	- On a wire this line runs across: the core takes it, and the
	  field of the wire it left is emptied by the refresh which
	  follows.
	- On a wire another cable holds: he is asked about it first. Yes
	  hands the wire over, and the other cable's core becomes a free
	  slot of that cable -- its slash and its colour leave the sheet
	  with it, which is what he saw happen. No leaves the other cable
	  exactly as it was and makes this core the free one instead: it
	  names no wire at all any more, so its own slash and colour go off
	  the sheet as well, its entry leaves the wire it came from, and it
	  waits as a free slot in the list of existing cables until he puts
	  it back.
	- Directly on the colour label of another core of the same cable:
	  the two exchange their wires, which is a deliberate exchange --
	  he put one colour on top of the other, so he means the second one
	  to take what the first is giving up.
	- On a wire another core of this same cable already holds, anywhere
	  but on its label: nothing changes.
	- On nothing at all: the label stays exactly where he dropped it,
	  and it stops naming a wire -- an entry follows its label and
	  stops where its label stopped, so that no entry is left standing
	  on a wire the label has gone away from. A label which has not
	  been moved keeps what it had, though: picking one up and letting
	  it go again leaves the drawing exactly as it was.

	Only the label he dragged may move: no other core of this cable is
	hunted down. A core is taken away from its own cable at one single
	place -- when he says no to the wire he has dropped its label on,
	so that it names nothing -- and that is the very thing the other
	cable's core becomes when he says yes to taking its wire. Both
	answers therefore leave the drawing the same way: a free slot, no
	slash, no colour, and the entry gone from the wire it stood on.

	Whatever comes out of it is kept as one undo step, the whole set of
	assignments before and after the gesture.
	@param core which core is being moved
	@param dropped_at scene coordinates where the slash was let go
	@param before_cores the set of assignments as it stood before the
	core was added to the cable, when a core is being put down rather
	than moved; nullptr otherwise
*/
bool CablePart::rebindCore(int core, const QPointF &dropped_at,
						   const QList<CableCore> *before_cores)
{
	if (!m_cable || !diagram() || !diagram()->project()) {
		return true;
	}

	const CableCore moved = m_cable->core(core);
	if (moved.core != core) {
		return true;
	}

		//What the cable held before this gesture, so undo can put it
		//back: the set as it stands while a core already on the sheet
		//is merely dragged, or -- when a core is being put down for the
		//first time -- the set from before it was added, which takes
		//the addition back as well.
	const QList<CableCore> before = before_cores ? *before_cores
												  : m_cable->usedCores();

		//Where the label has actually been drawn: on the line, at whole
		//grid steps from where it stood. It is taken as it is and never
		//worked out again, so letting go cannot shift it by a hair.
	const QPointF at = dropped_at;
	CableCore landed = moved;
	landed.position = at;

		//Which wire of this line stands at that very place, worked out
		//from where the label stands and never the other way round:
		//the label is not sent off to look for a wire, it names the
		//one it happens to be standing on.
	const QList<CableCrossing> crossings = CableManager::crossedConductors(
		diagram(), QLineF(m_p1, m_p2));
	Conductor *target = wireFor(crossings, moved, at);

		//Set when he refuses a wire another cable holds: this core then
		//names no wire at all any more and goes back to being a free
		//slot of the cable, so it is taken out rather than written.
	bool freed = false;

	if (target)
	{
			//Which core of this cable, if any, already stands on that
			//wire -- everything but the one being dragged.
		int holder = -1;
		for (const CableCore &other : m_cable->usedCores())
		{
			if (other.core == core || other.part != m_part_uuid) continue;
			if (other.conductor == target->uuid()) {
				holder = other.core;
			}
		}

		if (holder < 0)
		{
			const ClaimAnswer answer = mayClaim(target);
			if (answer == ClaimAnswer::Cancel)
			{
					//He called the drop off: the label goes on standing
					//where he picked it up, and nothing is written at all.
				return false;
			}
			if (answer == ClaimAnswer::Take)
			{
					//A wire is one piece of wire: whichever cable held it
					//before has to let it go.
				CableManager::claimConductor(diagram()->project(), target, m_cable.data());

					//A conductor keeps the identity a cable binds it by, even
					//when it was read back from a file written before it had
					//one.
				target->setUuid(target->uuid());

				landed.conductor = target->uuid();
			}
			else
			{
					//He said no: the other cable goes on holding that
					//wire, and this label has come to rest on it anyway.
					//So this core names no wire at all any more and goes
					//back to being a free slot of this cable -- exactly
					//what the other cable's core becomes when he says
					//yes -- taking its slash and its colour off the sheet
					//with it, while its entry leaves the wire the label
					//has gone away from. Only the other cable keeps its
					//wire, and he can put this core back wherever he
					//likes from the list of existing cables.
				freed = true;
			}
		}
		else if (labelAt(holder, at))
		{
				//One colour dropped straight onto another: the two
				//exchange their wires, so neither is left standing on
				//a wire the drawing does not name with it. The dragged
				//label stays where he put it, on the wire it has just
				//taken, and the other one comes to the place it gives
				//up -- that exchange is what he asked for.
			CableCore swapped = m_cable->core(holder);
			if (swapped.core == holder)
			{
				CableManager::claimConductor(diagram()->project(), target, m_cable.data());
				target->setUuid(target->uuid());

				landed.conductor = target->uuid();

				swapped.conductor = moved.conductor;
				swapped.position = moved.position;
				m_cable->setCore(swapped);
			}
		}
			//Held by another core of this cable but not dropped on its
			//label: leave everything alone, the dragged label merely
			//stays where it was let go.
	}
	else if (at != moved.position && !moved.conductor.isNull())
	{
			//He let go where this line runs across nothing, having
			//really brought the label there: the entry of the wire it
			//came from goes with it, while the label itself stays
			//exactly where he put it and merely names no wire at all
			//any more. Not moved means not touched at all, so picking
			//a label up and letting it go again never empties
			//anything.
		landed.conductor = QUuid();
	}

	if (freed)
	{
			//No wire names this core any more: it goes back to being a
			//free slot of the cable and stops being drawn, and the
			//refresh which follows empties the cable field of the wire
			//it used to name -- an entry follows its label, and this
			//label has come to rest where it may not name anything.
		m_cable->removeCore(core);
	}
	else
	{

		if (landed != moved) {
			m_cable->setCore(landed);
		}
	}

	const QList<CableCore> after = m_cable->usedCores();
	if (after != before) {
			//A core which has just been put down for the first time is
			//not being moved, and the step which takes it back is named
			//for what it did rather than for where it went.
		const QString text = before_cores
				? QCoreApplication::translate("ChangeCableCoresCommand",
											  "Placer une âme de câble")
				: QString();
		diagram()->undoStack().push(
			new ChangeCableCoresCommand(m_cable, before, after, text));
	}
	return true;
}

/**
	@brief CablePart::takeCoreOffLine
	Take one core off this very line: its colour label disappears from
	the sheet and the core goes back to being a free slot of its cable,
	listed among the ones which stand nowhere -- the exact opposite of
	putting one down from that list.

	Only a core standing on this line is taken off, never one of the
	other sections of the same cable: a cable running over two folios
	stands on two lines, and the one whose menu was asked for is the one
	he is working on.

	Whatever it named goes with it: the refresh which follows empties
	the cable field of the wire it used to name, since an entry follows
	the label it stands for, and this label no longer stands anywhere.

	One undo step, and a single undo puts the label back exactly where
	it stood.
	@param core which core standing on this line is taken off
	@return true when a core was taken off
*/
bool CablePart::takeCoreOffLine(int core)
{
	if (!m_cable || !diagram() || !diagram()->project()) return false;

	const CableCore there = m_cable->core(core);
	if (there.core != core || there.part != m_part_uuid) return false;


	const QList<CableCore> before = m_cable->usedCores();
	m_cable->removeCore(core);
	const QList<CableCore> after = m_cable->usedCores();
	if (after == before) return false;

	diagram()->undoStack().push(new ChangeCableCoresCommand(
		m_cable, before, after,
		QCoreApplication::translate("ChangeCableCoresCommand",
									"Retirer une âme de la ligne")));
	return true;
}

/**
	@brief CablePart::reconcileCores
	Where the colour labels of this line stand while the line is being
	moved: exactly where they stood, taken along by however far the line
	has come.

	Every label is carried along on every single step of the gesture --
	the trunk line, the description above it and the colour labels of the
	cores are one thing and move as one thing. Nothing is worked out
	again while he drags, so no label ever jumps, ever lands on a
	neighbour's wire or ever disappears because the line came a little
	too short: pulling the line back puts every label exactly back where
	it was.

	The place a label belongs to is worked out afterwards, when the
	line is let go (settleCores), and while a single core is moved on
	its own -- gestures which are about one wire rather than about the
	whole line, and which read the wire off the place of the label
	rather than sending the label off to find one.
	@param cores the whole set of assignments, changed in place
	@param delta how far the line has come in this gesture
*/
void CablePart::reconcileCores(QList<CableCore> &cores, const QPointF &delta) const
{
	for (CableCore &core : cores)
	{
		if (core.part != m_part_uuid) continue;

		core.position += delta;
	}
}

/**
	@brief CablePart::carryCompanions
	Every other line he picked up with this one comes along, with the
	same step and with the colours of its own cores: a selection of
	lines is one thing to move.

	Nothing of them is written while he drags -- they are drawn where
	the gesture has brought them, exactly the way this line draws its
	own labels, so pulling back puts every one of them back where it
	was and letting go works the entries out afterwards.
	@param delta how far the line under his finger has come
*/
void CablePart::carryCompanions(const QPointF &delta)
{
	for (const CarriedLine &carried : std::as_const(m_carried))
	{
		CablePart *other = carried.part;
		if (!other) continue;

		other->setLine(carried.p1 + delta, carried.p2 + delta);
		other->m_line_moved = true;
		other->m_line_drag_cores = carried.cores;
		other->reconcileCores(other->m_line_drag_cores, delta);
		other->update();
	}
}

/**
	@brief CablePart::carrySelection
	Everything else he marked at the same time comes along too: the
	elements, the texts and the images of the same selection are moved
	by the very step this line has taken, so that one gesture moves
	whatever the selection is made of instead of two halves of it going
	different ways -- the sheet has its own way of moving those, and
	this is the only place which knows how far the lines have come.

	The step handed over is the part of the gesture the sheet has not
	been told yet: what this line measures is how far it has come all in
	altogether, from where the press found it, while the sheet takes one
	step at a time.
	@param total_delta how far this line has come since the press
*/
void CablePart::carrySelection(const QPointF &total_delta)
{
	if (!diagram()) return;

	ElementsMover &mover = diagram()->elementsMover();
	if (!m_mover_started)
	{
			//Once per gesture rather than once per step: when nothing
			//but lines is marked the sheet has nothing to move of its
			//own, and must not be asked to start over at every step.
		m_mover_started = true;
		m_told_mover = QPointF();
		mover.beginMovement(diagram(), this);
	}

	const QPointF step = total_delta - m_told_mover;
	m_told_mover = total_delta;
	if (!step.isNull()) {
		mover.continueMovement(step);
	}
}

/**
	@brief CablePart::settleCompanions
	Where every other line of this gesture comes to rest, worked out
	exactly the way this line's own is: from where its colour labels
	have come to stand, asking before anything changes hands.

	The lines are settled one after the other rather than all at once,
	so that a wire one of them lets go of is already free for the next
	one instead of being asked about as if it were still taken.
	@param steps receives one undo step per line he let go of
	@return false when he cancelled one of their questions, in which
	case nothing at all has to be written and the caller puts every
	line back where it was picked up
*/
bool CablePart::settleCompanions(QList<QUndoCommand *> &steps)
{
	for (const CarriedLine &carried : std::as_const(m_carried))
	{
		CablePart *other = carried.part;
		if (!other || !other->m_cable) continue;

		QList<CableCore> after = other->m_line_drag_cores;
		other->readyToSettle(after);

		if (!other->settleCores(after)) {
			return false;
		}

		other->m_cable->setCores(after);
		steps.append(new MoveCablePartCommand(other->m_cable, other,
											  carried.p1, carried.p2,
											  other->m_p1, other->m_p2,
											  carried.cores, after));
	}
	return true;
}

/**
	@brief CablePart::putCompanionsBack
	Put every other line of this gesture back to where he picked it
	up, cores included: the wire fields are rewritten along the way,
	so that not one entry of the refused move is left standing.
*/
void CablePart::putCompanionsBack()
{
	for (const CarriedLine &carried : std::as_const(m_carried))
	{
		CablePart *other = carried.part;
		if (!other) continue;

		other->setLine(carried.p1, carried.p2);
		if (other->m_cable) {
			other->m_cable->setCores(carried.cores);
		}
		other->m_line_moved = false;
		other->m_line_drag_cores.clear();
		other->update();
	}
}

/**
	@brief CablePart::beginLineGesture
	Where this line stood when the sheet began moving the selection it
	is part of, kept here so that every step of that movement can carry
	the line -- and its colour labels, which are written on it -- along
	with it, exactly as if the finger had been on the line itself.

	Nothing is carried along by this line: the sheet carries every line
	of the selection itself, one after the other, so no line takes its
	neighbours along a second time.
*/
void CablePart::beginLineGesture()
{
	m_move_p1 = m_p1;
	m_move_p2 = m_p2;
	m_line_moved = false;
	m_line_drag_cores.clear();
	m_carried.clear();
	m_cores_before = m_cable ? m_cable->usedCores() : QList<CableCore>();
		//Where its own labels stand when the gesture begins: they go on
		//standing wherever the line is carried to, and undo has to know
		//where they were before that.
	m_line_drag_cores = m_cores_before;
	m_stretch_end = 0;
	m_dragged_core = -1;
}

/**
	@brief CablePart::continueLineGesture
	Take one more step of a move the sheet is driving: the line follows
	by that much and every colour label standing on it comes along with
	it, so line, description and colours move as one piece and no label
	ever jumps, lands on a neighbour's wire or disappears under his
	hand.
	@param delta how far to come, in scene units
*/
void CablePart::continueLineGesture(const QPointF &delta)
{
	if (!m_cable || delta.isNull()) return;

	setLine(m_p1 + delta, m_p2 + delta);
	m_line_moved = true;
		//Step by step rather than measured from where the gesture
		//began: the sheet hands one step over at a time
	reconcileCores(m_line_drag_cores, delta);
	update();
}

/**
	@brief CablePart::settleLineGesture
	Where this line comes to rest, worked out the same way as after a
	drag of its own: which wire every colour label has come to stand on,
	asked before anything changes hands, and no label moved for it.

	The step is handed over rather than pushed, so that the movement of
	the whole selection ends up as one single undo step.
	@param steps receives the undo step of this line, if any
	@return false when he called the gesture off -- this line is already
	back where it began then, and the sheet puts the rest back too
*/
bool CablePart::settleLineGesture(QList<QUndoCommand *> &steps)
{
	if (!m_line_moved) return true;
		//Back where it began: nothing was carried anywhere, so there is
		//nothing to work out and nothing to undo
	if (m_p1 == m_move_p1 && m_p2 == m_move_p2) return true;

	if (!m_cable || !diagram())
	{
		abortLineGesture();
		return true;
	}

	QList<CableCore> after = m_line_drag_cores;
	readyToSettle(after);

	if (!settleCores(after))
	{
		abortLineGesture();
		return false;
	}

	m_cable->setCores(after);
	steps.append(new MoveCablePartCommand(m_cable, this,
										  m_move_p1, m_move_p2,
										  m_p1, m_p2,
										  m_cores_before, after));
	return true;
}

/**
	@brief CablePart::abortLineGesture
	Put this line back where the gesture found it: the line goes back
	to its place and its cable gets the labels it held at that moment,
	whether they were written down during this gesture or not.
*/
void CablePart::abortLineGesture()
{
	if (m_line_moved)
	{
		if (m_cable) {
			m_cable->setCores(m_cores_before);
		}
		setLine(m_move_p1, m_move_p2);
		update();
	}
	m_line_moved = false;
	m_line_drag_cores.clear();
	m_cores_before.clear();
}

/**
	@brief CablePart::doneLineGesture
	The gesture is over for this line: forget where it began, the step
	which was worked out from it having been handed over already.
*/
void CablePart::doneLineGesture()
{
	m_line_moved = false;
	m_line_drag_cores.clear();
	m_cores_before.clear();
	m_stretch_end = 0;
	m_dragged_core = -1;
}

/**
	@brief CablePart::readyToSettle
	Make a set of carried cores ready to be written into the cable:
	this line's own labels wherever the gesture has brought them, and
	every other section of the cable exactly as the cable holds it at
	this moment.

	The other sections are read back rather than carried, because
	another line of this very gesture may have just settled and
	written them -- and a core which is no longer there has just been
	taken by another cable of the gesture, which makes it that
	cable's free slot rather than one of ours.
	@param cores the carried set, corrected in place
*/
void CablePart::readyToSettle(QList<CableCore> &cores) const
{
	if (!m_cable) return;

	for (int i = cores.size() - 1; i >= 0; --i)
	{
		const bool mine = cores.at(i).part == m_part_uuid;
		if (m_cable->hasCore(cores.at(i).core)) {
			if (!mine) {
				cores[i] = m_cable->core(cores.at(i).core);
			}
		} else {
			cores.removeAt(i);
		}
	}
}

/**
	@brief CablePart::wireFor
	Which wire this line runs across at the very place a colour label
	stands, so that the entry of a wire follows the label instead of
	the label being sent off to find a wire.

	The nearest crossing which is close enough wins, and where two are
	as close as that, the wire the core already names wins the draw: a
	label standing between two crossings goes on naming the one it had
	rather than sliding onto the other, while a label dropped straight
	onto another wire does take that one. Close enough means close
	enough for the slash and the wire to read as one thing -- and
	nothing at all that close means no wire: a label is never moved to
	reach one, it keeps whatever it already names and stands where the
	user left it.
	@param crossings every place this line runs across a wire
	@param core the core being worked out
	@param at where its label stands, in scene coordinates
	@return the wire to give the core, or nullptr
*/
Conductor *CablePart::wireFor(const QList<CableCrossing> &crossings,
							   const CableCore &core, const QPointF &at) const
{
	const CableCrossing *nearest = nullptr;
	const qreal tolerance = CableManager::bind_tolerance;
	qreal nearest_distance = tolerance;

	for (const CableCrossing &crossing : crossings)
	{
		if (!crossing.conductor) continue;

		const qreal distance = QLineF(crossing.position, at).length();
		if (distance > tolerance) continue;
		if (nearest && distance >= nearest_distance) {
				//Where it already stands wins a draw, so that a label
				//between two crossings never slides from one to the other
			if (distance > nearest_distance
				|| crossing.conductor->uuid() != core.conductor) {
				continue;
			}
		}

		nearest = &crossing;
		nearest_distance = distance;
	}
	return nearest ? nearest->conductor : nullptr;
}

/**
	@brief CablePart::askClaim
	The question every takeover is asked with: three answers, in
	French, of which the third one is the way out of the whole gesture
	-- he has let go in the wrong place, and would rather have nothing
	happen at all than take a wire he did not mean to take.
	@param parent the window the question is asked in
	@param title the title of the question
	@param question what is being asked
	@return what he answered
*/
CablePart::ClaimAnswer CablePart::askClaim(QWidget *parent,
											const QString &title,
											const QString &question) const
{
	QMessageBox box(QMessageBox::Question, title, question, QMessageBox::NoButton, parent);
	QPushButton *take = box.addButton(
		tr("Oui", "Prendre le conducteur à l'autre câble."),
		QMessageBox::YesRole);
	QPushButton *leave = box.addButton(
		tr("Non", "Laisser le conducteur à l'autre câble, on ne change rien."),
		QMessageBox::NoRole);
	QPushButton *cancel = box.addButton(
		tr("Annuler", "Annuler le geste entier : rien n'est écrit, la ligne et ses couleurs reviennent où elles étaient."),
		QMessageBox::RejectRole);
		//The safe answer is the one under his finger, and the escape
		//key throws the whole gesture away rather than taking a wire.
	box.setDefaultButton(leave);
	box.setEscapeButton(cancel);
	box.exec();

	const QAbstractButton *clicked = box.clickedButton();
	if (clicked == take) return ClaimAnswer::Take;
	if (clicked == leave) return ClaimAnswer::Leave;
	return ClaimAnswer::Cancel;
}

/**
	@brief CablePart::mayClaim
	Whether a wire may be given to this cable. A wire another cable
	already describes is asked about first: taking it empties the cable
	field of that other cable, and that is not something a drop or a
	movement does behind the back of the user.
	@param conductor the wire about to be taken
	@return what he answered
*/
CablePart::ClaimAnswer CablePart::mayClaim(Conductor *conductor) const
{
	if (!conductor || !m_cable || !diagram() || !diagram()->project()) {
		return ClaimAnswer::Leave;
	}

	const QUuid holder = conductor->properties().m_cable_uuid;
	if (holder.isNull() || holder == m_cable->uuid()) return ClaimAnswer::Take;

	const Cable *other = CableManager::cableByUuid(diagram()->project(), holder);
	const QString other_name = other ? other->designation() : QString();

	const QString text = other_name.isEmpty()
		? tr("Ce conducteur est déjà décrit par un autre câble. Voulez-vous l'attribuer au câble %1 ?",
			 "Un conducteur ne peut appartenir qu'à un câble : on demande avant de le prendre à l'autre.")
		: tr("Ce conducteur est déjà décrit par le câble %1. Voulez-vous l'attribuer au câble %2 ?",
			 "Un conducteur ne peut appartenir qu'à un câble : on demande avant de le prendre à l'autre.");

	QWidget *parent = nullptr;
	if (!diagram()->views().isEmpty()) {
		parent = diagram()->views().constFirst();
	}

	return askClaim(parent,
					tr("Conducteur déjà attribué", "Un conducteur ne peut appartenir qu'à un câble."),
					other_name.isEmpty() ? text.arg(m_cable->designation())
										 : text.arg(other_name, m_cable->designation()));
}

/**
	@brief CablePart::mayClaimSeveral
	The same question as mayClaim(), but asked once for a whole handful
	of conductors: pulling a line across several wires which another
	cable already describes must ask about them too, rather than either
	quietly taking them or quietly leaving them -- one question, one
	answer, and the other cable releases every wire he says yes to.
	@param conductors the wires which other cables hold
	@return what he answered
*/
CablePart::ClaimAnswer CablePart::mayClaimSeveral(const QList<Conductor *> &conductors) const
{
	if (conductors.isEmpty() || !m_cable || !diagram() || !diagram()->project()) {
		return ClaimAnswer::Leave;
	}

		//Which cables hold those wires: he is told the name of one of
		//them, and that is the one he is most likely to be looking for.
	QString other_name;
	QSet<QUuid> others;
	for (const Conductor *conductor : conductors)
	{
		const QUuid holder = conductor->properties().m_cable_uuid;
		if (holder.isNull() || holder == m_cable->uuid()) continue;
		others.insert(holder);
	}
	if (others.isEmpty()) return ClaimAnswer::Take;

	if (others.size() == 1) {
		const Cable *other = CableManager::cableByUuid(diagram()->project(), *others.constBegin());
		if (other) other_name = other->designation();
	}
		//Several other cables at once: no one name would say it all.
	const bool unnamed = (others.size() != 1) || other_name.isEmpty();

	const int n = conductors.size();
	const QString title = n <= 1
		? tr("Conducteur déjà attribué", "Un conducteur ne peut appartenir qu'à un câble.")
		: tr("Conducteurs déjà attribués", "Plusieurs conducteurs sont pris en une seule fois.", n);

	const QString text = n <= 1
		? (unnamed
			? tr("Ce conducteur est déjà décrit par un autre câble. Voulez-vous l'attribuer au câble %1 ?",
				 "Un conducteur ne peut appartenir qu'à un câble : on demande avant de le prendre à l'autre.")
			: tr("Ce conducteur est déjà décrit par le câble %1. Voulez-vous l'attribuer au câble %2 ?",
				 "Un conducteur ne peut appartenir qu'à un câble : on demande avant de le prendre à l'autre."))
		: (unnamed
			? tr("%n conducteurs sont déjà décrits par d'autres câbles. Voulez-vous les attribuer au câble %1 ?",
				 "On prend plusieurs conducteurs à la fois : on demande avant de les prendre aux autres câbles.", n)
			: tr("%n conducteurs sont déjà décrits par le câble %1. Voulez-vous les attribuer au câble %2 ?",
				 "On prend plusieurs conducteurs à la fois : on demande avant de les prendre à l'autre câble.", n));

	const QString question = unnamed
		? text.arg(m_cable->designation())
		: text.arg(other_name, m_cable->designation());

	QWidget *parent = nullptr;
	if (!diagram()->views().isEmpty()) {
		parent = diagram()->views().constFirst();
	}

	return askClaim(parent, title, question);
}

/**
	@brief CablePart::settleCores
	Which wire each core names once the line has come to rest: read
	off where the colour labels have stood themselves, so that moving a
	line as a whole updates the entries of the wires instead of leaving
	them behind on the wires it has gone away from.

	While he drags, every label is merely carried along, so nothing
	jumps, nothing lands on a neighbour's wire and nothing disappears
	because the line came a little short -- the drawing stays readable
	over the whole gesture, and pulling the line back puts every label
	exactly back where it was. Which wire each core names is worked out
	once, at the very end, and always from the place of the label: a
	label standing on a wire gives that wire to its core, and the wire
	it came from keeps no binding, so the refresh which follows empties
	its cable field. Not one label is moved here. A label which came
	to rest beside a wire rather than on it -- a whole grid step away,
	which is as far as one pull of the line can carry it -- lets go of
	that wire, because it no longer stands on it: an entry is always
	where its colour label stands, and the grid is what puts a label
	down on a crossing in the first place, so standing on that
	crossing is what counts. When the line has been drawn off into an
	empty part of the sheet, where it crosses no wire at all, every one
	of its cores lets go, so that no entry is left standing on a wire
	the label has gone away from. Pulling the line back puts every
	label on a crossing again, and with it the entries they had.

	The whole set is worked out at once, and not core after core in the
	order they happen to be stored. Otherwise a label coming to rest on
	a wire another label of this very line is standing on at this moment
	would be turned away for a core which is moving off that very wire
	in the same gesture -- and its entry would go on standing on a wire
	the line has gone away from, which is exactly the untidy entry that
	has to disappear. A label which does not move keeps its wire all the
	same, and where two labels want one wire, the first of them has it
	and the other keeps what it had.

	A core which has no wire at all yet -- a copy put down on a folio
	it was not wired on, because a copy is carried whole even where
	there is nothing to wire -- takes the wire its own label stands on,
	if there is one: pulling a copied line into place is what wires it.

	Only wires nobody else holds are taken -- and when the line comes
	to rest on wires another cable already describes, he is asked about
	them once, before anything is taken, naming the other cable and
	answering no unless he says yes. The answer covers the whole move:
	either those wires change hands and the other cable releases them,
	or none of them does.

	Whichever way he answers, a core whose label has come to rest on
	such a wire and names none of its own any more stops being a core
	of its cable: it goes back to being a free slot, so that neither its
	slash nor its colour is drawn on a wire which is not its own, and
	its entry leaves the wire the label went away from -- an entry
	following its label just as it does everywhere else, so that no
	entry is left standing where no colour label stands. Yes makes the
	other cable's core the free one instead; No makes it this cable's
	own, and the other cable keeps its wire either way. That is why
	both answers look the same on the sheet: one label standing there,
	and a free slot in the list of existing cables.

	And he may call the whole gesture off: nothing is then written at
	all, the line goes back to where he picked it up, and the drawing
	is once more what it was before he started.

	@param cores the whole set of assignments, changed in place
	@return false when he cancelled the gesture
*/
bool CablePart::settleCores(QList<CableCore> &cores) const
{
	if (!m_cable || !diagram()) return true;

	const QList<CableCrossing> crossings = CableManager::crossedConductors(
		diagram(), QLineF(m_p1, m_p2));
		//A line which runs across nothing at all is not a reason to call
		//the move off: it is the moment every core of this section lets
		//go of the wire it came from, since an entry belongs to a wire
		//the cable still touches. So the walk below happens either way,
		//and then simply comes out with nothing to keep.

		//Which wire every core ends up with is worked out for the whole
		//set before a single one of them is written down -- a dry run
		//first, which merely collects the wires of other cables on the
		//way, so that he is asked about all of them before any of them
		//changes hands.
		//
		//Working it out for the set as a whole, and not core by core in
		//the order they happen to be stored, is what makes the entries
		//come out right: a label coming to rest on a wire another label
		//of this very line is standing on at this moment must have that
		//wire, because the other label is moving off it in the same
		//gesture -- whereas a label which does not move, and keeps the
		//wire it names, must not be robbed of it.
	const auto settle_once = [this, &crossings](
			QList<CableCore> &list,
			const bool dry_run,
			QList<Conductor *> *foreign_wires,
			const bool take_foreign)
	{
			//A wire another cable already describes only changes hands
			//when he has said yes to taking it away from that one.
		const auto is_foreign = [this](Conductor *wire) {
			return wire && !wire->properties().m_cable_uuid.isNull()
				&& wire->properties().m_cable_uuid != m_cable->uuid();
		};

			//The wire each colour label of this line is standing on
			//right now -- worked out for every one of them first, so
			//that which wire a core gets does not depend on which core
			//is looked at first. None at all when a label stands on no
			//wire, and then that core lets go of whatever it named.
		QHash<int, Conductor *> wanted;
		for (const CableCore &core : list) {
			if (core.part != m_part_uuid) continue;
			wanted.insert(core.core, wireFor(crossings, core, core.position));
		}

			//Who lets go of nothing: a label standing on its own wire,
			//and every label of another part of this cable, which this
			//line never takes anything from. A label standing on no
			//wire at all is not among them: it lets go of what it
			//named, a whole grid step away from a crossing being
			//already beside that wire rather than on it.
		QSet<int> keepers;
			//Who ends up naming no wire at all: a label which has come
			//to rest on a wire another cable holds, and he has said no
			//to taking it. Such a core goes back to being a free slot of
			//this cable -- exactly what the other cable's core becomes
			//when he says yes -- so neither its slash nor its colour is
			//drawn on a wire which is not its own, and its entry leaves
			//the wire the label came from with it. The other cable keeps
			//its wire either way.
		QSet<int> refuses;
		for (const CableCore &core : list)
		{
			if (core.part != m_part_uuid) continue;
			Conductor *wire = wanted.value(core.core, nullptr);
				//Stands on no wire of this line: not a keeper, so it
				//lets go of whatever it named further down.
			if (!wire) continue;

			if (wire->uuid() == core.conductor) {
				keepers.insert(core.core);
				continue;
			}
			if (!take_foreign && is_foreign(wire)) {
				refuses.insert(core.core);
			}
		}

			//Which core ends up with which wire, over and over until it
			//stops moving: a core sent away from its own wire sends the
			//one waiting for that wire away too, and that one may in
			//turn free the wire the first one was after. The keepers
			//only ever grow, so this cannot go round for ever.
		QSet<QUuid> given_wires;
		bool grew = true;
		while (grew)
		{
			grew = false;
			given_wires.clear();

				//The wires nobody is letting go of
			QSet<QUuid> staying;
			for (const CableCore &core : list)
			{
				if (core.conductor.isNull()) continue;
				if (core.part != m_part_uuid || keepers.contains(core.core)) {
					staying.insert(core.conductor);
				}
			}

			for (const CableCore &core : list)
			{
				if (core.part != m_part_uuid
					|| keepers.contains(core.core)
					|| refuses.contains(core.core)) continue;

				Conductor *wanted_wire = wanted.value(core.core, nullptr);
				if (!wanted_wire) continue;   //stands on no wire: a keeper already

				const QUuid uuid = wanted_wire->uuid();
				if (staying.contains(uuid) || given_wires.contains(uuid))
				{
						//Another label of this line goes on standing on it,
						//or has just been given it: this one keeps the wire
						//it names now rather than taking it away from under
						//a label which has come to rest on it.
					keepers.insert(core.core);
					grew = true;
					continue;
				}
				given_wires.insert(uuid);
			}
		}

			//The whole set has come to rest; now it can be written down
			//without anything shifting under the feet of the next core.
			//Cores which name no wire at all any more are collected on
			//the way and taken out of the set only once the walk is over,
			//since a list cannot be shortened while it is being walked.
		QList<int> freed;
		for (CableCore &core : list)
		{
			if (core.part != m_part_uuid) continue;

			Conductor *wire = wanted.value(core.core, nullptr);

			if (refuses.contains(core.core))
			{
					//He said no, so this core names no wire at all any
					//more: it goes back to being a free slot of this
					//cable, exactly what the other cable's core becomes
					//when he says yes to taking its wire. Neither its
					//slash nor its colour stays standing on a wire which
					//is not its own, and its entry leaves the wire the
					//label came from with it -- while the other cable
					//keeps its wire either way.
				if (!dry_run) {
					freed << core.core;
				}
				continue;
			}

			if (!wire)
			{
					//The label has come to rest where this line crosses
					//nothing: the entry of the wire it came from goes
					//with it, while the label itself stays exactly
					//where he put it and merely names no wire at all
					//any more -- an entry follows its label, and stops
					//where its label stopped. A whole grid step away
					//from a crossing is already beside that wire
					//rather than on it, which is the case he means.
				if (!dry_run && !core.conductor.isNull()) {
					core.conductor = QUuid();
				}
				continue;
			}

			if (keepers.contains(core.core))
			{
				continue;
			}

			const QUuid uuid = wire->uuid();
				//Another cable holds this wire: it only changes hands
				//when he has said yes to taking it away from that one.
			const bool foreign = is_foreign(wire);
			if (foreign && foreign_wires) {
				foreign_wires->append(wire);
			}
			if (!dry_run) {

				wire->setUuid(uuid);
					//A wire is one piece of wire: whichever cable held it
					//before has to let it go, or its entry would stand on
					//the wire all the same.
				if (foreign) {
					CableManager::claimConductor(diagram()->project(), wire, m_cable.data());
					wire->setUuid(uuid);
				}
				core.conductor = uuid;
					//The wire this core has just left keeps no binding, so
					//the refresh which follows empties its cable field: an
					//entry follows its label, and stops where its label
					//stopped
			}
		}

			//A core which names no wire is not a core of this cable any
			//more: it goes back to being a free slot, and takes its slash
			//and its colour off the sheet with it -- the very thing the
			//other cable's core becomes when he says yes to taking its
			//wire, so that both answers behave the same way.
		for (int i = list.size() - 1; i >= 0; --i) {
			if (list.at(i).part == m_part_uuid
				&& freed.contains(list.at(i).core)) {
				list.removeAt(i);
			}
		}
	};

	QList<CableCore> trial = cores;
	QList<Conductor *> foreign_wires;
	settle_once(trial, true, &foreign_wires, true);

		//The whole move is asked about at once, and the wires of other
		//cables are taken either all together or not at all.
	ClaimAnswer answer = ClaimAnswer::Leave;
	if (!foreign_wires.isEmpty()) {
		answer = mayClaimSeveral(foreign_wires);
		if (answer == ClaimAnswer::Cancel) return false;
	}

	settle_once(cores, false, nullptr, answer == ClaimAnswer::Take);
	return true;
}

/**
	@brief CablePart::itemChange
	Fold any translation of the item into its own geometry: this item
	works in scene coordinates and keeps itself at the origin, so that
	one number -- the trunk line -- is all the cable ever has to store.

	The colour labels come along with it. A line moved as a whole --
	by a paste, by a duplicate, or together with the rest of a selection
	-- is one thing, and its labels belong to it, so they travel rather
	than staying behind at the place the line was copied from. The trunk
	line dragged by its own gesture does not come through here: it is
	given its new geometry directly, which is what lets that gesture
	carry the labels step by step and settle them only at the end.
	@param change
	@param value
	@return
*/
QVariant CablePart::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSelectedHasChanged)
	{
			//The two blue points at the ends come and go with the
			//selection, so the moment a line is picked it has to be
			//drawn again -- otherwise the points only appear at the
			//next redraw of any other kind.
		update();
	}
	if (change == ItemSceneHasChanged)
	{
			//The folio this line stands in is what its references to the
			//other folios of the cable are worked out from, and it is
			//also what has to be listened to: only now, once there is
			//one, can they be kept up to date -- and once this line
			//leaves that folio, they have to stop.
		setUpXrefHooks();
	}
	if (change == ItemPositionHasChanged)
	{
		const QPointF new_pos = value.toPointF();
		if (!new_pos.isNull())
		{
			prepareGeometryChange();
			m_p1 += new_pos;
			m_p2 += new_pos;
			if (m_cable) {
				m_cable->movePartCores(m_part_uuid, new_pos);
			}
			syncGeometry();
			update();
			QGraphicsItem::setPos(QPointF());
		}
	}
	return QetGraphicsItem::itemChange(change, value);
}

/**
	@brief CablePart::editProperty
	Open the dialog which edits the properties of the cable this line
	belongs to -- the number, the type, the installation, the place. They
	belong to the cable, so editing them here changes them for every line
	of that cable, on every folio.
*/
void CablePart::editProperty()
{
	if (!m_cable || !diagram() || diagram()->isReadOnly()) return;

		//Asking for the whole cable is another thing than putting one
		//of its cores down: anything which waited for a click stops
		//waiting -- on this line or on any other, since only one of
		//them can wait at a time -- and he can start over from this
		//dialog.
	cancelPlacing();

	CablePropertiesDialog dialog(m_cable, diagram(), this,
								 diagram()->views().isEmpty()
									 ? nullptr : diagram()->views().constFirst());
	dialog.exec();
}

/**
	@brief CablePart::startPlacingCore
	Give this line the mouse and wait for the click which puts that core
	of its cable down: the core follows the line wherever the mouse comes
	near it, drawn where the click would leave it -- on the line, at a
	grid step, and never past either end of it.

	It is always this very section which takes the core, never the first
	section of the cable: a cable running over two folios stands on two
	lines, and the one whose dialog was asked for is the one he is
	working on.
	@param core which core of the cable is being put down
*/
void CablePart::startPlacingCore(int core)
{
	if (!m_cable || !diagram() || diagram()->isReadOnly()) return;
	if (core < 0 || core >= m_cable->coreCount() || m_cable->hasCore(core)) return;
	if (m_dragged_core >= 0 || m_stretch_end > 0 || m_line_moved) return;
	if (m_pending_core == core)
	{
			//He asks again for the very core which is waiting already:
			//there is nothing to arm a second time, but the mouse still
			//goes to the line -- asking twice has to behave exactly like
			//asking once, and the mark is where he is being sent to.
		sendMouseToLine();
		return;
	}

	cancelPlacement();

		//Only one line at a time may wait for its click, and it is the
		//one whose dialog was asked for: a line standing on another
		//folio lets go as well, so Escape and the right button always
		//find the very line he is working on.
	cancelPlacing();

	m_pending_core = core;
	s_placing = this;
	m_drag_scene = m_p1;
	m_pending_valid = false;

		//Where the mouse stands at this very moment, so that a dialog
		//which happened to cover the line does not have to be closed by
		//a click the core would take for itself -- and then straight to
		//the line, which is where the next click belongs.
	sendMouseToLine();

	if (!isSelected()) {
		setSelected(true);
	}
	grabMouse();
	setCursor(Qt::CrossCursor);
	update();
}

/**
	@brief CablePart::sendMouseToLine
	Send the mouse to this line, onto the grid step the mark of the
	waiting core stands on, so that the very next click is the click
	which puts that core down: he picks the core out of a list, and the
	line may well be across the screen from where that list stands.

	The mouse goes wherever the line stands, as long as that line is
	standing in some window he can see -- there is nothing else to ask.
	It is never the mark which is held back: he asked for this core, the
	very next click is the one which puts it down, and the mouse is
	meant to be there when he clicks.
*/
void CablePart::sendMouseToLine()
{
	if (!scene() || scene()->views().isEmpty()) return;

	QGraphicsView *view = scene()->views().constFirst();
	const QPointF scene_pos = view->mapToScene(view->mapFromGlobal(QCursor::pos()));
	m_drag_scene = placePosition(scene_pos);
	m_pending_valid = QLineF(scene_pos, m_drag_scene).length() <= place_grab;
	view->setFocus();

		//The window he is actually looking at: a folio may stand in
		//more than one window, and only the one which is on screen can
		//be sent the mouse. The mark itself is drawn on the line
		//whenever he asks for the core, seen or not -- whether a click
		//keeps it or sends it away is worked out from the click itself
		//(see mousePressEvent), so nothing here may stop the mouse
		//from going where he just told it to go.
	QGraphicsView *shown = nullptr;
	for (QGraphicsView *candidate : scene()->views())
	{
		if (candidate && candidate->isVisible()) {
			shown = candidate;
			break;
		}
	}

	if (shown)
	{
		QWidget *viewport = shown->viewport();
		const QPoint there = viewport->mapFrom(shown, shown->mapFromScene(m_drag_scene));
		const QPoint target = viewport->mapToGlobal(there);
		const QPointF mark = m_drag_scene;
		QPointer<CablePart> self(this);

		QCursor::setPos(target);

			//And again a little later: a pop-up which has just closed
			//keeps the mouse in hand for a moment longer, and the system
			//only lets go once the menu is really gone from the screen.
			//A jump asked for earlier than that is dropped on the floor,
			//while one asked for afterwards always goes through -- so the
			//very same jump is asked for three times, as long as this
			//line is still the one waiting and its mark still stands on
			//that very spot.
		const int waits[] = {0, 70, 250};
		for (const int wait : waits)
		{
			QTimer::singleShot(wait, viewport, [self, target, mark]() {
				if (!self || self->m_pending_core < 0 || self->m_drag_scene != mark) return;

					//The same pop-up may have kept the mouse out of
					//this line's hands while it was closing: the line
					//waits for the click, so it has to be the one
					//holding the mouse.
				if (self->scene() && self->scene()->mouseGrabberItem() != self.data()) {
					self->grabMouse();
				}

				QCursor::setPos(target);
			});
		}
	}
	update();

}

/**
	@brief CablePart::cancelPlacement
	Stop waiting for the click which would put a core down, without
	putting anything down anywhere: the core goes back to being a free
	one in the dialog it came from.
*/
void CablePart::cancelPlacement()
{
	if (s_placing == this) s_placing = nullptr;
	if (m_pending_core < 0) return;

	m_pending_core = -1;
	m_pending_valid = false;
	ungrabMouse();
	unsetCursor();
	update();
}

/**
	@brief CablePart::placingPart
	@return the line which waits for the click putting a core down right
	now, or nullptr when no line does
*/
CablePart *CablePart::placingPart()
{
	return s_placing.data();
}

/**
	@brief CablePart::cancelPlacing
	End the wait of whichever line stands in it, without putting anything
	down anywhere: the core goes back to being a free one.
	@return true when a line was waiting for its click
*/
bool CablePart::cancelPlacing()
{
	if (!s_placing) return false;

	s_placing->cancelPlacement();
	return true;
}

/**
	@brief CablePart::placeCore
	Put a core which is not on the sheet yet onto this line, standing
	where the mouse says: on a wire this line runs across it takes that
	wire like any other label would -- including asking before taking
	one another cable holds -- and where the line runs across nothing it
	merely stands and names no wire at all.

	The core belongs to this very section, so a cable standing on two
	folios gets it on the line he asked for rather than on the first one.
	@param core which core of the cable is being put down
	@param at where its label is to stand, in scene coordinates
	@return false when nothing was put down
*/
bool CablePart::placeCore(int core, const QPointF &at)
{
	if (!m_cable || !diagram() || diagram()->isReadOnly()) return false;
	if (core < 0 || core >= m_cable->coreCount()) return false;
	if (m_cable->hasCore(core)) return false;

		//Every core the cable holds at this moment: the addition is
		//kept as one step of undo together with whatever the click
		//makes of it, so that taking it back leaves no trace.
	const QList<CableCore> before = m_cable->usedCores();

	CableCore fresh;
	fresh.core = core;
	fresh.part = m_part_uuid;
	fresh.conductor = QUuid();
	fresh.position = at;
	m_cable->setCore(fresh);

	if (!rebindCore(core, at, &before))
	{
			//He called the question off, so nothing is put down at
			//all: the core goes back to being a free one, and no step
			//of undo was written for what never happened.
		m_cable->setCores(before);
		return false;
	}
	return true;
}

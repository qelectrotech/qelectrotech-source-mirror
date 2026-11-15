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
#include "elementarticleswidget.h"

#include "dockfit.h"
#include "elementinfopartwidget.h"
#include "elementinfowidget.h"
#include "devicetypechangedialog.h"
#include "devicetypechooser.h"

#include <devicetype/devicetype.h>
#include <devicetype/devicetypeassign.h>
#include <devicetype/devicetypechange.h>
#include <devicetype/devicetypecheck.h>
#include <diagram.h>
#include <materiallist/materiallist.h>
#include <materiallist/materialselectiondialog.h>
#include <qetgraphicsitem/element.h>
#include <qetinformation.h>
#include <qetmessagebox.h>
#include <qetproject.h>
#include <undocommand/changeelementinformationcommand.h>
#include <undocommand/setdevicetypecommand.h>

#include <QApplication>
#include <QCursor>
#include <QDialog>
#include <QHash>
#include <QMenu>
#include <QPointer>
#include <QMessageBox>
#include <QDockWidget>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
	constexpr int kBlockRole = Qt::UserRole;
	constexpr int kSubtitleRole = Qt::UserRole + 1;

	/**
		@brief Draws each article list row with a bold title on line 1
		and a summary subtitle on line 2.
	*/
	class ArticleListDelegate : public QStyledItemDelegate
	{
		public:
			explicit ArticleListDelegate(QObject *parent = nullptr) :
				QStyledItemDelegate(parent)
			{}

			QSize sizeHint(const QStyleOptionViewItem &option,
						   const QModelIndex &index) const override
			{
				QStyleOptionViewItem opt(option);
				initStyleOption(&opt, index);

				QFont title_font(opt.font);
				title_font.setBold(true);
				const QFontMetrics title_fm(title_font);
				const QFontMetrics sub_fm(opt.font);
				const int height = title_fm.height() + sub_fm.height() + 14;
				return QSize(opt.rect.width(), qMax(42, height));
			}

			void paint(QPainter *painter,
					   const QStyleOptionViewItem &option,
					   const QModelIndex &index) const override
			{
				QStyleOptionViewItem opt(option);
				initStyleOption(&opt, index);

				const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
				const QString title = opt.text;
				const QString subtitle = index.data(kSubtitleRole).toString();
				const bool selected = opt.state & QStyle::State_Selected;
				if (selected) {
					// Keep active highlight contrast even while editing fields on the right pane.
					opt.state |= QStyle::State_Active;
					opt.palette.setCurrentColorGroup(QPalette::Active);
				}

				// Clear default text before letting the style paint selection/focus chrome.
				opt.text.clear();
				style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

				painter->save();
				const QRect content_rect = opt.rect.adjusted(8, 5, -8, -5);
				const QColor text_color = selected
					? opt.palette.color(QPalette::Active, QPalette::HighlightedText)
					: opt.palette.color(QPalette::Text);

				if (index.column() == 1) {
					QFont qty_font(opt.font);
					qty_font.setBold(true);
					painter->setFont(qty_font);
					painter->setPen(text_color);
					painter->drawText(content_rect, Qt::AlignRight | Qt::AlignVCenter, title);
					painter->restore();
					return;
				}

				QFont title_font(opt.font);
				title_font.setBold(true);
				const QFontMetrics title_fm(title_font);
				const int title_h = title_fm.height();

				const QRect title_rect(content_rect.left(), content_rect.top(),
									   content_rect.width(), title_h);
				painter->setFont(title_font);
				painter->setPen(text_color);
				painter->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter,
								  title_fm.elidedText(title, Qt::ElideRight, title_rect.width()));

				const QRect sub_rect(content_rect.left(), title_rect.bottom() + 2,
									 content_rect.width(), content_rect.bottom() - title_rect.bottom() - 2);
				QColor sub_color = text_color;
				if (!selected) {
					sub_color.setAlphaF(0.75);
				}
				painter->setFont(opt.font);
				painter->setPen(sub_color);
				const QFontMetrics sub_fm(opt.font);
				painter->drawText(sub_rect, Qt::AlignLeft | Qt::AlignVCenter,
								  sub_fm.elidedText(subtitle, Qt::ElideRight, sub_rect.width()));
				painter->restore();
			}

			QWidget *createEditor(QWidget *parent,
								  const QStyleOptionViewItem &option,
								  const QModelIndex &index) const override
			{
				if (index.column() != 1) {
					return nullptr;
				}
				return QStyledItemDelegate::createEditor(parent, option, index);
			}
	};

	QString articleTitleForBlock(int block)
	{
		return ElementArticlesWidget::tr("Article %1").arg(block + 1);
	}

	QStringList articleColumnsForBlock(int block)
	{
		QStringList columns = MaterialList::columnsForBlock(block);
		if (block == 0) {
			// Placement and context fields stay in the Information tab rather than Articles.
			columns.removeAll(QETInformation::ELMT_FUNCTION);
			columns.removeAll(QETInformation::ELMT_COMMENT);
			columns.removeAll(QETInformation::ELMT_NOTES);
			columns.removeAll(QETInformation::ELMT_PLANT);
			columns.removeAll(QETInformation::ELMT_LOCATION);
				// The device type has rows of its own below the model's
			columns.removeAll(QETInformation::ELMT_DEVICE_TYPE);
		}
		const int desc_idx = columns.indexOf(QETInformation::ELMT_DESCRIPTION);
		const int desig_idx = columns.indexOf(QETInformation::ELMT_DESIGNATION);
		if (desc_idx >= 0 && desig_idx > desc_idx) {
			columns.move(desig_idx, desc_idx);
		}
		return columns;
	}
}

/**
	@brief ElementArticlesWidget::ElementArticlesWidget
	@param elmt element whose articles are edited
	@param parent parent widget
*/
ElementArticlesWidget::ElementArticlesWidget(Element *elmt, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent)
{
	if (DockFit::enabled() || isHostedInDock()) {
		setMinimumWidth(0);
	}
	setElement(elmt);
}

/**
	@brief ElementArticlesWidget::~ElementArticlesWidget
*/
ElementArticlesWidget::~ElementArticlesWidget() = default;

/**
	@brief ElementArticlesWidget::isHostedInDock
	@return true when embedded in a QDockWidget (or with DockFit enabled)
*/
bool ElementArticlesWidget::isHostedInDock() const
{
	if (DockFit::enabled() || m_live_edit) {
		return true;
	}
	for (const QWidget *w = parentWidget(); w; w = w->parentWidget()) {
		if (qobject_cast<const QDockWidget *>(w)) {
			return true;
		}
	}
	return false;
}

/**
	@brief ElementArticlesWidget::sizeHint
	@return a spacious default size when shown inside a standalone properties dialog,
	or a compact size so the side dock can stay narrow.
*/
QSize ElementArticlesWidget::sizeHint() const
{
	if (isHostedInDock()) {
		return QSize(280, 380);
	}
	return AbstractElementPropertiesEditorWidget::sizeHint().expandedTo(QSize(860, 580));
}

/**
	@brief ElementArticlesWidget::minimumSizeHint
	@return minimal size constraint in the dock so the side panel can always shrink
*/
QSize ElementArticlesWidget::minimumSizeHint() const
{
	if (isHostedInDock()) {
		return QSize(0, 200);
	}
	return AbstractElementPropertiesEditorWidget::minimumSizeHint();
}

/**
	@brief ElementArticlesWidget::managedKeys
	@return all DiagramContext keys managed by the Articles tab across blocks 0..4.
*/
QStringList ElementArticlesWidget::managedKeys()
{
	static const QStringList keys = []() {
		QStringList result;
		for (int block = 0; block < kBlockCount; ++block) {
			for (const QString &column : articleColumnsForBlock(block)) {
				const QString key = MaterialList::elementInfoKey(column, block);
				if (!key.isEmpty() && !result.contains(key)) {
					result << key;
				}
			}
		}
		result << QETInformation::ELMT_DEVICE_TYPE;
		return result;
	}();
	return keys;
}

/**
	@brief ElementArticlesWidget::setElement
	@param element element to edit
*/
void ElementArticlesWidget::setElement(Element *element)
{
	if (m_element == element) {
		return;
	}

	if (m_element) {
		disconnect(m_element.data(), &Element::elementInfoChange,
				   this, &ElementArticlesWidget::elementInfoChange);
	}

	m_explicitly_added.fill(false);
	m_pending_extra_info.clear();
	m_element = element;
	updateUi();

	if (m_element) {
		connect(m_element.data(), &Element::elementInfoChange,
				this, &ElementArticlesWidget::elementInfoChange);
	}
}

/**
	@brief ElementArticlesWidget::apply
*/
void ElementArticlesWidget::apply()
{
	if (!m_element || !m_element->diagram()) {
		return;
	}
	if (QUndoCommand *undo = associatedUndo()) {
		m_element->diagram()->undoStack().push(undo);
		m_pending_extra_info.clear();
	}
	updateTypeRow();
}

/**
	@brief ElementArticlesWidget::reset
*/
void ElementArticlesWidget::reset()
{
	if (m_element) {
		m_explicitly_added.fill(false);
		m_pending_extra_info.clear();
		updateUi();
	}
}

/**
	@brief ElementArticlesWidget::associatedUndo
	A device type field naming a type of the user's library which the
	project has not got yet brings that type into the project, in the same
	step: the type is what the device is checked against.
	@return undo command when article fields changed or a type is to be
	brought, nullptr otherwise
*/
QUndoCommand *ElementArticlesWidget::associatedUndo() const
{
	if (!m_element) {
		return nullptr;
	}
	const DiagramContext old_info = m_element->elementInformations();
	const DiagramContext new_info = currentInfo();
	const DeviceType type = typeToBring(new_info);
	if (type.isNull()) {
		if (old_info != new_info) {
			return new ChangeElementInformationCommand(m_element, old_info, new_info);
		}
		return nullptr;
	}
	QETProject *project = m_element->diagram()->project();
	if (old_info == new_info) {
		return new SetDeviceTypeCommand(project, type);
	}
	auto *macro = new QUndoCommand(
				QObject::tr("Edit information of the element : %1")
				.arg(m_element->name()));
	new SetDeviceTypeCommand(project, type, macro);
	new ChangeElementInformationCommand(m_element, old_info, new_info, macro);
	return macro;
}

/**
	@brief ElementArticlesWidget::typeToBring
	@return the device type of the user's library which the device type
	field of @p info names and the project has not got, to be brought into the
	project; a null type if none, or while a live edit is typed
*/
DeviceType ElementArticlesWidget::typeToBring(const DiagramContext &info) const
{
	Diagram *diagram = m_element ? m_element->diagram() : nullptr;
	if (!diagram || (m_live_edit && !m_bring_type)) {
		return DeviceType();
	}
	const QString name = DeviceTypeCheck::typeNameOf(info);
	if (name.isEmpty() || !diagram->project()->deviceType(name).isNull()
			|| !libraryTypes().contains(name)) {
		return DeviceType();
	}
	return DeviceTypeLibrary::load(name);
}

/**
	@brief ElementArticlesWidget::bringTypeNow
	Live: apply, the type the device type field names included
*/
void ElementArticlesWidget::bringTypeNow()
{
	m_bring_type = true;
	apply();
	m_bring_type = false;
	updateTypeRow();
}

/**
	@brief ElementArticlesWidget::changeType
	Give the device another device type, as when a relay needs one more
	contact: choose the type, then its symbols take the parts of the new
	one, matched automatically or one by one (DeviceTypeChangeDialog).
*/
void ElementArticlesWidget::changeType()
{
	ElementInfoPartWidget *type_row = partForKey(QETInformation::ELMT_DEVICE_TYPE);
	Diagram *diagram = m_element ? m_element->diagram() : nullptr;
	if (!type_row || !diagram) {
		return;
	}
	bringTypeNow();
	const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(m_element.data());
	if (!device.known()) {
		return;
	}

	DeviceTypeChooser chooser(m_element.data(), this, device.typeName);
	chooser.setShowAll(true);
	if (chooser.exec() != QDialog::Accepted || chooser.chosen().isEmpty()) {
		return;
	}
	const QString name = chooser.chosen();
	DeviceType type = diagram->project()->deviceType(name);
	if (type.isNull()) {
		type = DeviceTypeLibrary::load(name);
	}
	if (type.isNull()) {
		return;
	}

		//A device whose part number names its type: changing it is said
	const QString of_part = DeviceTypeChange::partNumberType(device.symbols.value(0));
	if (!of_part.isEmpty() && of_part != name
		&& QET::QetMessageBox::question(this, tr("Change the device type"),
			//: %1 and %2 are device types' names, such as "CONTR10"
			tr("The part number of this device names the device type %1. Change it to %2 anyway?")
				.arg(of_part, name),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
		return;
	}

	QList<int> parts = DeviceTypeChange::automatic(device, type);
	const auto answer = QET::QetMessageBox::question(this, tr("Change the device type"),
		tr("Copy the existing contacts to the new device type automatically?"),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
	if (answer == QMessageBox::Cancel) {
		return;
	}
	if (answer == QMessageBox::No) {
		DeviceTypeChangeDialog manual(device, type, parts, this);
		if (manual.exec() != QDialog::Accepted) {
			return;
		}
		parts = manual.parts();
	}

	DeviceTypeChooser::remember(name);
	QPointer<QDialog> dialog = qobject_cast<QDialog *>(window());
	QString error;
	if (!DeviceTypeChange::apply(device, type, parts, &error) && !error.isEmpty()) {
		QET::QetMessageBox::warning(this, tr("Change the device type"), error);
	}
		//Its symbol may have been replaced: a window showing it closes
	if (dialog && !m_element->scene()) {
		dialog->reject();
		return;
	}
	updateUi();
}

/**
	@brief ElementArticlesWidget::chooseTypeSymbol
	Ask which part of the device type this symbol draws, and give it that
	part's terminals. The fields are saved first, the type brought into
	the project with them: the choice replaces the symbol, so a properties
	window showing it closes.
*/
void ElementArticlesWidget::chooseTypeSymbol()
{
	ElementInfoPartWidget *type_row = partForKey(QETInformation::ELMT_DEVICE_TYPE);
	Diagram *diagram = m_element ? m_element->diagram() : nullptr;
	if (!type_row || !diagram) {
		return;
	}
	bringTypeNow();
	const QString name = type_row->text().trimmed();
	const DeviceType type = diagram->project()->deviceType(name);
	if (type.isNull()) {
		return;
	}

	const auto device = DeviceTypeAssign::deviceOf(m_element.data(), type);
	const auto choices = DeviceTypeAssign::choices(m_element.data(), device);
	if (choices.isEmpty()) {
		QET::QetMessageBox::information(this, tr("Set this symbol from the device type"),
										tr("Every part of this device type is already drawn."));
		return;
	}
	QMenu menu(this);
	QHash<QAction *, int> chosen;
	for (int i = 0 ; i < choices.size() ; ++i) {
		QAction *action = menu.addAction(choices.at(i).text);
		action->setEnabled(choices.at(i).possible);
		action->setToolTip(choices.at(i).reason);
		chosen.insert(action, i);
	}
	menu.setToolTipsVisible(true);
	QAction *picked = menu.exec(QCursor::pos());
	if (!picked) {
		return;
	}

	QString error;
	QDialog *dialog = qobject_cast<QDialog *>(window());
	Element *fresh = DeviceTypeAssign::apply(m_element.data(), device,
											 choices.at(chosen.value(picked)), &error);
	if (!fresh) {
		QET::QetMessageBox::warning(this, tr("Set this symbol from the device type"), error);
		return;
	}
	if (fresh != m_element.data()) {
		if (dialog) {
			dialog->reject();
			return;
		}
		diagram->clearSelection();
		fresh->setSelected(true);
	}
}

/**
	@brief ElementArticlesWidget::libraryTypes
	@return the names of the user's device types (DeviceTypeLibrary), read
	the first time only
*/
QStringList ElementArticlesWidget::libraryTypes() const
{
	if (!m_library_read) {
		m_library_types = DeviceTypeLibrary::names();
		m_library_read = true;
	}
	return m_library_types;
}

/**
	@brief ElementArticlesWidget::updateTypeRow
	Below the device type field: the type it names, its text and, for the
	type the element has, the parts of the type the device does not draw.
	Hidden while the field is empty.
*/
void ElementArticlesWidget::updateTypeRow()
{
	ElementInfoPartWidget *type_row = partForKey(QETInformation::ELMT_DEVICE_TYPE);
	Diagram *diagram = m_element ? m_element->diagram() : nullptr;
	if (!m_type_label || !type_row || !diagram) {
		return;
	}
	const QString name = type_row->text().trimmed();
	if (m_type_symbol_button) {
		m_type_symbol_button->setVisible(false);
	}
	if (m_type_change_button) {
		m_type_change_button->setVisible(false);
	}
	if (name.isEmpty()) {
		m_type_label->setVisible(false);
		return;
	}
	QETProject *project = diagram->project();
	if (m_type_change_button) {
		m_type_change_button->setVisible(!project->deviceType(name).isNull());
	}
	if (m_type_symbol_button) {
		m_type_symbol_button->setVisible(!project->deviceType(name).isNull()
										 || libraryTypes().contains(name));
	}
	QString text;
	const DeviceType type = project->deviceType(name);
	if (type.isNull()) {
		if (libraryTypes().contains(name))
			//: %1 is a device type's name, such as "CONTR10"
			text = tr("Device type %1 of your library: the project keeps a copy once applied.").arg(name);
		else
			text = tr("No device type %1 in the project or your library.").arg(name);
	} else {
		const QString saved = DeviceTypeCheck::typeNameOf(m_element->elementInformations());
		const auto device = DeviceTypeCheck::checkDevice(m_element.data());
		const int parts = int(type.parts.size());
		if (saved != name || device.typeName != name) {
			//: %1 is the type's text, %n its number of parts
			text = tr("%1 (%n part(s))", nullptr, parts).arg(type.text());
		} else {
			//: %1 is the type's text, %2 the parts drawn, %n all of its parts
			text = tr("%1: %2 of %n part(s) drawn", nullptr, parts)
					.arg(type.text()).arg(parts - device.missing.size());
			QStringList missing;
			for (int part : device.missing)
				missing << DeviceTypeCheck::partText(type.parts.at(part));
			if (!missing.isEmpty())
				text += QLatin1Char('\n') + tr("Not drawn: %1").arg(missing.join(QStringLiteral(", ")));
			if (device.beyond())
				text += QLatin1Char('\n') + tr("%n symbol(s) beyond the type", nullptr, device.beyond());
		}
	}
	m_type_label->setText(text);
	m_type_label->setVisible(true);
}

/**
	@brief ElementArticlesWidget::setLiveEdit
	@param live_edit true to apply edits immediately
	@return true
*/
bool ElementArticlesWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) {
		return true;
	}
	m_live_edit = live_edit;

	if (m_live_edit) {
		enableLiveEdit();
	} else {
		disableLiveEdit();
	}
	return true;
}

/**
	@brief ElementArticlesWidget::enableLiveEdit
*/
void ElementArticlesWidget::enableLiveEdit()
{
	for (ElementInfoPartWidget *part : std::as_const(m_parts)) {
		connect(part, &ElementInfoPartWidget::textChanged,
				this, &ElementArticlesWidget::apply, Qt::UniqueConnection);
	}
}

/**
	@brief ElementArticlesWidget::disableLiveEdit
*/
void ElementArticlesWidget::disableLiveEdit()
{
	for (ElementInfoPartWidget *part : std::as_const(m_parts)) {
		disconnect(part, &ElementInfoPartWidget::textChanged,
				   this, &ElementArticlesWidget::apply);
	}
}

/**
	@brief ElementArticlesWidget::buildInterface
	Build the master-detail split view: left list of articles, right editable detail pages.
*/
void ElementArticlesWidget::buildInterface()
{
	const bool in_dock = isHostedInDock();
	if (in_dock) {
		setMinimumWidth(0);
	}

	auto *root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(6, 6, 6, 6);

	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->setChildrenCollapsible(in_dock);

	// Left column: article selector + Add / Remove buttons
	m_left_panel = new QWidget(m_splitter);
	m_left_panel->setMinimumWidth(in_dock ? 0 : 220);
	auto *left_layout = new QVBoxLayout(m_left_panel);
	left_layout->setContentsMargins(0, 0, 0, 0);
	left_layout->setSpacing(4);

	m_article_list = new QTreeWidget(m_left_panel);
	m_article_list->setObjectName(QStringLiteral("m_article_list"));
	m_article_list->setColumnCount(2);
	m_article_list->setHeaderLabels({tr("Article"), tr("Quantity")});
	m_article_list->setRootIsDecorated(false);
	m_article_list->setIndentation(0);
	m_article_list->setAllColumnsShowFocus(true);
	m_article_list->setUniformRowHeights(true);
	m_article_list->setItemDelegate(new ArticleListDelegate(m_article_list));
	m_article_list->setSelectionMode(QAbstractItemView::SingleSelection);
	m_article_list->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_article_list->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
	m_article_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	if (QHeaderView *hdr = m_article_list->header()) {
		hdr->setStretchLastSection(false);
		hdr->setSectionResizeMode(0, QHeaderView::Stretch);
		hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents);
		hdr->setMinimumSectionSize(in_dock ? 36 : 56);
	}
	if (in_dock) {
		m_article_list->setMinimumWidth(0);
	}
	left_layout->addWidget(m_article_list, 1);

	auto *btn_row = new QHBoxLayout();
	btn_row->setSpacing(4);
	m_add_btn = new QPushButton(in_dock ? QStringLiteral("+") : tr("Add article"), m_left_panel);
	m_add_btn->setObjectName(QStringLiteral("m_add_article_btn"));
	m_remove_btn = new QPushButton(in_dock ? QStringLiteral("−") : tr("Remove"), m_left_panel);
	m_remove_btn->setObjectName(QStringLiteral("m_remove_article_btn"));
	if (in_dock) {
		m_add_btn->setMinimumWidth(0);
		m_remove_btn->setMinimumWidth(0);
		m_add_btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
		m_remove_btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
	}
	btn_row->addWidget(m_add_btn, 1);
	btn_row->addWidget(m_remove_btn, in_dock ? 1 : 0);
	left_layout->addLayout(btn_row);

	// Right column: active article header + stacked 2-column field grids
	m_right_panel = new QWidget(m_splitter);
	m_right_panel->setMinimumWidth(in_dock ? 0 : 480);
	auto *right_layout = new QVBoxLayout(m_right_panel);
	right_layout->setContentsMargins(0, 0, 0, 0);
	right_layout->setSpacing(6);

	auto *header_row = new QHBoxLayout();
	m_page_header_label = new QLabel(m_right_panel);
	QFont header_font = m_page_header_label->font();
	header_font.setBold(true);
	m_page_header_label->setFont(header_font);
	if (in_dock) {
		m_page_header_label->setMinimumWidth(0);
	}
	header_row->addWidget(m_page_header_label, 1);

	m_pick_material_btn = new QPushButton(
		in_dock ? tr("List…") : tr("Choose from materials list…"),
		m_right_panel);
	m_pick_material_btn->setObjectName(QStringLiteral("m_pick_material_btn"));
	m_pick_material_btn->setToolTip(tr("Choose from materials list…"));
	if (in_dock) {
		m_pick_material_btn->setMinimumWidth(0);
		m_pick_material_btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	}
	header_row->addWidget(m_pick_material_btn);
	right_layout->addLayout(header_row);

	auto *separator = new QFrame(m_right_panel);
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
	right_layout->addWidget(separator);

	m_stack = new QStackedWidget(m_right_panel);
	m_stack->setObjectName(QStringLiteral("m_article_stack"));
	if (in_dock) {
		m_stack->setMinimumWidth(0);
	}
	for (int block = 0; block < kBlockCount; ++block) {
		m_block_pages[block] = buildBlockPage(block);
		m_stack->addWidget(m_block_pages[block]);
	}
	right_layout->addWidget(m_stack, 1);

	m_splitter->addWidget(m_left_panel);
	m_splitter->addWidget(m_right_panel);
	m_splitter->setStretchFactor(0, 0);
	m_splitter->setStretchFactor(1, 1);
	const QString state_key = in_dock
		? QStringLiteral("elementarticleswidget/dock_splitter_state")
		: QStringLiteral("elementarticleswidget/splitter_state");
	const QByteArray saved_state = QSettings().value(state_key).toByteArray();
	if (saved_state.isEmpty() || !m_splitter->restoreState(saved_state)) {
		if (in_dock) {
			m_splitter->setSizes({100, 180});
		} else {
			m_splitter->setSizes({260, 560});
		}
	}
	connect(m_splitter, &QSplitter::splitterMoved, this, [this, state_key]() {
		QSettings().setValue(state_key, m_splitter->saveState());
	});
	root_layout->addWidget(m_splitter);

	connect(m_article_list, &QTreeWidget::currentItemChanged, this,
			[this](QTreeWidgetItem *current, QTreeWidgetItem *) {
		currentArticleChanged(current ? m_article_list->indexOfTopLevelItem(current) : -1);
	});
	connect(m_article_list, &QTreeWidget::itemChanged,
			this, &ElementArticlesWidget::articleItemChanged);
	connect(m_add_btn, &QPushButton::clicked,
			this, &ElementArticlesWidget::addArticle);
	connect(m_remove_btn, &QPushButton::clicked,
			this, &ElementArticlesWidget::removeCurrentArticle);
	connect(m_pick_material_btn, &QPushButton::clicked, this, [this]() {
		const int block = currentBlock();
		if (block >= 0) {
			materialFromFile(block);
		}
	});
}

/**
	@brief ElementArticlesWidget::buildBlockPage
	Create a scrollable 2-column grid of ElementInfoPartWidgets for one article block.
	@param block 0 for main article, 1..4 for auxiliary articles
	@return page widget ready to be added to m_stack
*/
QWidget *ElementArticlesWidget::buildBlockPage(int block)
{
	auto *scroll = new QScrollArea(m_stack);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);

	auto *container = new QWidget(scroll);
	auto *grid = new QGridLayout(container);
	grid->setContentsMargins(2, 2, 2, 2);
	grid->setHorizontalSpacing(10);
	grid->setVerticalSpacing(4);

	const QStringList columns = articleColumnsForBlock(block);
		//The main article: its device type, in rows of their own below the
		//model's (the field, what the type is made of, the symbol button)
	const int model_row = block == 0
			? int(columns.indexOf(QETInformation::ELMT_MODEL)) / 2 : -1;
	for (int i = 0; i < columns.size(); ++i) {
		const QString &column = columns.at(i);
		const QString key = MaterialList::elementInfoKey(column, block);
		const QString label = MaterialList::translatedColumn(column);

		auto *part = new ElementInfoPartWidget(key, label, container);
		if (column == QLatin1String("description") || column == QLatin1String("designation")) {
			part->setMaterialButtonVisible(true);
			connect(part, &ElementInfoPartWidget::materialButtonClicked, this,
					[this, block]() { materialFromFile(block); });
		}
		if (column == QLatin1String("manufacturer")
				|| column == QLatin1String("description")
				|| column == QLatin1String("designation")
				|| column == QLatin1String("quantity")
				|| column == QLatin1String("auxiliary")) {
			connect(part, &ElementInfoPartWidget::textChanged, this,
					[this, block]() { updateArticleListRow(block); });
		}

		const int row = i / 2 + (model_row >= 0 && i / 2 > model_row ? 3 : 0);
		const int col = i % 2;
		grid->addWidget(part, row, col);
		m_block_parts[block] << part;
		m_parts << part;
	}
	int rows = (int(columns.size()) + 1) / 2;
	if (model_row >= 0) {
		auto *type_part = new ElementInfoPartWidget(
				QETInformation::ELMT_DEVICE_TYPE,
				QETInformation::translatedInfoKey(QETInformation::ELMT_DEVICE_TYPE), container);
		type_part->setToolTip(tr("What this device is made of: its device type, from the project or your library"));
		grid->addWidget(type_part, model_row + 1, 0);
		m_parts << type_part;
		connect(type_part, &ElementInfoPartWidget::textChanged,
				this, &ElementArticlesWidget::updateTypeRow);
		connect(type_part, &ElementInfoPartWidget::editingFinished, this, [this]() {
			if (m_live_edit) bringTypeNow();
		});
		m_type_label = new QLabel(container);
		m_type_label->setObjectName(QStringLiteral("deviceTypeLabel"));
		m_type_label->setWordWrap(true);
		m_type_label->setTextFormat(Qt::PlainText);
		m_type_label->setContentsMargins(6, 0, 6, 4);
		m_type_label->setVisible(false);
		grid->addWidget(m_type_label, model_row + 2, 0, 1, 2);
		m_type_symbol_button = new QPushButton(
				tr("Set this symbol from the device type..."), container);
		m_type_symbol_button->setObjectName(QStringLiteral("deviceTypeSymbolButton"));
		m_type_symbol_button->setToolTip(tr("Choose which part of the device type this symbol draws: it takes that part's terminals."));
		m_type_symbol_button->setVisible(false);
		m_type_change_button = new QPushButton(tr("Change the device type..."), container);
		m_type_change_button->setObjectName(QStringLiteral("deviceTypeChangeButton"));
		m_type_change_button->setToolTip(tr("Give the device another device type: its symbols take the parts of the new one."));
		m_type_change_button->setVisible(false);
		connect(m_type_change_button, &QPushButton::clicked,
				this, &ElementArticlesWidget::changeType);
		auto *type_buttons = new QHBoxLayout();
		type_buttons->setContentsMargins(0, 0, 0, 0);
		type_buttons->addWidget(m_type_symbol_button);
		type_buttons->addWidget(m_type_change_button);
		type_buttons->addStretch();
		grid->addLayout(type_buttons, model_row + 3, 0, 1, 2);
		connect(m_type_symbol_button, &QPushButton::clicked,
				this, &ElementArticlesWidget::chooseTypeSymbol);
		rows += 3;
	}

	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(1, 1);
	grid->setRowStretch(rows, 1);

	scroll->setWidget(container);
	return scroll;
}

/**
	@brief ElementArticlesWidget::updateUi
*/
void ElementArticlesWidget::updateUi()
{
	if (!m_ui_built) {
		buildInterface();
		m_ui_built = true;
	}
	if (!m_element) {
		return;
	}
	m_pending_extra_info.clear();

	const bool was_live = m_live_edit;
	if (was_live) {
		disableLiveEdit();
	}

	const int selected_before = currentBlock();
	const DiagramContext element_info = m_element->elementInformations();
	m_loaded_values.clear();

	for (ElementInfoPartWidget *part : std::as_const(m_parts)) {
		const QString val = element_info.value(part->key()).toString();
		m_loaded_values.insert(part->key(), val);
		part->setText(val);
	}

	ElementInfoWidget::populateSuggestions(m_element, m_parts);
	refreshArticleList(selected_before);
	updateTypeRow();

	if (was_live) {
		enableLiveEdit();
	}
}

/**
	@brief ElementArticlesWidget::currentInfo
	@return element informations with only the article keys updated from this widget
*/
DiagramContext ElementArticlesWidget::currentInfo() const
{
	DiagramContext info = m_element ? m_element->elementInformations() : DiagramContext();

	for (const ElementInfoPartWidget *part : std::as_const(m_parts)) {
		if (!part->hasAcceptableInput()) {
			continue;
		}
		const QString key = part->key();
		if (part->text().isEmpty()) {
			info.remove(key);
		} else {
			info.addValue(key, part->text());
		}
	}

	for (auto it = m_pending_extra_info.cbegin(); it != m_pending_extra_info.cend(); ++it) {
		if (it.value().isEmpty()) {
			info.remove(it.key());
		} else {
			info.addValue(it.key(), it.value());
		}
	}
	return info;
}

/**
	@brief ElementArticlesWidget::elementInfoChange
	Refresh the widget only when an external change modified article keys.
*/
void ElementArticlesWidget::elementInfoChange()
{
	if (!m_element) {
		return;
	}

	const DiagramContext elmt_info = m_element->elementInformations();
	bool external_article_change = false;
	for (const ElementInfoPartWidget *part : std::as_const(m_parts)) {
		const QString key = part->key();
		const QString elmt_val = elmt_info.value(key).toString();
		if (elmt_val != m_loaded_values.value(key) && elmt_val != part->text()) {
			external_article_change = true;
			break;
		}
	}

	if (external_article_change) {
		updateUi();
	}
}

/**
	@brief ElementArticlesWidget::blockHasData
	@return true if any field of @p block currently holds non-empty text
*/
bool ElementArticlesWidget::blockHasData(int block) const
{
	if (block < 0 || block >= kBlockCount) {
		return false;
	}
	for (const ElementInfoPartWidget *part : m_block_parts[block]) {
		if (!part->text().trimmed().isEmpty()) {
			return true;
		}
	}
	return false;
}

/**
	@brief ElementArticlesWidget::articleSubtitle
	Build the second-line summary shown beneath "Article N" in the left list.
*/
QString ElementArticlesWidget::articleSubtitle(int block) const
{
	if (block < 0 || block >= kBlockCount) {
		return QString();
	}
	auto valueFor = [this, block](const QString &column) -> QString {
		const QString key = MaterialList::elementInfoKey(column, block);
		if (ElementInfoPartWidget *part = partForKey(key)) {
			return part->text().trimmed();
		}
		return QString();
	};

	const QString designation = valueFor(QStringLiteral("designation"));
	const QString description = valueFor(QStringLiteral("description"));

	QStringList parts;
	if (!designation.isEmpty()) {
		parts << designation;
	}
	if (!description.isEmpty()) {
		parts << description;
	}
	if (!parts.isEmpty()) {
		return parts.join(QStringLiteral(" / "));
	}

	if (block > 0) {
		const QString auxiliary = valueFor(QStringLiteral("auxiliary"));
		if (!auxiliary.isEmpty()) {
			return auxiliary;
		}
		return tr("Empty article");
	}

	const QString manufacturer = valueFor(QStringLiteral("manufacturer"));
	return manufacturer.isEmpty() ? tr("Main article (empty)") : manufacturer;
}

/**
	@brief ElementArticlesWidget::articleQuantity
	@return the quantity text shown in column 1 of the article table
*/
QString ElementArticlesWidget::articleQuantity(int block) const
{
	if (block < 0 || block >= kBlockCount) {
		return QString();
	}
	const QString key = MaterialList::elementInfoKey(QStringLiteral("quantity"), block);
	if (ElementInfoPartWidget *part = partForKey(key)) {
		return part->text().trimmed();
	}
	return QString();
}

/**
	@brief ElementArticlesWidget::refreshArticleList
	Populate the left list with Article 1 plus any active or non-empty auxiliary articles.
	@param preferred_block block to keep selected if still present
*/
void ElementArticlesWidget::refreshArticleList(int preferred_block)
{
	if (!m_article_list) {
		return;
	}

	const QSignalBlocker blocker(m_article_list);
	m_article_list->clear();

	int row_to_select = 0;
	for (int block = 0; block < kBlockCount; ++block) {
		if (block == 0 || blockHasData(block) || m_explicitly_added[block]) {
			auto *item = new QTreeWidgetItem(m_article_list);
			item->setText(0, articleTitleForBlock(block));
			item->setData(0, kBlockRole, block);
			item->setData(0, kSubtitleRole, articleSubtitle(block));
			item->setText(1, articleQuantity(block));
			item->setFlags(item->flags() | Qt::ItemIsEditable);
			if (block == preferred_block) {
				row_to_select = m_article_list->topLevelItemCount() - 1;
			}
		}
	}

	if (m_article_list->topLevelItemCount() > 0) {
		m_article_list->setCurrentItem(m_article_list->topLevelItem(row_to_select));
		currentArticleChanged(row_to_select);
	}
	updateActionButtons();
}

/**
	@brief ElementArticlesWidget::updateArticleListRow
	Update the subtitle and quantity column for @p block when edited on the right.
*/
void ElementArticlesWidget::updateArticleListRow(int block)
{
	if (!m_article_list) {
		return;
	}
	const QSignalBlocker blocker(m_article_list);
	for (int row = 0; row < m_article_list->topLevelItemCount(); ++row) {
		QTreeWidgetItem *item = m_article_list->topLevelItem(row);
		if (item && item->data(0, kBlockRole).toInt() == block) {
			item->setData(0, kSubtitleRole, articleSubtitle(block));
			item->setText(1, articleQuantity(block));
			break;
		}
	}
	updateActionButtons();
}

/**
	@brief ElementArticlesWidget::articleItemChanged
	Sync inline quantity edits from column 1 back into the article's quantity field.
*/
void ElementArticlesWidget::articleItemChanged(QTreeWidgetItem *item, int column)
{
	if (!item || column != 1) {
		return;
	}
	const int block = item->data(0, kBlockRole).toInt();
	const QString key = MaterialList::elementInfoKey(QStringLiteral("quantity"), block);
	if (ElementInfoPartWidget *part = partForKey(key)) {
		const QString qty = item->text(1).trimmed();
		if (part->text() != qty) {
			part->setText(qty);
		}
	}
}

/**
	@brief ElementArticlesWidget::updateActionButtons
*/
void ElementArticlesWidget::updateActionButtons()
{
	if (!m_add_btn || !m_remove_btn || !m_article_list) {
		return;
	}

	const bool can_add = m_article_list->topLevelItemCount() < kBlockCount;
	m_add_btn->setEnabled(can_add);
	m_add_btn->setToolTip(can_add
		? tr("Add another article slot (up to %1)").arg(kBlockCount)
		: tr("All %1 native article slots are already shown").arg(kBlockCount));

	const int block = currentBlock();
	const bool can_remove = block > 0 || blockHasData(0);
	m_remove_btn->setEnabled(can_remove);
	m_remove_btn->setToolTip(block > 0
		? tr("Clear and remove the selected auxiliary article")
		: tr("Clear the fields of Article 1"));
}

/**
	@brief ElementArticlesWidget::currentBlock
	@return active block index 0..4, or -1 if none
*/
int ElementArticlesWidget::currentBlock() const
{
	if (!m_article_list) {
		return -1;
	}
	const QTreeWidgetItem *item = m_article_list->currentItem();
	return item ? item->data(0, kBlockRole).toInt() : -1;
}

/**
	@brief ElementArticlesWidget::currentArticleChanged
*/
void ElementArticlesWidget::currentArticleChanged(int row)
{
	if (!m_article_list || !m_stack || row < 0 || row >= m_article_list->topLevelItemCount()) {
		return;
	}
	const int block = m_article_list->topLevelItem(row)->data(0, kBlockRole).toInt();
	if (block >= 0 && block < kBlockCount) {
		m_stack->setCurrentIndex(block);
		m_page_header_label->setText(articleTitleForBlock(block));
	}
	updateActionButtons();
}

/**
	@brief ElementArticlesWidget::addArticle
	Show the next unused native article slot (2..5) and select it.
*/
void ElementArticlesWidget::addArticle()
{
	if (!m_article_list) {
		return;
	}
	for (int block = 1; block < kBlockCount; ++block) {
		if (!blockHasData(block) && !m_explicitly_added[block]) {
			m_explicitly_added[block] = true;
			refreshArticleList(block);
			if (!m_block_parts[block].isEmpty()) {
				m_block_parts[block].first()->setFocusTolineEdit();
			}
			return;
		}
	}
}

/**
	@brief ElementArticlesWidget::removeCurrentArticle
	Clear the active block and, for auxiliary articles (blocks 1..4), hide the row.
*/
void ElementArticlesWidget::removeCurrentArticle()
{
	const int block = currentBlock();
	if (block < 0 || block >= kBlockCount) {
		return;
	}

	const bool had_data = blockHasData(block);
	const bool live_edit = m_live_edit;
	if (live_edit) {
		disableLiveEdit();
	}

	for (ElementInfoPartWidget *part : m_block_parts[block]) {
		part->setText(QString());
	}
	if (block > 0) {
		m_explicitly_added[block] = false;
	}

	if (live_edit) {
		enableLiveEdit();
		if (had_data) {
			apply();
		}
	}

	refreshArticleList(block > 0 ? block - 1 : 0);
}

/**
	@brief ElementArticlesWidget::materialFromFile
	Open the materials catalogue and write the chosen record into @p block.
*/
void ElementArticlesWidget::materialFromFile(int block)
{
	QString path = MaterialList::configuredPath();
	if (path.isEmpty()) {
		path = MaterialList::defaultPath();
		MaterialList::setConfiguredPath(path);
	}

	if (MaterialList::isEmptyFile(path)) {
		const auto answer = QET::QetMessageBox::question(
			this,
			tr("No materials list"),
			tr("There is no materials list file at this location:\n"
			   "%1\n"
			   "\n"
			   "Create it?", "message asking to create the material file").arg(path),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::Yes);

		if (answer != QMessageBox::Yes) {
			return;
		}

		QString error;
		if (!MaterialList::createFile(path, &error)) {
			QET::QetMessageBox::critical(this,
										 tr("Cannot create"),
										 tr("Cannot create the file:\n%1\n%2")
											.arg(path, error));
			return;
		}
	}

	MaterialSelectionDialog dialog(path, this);
	if (dialog.exec() == QDialog::Accepted) {
		applyMaterialRecord(dialog.selectedRecord(), block);
	}
}

/**
	@brief ElementArticlesWidget::applyMaterialRecord
	Write a catalogue entry into @p block as a single atomic change.
*/
void ElementArticlesWidget::applyMaterialRecord(const MaterialRecord &record, int block)
{
	if (record.values.isEmpty()) {
		return;
	}

	const bool live_edit = m_live_edit;
	if (live_edit) {
		disableLiveEdit();
	}

	for (const QString &column : MaterialList::columnsForBlock(block)) {
		if (!record.values.contains(column)) {
			continue;
		}
		const QString value = record.value(column);
		if (value.isEmpty() && !MaterialList::isArticleBound(column)) {
			continue;
		}
		const QString key = MaterialList::elementInfoKey(column, block);
		if (key.isEmpty() || !QETInformation::elementInfoKeys().contains(key)) {
			continue;
		}
		if (ElementInfoPartWidget *part = partForKey(key)) {
			part->setText(value);
			m_pending_extra_info.remove(key);
		} else {
			m_pending_extra_info.insert(key, value);
		}
	}

	updateArticleListRow(block);

	if (live_edit) {
		enableLiveEdit();
		bringTypeNow();
	}
}

/**
	@brief ElementArticlesWidget::partForKey
*/
ElementInfoPartWidget *ElementArticlesWidget::partForKey(const QString &key) const
{
	for (ElementInfoPartWidget *part : std::as_const(m_parts)) {
		if (part->key() == key) {
			return part;
		}
	}
	return nullptr;
}

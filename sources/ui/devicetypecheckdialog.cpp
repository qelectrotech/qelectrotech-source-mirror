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
#include "devicetypecheckdialog.h"

#include <devicetype/devicetypeplacement.h>
#include <diagram.h>
#include <diagramevent/diagrameventaddelement.h>
#include <diagramview.h>
#include <qet.h>
#include <qetdiagrameditor.h>
#include <qetgraphicsitem/element.h>
#include <projectview.h>
#include <qetproject.h>

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopeGuard>
#include <QStyle>
#include <QTreeWidget>
#include <QVBoxLayout>

DeviceTypeCheckDialog::DeviceTypeCheckDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_tree(new QTreeWidget(this)),
	m_show_complete(new QCheckBox(this)),
	m_summary(new QLabel(this)),
	m_place(nullptr)
{
	setWindowTitle(tr("Check device types"));
	resize(820, 520);
	auto *layout = new QVBoxLayout(this);
	m_summary->setWordWrap(true);
	m_summary->setObjectName(QStringLiteral("deviceTypeSummary"));
	layout->addWidget(m_summary);
	auto *help = new QLabel(
		tr("A device takes the device type named on its Parts tab. Click a "
		   "device or a symbol to select and show it on its sheet. Double-click "
		   "a part not drawn to place it on the current folio. Refresh after "
		   "editing the project."), this);
	help->setWordWrap(true);
	layout->addWidget(help);
	m_show_complete->setObjectName(QStringLiteral("deviceTypeShowComplete"));
	m_show_complete->setText(tr("Also list the complete devices"));
	connect(m_show_complete, &QCheckBox::toggled, this, &DeviceTypeCheckDialog::refresh);
	layout->addWidget(m_show_complete);

	m_tree->setObjectName(QStringLiteral("deviceTypeDevices"));
	m_tree->setHeaderLabels({tr("Device / part"), tr("Device type"), tr("Sheet"), tr("Position")});
	m_tree->setRootIsDecorated(true);
	m_tree->setAlternatingRowColors(true);
	m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	for (int column = 1 ; column < 4 ; ++column)
		m_tree->header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
	layout->addWidget(m_tree);
	connect(m_tree, &QTreeWidget::itemClicked, this,
			[this](QTreeWidgetItem *row, int) {showSymbol(row);});
	connect(m_tree, &QTreeWidget::itemActivated, this,
			[this](QTreeWidgetItem *row, int) {showSymbol(row);});
	connect(m_tree, &QTreeWidget::itemDoubleClicked, this,
			[this](QTreeWidgetItem *row, int) {placeRow(row);});
	connect(m_tree, &QTreeWidget::currentItemChanged, this,
			&DeviceTypeCheckDialog::updatePlaceButton);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	auto *refresh_button = buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	refresh_button->setObjectName(QStringLiteral("refreshDeviceTypes"));
	m_place = buttons->addButton(tr("Place on the current folio"), QDialogButtonBox::ActionRole);
	m_place->setObjectName(QStringLiteral("placeDeviceTypePart"));
	m_place->setToolTip(tr("Place the selected part: the symbol follows the mouse until you click"));
	m_place->setEnabled(false);
	connect(m_place, &QPushButton::clicked, this, [this]() {placeRow(m_tree->currentItem());});
	connect(refresh_button, &QPushButton::clicked, this, &DeviceTypeCheckDialog::refresh);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
	if (project) connect(project, &QObject::destroyed, this, &QDialog::close);
	QET::trackDialogGeometry(this);
	refresh();
}

/**
	@brief DeviceTypeCheckDialog::buildFolios
	A device may be drawn on many folios: those not built yet (lazy
	folios) are built first, with their progress shown.
*/
void DeviceTypeCheckDialog::buildFolios()
{
	if (!m_project || !m_project->unloadedFolioCount()) return;
	QProgressDialog progress(tr("Reading the folios..."), QString(), 0,
							 m_project->unloadedFolioCount(),
							 parentWidget() ? parentWidget() : this);
	progress.setWindowTitle(windowTitle());
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setCancelButton(nullptr);
	progress.setMinimumDuration(0);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const auto restore = qScopeGuard([this]() {
		if (m_project) m_project->setFolioBuildObserver({});
		QApplication::restoreOverrideCursor();
	});
	m_project->setFolioBuildObserver([&progress](int built, int total) {
		progress.setMaximum(total);
		progress.setLabelText(tr("Reading folio %1 of %2...").arg(built).arg(total));
		progress.setValue(built);
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	});
	m_project->loadFolios();
}

void DeviceTypeCheckDialog::refresh()
{
	m_targets.clear();
	m_parts.clear();
	m_tree->clear();
	updatePlaceButton();
	if (!m_project) return;
	buildFolios();
	m_devices = DeviceTypeCheck::check(m_project);
	const auto &devices = m_devices;

	const auto where = [this](QTreeWidgetItem *row, Element *symbol) {
		Diagram *diagram = symbol ? symbol->diagram() : nullptr;
		if (!diagram) return;
		row->setText(2, tr("%1 — %2").arg(m_project->folioIndex(diagram) + 1)
										.arg(diagram->title()));
		row->setText(3, diagram->convertPosition(
						 symbol->sceneBoundingRect().center()).toString());
		m_targets.insert(row, symbol);
	};

	int complete = 0, incomplete = 0, unknown = 0;
	for (int index = 0 ; index < devices.size() ; ++index) {
		const auto &device = devices.at(index);
		if (!device.known()) ++unknown;
		else if (device.complete()) ++complete;
		else ++incomplete;
		if (device.complete() && !m_show_complete->isChecked()) continue;

		auto *root = new QTreeWidgetItem(m_tree);
		root->setText(1, device.typeName);
		Element *head = device.symbols.isEmpty() ? nullptr : device.symbols.first().data();
		where(root, head);
		if (!device.known()) {
			root->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxWarning));
			//: %1 is the device, such as "=P1+CP1-K12"; %2 its device type
			root->setText(0, tr("%1: the project has no device type %2")
						  .arg(device.designation, device.typeName));
			continue;
		}
		const int parts = int(device.type.parts.size());
		//: %1 is the device, such as "=P1+CP1-K12"
		root->setText(0, tr("%1: %2 of %n parts drawn", nullptr, parts)
					  .arg(device.designation).arg(parts - device.missing.size()));
		root->setToolTip(0, device.type.text());
		root->setIcon(0, style()->standardIcon(device.complete()
						  ? QStyle::SP_DialogApplyButton : QStyle::SP_MessageBoxCritical));
		for (int part : device.missing) {
			auto *row = new QTreeWidgetItem(root);
			//: %1 describes the part, such as "simple NO 13 14"
			row->setText(0, tr("Not drawn: %1")
						 .arg(DeviceTypeCheck::partText(device.type.parts.at(part))));
			m_parts.insert(row, {index, part});
			if (DeviceTypePlacement::placeableWays(device, part).isEmpty())
				row->setToolTip(0, device.type.parts.at(part).kind == QLatin1String("plc")
								? tr("Place PLC points with the PLC Manager.")
								: tr("The project does not have a symbol for this part."));
			else
				row->setToolTip(0, tr("Double-click to place it on the current folio."));
		}
		for (int i = 0 ; i < device.symbols.size() ; ++i) {
			if (device.drawn.at(i) != DeviceTypeCheck::Beyond) continue;
			Element *symbol = device.symbols.at(i).data();
			auto *row = new QTreeWidgetItem(root);
			//: %1 is the symbol's name
			row->setText(0, tr("Beyond the type: %1").arg(symbol ? symbol->name() : QString()));
			where(row, symbol);
		}
	}
	//: %n counts the devices which have a device type
	m_summary->setText(tr("%n device(s) with a device type: %1 complete, %2 incomplete, "
						  "%3 of a type the project does not have.", nullptr,
						  int(devices.size()))
					   .arg(complete).arg(incomplete).arg(unknown));
}

/**
	@brief DeviceTypeCheckDialog::updatePlaceButton
	The button is for a part not drawn which can be placed.
*/
void DeviceTypeCheckDialog::updatePlaceButton()
{
	if (!m_place) return;
	const auto row = m_parts.constFind(m_tree->currentItem());
	m_place->setEnabled(row != m_parts.constEnd()
			&& !DeviceTypePlacement::placeableWays(
				m_devices.at(row->first), row->second).isEmpty());
}

/**
	@brief DeviceTypeCheckDialog::placeRow
	Place the part of @p row; a part with several ways of being drawn (NO,
	NC, changeover) asks which.
*/
void DeviceTypeCheckDialog::placeRow(QTreeWidgetItem *row)
{
	if (!m_parts.contains(row)) return;
	const auto where = m_parts.value(row);
	const auto &device = m_devices.at(where.first);
	const QList<int> ways = DeviceTypePlacement::placeableWays(device, where.second);
	if (ways.isEmpty()) return;
	int way = ways.first();
	if (ways.size() > 1) {
		QMenu menu(this);
		for (int candidate : ways)
			menu.addAction(DeviceTypePlacement::wayText(device.type.parts.at(where.second), candidate))
					->setData(candidate);
		QAction *chosen = menu.exec(QCursor::pos());
		if (!chosen) return;
		way = chosen->data().toInt();
	}
	startPlacing(row, way);
}

/**
	@brief DeviceTypeCheckDialog::startPlacing
	The symbol of the part of @p row follows the mouse on @p folio, the
	folio the editor shows by default. The list is read again afterwards.
	@return false when it cannot be placed there
*/
bool DeviceTypeCheckDialog::startPlacing(QTreeWidgetItem *row, int way, Diagram *folio)
{
	if (!m_parts.contains(row) || !m_project) return false;
	DiagramView *view = nullptr;
	auto *editor = qobject_cast<QETDiagramEditor *>(parentWidget());
	if (!folio && editor) {
		editor->activateProject(m_project);
		if (ProjectView *project_view = editor->currentProjectView())
			view = project_view->currentDiagram();
		if (view) folio = view->diagram();
	}
	if (!folio || folio->project() != m_project) {
		m_summary->setText(tr("Open a folio of this project to place the part."));
		return false;
	}
	QPointF pos;
	if (view)
		pos = view->mapToScene(view->viewport()->rect().center());
	const auto where = m_parts.value(row);
	DiagramEventAddElement *placing = DeviceTypePlacement::begin(
				m_devices.at(where.first), where.second, way, folio, pos);
	if (!placing) {
		m_summary->setText(tr("This part cannot be placed."));
		return false;
	}
	connect(placing, &DiagramEventInterface::finish, this,
			&DeviceTypeCheckDialog::refresh, Qt::QueuedConnection);
	folio->setEventInterface(placing);
	if (editor) {
		editor->raise();
		editor->activateWindow();
	}
	if (view) view->setFocus();
	return true;
}

void DeviceTypeCheckDialog::showSymbol(QTreeWidgetItem *row)
{
	if (!m_targets.contains(row)) return;
	Element *target = m_targets.value(row).data();
	Diagram *diagram = target ? target->diagram() : nullptr;
	if (!m_project || !diagram || m_project->folioIndex(diagram) < 0) {
		m_summary->setText(tr("This symbol no longer exists. Refresh the results."));
		return;
	}
	if (auto *editor = qobject_cast<QETDiagramEditor *>(parentWidget()))
		editor->activateProject(m_project);
	diagram->showMe();
	diagram->clearSelection();
	target->setSelected(true);
	const QRectF bounds = target->sceneBoundingRect().adjusted(-100, -100, 100, 100);
	for (QGraphicsView *view : diagram->views()) {
		if (auto *diagram_view = qobject_cast<DiagramView *>(view))
			diagram_view->zoomToRect(bounds);
		else {
			view->fitInView(bounds, Qt::KeepAspectRatio);
			view->centerOn(bounds.center());
		}
	}
}

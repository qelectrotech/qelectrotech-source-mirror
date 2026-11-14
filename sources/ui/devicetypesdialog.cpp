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
#include "devicetypesdialog.h"

#include "../qetapp.h"
#include "../qetmessagebox.h"
#include "../qetproject.h"
#include "../undocommand/setdevicetypecommand.h"
#include "devicetypeeditor.h"
#include "../devicetype/devicetypelist.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QUndoStack>
#include <QVBoxLayout>

namespace
{
	QTreeWidget *typeList(QWidget *parent, const QString &name, bool used)
	{
		auto *list = new QTreeWidget(parent);
		list->setObjectName(name);
		list->setRootIsDecorated(false);
		list->setSortingEnabled(true);
		list->sortByColumn(0, Qt::AscendingOrder);
		QStringList headers = {DeviceTypesDialog::tr("Name"), DeviceTypesDialog::tr("Description"),
							   DeviceTypesDialog::tr("Parts")};
		if (used) headers << DeviceTypesDialog::tr("Symbols");
		list->setHeaderLabels(headers);
		list->header()->setSectionResizeMode(1, QHeaderView::Stretch);
		return list;
	}
}

/**
	@brief DeviceTypesDialog::DeviceTypesDialog
	@param project : the project whose types are listed and changed
	@param parent : parent widget
	@param library_folder : the user's library; empty for the usual one
	(DeviceTypeLibrary::folder())
*/
DeviceTypesDialog::DeviceTypesDialog(QETProject *project, QWidget *parent,
									 const QString &library_folder) :
	QDialog(parent),
	m_project(project),
	m_folder(library_folder)
{
	setWindowTitle(tr("Device types"));
	auto *layout = new QVBoxLayout(this);
	auto *help = new QLabel(
		tr("A device type says what a kind of device is made of. The project "
		   "keeps the types its devices use; your library keeps the ones you "
		   "make and share. A device takes a type in its Device type field on "
		   "the Parts tab."), this);
	help->setWordWrap(true);
	layout->addWidget(help);

	m_tabs = new QTabWidget(this);
	m_tabs->setObjectName(QStringLiteral("deviceTypeTabs"));
	m_project_list = typeList(this, QStringLiteral("projectDeviceTypes"), true);
	m_library_list = typeList(this, QStringLiteral("libraryDeviceTypes"), false);
	m_tabs->addTab(m_project_list, tr("This project"));
	m_tabs->addTab(m_library_list, tr("Your library"));
	layout->addWidget(m_tabs, 1);

	auto *buttons = new QHBoxLayout();
	auto *add = new QPushButton(tr("New..."), this);
	m_duplicate = new QPushButton(tr("Duplicate..."), this);
	m_edit = new QPushButton(tr("Edit..."), this);
	m_remove = new QPushButton(tr("Delete"), this);
	m_copy = new QPushButton(this);
	m_copy->setObjectName(QStringLiteral("deviceTypeCopy"));
	auto *list = new QPushButton(tr("Export list"), this);
	list->setObjectName(QStringLiteral("deviceTypeExportList"));
	list->setToolTip(tr("Save the device types of this tab as a CSV list"));
	auto *list_menu = new QMenu(list);
	list_menu->addAction(tr("Full list (every part and its terminals)..."), this,
						 [this]() {exportList(DeviceTypeList::Kind::Full);});
	list_menu->addAction(tr("Brief list (name and description)..."), this,
						 [this]() {exportList(DeviceTypeList::Kind::Brief);});
	list->setMenu(list_menu);
	for (QPushButton *button : {add, m_duplicate, m_edit, m_remove, m_copy})
		buttons->addWidget(button);
	buttons->addStretch();
	buttons->addWidget(list);
	layout->addLayout(buttons);
	connect(add, &QPushButton::clicked, this, &DeviceTypesDialog::newType);
	connect(m_duplicate, &QPushButton::clicked, this, &DeviceTypesDialog::duplicate);
	connect(m_edit, &QPushButton::clicked, this, &DeviceTypesDialog::edit);
	connect(m_remove, &QPushButton::clicked, this, &DeviceTypesDialog::removeSelected);
	connect(m_copy, &QPushButton::clicked, this, &DeviceTypesDialog::copySelected);

	auto *close = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(close, &QDialogButtonBox::rejected, this, &DeviceTypesDialog::reject);
	layout->addWidget(close);

	for (QTreeWidget *list : {m_project_list, m_library_list}) {
		connect(list, &QTreeWidget::itemSelectionChanged, this, &DeviceTypesDialog::updateButtons);
		connect(list, &QTreeWidget::itemDoubleClicked, this, &DeviceTypesDialog::edit);
	}
	connect(m_tabs, &QTabWidget::currentChanged, this, &DeviceTypesDialog::updateButtons);

	resize(720, 460);
	refresh();
}

bool DeviceTypesDialog::inLibrary() const
{
	return m_tabs->currentWidget() == m_library_list;
}

void DeviceTypesDialog::showLibrary(bool library)
{
	m_tabs->setCurrentWidget(library ? m_library_list : m_project_list);
}

QString DeviceTypesDialog::selectedName() const
{
	const QTreeWidget *list = inLibrary() ? m_library_list : m_project_list;
	const QTreeWidgetItem *item = list->currentItem();
	return item && item->isSelected() ? item->text(0) : QString();
}

void DeviceTypesDialog::select(const QString &name)
{
	QTreeWidget *list = inLibrary() ? m_library_list : m_project_list;
	for (int i = 0 ; i < list->topLevelItemCount() ; ++i)
		if (list->topLevelItem(i)->text(0) == name) {
			list->setCurrentItem(list->topLevelItem(i));
			return;
		}
}

/**
	@brief DeviceTypesDialog::refresh
	List both again, keeping what is selected
*/
void DeviceTypesDialog::refresh()
{
	const QString selected = selectedName();

		//How many symbols name each type: a column of element_info
	const QHash<QString, int> used = DeviceTypeList::usedBy(m_project);

	const QString lang = QETApp::langFromSetting();
	const auto fill = [&](QTreeWidget *list, const QList<DeviceType> &types, bool count) {
		list->setSortingEnabled(false);
		list->clear();
		for (const DeviceType &type : types) {
			auto *item = new QTreeWidgetItem(list);
			item->setText(0, type.name);
			item->setText(1, type.text(lang));
			item->setData(2, Qt::DisplayRole, int(type.parts.size()));
			if (count) item->setData(3, Qt::DisplayRole, used.value(type.name));
		}
		list->setSortingEnabled(true);
	};
	fill(m_project_list, m_project ? m_project->deviceTypes() : QList<DeviceType>(), true);
	QList<DeviceType> library;
	for (const QString &name : DeviceTypeLibrary::names(m_folder))
		library << DeviceTypeLibrary::load(name, m_folder);
	fill(m_library_list, library, false);

	if (!selected.isEmpty()) select(selected);
	updateButtons();
}

QStringList DeviceTypesDialog::names(bool library) const
{
	if (library) return DeviceTypeLibrary::names(m_folder);
	QStringList list;
	if (m_project)
		for (const DeviceType &type : m_project->deviceTypes()) list << type.name;
	return list;
}

DeviceType DeviceTypesDialog::typeNamed(const QString &name, bool library) const
{
	if (library) return DeviceTypeLibrary::load(name, m_folder);
	return m_project ? m_project->deviceType(name) : DeviceType();
}

/**
	@brief DeviceTypesDialog::freeName
	@return @p name, or with " 2", " 3"... added, whichever @p taken has not got
*/
QString DeviceTypesDialog::freeName(const QString &name, const QStringList &taken)
{
	if (!taken.contains(name)) return name;
	for (int n = 2 ; ; ++n) {
		const QString candidate = QStringLiteral("%1 %2").arg(name).arg(n);
		if (!taken.contains(candidate)) return candidate;
	}
}

/**
	@brief DeviceTypesDialog::keep
	Keep @p type in the project (one undo step) or in the library
*/
bool DeviceTypesDialog::keep(const DeviceType &type, bool library)
{
	if (type.isNull()) return false;
	bool done = true;
	if (library)
		done = DeviceTypeLibrary::save(type, m_folder);
	else if (m_project)
		m_project->undoStack()->push(new EditDeviceTypeCommand(m_project, type.name, type));
	else
		done = false;
	refresh();
	select(type.name);
	return done;
}

/**
	@brief DeviceTypesDialog::remove
	Delete the type @p name from the project (one undo step) or the library
*/
bool DeviceTypesDialog::remove(const QString &name, bool library)
{
	bool done = true;
	if (library)
		done = DeviceTypeLibrary::remove(name, m_folder);
	else if (m_project && !m_project->deviceType(name).isNull())
		m_project->undoStack()->push(new EditDeviceTypeCommand(m_project, name, DeviceType()));
	else
		done = false;
	refresh();
	return done;
}

/**
	@brief DeviceTypesDialog::copy
	Copy the type @p name from the project to the library, or back
*/
bool DeviceTypesDialog::copy(const QString &name, bool to_library)
{
	const DeviceType type = typeNamed(name, !to_library);
	if (type.isNull()) return false;
	return keep(type, to_library);
}

void DeviceTypesDialog::newType()
{
	const bool library = inLibrary();
	DeviceTypeEditor editor(DeviceType(), m_project, names(library), this);
	if (editor.exec() == QDialog::Accepted) keep(editor.type(), library);
}

void DeviceTypesDialog::duplicate()
{
	const bool library = inLibrary();
	DeviceType type = typeNamed(selectedName(), library);
	if (type.isNull()) return;
	const QStringList taken = names(library);
	type.name = freeName(type.name, taken);
	DeviceTypeEditor editor(type, m_project, taken, this);
	editor.setNameEditable(true);
	if (editor.exec() == QDialog::Accepted) keep(editor.type(), library);
}

void DeviceTypesDialog::edit()
{
	const bool library = inLibrary();
	const DeviceType type = typeNamed(selectedName(), library);
	if (type.isNull()) return;
	DeviceTypeEditor editor(type, m_project, QStringList(), this);
	if (editor.exec() == QDialog::Accepted && editor.type() != type)
		keep(editor.type(), library);
}

void DeviceTypesDialog::removeSelected()
{
	const bool library = inLibrary();
	const QString name = selectedName();
	if (name.isEmpty()) return;
	const QString question = library
		//: %1 is a device type's name, such as "CONTR10"
		? tr("Delete the device type %1 from your library? This cannot be undone.").arg(name)
		: tr("Delete the device type %1 from the project? Its devices keep their "
			 "Device type field and are listed as of a type the project does not have.").arg(name);
	if (QET::QetMessageBox::question(this, tr("Delete a device type"), question,
									 QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
			!= QMessageBox::Yes)
		return;
	remove(name, library);
}

void DeviceTypesDialog::copySelected()
{
	const bool library = inLibrary();
	const QString name = selectedName();
	if (name.isEmpty()) return;
	if (names(!library).contains(name)) {
		const QString question = library
			? tr("The project already has a device type %1. Replace it?").arg(name)
			: tr("Your library already has a device type %1. Replace it?").arg(name);
		if (QET::QetMessageBox::question(this, tr("Copy a device type"), question,
										 QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
				!= QMessageBox::Yes)
			return;
	}
	copy(name, !library);
}

/**
	@brief DeviceTypesDialog::listCsv
	@return the device types of the tab shown, as a list of @p kind,
	described in the language they are shown in; the brief list of the
	project's says how many symbols use each type
*/
QString DeviceTypesDialog::listCsv(DeviceTypeList::Kind kind) const
{
	QList<DeviceType> types;
	for (const QString &name : names(inLibrary())) types << typeNamed(name, inLibrary());
	const QHash<QString, int> used = DeviceTypeList::usedBy(m_project);
	return DeviceTypeList::csv(types, kind, inLibrary() ? nullptr : &used,
							   QETApp::langFromSetting());
}

void DeviceTypesDialog::exportList(DeviceTypeList::Kind kind)
{
	const QString path = QFileDialog::getSaveFileName(
				this, tr("Export the list of device types"),
				kind == DeviceTypeList::Kind::Full ? tr("device_types.csv")
												   : tr("device_types_brief.csv"),
				tr("CSV files (*.csv)"));
	if (path.isEmpty()) return;
	QString error;
	if (!DeviceTypeList::write(path, listCsv(kind), &error))
		QET::QetMessageBox::warning(this, tr("Export the list of device types"),
									//: %1 is a file, %2 why it cannot be written
									tr("Cannot write %1: %2").arg(path, error));
}

void DeviceTypesDialog::updateButtons()
{
	const bool selected = !selectedName().isEmpty();
	m_edit->setEnabled(selected);
	m_duplicate->setEnabled(selected);
	m_remove->setEnabled(selected);
	m_copy->setEnabled(selected && (inLibrary() ? !m_project.isNull() : true));
	m_copy->setText(inLibrary() ? tr("Copy to the project") : tr("Copy to your library"));
}

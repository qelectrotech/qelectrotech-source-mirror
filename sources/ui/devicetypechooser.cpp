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
#include "devicetypechooser.h"

#include "../ElementsCollection/elementslocation.h"
#include "../devicetype/devicetype.h"
#include "../devicetype/devicetypecheck.h"
#include "../diagram.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/element.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "../undocommand/setdevicetypecommand.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QRegularExpression>
#include <QUndoCommand>
#include <QVBoxLayout>

namespace
{
		/// The device types chosen lately, the last one first
	QStringList &recent()
	{
		static QStringList list;
		return list;
	}

		/// The user's library, read once: it may hold hundreds of types
	const QHash<QString, DeviceType> &library()
	{
		static QHash<QString, DeviceType> types;
		static int read = -1;
		const QStringList names = DeviceTypeLibrary::names();
		if (read != names.size()) {
			types.clear();
			for (const QString &name : names)
				types.insert(name, DeviceTypeLibrary::load(name));
			read = int(names.size());
		}
		return types;
	}

	QETProject *projectOf(Element *element)
	{
		return element && element->diagram() ? element->diagram()->project() : nullptr;
	}

	DeviceType typeNamed(Element *symbol, const QString &name)
	{
		QETProject *project = projectOf(symbol);
		const DeviceType own = project ? project->deviceType(name) : DeviceType();
		return own.isNull() ? library().value(name) : own;
	}

		/// Whether @p symbol can draw the head of @p type: a way of its first
		/// master part is drawn with an element of the same name
	bool drawsHead(Element *symbol, const DeviceType &type)
	{
		for (const DeviceType::Part &part : type.parts) {
			if (part.role != QLatin1String("master")) continue;
			const QString path = symbol->location().collectionPath(false);
			for (const DeviceType::Symbol &way : part.symbols) {
				if (DeviceTypeCheck::symbolLocation(way.location, projectOf(symbol)).name() == symbol->name())
					return true;
					//A symbol of a collection, imported into the project:
					//embed://import/ then the same path
				const int protocol = way.location.indexOf(QLatin1String("://"));
				const QString way_path = way.location.mid(protocol < 0 ? 0 : protocol + 3);
				if (!way_path.isEmpty() && (path == way_path
											|| path.endsWith(QLatin1Char('/') + way_path)))
					return true;
			}
			return false;
		}
		return false;
	}
}

/**
	@brief DeviceTypeChooser::DeviceTypeChooser
	@param symbol : the symbol whose device type is asked
	@param parent : parent widget
	@param current : its type now, left out of the list (changing a type)
*/
DeviceTypeChooser::DeviceTypeChooser(Element *symbol, QWidget *parent, const QString &current) :
	QDialog(parent),
	m_symbol(symbol),
	m_current(current)
{
	setWindowTitle(tr("Device type"));
	auto *layout = new QVBoxLayout(this);
	const QString label = symbol ? symbol->elementInformations()
								   .value(QETInformation::ELMT_LABEL).toString() : QString();
	auto *help = new QLabel(label.isEmpty()
		? tr("Which device type is this device? Its parts are then checked against it.")
		//: %1 is the device's label, such as "-K1"
		: tr("Which device type is %1? Its parts are then checked against it.").arg(label), this);
	help->setWordWrap(true);
	layout->addWidget(help);
	m_filter = new QLineEdit(this);
	m_filter->setObjectName(QStringLiteral("deviceTypeFilter"));
	m_filter->setPlaceholderText(tr("Filter, * for any characters"));
	m_filter->setClearButtonEnabled(true);
	layout->addWidget(m_filter);
	m_list = new QListWidget(this);
	m_list->setObjectName(QStringLiteral("deviceTypeChoices"));
	layout->addWidget(m_list, 1);
	m_all = new QCheckBox(tr("Show every device type, not only those this symbol can draw"), this);
	layout->addWidget(m_all);
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, this, &DeviceTypeChooser::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &DeviceTypeChooser::reject);
	connect(m_list, &QListWidget::itemDoubleClicked, this, &DeviceTypeChooser::accept);
	connect(m_filter, &QLineEdit::textChanged, this, &DeviceTypeChooser::fill);
	connect(m_all, &QCheckBox::toggled, this, &DeviceTypeChooser::fill);
	resize(460, 420);
	fill();
}

QString DeviceTypeChooser::chosen() const
{
	const QListWidgetItem *item = m_list->currentItem();
	return item ? item->data(Qt::UserRole).toString() : QString();
}

QStringList DeviceTypeChooser::shown() const
{
	QStringList names;
	for (int i = 0 ; i < m_list->count() ; ++i)
		names << m_list->item(i)->data(Qt::UserRole).toString();
	return names;
}

void DeviceTypeChooser::setFilter(const QString &text) {m_filter->setText(text);}
void DeviceTypeChooser::setShowAll(bool all) {m_all->setChecked(all);}

/**
	@brief DeviceTypeChooser::candidates
	@return the names of the project's and the library's types @p symbol
	can draw the head of (all of them when @p all): the ones chosen lately
	first, then the project's, then the library's, each by name
*/
QStringList DeviceTypeChooser::candidates(Element *symbol, bool all)
{
	QStringList project_names, library_names;
	if (QETProject *project = projectOf(symbol))
		for (const DeviceType &type : project->deviceTypes())
			if (all || drawsHead(symbol, type)) project_names << type.name;
	for (auto it = library().cbegin() ; it != library().cend() ; ++it)
		if (!project_names.contains(it.key()) && (all || drawsHead(symbol, it.value())))
			library_names << it.key();
	project_names.sort(Qt::CaseInsensitive);
	library_names.sort(Qt::CaseInsensitive);
	QStringList names;
	for (const QString &name : recent())
		if (project_names.contains(name) || library_names.contains(name)) names << name;
	for (const QString &name : project_names + library_names)
		if (!names.contains(name)) names << name;
	return names;
}

void DeviceTypeChooser::fill()
{
	m_list->clear();
	if (!m_symbol) return;
	const QString lang = QETApp::langFromSetting();
	const QRegularExpression filter(QRegularExpression::wildcardToRegularExpression(
										QStringLiteral("*") + m_filter->text().trimmed() + QStringLiteral("*"),
										QRegularExpression::UnanchoredWildcardConversion),
									QRegularExpression::CaseInsensitiveOption);
	QETProject *project = projectOf(m_symbol);
	for (const QString &name : candidates(m_symbol, m_all->isChecked())) {
		if (name == m_current) continue;
		const DeviceType type = typeNamed(m_symbol, name);
		const QString text = type.text(lang);
		if (!filter.match(name).hasMatch() && !filter.match(text).hasMatch()) continue;
		const bool own = project && !project->deviceType(name).isNull();
		auto *item = new QListWidgetItem(own
			//: %1 is a device type's name, %2 its description
			? tr("%1 - %2").arg(name, text)
			: tr("%1 - %2 (your library)").arg(name, text), m_list);
		item->setData(Qt::UserRole, name);
	}
	if (m_list->count()) m_list->setCurrentRow(0);
}

/**
	@brief DeviceTypeChooser::give
	@return the undo command giving @p symbol the device type @p name: its
	Device type field, and a copy of the library's type when the project has
	not got it; nullptr if it has that type already
*/
QUndoCommand *DeviceTypeChooser::give(Element *symbol, const QString &name)
{
	QETProject *project = projectOf(symbol);
	if (!project || name.isEmpty()) return nullptr;
	const DiagramContext old_info = symbol->elementInformations();
	if (DeviceTypeCheck::typeNameOf(old_info) == name) return nullptr;
	//: %1 is a device type's name, such as "CONTR10"
	auto *undo = new QUndoCommand(QObject::tr("Give the device type %1").arg(name));
	if (project->deviceType(name).isNull()) {
		const DeviceType from_library = library().value(name);
		if (!from_library.isNull()) new SetDeviceTypeCommand(project, from_library, undo);
	}
	DiagramContext new_info = old_info;
	new_info.addValue(QETInformation::ELMT_DEVICE_TYPE, name);
	new ChangeElementInformationCommand(symbol, old_info, new_info, undo);
	return undo;
}

void DeviceTypeChooser::remember(const QString &name)
{
	recent().removeAll(name);
	recent().prepend(name);
	while (recent().size() > 10) recent().removeLast();
}

/**
	@brief DeviceTypeChooser::askFor
	A master symbol just placed in a project following the device types:
	ask its type, if there is any to choose from, and give it
*/
void DeviceTypeChooser::askFor(Element *symbol, QWidget *parent)
{
	QETProject *project = projectOf(symbol);
	if (!project || !project->followDeviceTypes() || !(symbol->linkType() & Element::Master))
		return;
	if (!DeviceTypeCheck::typeNameOf(symbol->elementInformations()).isEmpty()) return;
	if (candidates(symbol, true).isEmpty()) return;
	DeviceTypeChooser dialog(symbol, parent);
	if (dialog.exec() != QDialog::Accepted || dialog.chosen().isEmpty() || !symbol->diagram())
		return;
	if (QUndoCommand *undo = give(symbol, dialog.chosen())) {
		symbol->diagram()->undoStack().push(undo);
		remember(dialog.chosen());
	}
}

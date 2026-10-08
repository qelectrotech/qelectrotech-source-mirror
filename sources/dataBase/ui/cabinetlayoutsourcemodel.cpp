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

#include "cabinetlayoutsourcemodel.h"

#include "../projectdatabase.h"
#include "../../qetproject.h"
#include "../../diagram.h"
#include "../../qetgraphicsitem/cabinetlayoutreferenceitem.h"
#include "../../qetgraphicsitem/element.h"
#include "../../elementprovider.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMimeData>
#include <QFont>
#include <QBrush>

const QString CabinetLayoutSourceModel::CABINET_LAYOUT_SOURCE_MIME_TYPE =
		QStringLiteral("application/x-qet-cabinet-layout-uuid");

/**
	@brief CabinetLayoutSourceModel::CabinetLayoutSourceModel
	@param project the project whose placed elements are listed.
	@param parent
*/
CabinetLayoutSourceModel::CabinetLayoutSourceModel(QETProject *project, QObject *parent) :
	QStandardItemModel(parent),
	m_project(project)
{
	setColumnCount(1);
	setHorizontalHeaderLabels({tr("Éléments du projet")});

	if (m_project && m_project->dataBase()) {
		connect(m_project->dataBase(), &projectDataBase::dataBaseUpdated,
				this, &CabinetLayoutSourceModel::reload);
	}

	if (m_project) {
		for (Diagram *dia : m_project->diagrams())
			connectDiagram(dia);

		connect(m_project, &QETProject::diagramAdded,
				this, [this](QETProject *, Diagram *dia) { connectDiagram(dia); });
	}
}

/**
	@brief CabinetLayoutSourceModel::setActiveDiagram
	See header.
*/
void CabinetLayoutSourceModel::setActiveDiagram(Diagram *diagram)
{
	if (m_active_diagram == diagram)
		return;

	m_active_diagram = diagram;
	reload();
}

/**
	@brief CabinetLayoutSourceModel::connectDiagram
	See header.
*/
void CabinetLayoutSourceModel::connectDiagram(Diagram *diagram)
{
	if (!diagram)
		return;

	connect(diagram, &Diagram::cabinetLayoutReferencesChanged,
			this, &CabinetLayoutSourceModel::reload);
}

/**
	@brief CabinetLayoutSourceModel::reload
	Rebuild the whole tree from the project's element_nomenclature_view.
	Folios are the top-level items (one per diagram, ordered by folio
	number where numeric), elements placed on that folio are their
	children. Elements with neither a width nor a height set are
	skipped, since they cannot yield a meaningful cabinet layout box.
*/
void CabinetLayoutSourceModel::reload()
{

	removeRows(0, rowCount());

	if (!m_project || !m_project->dataBase())
		return;

	QSqlQuery query = m_project->dataBase()->newQuery(
		"SELECT title, folio, element_uuid, label, function, "
		"		description, manufacturer, "
		"       width, height, depth "
		"FROM element_nomenclature_view "
		"ORDER BY CAST(folio AS INTEGER), folio, label"
	);

	if (!query.exec()) {
		qWarning() << "CabinetLayoutSourceModel::reload SQL error:"
				   << query.lastError();
		return;
	}

	QStandardItem *root = invisibleRootItem();
	QStandardItem *current_folio_item = nullptr;
	QString current_folio_key;
	ElementProvider provider(m_project);

	while (query.next())
	{
		const QString title		= query.value("title").toString();
		const QString folio		= query.value("folio").toString();
		const QString uuid		= query.value("element_uuid").toString();
		QString label			= query.value("label").toString();
		{
			const QList<Element *> found = provider.fromUuids({QUuid(uuid)});
			if (!found.isEmpty()) {
				const QString live = found.first()->actualLabel();
				if (!live.isEmpty()) {
					label = live;
				}
			}
		}
		const QString func		= query.value("function").toString();
		const QString desc		= query.value("description").toString();
		const QString mfr		= query.value("manufacturer").toString();
		const QString width		= query.value("width").toString();
		const QString height	= query.value("height").toString();
		const QString depth		= query.value("depth").toString();

		const bool is_front = !m_active_diagram || m_active_diagram->cabinetLayoutView() == Diagram::CabinetLayoutFront;
		const QString relevant_dimension = is_front ? width : depth;
		const bool has_dimensions = relevant_dimension.toDouble() > 0.0 && height.toDouble() > 0.0;

		const QString folio_key = folio + '\x1f' + title;
		if (folio_key != current_folio_key)
		{
			const QString folio_label = folio.isEmpty()
					? title
					: tr("%1 — %2").arg(folio, title);
			current_folio_item = new QStandardItem(folio_label);
			current_folio_item->setEditable(false);
			current_folio_item->setSelectable(false);
			current_folio_item->setDragEnabled(false);
			root->appendRow(current_folio_item);
			current_folio_key = folio_key;
		}

		QString element_label = label.isEmpty() ? tr("(element sans label)") : label;
		if (!func.isEmpty())
			element_label += QStringLiteral(" — ") + func;
		if (!desc.isEmpty())
			element_label += QStringLiteral(" — ") + desc;
		if (!mfr.isEmpty())
			element_label += QStringLiteral(" — ") + mfr;
		if (!has_dimensions)
			element_label = QStringLiteral("⚠ ") + element_label;

		const bool already_placed = m_active_diagram
				&& m_active_diagram->cabinetLayoutEnabled()
				&& CabinetLayoutReferenceItem::existsReferenceFor(
					   m_project, QUuid(uuid),
					   m_active_diagram->cabinetLayoutView() == Diagram::CabinetLayoutSide);

		auto *element_item = new QStandardItem(element_label);
		element_item->setEditable(false);
		element_item->setDragEnabled(has_dimensions && !already_placed);
		element_item->setDropEnabled(false);
		element_item->setData(uuid, Qt::UserRole);
		element_item->setData(width,  Qt::UserRole + 1);
		element_item->setData(height, Qt::UserRole + 2);
		element_item->setData(depth,  Qt::UserRole + 3);

		if (!has_dimensions) {
			QFont font = element_item->font();
			font.setItalic(true);
			element_item->setFont(font);
			element_item->setForeground(QBrush(Qt::gray));
			element_item->setToolTip(
				tr("Aucune dimension renseignée pour cet élément -- "
				   "ajoutez au moins la largeur ou la hauteur dans "
				   "l'éditeur d'élément pour pouvoir le glisser ici."));
		} else if (already_placed) {
				//Deliberately not italic (unlike the missing-dimensions
				//case above): italic is reserved for "this entry needs
				//attention/action", whereas an already-placed device is
				//simply unavailable right now, not a problem to fix.
			element_item->setForeground(QBrush(Qt::gray));
			element_item->setToolTip(
				tr("Déjà placé sur ce type de vue (face/côté) -- "
				   "changez de folio ou de vue pour le replacer."));
		} else {
			element_item->setToolTip(
				tr("Largeur : %1 mm, Hauteur : %2 mm, Profondeur : %3 mm")
					.arg(width.isEmpty()  ? QStringLiteral("—") : width)
					.arg(height.isEmpty() ? QStringLiteral("—") : height)
					.arg(depth.isEmpty()  ? QStringLiteral("—") : depth));
		}

		if (current_folio_item)
			current_folio_item->appendRow(element_item);
	}
}

QStringList CabinetLayoutSourceModel::mimeTypes() const
{
	return {CABINET_LAYOUT_SOURCE_MIME_TYPE};
}

/**
	@brief CabinetLayoutSourceModel::mimeData
	Only the element's UUID is carried in the drag payload -- width,
	height and label are looked up fresh from the database at drop
	time, the same way ElementsCollectionModel only carries a
	collection path rather than the symbol's full geometry. This
	avoids placing a cabinet layout box with stale dimensions if the
	source element was edited after the drag started.
*/
QMimeData *CabinetLayoutSourceModel::mimeData(const QModelIndexList &indexes) const
{
	if (indexes.isEmpty())
		return nullptr;

	const QModelIndex index = indexes.first();
	const QString uuid = index.data(Qt::UserRole).toString();
	if (uuid.isEmpty())
		return nullptr;

	auto *mime_data = new QMimeData();
	mime_data->setData(CABINET_LAYOUT_SOURCE_MIME_TYPE, uuid.toLatin1());
	return mime_data;
}
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

#ifndef CABINETLAYOUTSOURCEMODEL_H
#define CABINETLAYOUTSOURCEMODEL_H

#include <QStandardItemModel>
#include <QPointer>

class QETProject;
class Diagram;

/**
	@brief The CabinetLayoutSourceModel class
	Populates a tree of the elements currently placed in a project,
	grouped by the folio (diagram) they are placed on, for use as a
	drag source when building a scale-accurate cabinet/panel layout
	drawing ("disposition des armoires").

	Top-level items represent folios (one per diagram, labelled with
	its title and folio number). Their children represent the elements
	placed on that folio that are eligible for a cabinet layout box
	(i.e. not excluded from the BOM and carrying at least a width or
	height).

	Each element item carries, via Qt::UserRole, the source element's
	UUID -- used at drop time to look up the current width/height/depth
	and label directly from the project database, rather than caching
	values that could go stale.

	The model rebuilds itself whenever the project's database signals
	that it was updated (@see projectDataBase::dataBaseUpdated), so it
	stays in sync as elements are added, removed or edited.

	Devices already placed as a cabinet layout reference matching the
	*currently active folio's* view (front/side, @see
	Diagram::cabinetLayoutView()) are shown greyed out and not
	draggable, since CabinetLayoutReferenceItem::existsReferenceFor()
	would reject a second one for that exact (device, view)
	combination anyway. This is folio-dependent, not global: the same
	device is drag-enabled again once the active folio's view differs
	from the one it's already placed as -- @see setActiveDiagram(),
	called whenever the active MDI subwindow changes.
*/
class CabinetLayoutSourceModel : public QStandardItemModel
{
	Q_OBJECT

	public:
		explicit CabinetLayoutSourceModel(QETProject *project, QObject *parent = nullptr);
		~CabinetLayoutSourceModel() override = default;

		QStringList mimeTypes() const override;
		QMimeData *mimeData(const QModelIndexList &indexes) const override;

		static const QString CABINET_LAYOUT_SOURCE_MIME_TYPE;

		/**
			@brief setActiveDiagram
			@param diagram the folio the user currently has active, whose
			cabinetLayoutView() determines which already-placed devices
			get greyed out on the next reload(). Passing nullptr (no
			folio active) greys out nothing.
		*/
		void setActiveDiagram(Diagram *diagram);

	private slots:
		void connectDiagram(Diagram *diagram);

	public slots:
		void reload();

	private:
		QPointer<QETProject> m_project;
		QPointer<Diagram> m_active_diagram;
};

#endif // CABINETLAYOUTSOURCEMODEL_H
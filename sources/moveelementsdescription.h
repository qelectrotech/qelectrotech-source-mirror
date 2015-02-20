/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef MOVE_ELEMENTS_DESCRIPTION_H
#define MOVE_ELEMENTS_DESCRIPTION_H
#include <QObject>
class ElementsCollectionItem;
class ElementsCategory;
class MoveElementsHandler;
/**
	This class describes the evolution of an elements item move (actually
	either move or copy), i.e. whether the source was deleted, whether the
	target changed and to what path, etc.
*/
class MoveElementsDescription : public QObject {
	Q_OBJECT
	
	// constructors, destructor
	public:
	MoveElementsDescription(QObject * = 0);
	virtual ~MoveElementsDescription();
	private:
	MoveElementsDescription(const MoveElementsDescription &);
	
	// methods
	public:
	bool isRecursive() const;
	void setRecursive(bool);
	
	MoveElementsHandler *handler() const;
	void setHandler(MoveElementsHandler *);
	
	ElementsCategory *destinationParentCategory() const;
	void setDestinationParentCategory(ElementsCategory *);
	
	QString originalDestinationInternalName() const;
	void setOriginalDestinationInternalName(const QString &);
	
	QString finalDestinationInternalName() const;
	void setFinalDestinationInternalName(const QString &);
	
	ElementsCollectionItem *createdItem() const;
	void setCreatedItem(ElementsCollectionItem *);
	
	bool sourceItemWasDeleted() const;
	void setSourceItemDeleted(bool);
	
	bool mustAbort() const;
	void abort();
	
	// attributes
	private:
	bool recursive_;
	MoveElementsHandler *handler_;
	ElementsCategory *destination_;
	QString dest_internal_name_orig_;
	QString dest_internal_name_final_;
	ElementsCollectionItem *created_item_;
	bool src_deleted_;
	bool abort_;
};
#endif

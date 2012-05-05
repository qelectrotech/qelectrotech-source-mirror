/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef EDITOR_COMMANDS_H
#define EDITOR_COMMANDS_H
#include "customelementpart.h"
#include "partpolygon.h"
#include "elementview.h"
#include "elementscene.h"
#include "elementcontent.h"
#include "qgimanager.h"
#include <QtGui>
/**
	Cette classe represente l'action de supprimer une ou plusieurs
	parties lors de l'edition d'un element
*/
class DeletePartsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	DeletePartsCommand(ElementScene *, const QList<QGraphicsItem *>, QUndoCommand * = 0);
	virtual ~DeletePartsCommand();
	private:
	DeletePartsCommand(const DeletePartsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Liste des parties supprimees
	QList<QGraphicsItem *> deleted_parts;
	/// scene sur laquelle se produisent les actions
	ElementScene *editor_scene;
};

/**
	Cette classe represente l'action de coller quelque chose dans un element
*/
class PastePartsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	PastePartsCommand(ElementView *, const ElementContent &, QUndoCommand * = 0);
	virtual ~PastePartsCommand();
	private:
	PastePartsCommand(const PastePartsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	virtual void setOffset(int, const QPointF &, int, const QPointF &);
	
	// attributs
	private:
	/// contenu  ajoute
	ElementContent content_;
	/// schema sur lequel on colle les elements et conducteurs
	ElementView *editor_view_;
	ElementScene *editor_scene_;
	/// Informations pour annuler un c/c avec decalage
	int old_offset_paste_count_;
	QPointF old_start_top_left_corner_;
	int new_offset_paste_count_;
	QPointF new_start_top_left_corner_;
	bool uses_offset;
	/// booleen pour empecher le premier appel a redo
	bool first_redo;
};

/**
	Cette classe represente l'action de supprimer des parties d'un element
*/
class CutPartsCommand : public DeletePartsCommand {
	// constructeurs, destructeur
	public:
	CutPartsCommand(ElementScene *, const QList<QGraphicsItem *>, QUndoCommand * = 0);
	virtual ~CutPartsCommand();
	private:
	CutPartsCommand(const CutPartsCommand &);
};

/**
	Cette classe represente l'action de deplacer une ou plusieurs
	parties lors de l'edition d'un element
*/
class MovePartsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	MovePartsCommand(const QPointF &, ElementScene *, const QList<QGraphicsItem *>, QUndoCommand * = 0);
	virtual ~MovePartsCommand();
	private:
	MovePartsCommand(const MovePartsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Liste des parties supprimees
	QList<QGraphicsItem *> moved_parts;
	/// scene sur laquelle se produisent les actions
	ElementScene *editor_scene;
	/// translation appliquee
	QPointF movement;
	/// booleen pour eviter d'appeler redo() lors de la construction de l'objet
	bool first_redo;
};

/**
	Cette classe represente l'action d'ajouter une partie lors de l'edition
	d'un element
*/
class AddPartCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	AddPartCommand(const QString &, ElementScene *, QGraphicsItem *, QUndoCommand * = 0);
	virtual ~AddPartCommand();
	private:
	AddPartCommand(const AddPartCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Liste des parties supprimees
	QGraphicsItem *part;
	/// scene sur laquelle se produisent les actions
	ElementScene *editor_scene;
	/// booleen pour eviter d'appeler redo() lors de la construction de l'objet
	bool first_redo;
};

/**
	Cette classe represente l'action de modifier une propriete d'une partie
	lors de l'edition d'un element
*/
class ChangePartCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangePartCommand(const QString &, CustomElementPart *, const QString &, const QVariant &, const QVariant &, QUndoCommand * = 0);
	virtual ~ChangePartCommand();
	private:
	ChangePartCommand(const ChangePartCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Partie modifiee
	CustomElementPart *cep;
	/// Propriete modifiee
	QString property;
	/// ancienne valeur
	QVariant old_value;
	/// nouvelle valeur
	QVariant new_value;
};

/**
	Cette classe represente l'action de modifier les points composants un polygone
*/
class ChangePolygonPointsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangePolygonPointsCommand(PartPolygon *, const QVector<QPointF> &, const QVector<QPointF> &, QUndoCommand * = 0);
	virtual ~ChangePolygonPointsCommand();
	private:
	ChangePolygonPointsCommand(const ChangePolygonPointsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	/// Polygone  modifie
	PartPolygon *polygon;
	/// anciens points
	QVector<QPointF> old_points;
	/// nouveaux points
	QVector<QPointF> new_points;
};

/**
	Cette classe represente l'action de modifier les dimensions et le point de saisie d'un element
*/
class ChangeHotspotCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeHotspotCommand(ElementScene *, const QSize &, const QSize &, const QPoint &, const QPoint &, const QPoint & = QPoint(), QUndoCommand * = 0);
	virtual ~ChangeHotspotCommand();
	private:
	ChangeHotspotCommand(const ChangeHotspotCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	private:
	void applyOffset(const QPointF &);
	
	// attributs
	/// Element edite auquel il faut appliquer les modifications
	ElementScene *element;
	/// dimensions avant l'action
	QSize size_before;
	/// dimensions apres l'action
	QSize size_after;
	/// point de saisie avant l'action
	QPoint hotspot_before;
	/// point de saisie apres l'action
	QPoint hotspot_after;
	/// decalage a appliquer aux elements
	QPoint offset;
};

/**
	Cette classe represente l'action de changer les noms d'un element
*/
class ChangeNamesCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeNamesCommand(ElementScene *, const NamesList &, const NamesList &, QUndoCommand * = 0);
	virtual ~ChangeNamesCommand();
	private:
	ChangeNamesCommand(const ChangeNamesCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Liste des noms avant changement
	NamesList names_before;
	/// Liste des noms apres changement
	NamesList names_after;
	/// Element edite auquel il faut appliquer les modifications
	ElementScene *element;
};

/**
	Cette classe represente l'action de changer les noms d'un element
*/
class ChangeOrientationsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeOrientationsCommand(ElementScene *, const OrientationSet &, const OrientationSet &, QUndoCommand * = 0);
	virtual ~ChangeOrientationsCommand();
	private:
	ChangeOrientationsCommand(const ChangeOrientationsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Orientations avant changement
	OrientationSet ori_before;
	/// Orientations apres changement
	OrientationSet ori_after;
	/// Element edite auquel il faut appliquer les modifications
	ElementScene *element;
};

/**
	Cette classe represente l'action de changer les noms d'un element
*/
class ChangeZValueCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	/// Qualifie le type de changement de zValue
	enum Option {
		BringForward, ///< Amene la partie a l'avant-plan ; elle a alors la plus haute zValue
		Raise,        ///< Amene la partie un plan au-dessus ; la zValue de la partie est incrementee
		Lower,        ///< Envoie la partie un plan en-dessous ; la zValue de la partie est decrementee
		SendBackward  ///< Envoie la partie a l'arriere-plan ; elle a alors la plus faible zValue
	};
	ChangeZValueCommand(ElementScene *, Option, QUndoCommand * = 0);
	virtual ~ChangeZValueCommand();
	private:
	ChangeZValueCommand(const ChangeZValueCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	private:
	void applyBringForward(const QList<QGraphicsItem *> &);
	void applyRaise(const QList<QGraphicsItem *> &);
	void applyLower(const QList<QGraphicsItem *> &);
	void applySendBackward(const QList<QGraphicsItem *> &);
	
	// attributs
	private:
	/// zValues avant changement
	QHash<QGraphicsItem *, qreal> undo_hash;
	/// zValues apres changement
	QHash<QGraphicsItem *, qreal> redo_hash;
	/// Element edite auquel il faut appliquer les modifications
	ElementScene *element;
	/// type de traitement
	Option option;
};

/**
	Cette classe represente l'action d'autoriser ou non les connexions
	internes pour un element.
*/
class AllowInternalConnectionsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	AllowInternalConnectionsCommand(ElementScene *, bool, QUndoCommand * = 0);
	virtual ~AllowInternalConnectionsCommand();
	private:
	AllowInternalConnectionsCommand(const AllowInternalConnectionsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Element edite auquel il faut appliquer les modifications
	ElementScene *element;
	/// autorisation des connexions internes apres modification
	bool ic;
};

/**
	Cette classe represente l'action de changer les informations
	complementaires d'un element.
*/
class ChangeInformationsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeInformationsCommand(ElementScene *, const QString &, const QString &, QUndoCommand * = 0);
	virtual ~ChangeInformationsCommand();
	private:
	ChangeInformationsCommand(const ChangeInformationsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// Element edite auquel il faut appliquer les modifications
	ElementScene *element;
	/// Informations avant modification
	QString old_informations_;
	/// Informations apres modification
	QString new_informations_;
};
#endif

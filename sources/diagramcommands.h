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
#ifndef DIAGRAM_COMMANDS_H
#define DIAGRAM_COMMANDS_H
#include <QtGui>
#include "borderproperties.h"
#include "conductor.h"
#include "conductorproperties.h"
#include "diagramcontent.h"
#include "titleblockproperties.h"
#include "qet.h"
class Diagram;
class DiagramTextItem;
class Element;
class ElementTextItem;
class IndependentTextItem;

/**
	Cette classe represente l'action d'ajouter un element au schema
*/
class AddElementCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	AddElementCommand(Diagram *, Element *, const QPointF &, QUndoCommand * = 0);
	virtual ~AddElementCommand();
	private:
	AddElementCommand(const AddElementCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// element ajoute
	Element *element;
	/// schema sur lequel on ajoute l'element
	Diagram *diagram;
	/// position de l'element sur le schema
	QPointF position;
};

/**
	Cette classe represente l'action d'ajouter du texte au schema
*/
class AddTextCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	AddTextCommand(Diagram *, IndependentTextItem *, const QPointF &, QUndoCommand * = 0);
	virtual ~AddTextCommand();
	private:
	AddTextCommand(const AddTextCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// texte ajoute
	IndependentTextItem *textitem;
	/// schema sur lequel on ajoute le texte
	Diagram *diagram;
	/// position du texte sur le schema
	QPointF position;
};

/**
	Cette classe represente l'action d'ajouter un conducteur au schema
*/
class AddConductorCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	AddConductorCommand(Diagram *, Conductor *, QUndoCommand * = 0);
	virtual ~AddConductorCommand();
	private:
	AddConductorCommand(const AddConductorCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// conducteur ajoute
	Conductor *conductor;
	/// schema auquel on ajoute le conducteur
	Diagram *diagram;
};

/**
	Cette classe represente l'action de supprimer des elements, conducteurs
	et / ou textes independants d'un schema
*/
class DeleteElementsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	DeleteElementsCommand(Diagram *, const DiagramContent &, QUndoCommand * = 0);
	virtual ~DeleteElementsCommand();
	private:
	DeleteElementsCommand(const DeleteElementsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// contenu enleve
	DiagramContent removed_content;
	/// schema dont on supprime des elements et conducteurs
	Diagram *diagram;
};

/**
	Cette classe represente l'action de coller quelque chose sur un schema
*/
class PasteDiagramCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	PasteDiagramCommand(Diagram *, const DiagramContent &, QUndoCommand * = 0);
	virtual ~PasteDiagramCommand();
	private:
	PasteDiagramCommand(const PasteDiagramCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// contenu  ajoute
	DiagramContent content;
	/// schema sur lequel on colle les elements et conducteurs
	Diagram *diagram;
	/// entien pour filtrer le contenu du schema
	int filter;
	/// booleen pour empecher le premier appel a redo
	bool first_redo;
};

/**
	Cette classe represente l'action de supprimer des elements et / ou
	conducteurs d'un schema
*/
class CutDiagramCommand : public DeleteElementsCommand {
	// constructeurs, destructeur
	public:
	CutDiagramCommand(Diagram *, const DiagramContent &, QUndoCommand * = 0);
	virtual ~CutDiagramCommand();
	private:
	CutDiagramCommand(const CutDiagramCommand &);
};

/**
	Cette classe represente l'action de deplacer des elements et des
	conducteurs sur un schema
*/
class MoveElementsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	MoveElementsCommand(Diagram *, const DiagramContent &, const QPointF &m, QUndoCommand * = 0);
	virtual ~MoveElementsCommand();
	private:
	MoveElementsCommand(const MoveElementsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	virtual void move(const QPointF &);
	virtual void addConductorTextItemMovement(ConductorTextItem *, const QPointF &, const QPointF &);
	
	// attributs
	private:
	/// schema sur lequel on deplace les elements
	Diagram *diagram;
	/// contenu a deplacer
	DiagramContent content_to_move;
	/// mouvement effectue
	QPointF movement;
	/**
		Deplacer des elements ou champs de texte entraine des conducteurs.
		Soit ces conducteurs sont betement deplaces, soit leur trajet est
		recalcule.
		Si leur trajet est recalcule, les champs de texte dont la position a ete
		personnalisee par l'utilisateur
		Liste des champs de texte de conducteurs dont la position a ete modifiee
		par des mises
	*/
	QHash<ConductorTextItem *, QPair<QPointF, QPointF> > moved_conductor_texts_;
	/// booleen pour ne pas executer le premier redo()
	bool first_redo;
};

/**
	Cette classe represente l'action de deplacer des champs de texte rattaches
	a des elements sur un schema
*/
class MoveElementsTextsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	MoveElementsTextsCommand(Diagram *, const QSet<ElementTextItem *> &, const QPointF &m, QUndoCommand * = 0);
	virtual ~MoveElementsTextsCommand();
	private:
	MoveElementsTextsCommand(const MoveElementsTextsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	virtual void move(const QPointF &);
	
	// attributs
	private:
	/// schema sur lequel on deplace les elements
	Diagram *diagram;
	/// liste des champs de texte a deplacer
	QSet<ElementTextItem *> texts_to_move;
	/// mouvement effectue
	QPointF movement;
	/// booleen pour ne pas executer le premier redo()
	bool first_redo;
};

/**
	Cette classe represente l'action de deplacer des champs de texte rattaches
	a des conducteurs sur un schema
*/
class MoveConductorsTextsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	MoveConductorsTextsCommand(Diagram *, QUndoCommand * = 0);
	virtual ~MoveConductorsTextsCommand();
	private:
	MoveConductorsTextsCommand(const MoveConductorsTextsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	virtual void addTextMovement(ConductorTextItem *, const QPointF &, const QPointF &, bool = false);
	
	private:
	void regenerateTextLabel();
	
	// attributs
	private:
	/// schema sur lequel on deplace les elements
	Diagram *diagram;
	/// liste des champs de texte a deplacer
	QHash<ConductorTextItem *, QPair<QPointF, bool> > texts_to_move_;
	/// booleen pour ne pas executer le premier redo()
	bool first_redo;
};

/**
	Cette classe represente la modification d'un champ de texte
*/
class ChangeDiagramTextCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeDiagramTextCommand(DiagramTextItem *, const QString &before, const QString &after, QUndoCommand * = 0);
	virtual ~ChangeDiagramTextCommand();
	private:
	ChangeDiagramTextCommand(const ChangeDiagramTextCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// DiagramTextItem modifie
	DiagramTextItem *text_item;
	/// texte avant changement
	QString text_before;
	/// texte apres changement
	QString text_after;
	/// booleen pour ne pas executer le premier redo()
	bool first_redo;
};

/**
	Cette classe represente l'action de pivoter plusieurs elements ou champs de textes avec un meme angle
*/
class RotateElementsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	RotateElementsCommand(const QHash<Element *, QET::Orientation> &elements, const QList<DiagramTextItem *> &, QUndoCommand * = 0);
	virtual ~RotateElementsCommand();
	private:
	RotateElementsCommand(const RotateElementsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	qreal appliedRotationAngle() const;
	void setAppliedRotationAngle(const qreal &);
	static void rotateElement(Element *, QET::Orientation);
	
	// attributs
	private:
	/// elements pivotes associes a leur ancienne orientation
	QHash<Element *, QET::Orientation> elements_to_rotate;
	/// textes a pivoter
	QList<DiagramTextItem *> texts_to_rotate;
	/// angle de rotation a appliquer aux textes (valeur utilisee dans le redo
	qreal applied_rotation_angle_;
};

/**
	Cette classe represente l'action d'orienter plusieurs textes a un meme angle de rotation bien precis
*/
class RotateTextsCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	RotateTextsCommand(const QHash<DiagramTextItem *, double> &, double, QUndoCommand * = 0);
	RotateTextsCommand(const QList<DiagramTextItem *> &,         double, QUndoCommand * = 0);
	virtual ~RotateTextsCommand();
	private:
	RotateTextsCommand(const RotateTextsCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	private:
	void defineCommandName();
	
	// attributs
	private:
	/// textes pivotes associes a leur ancienne orientation
	QHash<DiagramTextItem *, double> texts_to_rotate;
	/// angle de rotation a appliquer aux textes
	double applied_rotation_angle_;
};

/**
	Cette classe represente l'action de modifier un conducteur
*/
class ChangeConductorCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeConductorCommand(Conductor *, const ConductorProfile &, const ConductorProfile &, Qt::Corner, QUndoCommand * = 0);
	virtual ~ChangeConductorCommand();
	private:
	ChangeConductorCommand(const ChangeConductorCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	virtual void setConductorTextItemMove(const QPointF &, const QPointF &);
	
	// attributs
	private:
	/// conducteur modifie
	Conductor *conductor;
	/// profil avant changement
	ConductorProfile old_profile;
	/// profil apres changement
	ConductorProfile new_profile;
	/// Type de trajet
	Qt::Corner path_type;
	/// Position du champ de texte avant le changement
	QPointF text_pos_before_mov_;
	/// Position du champ de texte apres le changement
	QPointF text_pos_after_mov_;
	/// booleen pour ne pas executer le premier redo()
	bool first_redo;
};

/**
	Cette classe represente l'action de reinitialiser des conducteurs
*/
class ResetConductorCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ResetConductorCommand(const QHash<Conductor *, ConductorProfilesGroup> &, QUndoCommand * = 0);
	virtual ~ResetConductorCommand();
	private:
	ResetConductorCommand(const ResetConductorCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// conducteurs reinitialises associes a leur ancien profil
	QHash<Conductor *, ConductorProfilesGroup> conductors_profiles;
};

/**
	Cette classe represente l'action de modifier les informations du cartouche d'un schema
*/
class ChangeTitleBlockCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeTitleBlockCommand(Diagram *, const TitleBlockProperties &, const TitleBlockProperties &, QUndoCommand * = 0);
	virtual ~ChangeTitleBlockCommand();
	private:
	ChangeTitleBlockCommand(const ChangeTitleBlockCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// schema modifie
	Diagram *diagram;
	/// proprietes avant changement
	TitleBlockProperties old_titleblock;
	/// proprietes apres changement
	TitleBlockProperties new_titleblock;
};

/**
	Cette classe represente l'action de modifier les dimensions d'un schema
*/
class ChangeBorderCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeBorderCommand(Diagram *, const BorderProperties &, const BorderProperties &, QUndoCommand * = 0);
	virtual ~ChangeBorderCommand();
	private:
	ChangeBorderCommand(const ChangeBorderCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	
	// attributs
	private:
	/// schema modifie
	Diagram *diagram;
	public:
	/// anciennes dimensions du schema
	BorderProperties old_properties;
	/// nouvelles dimensions du schema
	BorderProperties new_properties;
};

/**
	Cette classe represente l'action de modifier les proprietes d'un conducteur
*/
class ChangeConductorPropertiesCommand : public QUndoCommand {
	// constructeurs, destructeur
	public:
	ChangeConductorPropertiesCommand(Conductor *, QUndoCommand * = 0);
	virtual ~ChangeConductorPropertiesCommand();
	private:
	ChangeConductorPropertiesCommand(const ChangeConductorPropertiesCommand &);
	
	// methodes
	public:
	virtual void undo();
	virtual void redo();
	virtual void setOldSettings(const ConductorProperties &);
	virtual void setNewSettings(const ConductorProperties &);
	
	// attributs
	private:
	/// conducteur modifie
	Conductor *conductor;
	/// anciennes proprietes
	ConductorProperties old_properties;
	/// nouvelles proprietes
	ConductorProperties new_properties;
	/// booleens indiquant si les proprietes ont ete definies ou non
	bool old_settings_set;
	bool new_settings_set;
};
#endif

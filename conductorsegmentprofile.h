#ifndef CONDUCTOR_SEGMENT_PROFILE_H
#define CONDUCTOR_SEGMENT_PROFILE_H
#include <QtCore>
#include "conductorsegment.h"
/**
	Cette classe contient le profil (= les caracteristiques essentielles) d'un
	segment de conducteur.
*/
class ConductorSegmentProfile {
	// constructeurs, destructeur
	public:
	/**
		Constructeur
		@param l longueur du segment
		@param ori true si le segment est horizontal, false s'il est vertical
	*/
	ConductorSegmentProfile(qreal l, bool ori = true) :
		length(l),
		isHorizontal(ori)
	{
	}
	
	/**
		Constructeur
		@param segment ConductorSegment dont il faut extraire le profil
	*/
	ConductorSegmentProfile(ConductorSegment *segment) :
		length(segment -> length()),
		isHorizontal(segment -> isHorizontal())
	{
	}
	
	/// Destructeur
	virtual ~ConductorSegmentProfile() {
	}
	
	// attributs
	public:
	qreal length; /// longueur du segment
	bool isHorizontal; /// orientation du segment
};
#endif

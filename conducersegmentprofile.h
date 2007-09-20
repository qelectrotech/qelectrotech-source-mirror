#ifndef CONDUCER_SEGMENT_PROFILE_H
#define CONDUCER_SEGMENT_PROFILE_H
#include <QtCore>
#include "conducersegment.h"
/**
	Cette classe contient le profil (= les caracteristiques essentielles) d'un
	segment de conducteur.
*/
class ConducerSegmentProfile {
	// constructeurs, destructeur
	public:
	/**
		Constructeur
		@param l longueur du segment
		@param ori true si le segment est horizontal, false s'il est vertical
	*/
	ConducerSegmentProfile(qreal l, bool ori = true) :
		length(l),
		isHorizontal(ori)
	{
	}
	
	/**
		Constructeur
		@param segment ConducerSegment dont il faut extraire le profil
	*/
	ConducerSegmentProfile(ConducerSegment *segment) :
		length(segment -> length()),
		isHorizontal(segment -> isHorizontal())
	{
	}
	
	/// Destructeur
	virtual ~ConducerSegmentProfile() {
	}
	
	// attributs
	public:
	qreal length; /// longueur du segment
	bool isHorizontal; /// orientation du segment
};
#endif

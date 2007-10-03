#ifndef CONDUCTOR_PROFILE_H
#define CONDUCTOR_PROFILE_H
#include <QList>
#include "qet.h"
class Conductor;
class ConductorSegmentProfile;
/**
	Cette classe contient le profil (= les caracteristiques essentielles) d'un
	conducteur.
*/
class ConductorProfile {
	public:
	// constructeurs, destructeur
	ConductorProfile();
	ConductorProfile(Conductor *conductor);
	ConductorProfile(const ConductorProfile &);
	ConductorProfile &operator=(const ConductorProfile &);
	virtual ~ConductorProfile();
	
	// attributs
	public:
	QList<ConductorSegmentProfile *> segments;
	QET::Orientation beginOrientation;
	QET::Orientation endOrientation;
	
	// methodes
	public:
	bool isNull() const;
	void setNull();
	qreal width() const;
	qreal height() const;
	uint nbSegments(QET::ConductorSegmentType) const;
	QList<ConductorSegmentProfile *> horizontalSegments();
	QList<ConductorSegmentProfile *> verticalSegments();
	void fromConductor(Conductor *);
};
QDebug &operator<<(QDebug d, ConductorProfile &);
#endif

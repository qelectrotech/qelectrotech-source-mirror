#ifndef ORIENTATION_SET_H
#define ORIENTATION_SET_H
#include "qet.h"
class OrientationSet {
	
	// constructeurs, destructeur
	public:
	OrientationSet();
	virtual ~OrientationSet() {};
	
	// attributs
	private:
	bool north_ori;
	bool east_ori;
	bool south_ori;
	bool west_ori;
	QET::Orientation default_ori;
	QET::Orientation current_ori;
	
	// methodes
	public:
	bool north() const;
	bool east() const;
	bool south() const;
	bool west() const;
	bool setNorth(bool);
	bool setEast(bool);
	bool setSouth(bool);
	bool setWest(bool);
	QET::Orientation defaultOrientation() const;
	void setDefaultOrientation(const QET::Orientation &);
	QET::Orientation current() const;
	bool setCurrent(QET::Orientation);
	QET::Orientation next() const;
	QET::Orientation previous() const;
	QET::Orientation setNext();
	QET::Orientation setPrevious();
	bool accept(QET::Orientation) const;
	const OrientationSet operator++(int);
	const OrientationSet operator--(int);
	const OrientationSet operator++();
	const OrientationSet operator--();
	bool operator==(const OrientationSet &) const;
	bool operator!=(const OrientationSet &) const;
	bool fromString(const QString &);
	QString toString() const;
};

inline bool OrientationSet::north() const {
	return(north_ori);
}

inline bool OrientationSet::east() const {
	return(east_ori);
}

inline bool OrientationSet::south() const {
	return(south_ori);
}

inline bool OrientationSet::west() const {
	return(west_ori);
}

inline void OrientationSet::setDefaultOrientation(const QET::Orientation& theValue) {
	default_ori = theValue;
	
}

inline QET::Orientation OrientationSet::defaultOrientation() const {
	return(default_ori);
}

inline QET::Orientation OrientationSet::current() const {
	return(current_ori);
}

#endif

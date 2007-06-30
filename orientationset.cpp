#include "orientationset.h"

OrientationSet::OrientationSet() :
	north_ori(true),
	east_ori(true),
	south_ori(true),
	west_ori(true),
	default_ori(QET::North),
	current_ori(QET::North)
{}

bool OrientationSet::setNorth (bool ori) {
	// pour desactiver une orientation, il doit y avoir au moins une autre orientation possible
	bool can_set_ori = ori ? true : east_ori || south_ori || west_ori;
	if (can_set_ori) {
		north_ori = ori;
		// en cas de desactivation d'une orientation, il faut verifier voire corriger les orientations courante et par defaut 
		if (!ori) {
			if (default_ori == QET::North) default_ori = next();
			if (current_ori == QET::North) current_ori = next();
		}
	}
	return(can_set_ori);
}

bool OrientationSet::setEast (bool ori) {
	// pour desactiver une orientation, il doit y avoir au moins une autre orientation possible
	bool can_set_ori = ori ? true : south_ori || west_ori || north_ori;
	if (can_set_ori) {
		east_ori = ori;
		// en cas de desactivation d'une orientation, il faut verifier voire corriger les orientations courante et par defaut 
		if (!ori) {
			if (default_ori == QET::East) default_ori = next();
			if (current_ori == QET::East) current_ori = next();
		}
	}
	return(can_set_ori);
}

bool OrientationSet::setSouth (bool ori) {
	// pour desactiver une orientation, il doit y avoir au moins une autre orientation possible
	bool can_set_ori = ori ? true : west_ori || north_ori || east_ori;
		if (can_set_ori) {
		south_ori = ori;
		// en cas de desactivation d'une orientation, il faut verifier voire corriger les orientations courante et par defaut 
		if (!ori) {
			if (default_ori == QET::South) default_ori = next();
			if (current_ori == QET::South) current_ori = next();
		}
	}
	return(can_set_ori);
}

bool OrientationSet::setWest (bool ori) {
	// pour desactiver une orientation, il doit y avoir au moins une autre orientation possible
	bool can_set_ori = ori ? true : north_ori || east_ori || south_ori;
	if (can_set_ori) {
		west_ori = ori;
		// en cas de desactivation d'une orientation, il faut verifier voire corriger les orientations courante et par defaut 
		if (!ori) {
			if (default_ori == QET::West) default_ori = next();
			if (current_ori == QET::West) current_ori = next();
		}
	}
	return(can_set_ori);
}

bool OrientationSet::setCurrent(QET::Orientation ori) {
	bool can_set_ori = accept(ori);
	if (can_set_ori) current_ori = ori;
	return(can_set_ori);
}

QET::Orientation OrientationSet::next() const {
	QET::Orientation result = current_ori;
	do result = QET::nextOrientation(result); while (!accept(result));
	return(result);
}

QET::Orientation OrientationSet::previous() const {
	QET::Orientation result = current_ori;
	do result = QET::previousOrientation(result); while (!accept(result));
	return(result);
}

const OrientationSet OrientationSet::operator++(int) {
	OrientationSet before(*this);
	setNext();
	return(before);
}

const OrientationSet OrientationSet::operator--(int) {
	OrientationSet before(*this);
	setPrevious();
	return(before);
}

/**
	Permet de savoir si une orientation donnee peut etre utilisee.
	@param o L'orientation en question
	@return true si l'orientation est utilisable, false sinon
*/
bool OrientationSet::accept(QET::Orientation ori) const {
	bool accepted_ori = false;
	switch(ori) {
		case QET::North: accepted_ori = north_ori; break;
		case QET::East : accepted_ori = east_ori;  break;
		case QET::South: accepted_ori = south_ori; break;
		case QET::West : accepted_ori = west_ori;  break;
	}
	return(accepted_ori);
}

QET::Orientation OrientationSet::setNext() {
	setCurrent(next());
	return(current_ori);
}

QET::Orientation OrientationSet::setPrevious() {
	setCurrent(previous());
	return(current_ori);
}

const OrientationSet OrientationSet::operator++() {
	setNext();
	return(*this);
}

const OrientationSet OrientationSet::operator--() {
	setPrevious();
	return(*this);
}

bool OrientationSet::fromString(const QString &str) {
	QRegExp osv("^([dyn])([dyn])([dyn])([dyn])$");	// osv : Orientation String Validator
	if (osv.indexIn(str) == -1) return(false);
	QStringList matches = osv.capturedTexts();
	
	// il doit y avoir exactement UN d dans les 4 lettres capturees
	if (matches.count("d") != 1) return(false);
	
	bool *ori_pointers[4] = { &north_ori, &east_ori, &south_ori, &west_ori };
	QET::Orientation ori_ints[4] = { QET::North, QET::East, QET::South, QET::West };
	for(int i = 0 ; i < 4 ; ++ i) {
		QString current = matches.at(i + 1);
		if (current == "d") {
			current_ori = default_ori = ori_ints[i];
			current = "y";
		}
		*(ori_pointers[i]) = (current == "y");
	}
	return(true);
}

QString OrientationSet::toString() const {
	bool ori_pointers[4] = { north_ori, east_ori, south_ori, west_ori };
	QET::Orientation ori_ints[4] = { QET::North, QET::East, QET::South, QET::West };
	QString result("");
	for(int i = 0 ; i < 4 ; ++ i) {
		if (default_ori == ori_ints[i]) result += "d";
		else result += (ori_pointers[i] ? "y" : "n");
	}
	return(result);
}

#ifndef _QET_H
#define _QET_H
#include <QtXml>
/**
	Ce fichier contient des fonctions utiles pouvant etre appelees depuis
	n'importe ou. Il contient egalement des enums utilises dans plusieurs
	classes de l'application
*/
namespace QET {
	const QString version = "0.1";
	enum Orientation {North, East, South, West};
	enum ConductorSegmentType { Horizontal = 1, Vertical = 2, Both = 3 };
	QET::Orientation nextOrientation(QET::Orientation);
	QET::Orientation previousOrientation(QET::Orientation);
	QET::Orientation orientationFromString(const QString &);
	QString orientationToString(QET::Orientation);
	bool surLeMemeAxe(QET::Orientation, QET::Orientation);
	bool estHorizontale(QET::Orientation);
	bool estVerticale(QET::Orientation);
	bool attributeIsAnInteger(const QDomElement &, QString , int * = NULL);
	bool attributeIsAReal(const QDomElement &, QString , double * = NULL);
	QString ElementsAndConductorsSentence(int, int);
}
#endif

/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef _QET_H
#define _QET_H
#include <QtXml>
/**
	This file provides useful functions and enums that may be used from
	anywhere else within the QElectroTech application.
*/
namespace QET {
	/// QElectroTech version (as string, used to mark projects and elements XML documents)
	const QString version = "0.4";
	/// QElectroTech displayed version
	const QString displayedVersion = "0.4-dev-$Revision$";
	QString license();
	/// Orientation (used for electrical elements and their terminals)
	enum Orientation {North, East, South, West};
	
	/// Oriented movements
	enum OrientedMovement {
		ToNorth,
		ToNorthEast,
		ToEast,
		ToSouthEast,
		ToSouth,
		ToSouthWest,
		ToWest,
		ToNorthWest
	};
	
	/// List areas related to some common operations
	enum OperationAreas {
		ChangeInnerPoints = -4,
		RotateArea = -3,
		MoveArea = -2,
		NoOperation = -1,
		ResizeFromTopLeftCorner = 0,
		ResizeFromTopCenterCorner = 1,
		ResizeFromTopRightCorner = 2,
		ResizeFromMiddleLeftCorner = 3,
		ResizeFromMiddleRightCorner = 4,
		ResizeFromBottomLeftCorner = 5,
		ResizeFromBottomCenterCorner = 6,
		ResizeFromBottomRightCorner = 7
	};
	
	/// Supported types of interactive scaling, typically for a single element primitive
	enum ScalingMethod {
		FreeScaling,              ///< do not interfer with the default scaling process
		SnapScalingPointToGrid,   ///< snap the point used to define the new bounding rectangle to the grid
		RoundScaleRatios          ///< adjust the scaling movement so that the induced scaling ratios are rounded
	};
	
	/// Known kinds of conductor segments
	enum ConductorSegmentType {
		Horizontal = 1, ///< Horizontal segment 
		Vertical = 2,   ///< Vertical segment 
		Both = 3        ///< Invalid segment
	};
	
	/**
		This enum lists the various available endings for line primitives when drawing
		an electrical element.
	*/
	enum EndType {
		None,      ///< Regular line
		Simple,    ///< Base-less triangle
		Triangle,  ///< Triangle
		Circle,    ///< Circle
		Diamond    ///< Diamond
	};
	
	/**
		This enums lists the various kind of items users can manage within the
		application.
	*/
	enum ItemType {
		Element                           =    1,
		ElementsCategory                  =    2,
		ElementsCollection                =    4,
		ElementsContainer                 =    6,
		ElementsCollectionItem            =    7,
		TitleBlockTemplate                =    8,
		TitleBlockTemplatesCollection     =   16,
		TitleBlockTemplatesCollectionItem =   24,
		Diagram                           =   32,
		Project                           =   64,
		All                               =  127
	};
	
	/**
		This enum represents the various steps when applying a filter.
	*/
	enum Filtering {
		BeginFilter,
		RegularFilter,
		EndFilter
	};
	
	/**
		This enums lists the various ways to handle a standard problem when copying
		or moving element items (collections, categories, elements).
		@see MoveElementsHandler
	*/
	enum Action {
		Retry,   ///< The operation must be tried again
		Ignore,  ///< Skip the current item
		Erase,   ///< Erase the target content
		Abort,   ///< abort the whole operation, ignoring the curent item
		Managed, ///< the current item was handled by the Strategy object: do not treat it and continue
		Rename   ///< the target has to be renamed
	};
	
	/**
		This enum represents diagram areas that may be exported/printed.
	*/
	enum DiagramArea {
		BorderArea,      ///< Export the diagram along with its border and title block
		ElementsArea     ///< Export the content of the diagram only
	};
	
	/// enum used to specify the type of a length
	enum TitleBlockColumnLength {
		Absolute,                   ///< the length is absolute and should be applied as is
		RelativeToTotalLength,      ///< the length is just a fraction of the total available length
		RelativeToRemainingLength   ///< the length is just a fraction of the length that is still available when other types of lengths have been removed
	};
	
	QET::Orientation nextOrientation(QET::Orientation);
	QET::Orientation previousOrientation(QET::Orientation);
	QET::Orientation orientationFromString(const QString &);
	QString orientationToString(QET::Orientation);
	bool surLeMemeAxe(QET::Orientation, QET::Orientation);
	bool estHorizontale(QET::Orientation);
	bool estVerticale(QET::Orientation);
	bool lineContainsPoint(const QLineF &, const QPointF &);
	bool orthogonalProjection(const QPointF &, const QLineF &, QPointF * = 0);
	bool attributeIsAnInteger(const QDomElement &, QString , int * = NULL);
	bool attributeIsAReal(const QDomElement &, QString , qreal * = NULL);
	QString ElementsAndConductorsSentence(int, int, int = 0, int = 0);
	QList<QDomElement> findInDomElement(const QDomElement &, const QString &);
	QList<QDomElement> findInDomElement(const QDomElement &, const QString &, const QString &);
	QList<QChar> forbiddenCharacters();
	QString forbiddenCharactersString(bool = false);
	bool containsForbiddenCharacters(const QString &);
	QString stringToFileName(const QString &);
	QString escapeSpaces(const QString &);
	QString unescapeSpaces(const QString &);
	QString joinWithSpaces(const QStringList &);
	QStringList splitWithSpaces(const QString &);
	QString endTypeToString(const QET::EndType &);
	QET::EndType endTypeFromString(const QString &);
	QString diagramAreaToString(const QET::DiagramArea &);
	QET::DiagramArea diagramAreaFromString(const QString &);
	QString pointerString(void *);
	qreal round(qreal, qreal);
	QPointF roundPoint(const QPointF &, qreal);
	qreal correctAngle(const qreal &);
	bool compareCanonicalFilePaths(const QString &, const QString &);
	QString titleBlockColumnLengthToString(const TitleBlockColumnLength  &);
	bool writeXmlFile(QDomDocument &, const QString &, QString * = 0);
	QPointF graphicsSceneEventPos(QEvent *);
}
#endif

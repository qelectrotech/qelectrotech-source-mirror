/*
 * binarygrid.cpp
 *
 *  Created on: 8 nov. 2014
 *      Author: rene
 */



#include <QtDebug>
#include "element.h"
#include "binarygrid.h"


/*
 *
 */
BinaryGrid::BinaryGrid( Diagram* d )	{
	diagram = d;
	reset();
}


/*
 *
 */
BinaryGrid::~BinaryGrid()	{
}





/*
 *
 */
QString reverse( QString s )	{
	QString ret = "";
	const int nb = s.count();
	for(int i=0; i<nb; i++)	{
		ret.append(s.at(nb-i-1));
	}
	return ret;
}


/*
 *
 */
void printVec2d( QString str, vec2d v )	{
	qDebug() << str <<" ("<< v.x <<","<< v.y <<")";
}


/*
 *
 */
void printDoubl( QString str, double d )	{
	qDebug() << str <<" "<< d;
}

/*
 *
 */
QString getStringLine( U64 * line )	{
	QString ret = "";
	for ( int i=0; i<1; i++ )	{
		U64 mask = 1;
		for( int j=0; j<64; j++ )	{
			if ( (line[i] & mask) == 0 )		ret += ".";
			else								ret += "1";
			mask <<= 1;
		}
	}
	return ret;
}





void rect2side( vec2d* topLeft, vec2d* bottomRight, QRectF qRectF )	{
	*topLeft		= vec2d(  qRectF.topLeft()     );
	*bottomRight	= vec2d(  qRectF.bottomRight() );
}


/*
 *
 */
void BinaryGrid::reset()	{
	for( int i=0; i<400; i++ )	{
		for( int j=0; j<5; j++ )	{
			bitHrzt [i][j] = 0ULL;
			bitVrtc [i][j] = 0ULL;
		}
	}
}


/*
 *
 */
void BinaryGrid::build( Conductor* pExceptConductor)	{
	if ( diagram ){
		QList<Element *> elements = diagram -> elements();
		for ( int i=0; i<elements.size(); i++  )	{
			Element* element = elements[i];
			if (  element -> terminals().size() == 0 )		continue;

			vec2d topLeft, bottomRight;
			rect2side( &topLeft,  &bottomRight, element -> boundingRectTrue() );

			add2grid( topLeft, bottomRight, 3 );

			QList<Conductor*> conductors = element->conductors();
			const int nb = conductors.size();

			for( int i=0; i<nb; i++ )	{
				if ( conductors[i] == pExceptConductor )		continue;
				add2grid( conductors[i] );
			}

		}
	}
}




/*
 *
 */
void BinaryGrid::reBuild(Conductor* pExceptConductor)	{
	reset();
	build(pExceptConductor);
}




/*
 * bit 0 for x = 0
 * bit 1 for x = 10
 * bit 2 for x = 20
 *  ....
 *
 * Compute mask until haven't point (one more)
 * for example  x = 100
 * mask = 0001 1111 1111
 */
void BinaryGrid::computeFirstMask( double d, U64* mask )	{
	int n = (int)(d) /640;
	d = fmod(d, 640);
	U64 uMask = (U64)(d - 1.0) / 10;
	mask[n] = 1ULL << ++uMask;
  	(U64)(mask[n]--);
	// Because at umask = 64 the result is mask[n]=256   why ???? 64bits => 8 bits ???
 	if ( uMask == 64 ) 	mask[n] = (U64)-1;

	for( int i=0; i<n; i++)		mask[i] = (U64)-1;
	for( int i=n+1; i<5; i++)	mask[i] = (U64)0;
}



/*
 *
 * Compute mask until haven't point
 * for example  x = 90
 * mask = 0001 1111 1111
 *
 * result :
 * for a horizontal line from 50 to 110
 * first  mask = 0000 0000 1111  (compute from 50)
 * second mask = 0111 1111 1111  (compute from 110)
 *
 * xor result  = 0111 1111 0000
 *
 */
void BinaryGrid::computeSecondMask( double d, U64* mask )	{
	int n = (int)d /640;
	d = fmod(d, 640);
	U64 uMask = (U64)(d) / 10;
	mask[n] = 1ULL << ++uMask;
	(U64)(mask[n]--);
	// Because at umask = 64 the result is mask[n]=256   why ???? 64bits => 8 bits ???
	if ( uMask == 64 ) 	mask[n] = (U64)-1;

	for( int i=0; i<n; i++)		mask[i] = (U64)-1;
	for( int i=n+1; i<5; i++)	mask[i] = (U64)0;
}




/*
 *
 */
int BinaryGrid::computeFirstLine( double d)	{
	return (int) (((d-1)/10)+1);
}

/*
 *
 */
int BinaryGrid::computeLastLine( double d)	{
	return (int) d/10;
}


/*
 *
 */
void BinaryGrid::debugGrid(int deb, int fin)	{
	for ( int i=deb; i<=fin; i++ )	{
//		char * binary0 = getStringLine( bitHrzt[i] ).toStdString();
//		qDebug( "line : %02d %s %s", i, getStringLine( bitHrzt[i] ).toStdString(), getStringLine( bitVrtc[i] ).toStdString() );

		qDebug() <<"line "<< (i+10) <<" "<< getStringLine( bitHrzt[i] ) <<"   "<< getStringLine( bitVrtc[i] );
	}
}


/*
 *
 */
void BinaryGrid::debugGrid()	{
	debugGrid(2,45);
}



/*
 *
 */
void BinaryGrid::add2grid( vec2d topLeft, vec2d bottomRight, int map )	{
	double left		=     vec2d(topLeft).mul(vec2d(1.0,0.0)).addCoord();
	double right	= vec2d(bottomRight).mul(vec2d(1.0,0.0)).addCoord();

	double top		=     vec2d(topLeft).mul(vec2d(0.0,1.0)).addCoord();
	double bottom	= vec2d(bottomRight).mul(vec2d(0.0,1.0)).addCoord();

	// mask 0 and mask 1
	U64 M0[5];
	computeFirstMask( left, M0 );
	U64 M1[5];
	computeSecondMask( right, M1 );

	// mask = mask0 xor mask1

	U64 mask[5];
	for( int i=0; i<5; i++ )	mask[i] = M0[i] ^ M1[i];

	int idx0 = computeFirstLine(top);
	int idx1 = computeLastLine(bottom);

	for ( int i= idx0; i<idx1+1; i++ )	{
		for ( int j=0; j<5; j++ )	{
			if ( map & HORI )			bitHrzt[i][j] |= mask[j];
			if ( map & VERT )			bitVrtc[i][j] |= mask[j];
		}
	}
}




/*
 *
 */
void BinaryGrid::orderVect( vec2d& v1, vec2d& v2 )	{
	if ( v1.x > v2.x || v1.y > v2.y )	{
		vec2d v = vec2d( v1 );
		v1 = v2;
		v2 = v;
	}
}



/*
 *
 */
void BinaryGrid::add2grid( Conductor* pConductor )	{
	QList <vec2d>	vec2ds = pConductor -> getVec2ds();

	const int nb = vec2ds.size() - 1;
	for( int i=0; i<nb; i++ )	{
		vec2d v1 = vec2d( vec2ds[i] + vec2d(pConductor->scenePos()) );
		vec2d v2 = vec2d( vec2ds[i+1] + vec2d(pConductor->scenePos()) );

		orderVect( v1, v2 );

		vec2d vDir = vec2d( v1, v2 );
		if ( vDir.dot( vec2d(0.0,1.0) ) == 0 )		add2grid( v1, v2, VERT);
		else										add2grid( v1, v2, HORI );
	}
}



/*
 *
 */
bool BinaryGrid::testMask( U64* mask, int lineBeg, int lineEnd, int map )	{
	for ( int i=lineBeg; i<=lineEnd; i++ )	{
		for( int j=0; j<5; j++ )	{
			U64 val = 0;
			if ( (map & HORI) == HORI )		val = bitHrzt[i][j];
			if ( (map & VERT) == VERT )		val = bitVrtc[i][j];

			if ( (val|mask[j]) != (val^mask[j]) )		return true;
		}
	}
	return false;
}




/*
 *
 */
bool BinaryGrid::test( vec2d p1, vec2d p2 )	{
	qDebug() <<"BinaryGrid::test";

	orderVect( p1, p2 );
//	printVec2d( "p1 : ", p1 );
//	printVec2d( "p2 : ", p2 );

	double left		= vec2d(p1).mul(vec2d(1.0,0.0)).addCoord();
	double right	= vec2d(p2).mul(vec2d(1.0,0.0)).addCoord();

	double top		= vec2d(p1).mul(vec2d(0.0,1.0)).addCoord();
	double bottom	= vec2d(p2).mul(vec2d(0.0,1.0)).addCoord();

	int map;
	vec2d vDir = vec2d( p1, p2 );
	if ( vDir.dot( vec2d(0.0,1.0) ) == 0 )		map = HORI;
	else										map = VERT;


	// compute mask 0 and mask 1
	U64 M0[5];	computeFirstMask( left, M0 );
	U64 M1[5];	computeSecondMask( right, M1 );

	// mask = mask0 xor mask1
	U64 mask[5];
	for( int i=0; i<5; i++ )	mask[i] = M0[i] ^ M1[i];

	int idx0 = computeFirstLine(top);
	int idx1 = computeLastLine(bottom);

//	qDebug() << "idx 0 : "<< idx0;
//	qDebug() << "idx 1 : "<< idx1;
//
//	debugGrid( idx0-2, idx1+2);

	return testMask( mask, idx0, idx1, map );
}




/*
 *
 */
bool BinaryGrid::computeMinSegment( vec2d p1, vec2d p2 )	{
	qDebug() <<"BinaryGrid::test";

	orderVect( p1, p2 );
//	printVec2d( "p1 : ", p1 );
//	printVec2d( "p2 : ", p2 );

	double left		= vec2d(p1).mul(vec2d(1.0,0.0)).addCoord();
	double right	= vec2d(p2).mul(vec2d(1.0,0.0)).addCoord();

	double top		= vec2d(p1).mul(vec2d(0.0,1.0)).addCoord();
	double bottom	= vec2d(p2).mul(vec2d(0.0,1.0)).addCoord();

	int map;
	vec2d vDir = vec2d( p1, p2 );
	if ( vDir.dot( vec2d(0.0,1.0) ) == 0 )		map = HORI;
	else										map = VERT;


	// compute mask 0 and mask 1
	U64 M0[5];	computeFirstMask( left, M0 );
	U64 M1[5];	computeSecondMask( right, M1 );

	// mask = mask0 xor mask1
	U64 mask[5];
	for( int i=0; i<5; i++ )	mask[i] = M0[i] ^ M1[i];

	int idx0 = computeFirstLine(top);
	int idx1 = computeLastLine(bottom);

//	qDebug() << "idx 0 : "<< idx0;
//	qDebug() << "idx 1 : "<< idx1;
//
//	debugGrid( idx0-2, idx1+2);

	return testMask( mask, idx0, idx1, map );
}





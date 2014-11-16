/*
 * binarygrid.h
 *
 *  Created on: 8 nov. 2014
 *      Author: rene
 */

#ifndef BINARYGRID_H_
#define BINARYGRID_H_

#include "diagram.h"
#include "conductor.h"
#include "Mathlib.h"

typedef unsigned long long		U64;

#define HORI		0x01
#define VERT		0x02

class BinaryGrid {

	public:
							 BinaryGrid(Diagram * diagram);
							~BinaryGrid();

		void				reset();
		void				build(Conductor*);
		void				reBuild(Conductor*);

		void			 	computeFirstMask( double, U64* );
		void				computeSecondMask( double, U64*);
		int					computeFirstLine( double );
		int					computeLastLine( double );

		void				debugGrid(int, int);
		void				debugGrid();
		void				add2grid( vec2d, vec2d, int );
		void				orderVect( vec2d&, vec2d& );
		void				add2grid( Conductor* );

		bool				testMask( U64*, int, int, int  );
		bool				test( vec2d, vec2d );
		bool				computeMinSegment( vec2d, vec2d );

	private:
		U64 				bitHrzt[400][5];
		U64					bitVrtc[400][5];

		Diagram*			diagram;
};

#endif /* BINARYGRID_H_ */

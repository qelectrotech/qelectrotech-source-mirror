/***************************************************************************
 * Mathlib
 *
 * Copyright (C) 2003-2004, Alexander Zaprjagaev <frustum@frustum.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 ***************************************************************************
 * Update 2004/08/19
 *
 * added ivec2, ivec3 & ivec4 methods
 * vec2d, vec3d & vec4d data : added texture coords (s,t,p,q) and color enums (r,g,b,a)
 * mat3d & mat4d : added multiple double constructor ad modified methods returning mat3d or mat4d
 * optimisations like "x / 2.0f" replaced by faster "x * 0.5f"
 * defines of multiples usefull maths values and radian/degree conversions
 * vec2d : added methods : set, reset, compare, dot, closestPointOnLine, closestPointOnSegment,
 *                        projectionOnLine, lerp, angle
 * vec3d : added methods : set, reset, compare, dot, cross, closestPointOnLine, closestPointOnSegment,
 *                        projectionOnLine, lerp, angle
 * vec4d : added methods : set, reset, compare
 ***************************************************************************
 * Update 2014/08/20
 *
 * Transform float to double
 * for using with QT program
 * author : Rene Negre <runsys@qelectotech.org>
 ***************************************************************************
 */

#ifndef __HMATHLIB_D__
#define __HMATHLIB_D__

#include <math.h>
#include <stdlib.h>
#include <QPointF>

#define EPSILON				0.00000001
//#define M_PI				3.141592653589793238462643383279f		// PI
#define M_PIDIV2			1.570796326794896619231321691639f		// PI / 2
#define M_2PI				6.283185307179586476925286766559f		// 2 * PI
#define M_PI2				9.869604401089358618834490999876f		// PI au carre
#define M_PIDIV180			0.01745329251994329576923690768488f		// PI / 180
#define M_180DIVPI			57.295779513082320876798154814105f		// 180 / PI

#define DegToRadd(a)	(a)*=M_PIDIV180
#define RadToDegd(a)	(a)*=M_180DIVPI
#define DEG2RADd(a)	((a)*M_PIDIV180)
#define RAD2DEGd(a)	((a)*M_180DIVPI)
#define RADIANSd(a)	((a)*M_PIDIV180)
#define DEGRESd(a)	((a)*M_180DIVPI)
#define DegToRad		DEG2RADd


const double INV_RAND_MAX = 1.0 / (RAND_MAX);
const double INV_RAND_MAXd = 1.0 / (RAND_MAX );
inline double randomd(double max=1.0) { return max * rand() * INV_RAND_MAX; }
inline double randomd(double min, double max) { return min + (max - min) * INV_RAND_MAX * rand(); }
inline int randomd(int max=RAND_MAX) { return rand()%(max+1); }

class vec2d;
class ivec2;


/*****************************************************************************/
/*                                                                           */
/* vec2d                                                                      */
/*                                                                           */
/*****************************************************************************/

class vec2d {
public:
	static int count;
	vec2d(void) : x(0), y(0)						{ count++; }
	vec2d(double _x,double _y) : x(_x), y(_y)		{ count++; }
	vec2d(const double *_v) : x(_v[0]), y(_v[1])	{ count++; }
	vec2d(const vec2d &_p1, const vec2d & _p2) : x(_p2.x-_p1.x), y(_p2.y-_p1.y) { count++; }
	vec2d(const QPointF &_qp) : x(_qp.x()), y(_qp.y()) { count++; }
	
	~vec2d()										{ count--; }

	bool operator==(const vec2d &_v) { return (fabs(this->x - _v.x) < EPSILON && fabs(this->y - _v.y) < EPSILON); }
	int operator!=(const vec2d &_v) { return !(*this == _v); }

	vec2d &operator=(double _f) { this->x=_f; this->y=_f; return (*this); }
	const vec2d operator*(double _f) const { return vec2d(this->x * _f,this->y * _f); }
	const vec2d operator/(double _f) const {
		if(fabs(_f) < EPSILON) return *this;
		_f = 1.0f / _f;
		return (*this) * _f;
	}
	const vec2d operator+(const vec2d &_v) const { return vec2d(this->x + _v.x,this->y + _v.y); }
	const vec2d operator-() const { return vec2d(-this->x,-this->y); }
	const vec2d operator-(const vec2d &_v) const { return vec2d(this->x - _v.x,this->y - _v.y); }

	vec2d &operator*=(double _f) { return *this = *this * _f; }
	vec2d &operator/=(double _f) { return *this = *this / _f; }
	vec2d &operator+=(const vec2d &_v) { return *this = *this + _v; }
	vec2d &operator-=(const vec2d &_v) { return *this = *this - _v; }

	double operator*(const vec2d &_v) const { return this->x * _v.x + this->y * _v.y; }

	operator double*() { return this->v; }
	operator const double*() const { return this->v; }
//	double &operator[](int _i) { return this->v[_i]; }
//	const double &operator[](int _i) const { return this->v[_i]; }

	void set(double _x,double _y) { this->x = _x; this->y = _y; }
	void reset(void) { this->x = this->y = 0; }
	double length(void) const { return sqrtf(this->x * this->x + this->y * this->y); }
	double normalize(void) {
		double inv,l = this->length();
		if(l < EPSILON) return 0.0f;
		inv = 1.0f / l;
		this->x *= inv;
		this->y *= inv;
		return l;
	}
	double dot(const vec2d &v) { return ((this->x*v.x) + (this->y*v.y)); } // Produit scalaire
	bool compare(const vec2d &_v,double epsi=EPSILON) { return (fabs(this->x - _v.x) < epsi && fabs(this->y - _v.y) < epsi); }
	// retourne les coordonnée du point le plus proche de *this sur la droite passant par vA et vB
	vec2d closestPointOnLine(const vec2d &vA, const vec2d &vB) { return (((vB-vA) * this->projectionOnLine(vA, vB)) + vA); }
	// retourne les coordonnée du point le plus proche de *this sur le segment vA,vB
	vec2d closestPointOnSegment(const vec2d &vA, const vec2d &vB) {
		double factor = this->projectionOnLine(vA, vB);
		if (factor <= 0.0f) return vA;
		if (factor >= 1.0f) return vB;
		return (((vB-vA) * factor) + vA);
	}
	// retourne le facteur de la projection de *this sur la droite passant par vA et vB
	double projectionOnLine(const vec2d &vA, const vec2d &vB) {
		vec2d v(vB - vA);
		return v.dot(*this - vA) / v.dot(v);
	}
	// Fonction d'interpolation linéaire entre 2 vecteurs
	vec2d lerp(vec2d &u, vec2d &v, double factor) { return ((u * (1 - factor)) + (v * factor)); }
	vec2d lerp(vec2d &u, vec2d &v, vec2d& factor) { return (vec2d((u.x * (1 - factor.x)) + (v.x * factor.x), (u.y * (1 - factor.y)) + (v.y * factor.y))); }
	double angle(void) { return (double)atan2(this->y,this->x); }
	double angle(const vec2d &v) { return (double)atan2(v.y-this->y,v.x-this->x); }

	vec2d		mul(const vec2d &v)					{ return vec2d(this->x*=v.x, this->y*=v.y); } // Produit des ccordonnes
	vec2d&		mulCoord(const vec2d &v)			{ this->x*=v.x, this->y*=v.y; return *this; }			// Produit des ccordonnes
	double		addCoord()							{ return this->x + this->y; }			// Produit des ccordonnes
	vec2d		vabs(void)							{ return vec2d( fabs(this->x), fabs(this->y) );	}
	void		abs(void) 							{ this->x = fabs(this->x); this->y = fabs(this->y);	}
	vec2d		exch(void)							{ return vec2d( this->y, this->x ); }
	QPointF		toQPointF()							{ return  QPointF(this->x, this->y); }

	vec2d vnormalize(void) {
		double inv,l = this->length();
		if(l < EPSILON) return vec2d(0.0,0.0);
		inv = 1.0f / l;
		return vec2d(this->x * inv, this->y * inv);
	}


	union {
		struct {double x,y;};
		struct {double s,t;};
		double v[2];
	};
};

inline vec2d operator*(double fl, const vec2d& v)	{ return vec2d(v.x*fl, v.y*fl);}

inline double Dot(const vec2d& a, const vec2d& b) { return(a.x*b.x+a.y*b.y); }


/*****************************************************************************/
/*                                                                           */
/* ivec2                                                                     */
/*                                                                           */
/*****************************************************************************/

class ivec2 {
public:
	ivec2(void) : a(0), b(0) { }
	ivec2(long _a,long _b) : a(_a), b(_b) { }
	ivec2(const long *iv) : a(iv[0]), b(iv[1]) { }
	ivec2(const ivec2 &iv) : a(iv.a), b(iv.b) { }
	ivec2(const vec2d &v) : x((int)v.x), y((int)v.y) { }

	int operator==(const ivec2 &iv) { return ((this->a == iv.a) && (this->b == iv.b)); }
	int operator!=(const ivec2 &iv) { return !(*this == iv); }

	ivec2 &operator=(long _i) { this->x=_i; this->y=_i; return (*this); }
	const ivec2 operator*(long _i) const { return ivec2(this->a * _i,this->b * _i); }
	const ivec2 operator/(long _i) const { return ivec2(this->a / _i,this->b / _i); }
	const ivec2 operator+(const ivec2 &iv) const { return ivec2(this->a + iv.a,this->b + iv.b); }
	const ivec2 operator-() const { return ivec2(-this->a,-this->b); }
	const ivec2 operator-(const ivec2 &iv) const { return ivec2(this->a - iv.a,this->b - iv.b); }

	ivec2 &operator*=(long _i) { return *this = *this * _i; }
	ivec2 &operator/=(long _i) { return *this = *this / _i; }
	ivec2 &operator+=(const ivec2 &iv) { return *this = *this + iv; }
	ivec2 &operator-=(const ivec2 &iv) { return *this = *this - iv; }

	long operator*(const ivec2 &iv) const { return this->a * iv.a + this->b * iv.b; }

	operator long*() { return this->i; }
	operator const long*() const { return this->i; }
//	long &operator[](int _i) { return this->i[_i]; }
//	const long &operator[](int _i) const { return this->i[_i]; }

	void set(long _a,long _b) { this->a = _a; this->b = _b; }
	void reset(void) { this->a = this->b = 0; }
	void swap(ivec2 &iv) { long tmp=a; a=iv.a; iv.a=tmp; tmp=b; b=iv.b; iv.b=tmp; }
	void swap(ivec2 *iv) { this->swap(*iv); }

	union {
		struct {long a,b;};
		struct {long x,y;};
		long i[2];
	};
};



#endif // __HMATHLIB_D__


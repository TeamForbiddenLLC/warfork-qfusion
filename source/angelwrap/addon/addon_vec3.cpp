/*
Copyright (C) 2008 German Garcia
Copyright (C) 2011 Chasseur de bots

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "../qas_precompiled.h"
#include "addon_vec3.h"

// CLASS: Vec3
void objectVec3_DefaultConstructor( asvec3_t *self )
{
	self->v[0] = self->v[1] = self->v[2] = 0;
}

void objectVec3_Constructor3F( float x, float y, float z, asvec3_t *self )
{
	self->v[0] = x;
	self->v[1] = y;
	self->v[2] = z;
}

void objectVec3_Constructor1F( float v, asvec3_t *self )
{
	self->v[0] = self->v[1] = self->v[2] = v;
}

void objectVec3_CopyConstructor( asvec3_t *other, asvec3_t *self )
{
	self->v[0] = other->v[0];
	self->v[1] = other->v[1];
	self->v[2] = other->v[2];
}

static asvec3_t *objectVec3_AssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorCopy( other->v, self->v );
	return self;
}

static asvec3_t *objectVec3_AssignBehaviourD( float other, asvec3_t *self )
{
	VectorSet( self->v, other, other, other );
	return self;
}

static asvec3_t *objectVec3_AssignBehaviourI( int other, asvec3_t *self )
{
	VectorSet( self->v, other, other, other );
	return self;
}

static asvec3_t *objectVec3_AddAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorAdd( self->v, other->v, self->v );
	return self;
}

static asvec3_t *objectVec3_SubAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorSubtract( self->v, other->v, self->v );
	return self;
}

static asvec3_t *objectVec3_MulAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	vec_t product = DotProduct( self->v, other->v );

	VectorScale( self->v, product, self->v );
	return self;
}

static asvec3_t *objectVec3_XORAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	vec3_t product;

	CrossProduct( self->v, other->v, product );
	VectorCopy( product, self->v );
	return self;
}

static asvec3_t *objectVec3_MulAssignBehaviourI( int other, asvec3_t *self )
{
	VectorScale( self->v, other, self->v );
	return self;
}

static asvec3_t *objectVec3_MulAssignBehaviourD( float other, asvec3_t *self )
{
	VectorScale( self->v, other, self->v );
	return self;
}

static asvec3_t objectVec3_AddBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t vec;

	VectorAdd( first->v, second->v, vec.v );
	return vec;
}

static asvec3_t objectVec3_SubtractBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t vec;

	VectorSubtract( first->v, second->v, vec.v );
	return vec;
}

static float objectVec3_MultiplyBehaviour( asvec3_t *first, asvec3_t *second )
{
	return DotProduct( first->v, second->v );
}

static asvec3_t objectVec3_MultiplyBehaviourVD( asvec3_t *first, float second )
{
	asvec3_t vec;

	VectorScale( first->v, second, vec.v );
	return vec;
}

static asvec3_t objectVec3_MultiplyBehaviourDV( float first, asvec3_t *second )
{
	return objectVec3_MultiplyBehaviourVD( second, first );
}

static asvec3_t objectVec3_MultiplyBehaviourVI( asvec3_t *first, int second )
{
	asvec3_t vec;

	VectorScale( first->v, second, vec.v );
	return vec;
}

static asvec3_t objectVec3_MultiplyBehaviourIV( int first, asvec3_t *second )
{
	return objectVec3_MultiplyBehaviourVI( second, first );
}

static asvec3_t objectVec3_XORBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t vec;

	CrossProduct( first->v, second->v, vec.v );
	return vec;
}

static bool objectVec3_EqualBehaviour( asvec3_t *first, asvec3_t *second )
{
	return VectorCompare( first->v, second->v );
}

static void objectVec3_Set( float x, float y, float z, asvec3_t *vec )
{
	VectorSet( vec->v, x, y, z );
}

static float objectVec3_Length( const asvec3_t *vec )
{
	return VectorLength( vec->v );
}

static float objectVec3_Normalize( asvec3_t *vec )
{
	return VectorNormalize( vec->v );
}

static float objectVec3_Distance( asvec3_t *other, asvec3_t *self )
{
	return Distance( self->v, other->v );
}

static void objectVec3_AngleVectors( asvec3_t *f, asvec3_t *r, asvec3_t *u, asvec3_t *self )
{
	AngleVectors( self->v, f->v, r->v, u->v );
}

static asvec3_t objectVec3_VecToAngles( asvec3_t *self )
{
	asvec3_t angles;

	VecToAngles( self->v, angles.v );
	return angles;
}

static asvec3_t objectVec3_Perpendicular( asvec3_t *self )
{
	asvec3_t dst;

	PerpendicularVector( dst.v, self->v );
	return dst;
}

static void objectVec3_MakeNormalVectors( asvec3_t *r, asvec3_t *u, asvec3_t *self )
{
	MakeNormalVectors( self->v, r->v, u->v );
}

void PreRegisterVec3Addon( asIScriptEngine *engine )
{
	int r;

	// register the vector type
	r = engine->RegisterObjectType( "Vec3", sizeof( asvec3_t ), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_C | asOBJ_APP_CLASS_ALLFLOATS ); assert( r >= 0 );

	(void)sizeof(r); // hush the compiler
}

void RegisterVec3Addon( asIScriptEngine *engine )
{
	int r;

	// register object behaviours
	r = engine->RegisterObjectBehaviour( "Vec3", asBEHAVE_CONSTRUCT, "void f()", AS_OL(objectVec3_DefaultConstructor ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Vec3", asBEHAVE_CONSTRUCT, "void f(float x, float y, float z)", AS_OL(objectVec3_Constructor3F ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Vec3", asBEHAVE_CONSTRUCT, "void f(float v)", AS_OL(objectVec3_Constructor1F ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Vec3", asBEHAVE_CONSTRUCT, "void f(const Vec3 &in)", AS_OL(objectVec3_CopyConstructor ), AS_CC_OL ); assert( r >= 0 );

	// register object methods

	// assignments
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opAssign(Vec3 &in)", AS_OL(objectVec3_AssignBehaviour ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opAssign(int)", AS_OL(objectVec3_AssignBehaviourI ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opAssign(float)", AS_OL(objectVec3_AssignBehaviourD ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opAddAssign(Vec3 &in)", AS_OL(objectVec3_AddAssignBehaviour ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opSubAssign(Vec3 &in)", AS_OL(objectVec3_SubAssignBehaviour ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opMulAssign(Vec3 &in)", AS_OL(objectVec3_MulAssignBehaviour ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opXorAssign(Vec3 &in)", AS_OL(objectVec3_XORAssignBehaviour ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opMulAssign(int)", AS_OL(objectVec3_MulAssignBehaviourI ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 &opMulAssign(float)", AS_OL(objectVec3_MulAssignBehaviourD ), AS_CC_OL ); assert( r >= 0 );

	r = engine->RegisterObjectMethod( "Vec3", "Vec3 opAdd(Vec3 &in) const", AS_OF(objectVec3_AddBehaviour ), AS_CC_OF ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 opSub(Vec3 &in) const", AS_OF(objectVec3_SubtractBehaviour ), AS_CC_OF ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "float opMul(Vec3 &in) const", AS_OF(objectVec3_MultiplyBehaviour ), AS_CC_OF ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 opMul(float) const", AS_OF(objectVec3_MultiplyBehaviourVD ), AS_CC_OF ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 opMul_r(float) const", AS_OL(objectVec3_MultiplyBehaviourDV ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 opMul(int) const", AS_OF(objectVec3_MultiplyBehaviourVI ), AS_CC_OF ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 opMul_r(int) const", AS_OL(objectVec3_MultiplyBehaviourIV ), AS_CC_OL ); assert( r >= 0 );

	r = engine->RegisterObjectMethod( "Vec3", "Vec3 opXor(const Vec3 &in) const", AS_OF(objectVec3_XORBehaviour ), AS_CC_OF ); assert( r >= 0 );

	// == !=
	r = engine->RegisterObjectMethod( "Vec3", "bool opEquals(const Vec3 &in) const", AS_OF(objectVec3_EqualBehaviour ), AS_CC_OF ); assert( r >= 0 );

	r = engine->RegisterObjectMethod( "Vec3", "void set(float x, float y, float z)", AS_OL(objectVec3_Set ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "float length() const", AS_OL(objectVec3_Length ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "float normalize() const", AS_OL(objectVec3_Normalize ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "float distance(const Vec3 &in) const", AS_OL(objectVec3_Distance ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "void angleVectors(Vec3 &out, Vec3 &out, Vec3 &out) const", AS_OL(objectVec3_AngleVectors ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 toAngles() const", AS_OL(objectVec3_VecToAngles ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "Vec3 perpendicular() const", AS_OL(objectVec3_Perpendicular ), AS_CC_OL ); assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vec3", "void makeNormalVectors(Vec3 &out, Vec3 &out) const", AS_OL(objectVec3_MakeNormalVectors ), AS_CC_OL ); assert( r >= 0 );

	// properties
	r = engine->RegisterObjectProperty( "Vec3", "float x", asOFFSET( asvec3_t, v[0] ) ); assert( r >= 0 );
	r = engine->RegisterObjectProperty( "Vec3", "float y", asOFFSET( asvec3_t, v[1] ) ); assert( r >= 0 );
	r = engine->RegisterObjectProperty( "Vec3", "float z", asOFFSET( asvec3_t, v[2] ) ); assert( r >= 0 );

	(void)sizeof(r); // hush the compiler
}

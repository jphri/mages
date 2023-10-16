#ifndef VECMATH_H
#define VECMATH_H

#include <string.h>
#include <math.h>

#define MATH_FUNC static inline

#define DEFINE_VECTOR(SIZE) \
typedef float vec##SIZE[SIZE];\
MATH_FUNC void vec##SIZE##_dup(vec##SIZE r, vec##SIZE const a)\
{\
	for(int i = 0; i < SIZE; i++)\
		r[i] = a[i];\
}\
\
MATH_FUNC void vec##SIZE##_add(vec##SIZE r, vec##SIZE const a, vec##SIZE const b)\
{\
	for(int i = 0; i < SIZE; i++) \
		r[i] = a[i] + b[i];\
}\
\
MATH_FUNC void vec##SIZE##_mul(vec##SIZE r, vec##SIZE const a, vec##SIZE const b)\
{\
	for(int i = 0; i < SIZE; i++) \
		r[i] = a[i] * b[i];\
}\
\
MATH_FUNC void vec##SIZE##_sub(vec##SIZE r, vec##SIZE const a, vec##SIZE const b)\
{\
	for(int i = 0; i < SIZE; i++) \
		r[i] = a[i] - b[i];\
}\
\
MATH_FUNC void vec##SIZE##_div(vec##SIZE r, vec##SIZE const a, vec##SIZE const b)\
{\
	for(int i = 0; i < SIZE; i++) \
		r[i] = a[i] / b[i];\
}\
\
MATH_FUNC void vec##SIZE##_add_scaled(vec##SIZE r, vec##SIZE const a, vec##SIZE const b, float scale)\
{\
	for(int i = 0; i < SIZE; i++) \
		r[i] = a[i] + b[i] * scale;\
}\
\
MATH_FUNC void vec##SIZE##_sub_scaled(vec##SIZE r, vec##SIZE const a, vec##SIZE const b, float scale)\
{\
	for(int i = 0; i < SIZE; i++) \
		r[i] = a[i] - b[i] * scale;\
}\
\
MATH_FUNC float vec##SIZE##_dot(vec##SIZE const a, vec##SIZE const b) \
{\
	float r = 0.0;\
	for(int i = 0; i < SIZE; i++) {\
		r += a[i] * b[i];\
	}\
	return r;\
}\
\
MATH_FUNC void vec##SIZE##_normalize(vec##SIZE r, vec##SIZE const a)\
{\
	float distr = 1.0/sqrtf(vec##SIZE##_dot(a, a));\
	for(int i = 0; i < SIZE; i++) {\
		r[i] = a[i] * distr;\
	}\
}\
\
MATH_FUNC void vec##SIZE##_reflect(vec##SIZE r, vec##SIZE const incid, vec##SIZE const normal)\
{\
	float dot = vec##SIZE##_dot(incid, normal);\
	for(int i = 0; i < SIZE; i++) {\
		r[i] = incid[i] - 2 * dot * normal[i];\
	}\
}

DEFINE_VECTOR(2)
DEFINE_VECTOR(3)
DEFINE_VECTOR(4)

MATH_FUNC void vec3_cross(vec3 r, vec3 const a, vec3 const b) 
{
	r[0] = a[1] * b[2] - a[2] * b[1];
	r[1] = a[2] * b[0] - a[0] * b[2];
	r[2] = a[0] * b[1] - a[1] * b[0];
}

#define DEFINE_MATRIX(SIZE)\
typedef float mat##SIZE[SIZE][SIZE];\
\
MATH_FUNC void mat##SIZE##_add(mat##SIZE r, mat##SIZE const a, mat##SIZE const b)\
{\
	for(int y = 0; y < SIZE; y++)\
	for(int x = 0; x < SIZE; x++)\
		r[y][x] = a[y][x] + b[y][x];\
}\
\
MATH_FUNC void mat##SIZE##_sub(mat##SIZE r, mat##SIZE const a, mat##SIZE const b)\
{\
	for(int y = 0; y < SIZE; y++)\
	for(int x = 0; x < SIZE; x++)\
		r[y][x] = a[y][x] - b[y][x];\
}\
\
MATH_FUNC void mat##SIZE##_mul(mat##SIZE r, mat##SIZE const a, mat##SIZE const b)\
{\
	mat##SIZE result = {0};\
	for(int y = 0; y < SIZE; y++)\
	for(int x = 0; x < SIZE; x++) {\
		for(int i = 0; i < SIZE; i++) {\
			result[y][x] = a[y][i] * b[i][x];\
		}\
	}\
	memcpy(r, result, sizeof(result));\
}\
\
MATH_FUNC void mat##SIZE##_mul_vec##SIZE(vec##SIZE r, mat##SIZE const a, vec##SIZE const b)\
{\
	for(int y = 0; y < SIZE; y++)\
		for(int i = 0; i < SIZE; i++) {\
			r[i] = a[y][i] * b[i];\
		}\
}\
\
MATH_FUNC void mat##SIZE##_ident(mat##SIZE r)\
{\
	for(int y = 0; y < SIZE; y++)\
	for(int x = 0; x < SIZE; x++) {\
		r[y][x] = x == y;\
	}\
}

DEFINE_MATRIX(2)
DEFINE_MATRIX(3)
DEFINE_MATRIX(4)

MATH_FUNC void affine2d_rotate(mat3 r, float angle)
{
	const float sinv = sinf(angle),
		        cosv = cosf(angle);

	r[0][0] = cosv;
	r[0][1] = -sinv;
	r[1][0] = sinv;
	r[1][1] = cosv;
}

MATH_FUNC void affine2d_translate(mat3 r, vec2 pos)
{
	r[0][2] += pos[0];
	r[1][2] += pos[1];
}

#undef MATH_FUNC

#endif

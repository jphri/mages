#ifndef VECMATH_H
#define VECMATH_H

#include <string.h>
#include <math.h>
#include <stdio.h>

#define MATH_FUNC static inline

#define DEFINE_VECTOR(SIZE) \
typedef float  vec##SIZE[SIZE];\
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
}\
MATH_FUNC void vec##SIZE##_print(vec##SIZE r)\
{\
	for(int i = 0; i < SIZE; i++) {\
		printf("%f ", r[i]);\
	}\
	printf("\n");\
}

#define VEC2_DUP(v) (vec2){ v[0], v[1] }
#define VEC3_DUP(v) (vec2){ v[0], v[1], v[2] }
#define VEC4_DUP(v) (vec2){ v[0], v[1], v[2], v[3] }

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
MATH_FUNC void mat##SIZE##_add(mat##SIZE r, mat##SIZE a, mat##SIZE b)\
{\
	for(int y = 0; y < SIZE; y++)\
	for(int x = 0; x < SIZE; x++)\
		r[y][x] = a[y][x] + b[y][x];\
}\
\
MATH_FUNC void mat##SIZE##_sub(mat##SIZE r, mat##SIZE a, mat##SIZE b)\
{\
	for(int y = 0; y < SIZE; y++)\
	for(int x = 0; x < SIZE; x++)\
		r[y][x] = a[y][x] - b[y][x];\
}\
\
MATH_FUNC void mat##SIZE##_mul(mat##SIZE r, mat##SIZE a, mat##SIZE b)\
{\
	mat##SIZE result = {0};\
	for(int y = 0; y < SIZE; y++)\
	for(int x = 0; x < SIZE; x++) {\
		for(int i = 0; i < SIZE; i++) {\
			result[x][y] += a[i][y] * b[x][i];\
		}\
	}\
	memcpy(r, result, sizeof(result));\
}\
\
MATH_FUNC void mat##SIZE##_ident(mat##SIZE r)\
{\
	for(int y = 0; y < SIZE; y++)\
	for(int x = 0; x < SIZE; x++) {\
		r[x][y] = x == y ? 1.0 : 0.0;\
	}\
}\
MATH_FUNC void mat##SIZE##_mul_vec##SIZE(vec##SIZE r, mat##SIZE m, vec##SIZE v)\
{\
	vec##SIZE result = {0};\
	for(int i = 0; i < SIZE; i++) {\
		for(int y = 0; y < SIZE; y++) {\
			result[i] += m[i][y] * v[i];\
		}\
	}\
	memcpy(r, result, sizeof(result));\
}\
MATH_FUNC void mat##SIZE##_print(mat##SIZE r)\
{\
	float *f = &r[0][0];\
	for(int i = 0; i < SIZE * SIZE; i++) {\
		if(i % SIZE == 0)\
			printf("\n");\
		printf("%f ", f[i]);\
	}\
	printf("\n-----------\n");\
}\

DEFINE_MATRIX(2)
DEFINE_MATRIX(3)
DEFINE_MATRIX(4)

MATH_FUNC void affine2d_rotate(mat4 r, float angle)
{
	const float sinv = sinf(angle),
		        cosv = cosf(angle);

	mat4 trans = {
		{ cosv, -sinv, 0, 0 },
		{ sinv, cosv, 0, 0 },
		{ 0,        0, 1, 0 },
		{ 0,        0, 0, 1 }
	};
	mat4_mul(r, trans, r);
}

MATH_FUNC void affine2d_translate(mat4 r, vec2 pos)
{
	mat4 trans = {
		{ 1, 0,           0, 0 },
		{ 0, 1,           0, 0 },
		{ 0, 0,           1, 0 },
		{ pos[0], pos[1], 0, 1 }
	};
	mat4_mul(r, trans, r);
}

MATH_FUNC void affine2d_scale(mat4 r, vec2 s)
{
	mat4 trans = {
		{ s[0], 0, 0, 0 },
		{ 0, s[1], 0, 0 },
		{ 0, 0,    1, 0 },
		{ 0, 0,    0, 1 }
	};
	mat4_mul(r, trans, r);

}

MATH_FUNC void affine2d_setup_ortho_window(mat4 r, const float w, const float h)
{
	r[0][0] =  2.0 / w;
	r[1][1] = -2.0 / h;
	r[2][2] = -1.0;
	r[3][0] = -1.0;
	r[3][1] =  1.0;
	r[3][2] =  0.0;
	r[3][3] =  1.0;
}

#undef MATH_FUNC

#endif

#ifndef RR_PSYQ_LIBGTE_GTEMAT5_H
#define RR_PSYQ_LIBGTE_GTEMAT5_H

#include "rr/matrix_words.h"
#include "rr/geometry.h"

/* The input is two packed words (x,y,z,padding), aligned for word loads.
 * PsyZ: void ApplyMatrix(MATRIX *m, SVECTOR *v0, VECTOR *v1) writes the three
 * MAC words. ApplyMatrixSV (not in PsyZ, src/main/psyq/libgte/gtemat.c)
 * writes three saturated IR halfwords and returns output. */
MatrixWords32 *RotMatrixZYX(const Vec3sFields8 *angles, MatrixWords32 *matrix);
void ApplyMatrix(const MatrixWords32 *matrix, const Vec3s8 *input, Vec3i *output);
s16 *ApplyMatrixSV(const MatrixWords32 *matrix, const Vec3s8 *input, s16 output[3]);

#endif

#ifndef MATRIX_UTILS_H_
#define MATRIX_UTILS_H_

#include <cmath>
#include "vr/gvr/capi/include/gvr_types.h"

class MatrixUtils {
  
  public:
  
    // Generate a rotation matrix
    static gvr::Mat4f RotateM(float angle, float u, float v, float w);
  
    // Multiply two matrices
    static gvr::Mat4f MultiplyMM(const gvr::Mat4f& matA, const gvr::Mat4f& matB);
    
    // Obtain perspective matrix
    static gvr::Mat4f Perspective(const gvr::Rectf& fov, float near, float far);
    
    // Perform matrix transpose
    static gvr::Mat4f Transpose(const gvr::Mat4f& matrix);
};

#endif  // MATRIX_UTILS_H_
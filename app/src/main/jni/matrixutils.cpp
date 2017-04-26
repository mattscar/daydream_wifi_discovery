#include "matrixutils.h"

gvr::Mat4f MatrixUtils::RotateM(float angle, float u, float v, float w) {
  
  gvr::Mat4f rotMatrix;
  
  // Determine length of rotation axis
  float length = sqrt(u*u + v*v + w*w);    
  
  // Compute normalized vector components
  float x = u/length;
  float y = v/length;
  float z = w/length;   
  
  // Compute constants based on angle
  float c = cos(angle);
  float t = 1 - c;
  float s = sin(angle);
  
  // Compute components of rotation matrix
  rotMatrix.m[0][0] = t*x*x + c;
  rotMatrix.m[0][1] = t*x*y - s*z;
  rotMatrix.m[0][2] = t*x*z + s*y;
  rotMatrix.m[0][3] = 0.0; 

  rotMatrix.m[1][0] = t*x*y + s*z;
  rotMatrix.m[1][1] = t*y*y + c;
  rotMatrix.m[1][2] = t*y*z - s*x;
  rotMatrix.m[1][3] = 0.0; 

  rotMatrix.m[2][0] = t*x*z - s*y;
  rotMatrix.m[2][1] = t*y*z + s*x;
  rotMatrix.m[2][2] = t*z*z + c;
  rotMatrix.m[2][3] = 0.0;   

  rotMatrix.m[3][0] = 0.0;
  rotMatrix.m[3][1] = 0.0;
  rotMatrix.m[3][2] = 0.0;
  rotMatrix.m[3][3] = 1.0;
  
  return rotMatrix;
} 

gvr::Mat4f MatrixUtils::MultiplyMM(const gvr::Mat4f& matA, const gvr::Mat4f& matB) {
  
  gvr::Mat4f product;
  
  // Compute matrix product
  for(int i = 0; i < 4; ++i) {
    for(int j = 0; j < 4; ++j) {
      product.m[i][j] = 0.0f;
      for(int k = 0; k < 4; ++k) {
        product.m[i][j] += matA.m[i][k] * matB.m[k][j];
      }
    }
  }
  return product;
}

gvr::Mat4f MatrixUtils::Perspective(const gvr::Rectf& fov,
    float near, float far) {

  gvr::Mat4f matrix;

  const float x_left = -tan(fov.left * (float)M_PI / 180.0f) * near;
  const float x_right = tan(fov.right * (float)M_PI / 180.0f) * near;
  const float y_bottom = -tan(fov.bottom * (float)M_PI / 180.0f) * near;
  const float y_top = tan(fov.top * (float)M_PI / 180.0f) * near;

  matrix.m[0][0] = (2 * near) / (x_right - x_left);
  matrix.m[0][1] = 0.0f;
  matrix.m[0][2] = (x_right + x_left) / (x_right - x_left);
  matrix.m[0][3] = 0.0f;
  
  matrix.m[1][0] = 0.0f;
  matrix.m[1][1] = (2 * near) / (y_top - y_bottom);
  matrix.m[1][2] = (y_top + y_bottom) / (y_top - y_bottom);
  matrix.m[1][3] = 0.0f;
  
  matrix.m[2][0] = 0.0f;
  matrix.m[2][1] = 0.0f;
  matrix.m[2][2] = (near + far) / (near - far);
  matrix.m[2][3] = (2 * near * far) / (near - far);

  matrix.m[3][0] = 0.0f;
  matrix.m[3][1] = 0.0f;
  matrix.m[3][2] = -1.0f;
  matrix.m[3][3] = 0.0f;

  return matrix;
}

gvr::Mat4f MatrixUtils::Transpose(const gvr::Mat4f& mat) {

  gvr::Mat4f matrix;

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      matrix.m[j][i] = mat.m[i][j];
    }
  }
  return matrix;
}
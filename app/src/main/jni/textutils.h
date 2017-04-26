#ifndef TEXT_UTILS_H_
#define TEXT_UTILS_H_

#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>

typedef struct {
  float x, y, width, height;
  float xOffset, yOffset, xAdvance;
} TextureChar;

typedef struct {
  unsigned int lineHeight;
  unsigned int textureWidth, textureHeight;
  std::map<unsigned int, TextureChar> charMap;
} TextureAtlas;

class TextUtils {
  
  public:
  
    // Create a texture atlas from the font file
    static TextureAtlas CreateAtlas();

    // Generate vertices from a string
    static void GenerateVertices(std::string name, std::vector<GLfloat>& vertices,
      float x, float y, float displayScale, TextureAtlas& atlas);
};

#endif  // TEXT_UTILS_H_
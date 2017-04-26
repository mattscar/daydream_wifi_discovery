#ifndef SHADER_UTILS_H_
#define SHADER_UTILS_H_

#include <cstdlib>
#include <string>
#include <vector>

#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>

class ShaderUtils {
  
  public:
  
    // Read text from shader file in assets folder  
    static std::string ReadFile(AAssetManager* mgr, const char* fileName);
    
    // Compile the shader program
    static void CompileShader(GLuint shader);
    
    // Link shaders into the program
    static void LinkProgram(GLuint program);
};

#endif  // SHADER_UTILS_H_
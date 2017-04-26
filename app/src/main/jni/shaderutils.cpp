#include "shaderutils.h"

static const char* TAG = "NetworkDiscovery";

std::string ShaderUtils::ReadFile(AAssetManager* mgr, const char* fileName) {

  std::string shaderCode;

  // Read shader file
  AAsset* asset = AAssetManager_open(mgr,
    fileName, AASSET_MODE_BUFFER);
  unsigned int length =
      static_cast<unsigned int>(AAsset_getLength(asset));
  
  // Read shader text into string
  shaderCode.reserve(length+1);
  AAsset_read(asset, &shaderCode[0], length);
  AAsset_close(asset);
  shaderCode[length] = '\0';

  return shaderCode;
}

void ShaderUtils::CompileShader(GLuint shader) {
  int status = GL_TRUE;
  GLsizei logLength = 0;
  std::string log;

  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(status == GL_FALSE) {
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    log.reserve((unsigned int)logLength);
    glGetShaderInfoLog(shader, logLength, &logLength, (GLchar*)log.c_str());
    __android_log_write(ANDROID_LOG_ERROR, TAG, log.c_str());
    glDeleteShader(shader);
    exit(EXIT_FAILURE);
  }
}

void ShaderUtils::LinkProgram(GLuint program) {
  int status = GL_TRUE;
  GLsizei logLength = 0;
  std::string log;
  
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if(status == GL_FALSE) {
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);   
    log.reserve((unsigned int)logLength);
    glGetProgramInfoLog(program, logLength, &logLength, (GLchar*)log.c_str());
    __android_log_write(ANDROID_LOG_ERROR, TAG, log.c_str());
    glDeleteProgram(program);    
    exit(EXIT_FAILURE);
  }
}
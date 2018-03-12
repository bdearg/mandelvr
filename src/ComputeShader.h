#pragma once

// most of this is frankenstein'd from Program.h
// "actually, it's frankenstein's m-" *shot*

#include <map>
#include <string>

#include <glad/glad.h>
#include "GLSL.h"

class ComputeShader
{
public:
  void setVerbose(const bool v) { verbose = v; }
  bool isVerbose() const { return verbose; }
  
  void setShaderName(const std::string &c);
  void setDimensions(GLuint newx, GLuint newy, GLuint newz);
  // these could be virtual if I planned to subclass this
  bool init();
  void dispatch();
  
  void addAttribute(const std::string &name);
  void addUniform(const std::string &name);
  GLint getAttribute(const std::string &name) const;
  GLint getUniform(const std::string &name) const;
  GLuint pid = 0;
  
protected:
  std::string cShaderName;
  
private:
  std::map<std::string, GLint> attributes;
  std::map<std::string, GLint> uniforms;
  GLuint xsize = 0;
  GLuint yzize = 1;
  GLuint zsize = 1;
  bool verbose = true;
};

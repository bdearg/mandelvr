#include <iostream>
#include <cassert>
#include <fstream>

#include "ComputeShader.h"
#include "FileUtils.h"

void ComputeShader::setShaderName(const std::string &c)
{
  cShaderName = c;
}

/*
void ComputeShader::setDimensions(GLuint newx, GLuint newy, GLuint newz)
{
  xsize = newx;
  ysize = newy;
  zsize = newz;
}*/

bool ComputeShader::init()
{
  GLint rc;
  
  GLuint CS = glCreateShader(GL_COMPUTE_SHADER);
}

/*
void ComputeShader::dispatch()
{
  CHECKED_GL_CALL(glUseProgram(pid));
//  CHECKED_GL_CALL(glDispatchCompute(xsize, ysize, zsize));
  glDispatchCompute(xsize, ysize, zsize);
  CHECKED_GL_CALL(glUseProgram(0));
}*/

void ComputeShader::addAttribute(const std::string &name)
{
  attributes[name] = GLSL::getAttribLocation(pid, name.c_str(), isVerbose());
}

void ComputeShader::addUniform(const std::string &name)
{
  uniforms[name] = GLSL::getUniformLocation(pid, name.c_str(), isVerbose());
}

GLint ComputeShader::getAttribute(const std::string &name) const
{
	auto attribute = attributes.find(name.c_str());
	if (attribute == attributes.end())
	{
		if (isVerbose())
		{
			std::cout << name << " is not an attribute variable" << std::endl;
		}
		return -1;
	}
	return attribute->second;
}

GLint ComputeShader::getUniform(const std::string &name) const
{
	auto uniform = uniforms.find(name.c_str());
	if (uniform == uniforms.end())
	{
		if (isVerbose())
		{
			std::cout << name << " is not a uniform variable" << std::endl;
		}
		return -1;
	}
	return uniform->second;
}



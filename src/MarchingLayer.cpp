#include "MarchingLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

Shape MarchingLayer::skybox_mesh;

MarchingLayer::MarchingLayer(int mapLevel, GLuint stencil, int w, int h)
{
  mappinglevel = mapLevel;
  stencilID = stencil;
  width = w;
  height = h;
  
  initTextures();
}

MarchingLayer::MarchingLayer(MarchingLayer &&other)
{
  claimGLObjects(other);
}

MarchingLayer &MarchingLayer::operator=(MarchingLayer &&other)
{
  if(this != &other)
  {
    release();
    
    claimGLObjects(other);
  }
  
  return *this;
}

MarchingLayer::~MarchingLayer()
{
  release();
}

void MarchingLayer::initTextures()
{
  glGenTextures(1, &texArray);
  glGenTextures(1, &marchDepthBufArray);
  glGenFramebuffers(NUM_SIDES, framebufs.data());
  
  glBindTexture(GL_TEXTURE_2D_ARRAY, texArray);
      
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height, NUM_SIDES);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  glBindTexture(GL_TEXTURE_2D_ARRAY, marchDepthBufArray);
  
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, width, height, NUM_SIDES);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  for(int i = 0; i < NUM_SIDES; i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, framebufs[i]);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texArray, 0, i);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, stencilID, 0, i);
  }
}

void MarchingLayer::release()
{
  glDeleteTextures(1, &texArray);
  glDeleteTextures(1, &marchDepthBufArray);
  glDeleteFramebuffers(NUM_SIDES, framebufs.data());
}

void MarchingLayer::claimGLObjects(MarchingLayer &other)
{
    stencilID = other.stencilID;
    texArray = other.texArray;
    marchDepthBufArray = other.marchDepthBufArray;
    framebufs = other.framebufs;
    
    other.stencilID = 0;
    other.texArray = 0;
    other.marchDepthBufArray = 0;
    other.framebufs.fill(0);
}

GLint MarchingLayer::getMarchDepthBuf()
{
  return marchDepthBufArray;
}

// display the cached texture
void MarchingLayer::draw(camera &cam, std::shared_ptr<Program> &ccSphereshader)
{
  ccSphereshader->bind();
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texArray);
  glUniform1i(ccSphereshader->getUniform("sphereMap"), 0);
  
  glm::vec3 camdir = cam.getForward();
  
  glm::mat4 camAtOrigin = glm::inverse(glm::lookAt(glm::vec3(0, 0, 0), camdir, cam.getUp())) * glm::translate(glm::mat4(1), glm::vec3(0, 0, 1.));
  glUniformMatrix4fv(ccSphereshader->getUniform("MVP"), 1, GL_TRUE, glm::value_ptr(camAtOrigin));
  MarchingLayer::skybox_mesh.draw(ccSphereshader);
  ccSphereshader->unbind();
}

// update the cached texture
void MarchingLayer::redraw(camera &cam, std::shared_ptr<Program> &mandelShader, MandelRenderer &mandel, GLuint inputDepthBuf, bool isRoot)
{
  for(int i = 0; i < NUM_SIDES; i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, framebufs[i]);
    mandel.render(mandelShader, cam.pos, dirEnumToDirection(i), dirEnumToUp(i), cam.zoomLevel, glm::vec2(width, height), *this, inputDepthBuf, i, isRoot);
  }
}

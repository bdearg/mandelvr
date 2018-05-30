#include "MarchingManager.h"

#include <algorithm>

MarchingManager::MarchingManager(int w, int h)
{
  width = w;
  height = h;
  
  createTextures();
  
  layers.emplace_back(1, stencilArray, width, height);
  layer_display_list.push_back(1);
}

MarchingManager::~MarchingManager()
{
  glDeleteTextures(1, &stencilArray);
  glDeleteTextures(1, &dummyDepthBuf);
  glDeleteFramebuffers(NUM_SIDES, flushbufs.data());
  // the layers *should* be destructed by the array destructor
}
  
void MarchingManager::createTextures()
{
  // init the common stencil we're using
  glGenTextures(1, &stencilArray);
  glGenTextures(1, &dummyDepthBuf);
  glGenFramebuffers(NUM_SIDES, flushbufs.data());
  
  // Stencil attachment
  glBindTexture(GL_TEXTURE_2D_ARRAY, stencilArray);
  
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_STENCIL_INDEX8, width, height, NUM_SIDES);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  glBindTexture(GL_TEXTURE_2D_ARRAY, dummyDepthBuf);
  
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, width, height, NUM_SIDES);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // need to somehow clear out the dummy depth buffer
  
  for(int i = 0; i < NUM_SIDES; i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, flushbufs[i]);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, stencilArray, 0, i);
  }
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void MarchingManager::setDepth(unsigned int depth)
{
  depth = std::max<unsigned int>(depth, 1);
  if(depth > layers.size()) // Adding more layers
  {
    while(depth > layers.size())
    {
      layers.emplace_back(layers.size()+1, stencilArray, width, height);
      layer_display_list.push_back(1);
    }
  }
  else if(depth < layers.size()) // Peeling away layers
  {
    while(depth < layers.size())
    {
      layers.pop_back();
      layer_display_list.pop_back();
    }
  }
  // if equal, we are already there - do nothing
}

int MarchingManager::getDepth()
{
  return layers.size();
}

GLuint MarchingManager::getDepthBufArray(int layer)
{
  auto a = std::next(layers.begin(), layer);
  return a->getMarchDepthBuf();
}

void MarchingManager::draw(camera &cam, std::shared_ptr<Program> &ccSphereshader)
{
  int j = 0;
  for(auto i = layers.begin(); i != layers.end(); i++, j++)
  {
    if(!layer_display_list[j])
      continue;
    i->draw(cam, ccSphereshader);
  }
}

void MarchingManager::redraw(camera &cam, std::shared_ptr<Program> &mandelShader, MandelRenderer &mandel)
{
  // reset the stencil buffer, unset it when pixels are drawn
  glEnable(GL_STENCIL_TEST);
  glClearStencil(0x01);
  // Clear away the stencil bits for all sides
  for(int n = 0; n < NUM_SIDES; n++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, flushbufs[n]);
    glClear(GL_STENCIL_BUFFER_BIT);
  }
  glStencilFunc(GL_EQUAL, 0x01, 0x01);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  
  GLuint dBuf = dummyDepthBuf;
  
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Is this necessary?
  auto i = layers.rbegin();
  for(unsigned int j = 0; j < layers.size() - 1; i++, j++)
  {
    i->redraw(cam, mandelShader, mandel, dBuf, false);
    dBuf = i-> getMarchDepthBuf();
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Is this necessary?
  }
  i->redraw(cam, mandelShader, mandel, dBuf, true);
  glDisable(GL_STENCIL_TEST);
}

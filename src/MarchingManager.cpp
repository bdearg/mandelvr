#include "MarchingManager.h"

#include <algorithm>

MarchingManager::MarchingManager(int w, int h)
{
  width = w;
  height = h;
  
  createStencil();
  createMarchDepthbuffer();
  
  layers.emplace_back(1, stencilArray, width, height);
  layer_display_list.push_back(1);
}

MarchingManager::~MarchingManager()
{
  glDeleteTextures(1, &stencilArray);
  glDeleteFramebuffers(NUM_SIDES, flushbufs.data());
  // the layers *should* be destructed by the array destructor
}
  
void MarchingManager::createStencil()
{
  // init the common stencil we're using
  glGenTextures(1, &stencilArray);
  glGenFramebuffers(NUM_SIDES, flushbufs.data());
  
  // Stencil attachment
  glBindTexture(GL_TEXTURE_2D_ARRAY, stencilArray);
  
  glTextureStorage3D(stencilArray, 1, GL_STENCIL_INDEX8, width, height, NUM_SIDES);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  for(int i = 0; i < NUM_SIDES; i++)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, flushbufs[i]);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, stencilArray, 0, i);
  }
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void MarchingManager::createMarchDepthbuffer()
{
  glGenTextures(1, &marchDepthArray);
  
  glBindTexture(GL_TEXTURE_2D_ARRAY, marchDepthArray);
  
  glTextureStorage3D(marchDepthArray, 1, GL_R32F, width, height, NUM_SIDES);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
  auto i = layers.rbegin();
  for(unsigned int j = 0; j < layers.size() - 1; i++, j++)
  {
    i->redraw(cam, mandelShader, mandel);
  }
  i->redraw(cam, mandelShader, mandel, true);
  glDisable(GL_STENCIL_TEST);
}

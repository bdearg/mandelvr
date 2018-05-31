#ifndef __MARCHINGMANAGER_H
#define __MARCHINGMANAGER_H

#include <list>

#include "MarchingLayer.h"

class MarchingManager
{
  // A list is used to avoid spurious destructions which would wreak havoc with GL buffer names
  std::list<MarchingLayer> layers;
  std::array<GLuint, NUM_SIDES> flushbufs;
  GLuint stencilArray, dummyDepthBuf;
  int width, height;
  int steps_per_layer = 32;
  
  void createTextures();

public:
  std::vector<char> layer_display_list;

  void setDepth(unsigned int depth);
  int getDepth();
  
  GLuint getDepthBufArray(int layer);
  
  void draw(camera &cam, std::shared_ptr<Program> &ccSphereshader);
  void redraw(camera &cam, std::shared_ptr<Program> &mandelShader, MandelRenderer &mandel);
  void redraw_if_needed(camera &cam, std::shared_ptr<Program> &mandelShader, MandelRenderer &mandel);
  
  MarchingManager(int width, int height);
  ~MarchingManager();
};

#endif

#ifndef __MARCHINGLAYER_H
#define __MARCHINGLAYER_H

#include <memory>
#include <array>
#include <glad/glad.h>

#include "camera.h"
#include "Program.h"
#include "Shape.h"
#include "directions.h"

// mutual dependencies
class MarchingLayer;
#include "MandelRenderer.h"

class MarchingLayer {
  int stepcount;
  GLuint stencilID, texArray, marchDepthBufArray;
  std::array<GLuint, NUM_SIDES> framebufs;
  
  // internal function used during construction
  void initTextures();
public:
  int mappinglevel;
  int width, height;

  MarchingLayer(int maplevel, GLuint stencil, int w, int h);
  ~MarchingLayer();
  
  void draw(camera &cam, std::shared_ptr<Program> &ccSphereshader);
  void redraw(camera &cam, std::shared_ptr<Program> &mandelShader, MandelRenderer &mandel, bool isRoot=false);
  
  static Shape skybox_mesh;
};

#endif

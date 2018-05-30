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
  
  // Variables keeping track of where this snapshot is, for determining
  // if an update is needed
  glm::vec3 capturePos;
  float captureScale;
  
  // internal function used during construction
  void initTextures();
public:
  int mappinglevel;
  int width, height;

  MarchingLayer(int maplevel, GLuint stencil, int w, int h);
  ~MarchingLayer();
  
  GLint getMarchDepthBuf();
  
  void draw(camera &cam, std::shared_ptr<Program> &ccSphereshader);
  void redraw(camera &cam, std::shared_ptr<Program> &mandelShader, MandelRenderer &mandel, GLuint inputDepthBuf, bool isRoot);
  
  static Shape skybox_mesh;
};

#endif

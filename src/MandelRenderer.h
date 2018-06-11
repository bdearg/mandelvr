#ifndef __MANDELRENDERER_H
#define __MANDELRENDERER_H

#include <glm/glm.hpp>
#include <memory>
#include <glad/glad.h>
#include "Program.h"
#include "imgui.h"

struct MandelRenderer
{
  struct RenderData {
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    ImVec4 y_color = ImVec4(0.10,0.20,0.30,1.);
    ImVec4 z_color = ImVec4(0.02,0.10,0.30,1.);
    ImVec4 w_color = ImVec4(0.30,0.10,0.02,1.);
    
    ImVec4 diff1 = ImVec4(1.50,1.10,0.70,1.);
    ImVec4 diff2 = ImVec4(0.25,0.20,0.15,1.);
    ImVec4 diff3 = ImVec4(0.10,0.20,0.30,1.);
    
    GLfloat intersect_threshold = 0.0025;
    GLint intersect_step_count = 128;
    GLfloat intersect_step_factor = 1.;
    
    GLfloat zoom_level = 1.0;
    GLfloat map_start_offset = 1.0;
    
    GLfloat fle = 1.;
    
    GLint modulo = 8;
    GLfloat escape_factor = 1.;
    GLfloat map_result_factor = 1.;
    GLint map_iter_count = 4;
    GLfloat juliaFactor = 0.;
    
    ImVec4 juliaPoint = ImVec4(0., 0., 0., 0.);
    
    // when set, the renderer will "guess" what's at the end
    // of a raymarching
    GLboolean exhaust = 0;
    
    GLfloat time = 0.;
    int direction = 0;
  } data;
  
  static GLuint VertexArrayUnitPlane;
  static GLuint VertexBufferUnitPlane;
  
  // prepares internal rendering structures
  static void init();
  
  void render(std::shared_ptr<Program> prog, float zoomLevel, glm::vec2 size, bool exhaust);
  
  void render(std::shared_ptr<Program> prog, float zoomLevel, glm::vec2 size, MarchingLayer &marcher, GLuint inputDepthBuf, int direction, bool isRoot);
  
private:
  void render_internal(std::shared_ptr<Program> prog, glm::vec2 size, RenderData &dat);
};

#endif

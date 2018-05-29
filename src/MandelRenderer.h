#ifndef __MANDELRENDERER_H
#define __MANDELRENDERER_H

#include <glm/glm.hpp>
#include <memory>
#include <glad/glad.h>
#include "Program.h"
#include "imgui.h"

// mutual dependencies
struct MandelRenderer;
#include "MarchingLayer.h"

struct MandelRenderer
{
  struct RenderData {
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    ImVec4 y_color = ImVec4(0.10,0.20,0.30,1.);
    ImVec4 z_color = ImVec4(0.02,0.10,0.30,1.);
    ImVec4 w_color = ImVec4(0.30,0.10,0.02,1.);
    
    GLfloat intersect_threshold = 0.0025;
    GLint intersect_step_count = 32;
    
    GLfloat zoom_level = 1.0;
    GLfloat start_offset = 1.0;
    
    GLfloat fle = 1.;
    
    GLint modulo = 8;
    GLfloat escape_factor = 1.;
    GLfloat map_result_factor = 1.;
    GLint map_iter_count = 1;
    
    // when set, the renderer will "guess" what's at the end
    // of a raymarching
    GLboolean exhaust = 0;
  } data;
  
  static GLuint VertexArrayUnitPlane;
  static GLuint VertexBufferUnitPlane;
  
  // prepares internal rendering structures
  static void init();
  
  void render(std::shared_ptr<Program> prog, glm::vec3 pos, glm::vec3 forward, glm::vec3 up, float zoomLevel, glm::vec2 size);
  
  void render(std::shared_ptr<Program> prog, glm::vec3 pos, glm::vec3 forward, glm::vec3 up, float zoomLevel, glm::vec2 size, MarchingLayer &marcher, bool isRoot);
  
private:
  void render_internal(std::shared_ptr<Program> prog, glm::vec3 pos, glm::vec3 forward, glm::vec3 up, glm::vec2 size, RenderData &dat);
};

#endif

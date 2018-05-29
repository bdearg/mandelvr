#include "MandelRenderer.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


  
GLuint MandelRenderer::VertexArrayUnitPlane;
GLuint MandelRenderer::VertexBufferUnitPlane;

void MandelRenderer::init()
{
  glGenVertexArrays(1, &VertexArrayUnitPlane);
  glBindVertexArray(VertexArrayUnitPlane);

  //generate vertex buffer to hand off to OGL
  glGenBuffers(1, &VertexBufferUnitPlane);
  //set the current state to focus on our vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, VertexBufferUnitPlane);

  GLfloat *ver = new GLfloat[6 * 3];
  // front
  int verc = 0;

  ver[verc++] = 0.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
  ver[verc++] = 1.0, ver[verc++] = 1.0, ver[verc++] = 0.0;
  ver[verc++] = 0.0, ver[verc++] = 1.0, ver[verc++] = 0.0;
  ver[verc++] = 0.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
  ver[verc++] = 1.0, ver[verc++] = 0.0, ver[verc++] = 0.0;
  ver[verc++] = 1.0, ver[verc++] = 1.0, ver[verc++] = 0.0;

  //actually memcopy the data - only do this once
  glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), ver, GL_STATIC_DRAW);
  //we need to set up the vertex array
  glEnableVertexAttribArray(0);
  //key function to get up how many elements to pull out at a time (3)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
}

void MandelRenderer::render(std::shared_ptr<Program> prog, glm::vec3 pos, glm::vec3 forward, glm::vec3 up, float zoomLevel, glm::vec2 size)
{
  // this is probably real inefficient oops
  RenderData d2 = data;
  d2.zoom_level = zoomLevel;
  render_internal(prog, pos, forward, up, size, d2);
}
  
void MandelRenderer::render(std::shared_ptr<Program> prog, glm::vec3 pos, glm::vec3 forward, glm::vec3 up, float zoomLevel, glm::vec2 size, MarchingLayer &marcher, bool isRoot)
{
  RenderData d2 = data;
  d2.zoom_level = zoomLevel;
  d2.map_iter_count = marcher.mappinglevel;
  d2.exhaust = isRoot;
  render_internal(prog, pos, forward, up, size, d2);
}

void MandelRenderer::render_internal(std::shared_ptr<Program> prog, glm::vec3 pos, glm::vec3 forward, glm::vec3 up, glm::vec2 size, RenderData &dat)
{
  glViewport(0, 0, size.x, size.y);
  
  glm::mat4 view = glm::lookAt(pos, pos + forward, up);
  glm::vec3 camorigin = pos * dat.zoom_level;
  glClearColor(0.f, 0.f, 0.f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  
  prog->bind();
  glUniform2f(prog->getUniform("resolution"), static_cast<float>(size.x), static_cast<float>(size.y));
  glUniform1f(prog->getUniform("intersectThreshold"), dat.intersect_threshold);
  glUniform1i(prog->getUniform("intersectStepCount"), dat.intersect_step_count);
  glUniform3fv(prog->getUniform("clearColor"), 1, (float*)&dat.clear_color);
  glUniform3fv(prog->getUniform("yColor"), 1, (float*)&dat.y_color);
  glUniform3fv(prog->getUniform("zColor"), 1, (float*)&dat.z_color);
  glUniform3fv(prog->getUniform("wColor"), 1, (float*)&dat.w_color);
  glUniform1f(prog->getUniform("zoomLevel"), dat.zoom_level);
  glUniform1f(prog->getUniform("startOffset"), dat.start_offset);
  glUniform1f(prog->getUniform("fle"), dat.fle);
  glUniform1i(prog->getUniform("modulo"), dat.modulo);
  glUniform1f(prog->getUniform("escapeFactor"), dat.escape_factor);
  glUniform1f(prog->getUniform("mapResultFactor"), dat.map_result_factor);
  glUniform1i(prog->getUniform("mapIterCount"), dat.map_iter_count);
  glUniform3fv(prog->getUniform("camOrigin"), 1, glm::value_ptr(pos));
  glUniform1i(prog->getUniform("exhaust"), dat.exhaust);
  
  glUniformMatrix4fv(prog->getUniform("view"), 1, GL_TRUE, glm::value_ptr(view));
  
  glBindVertexArray(VertexArrayUnitPlane);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  prog->unbind();
}
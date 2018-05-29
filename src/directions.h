#ifndef __DIRECTIONS_H
#define __DIRECTIONS_H

#include <glm/glm.hpp>
  
enum texture_dirs {
  FRONT,
  RIGHT,
  LEFT,
  BACK,
  BOTTOM,
  TOP,
  NUM_SIDES
};

glm::vec4 dirEnumToUp(int dir);
glm::vec4 dirEnumToDirection(int dir);

#ifdef DIRECTION_IMPLEMENTATION
#include <iostream>
// ugh this is a gross hack and I need to think of a more algorithmic/sane thing to do here
glm::vec4 dirEnumToUp(int dir)
{
  switch(dir)
  {
    default:
      std::cerr << "Invalid direction enum: " << dir << std::endl;
    case FRONT:
    case BACK:
    case LEFT:
    case RIGHT:
      return glm::vec4(0, +1, 0, 0);
      break;
    case TOP:
      return glm::vec4(0, 0, +1, 0);
      break;
    case BOTTOM:
      return glm::vec4(0, 0, -1, 0);
      break;
  }
}

glm::vec4 dirEnumToDirection(int dir)
{
  switch(dir)
  {
    default:
      std::cerr << "Invalid direction enum: " << dir << std::endl;
    case FRONT:
      return glm::vec4(0, 0, +1, 0);
      break;
    case BACK:
      return glm::vec4(0, 0, -1, 0);
      break;
    case LEFT:
      return glm::vec4(-1, 0, 0, 0);
      break;
    case RIGHT:
      return glm::vec4(+1, 0, 0, 0);
      break;
    case TOP:
      return glm::vec4(0, +1, 0, 0);
      break;
    case BOTTOM:
      return glm::vec4(0, -1, 0, 0);
      break;
  }
}
#endif

#endif

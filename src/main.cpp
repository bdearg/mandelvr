/* Lab 6 base code - transforms using local matrix functions
  to be written by students -
  based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
  & Ian Dunn, Christian Eckhardt
*/
#include <iostream>
#include <chrono>
#include <algorithm>
#include <array>
#include <cmath>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "camera.h"

#include "imgui_impl_glfw_gl3.h"

// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
using namespace std;
using namespace glm;

#define FPSBUFSIZE 15

// VR defaults
#if 0
#define FRAMEWIDTH  2160
#define FRAMEHEIGHT 1200
#else
#define FRAMEWIDTH  600
#define FRAMEHEIGHT 480
#endif

#define BOXTEXSIZE 1024

class MandelBulbRenderer
{
  public:
  // some sort of voxel data structure within which to store our "render"?
  // openGL Compute program IDs
};

class Application : public EventCallbacks
{

public:

  WindowManager * windowManager = nullptr;

  // Our shader program
  std::shared_ptr<Program> mandelshader;
  
  std::shared_ptr<Program> ccSphereshader;

  //camera
  camera mycam;
  
  mat4 bulb_xfrm = mat4(1.);
  
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  
  ImVec4 y_color = ImVec4(0.10,0.20,0.30,1.);
  ImVec4 z_color = ImVec4(0.02,0.10,0.30,1.);
  ImVec4 w_color = ImVec4(0.30,0.10,0.02,1.);
  
  GLfloat intersect_step_size = 0.0025;
  GLint intersect_step_count = 128;
  
  GLfloat zoom_level = 1.0;
  GLfloat start_offset = 1.0;
  
  GLfloat fle = 1.5;
  
  GLint modulo = 8;
  GLfloat escape_factor = 1.;
  GLfloat map_result_factor = 1.;
  GLint map_iter_count = 1;

  GLuint VertexArrayUnitPlane, VertexBufferUnitPlane;
  
  vec3 skybox_translate = vec3(0, 0, 1.);
  
  int foo = 0;
  bool cubemode = false;
  
  string shaderLoc;
  
  enum texture_dirs {
    TOP = 0,
    BOTTOM,
    LEFT,
    RIGHT,
    FRONT,
    BACK,
    NUM_SIDES
  };
  
  Shape skybox_mesh;
  
  struct CCsphere {
    array<GLuint, NUM_SIDES> skyFBO;
    GLuint skyTex;
    GLint xres, yres;
  } ccsphere;
  
  void addShaderAttributes()
  {
    mandelshader->addAttribute("vertPos");
    mandelshader->addUniform("resolution");
    mandelshader->addUniform("time");
    mandelshader->addUniform("view");
    mandelshader->addUniform("clearColor");
    mandelshader->addUniform("yColor");
    mandelshader->addUniform("zColor");
    mandelshader->addUniform("wColor");
    mandelshader->addUniform("intersectStepSize");
    mandelshader->addUniform("intersectStepCount");
    mandelshader->addUniform("zoomLevel");
    mandelshader->addUniform("modulo");
    mandelshader->addUniform("fle");
    mandelshader->addUniform("escapeFactor");
    mandelshader->addUniform("mapResultFactor");
    mandelshader->addUniform("mapIterCount");
    mandelshader->addUniform("startOffset");
    mandelshader->addUniform("bulbXfrm");
  }

  void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
  {
    // IMGUI STUFF
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    if(io.WantCaptureKeyboard)
      return;
    // FIN IMGUI STUFF
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
      mycam.w = 1;
    }
    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
    {
      mycam.w = 0;
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
      mycam.s = 1;
    }
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
    {
      mycam.s = 0;
    }
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
      mycam.a = 1;
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
      mycam.a = 0;
    }
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
      mycam.d = 1;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
      mycam.d = 0;
    }
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
      mycam.q = 1;
    }
    if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
    {
      mycam.q = 0;
    }
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
      mycam.e = 1;
    }
    if (key == GLFW_KEY_E && action == GLFW_RELEASE)
    {
      mycam.e = 0;
    }
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
      mat4 look = lookAt(
        vec3(0, 0, 2),// eye
        vec3(0, 0, 0),// target
        vec3(0, 1, 0)// up
        );
      vec4 eyepos = look * vec4(0, 0, 0, 1);
      mycam.pos = vec3(eyepos);
      mycam.rot = vec3(0, 0, 0);
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
      foo = (foo + 1) % NUM_SIDES;
      cout << "Showing side: ";
      switch(foo)
      {
        case TOP:
          cout << "Top";
          break;
        case BOTTOM:
          cout << "Bottom";
          break;
        case LEFT:
          cout << "Left";
          break;
        case RIGHT:
          cout << "Right";
          break;
        case FRONT:
          cout << "Front";
          break;
        case BACK:
          cout << "Back";
          break;
      }
      cout << endl;
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
      cubemode = !cubemode;
    }

		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			// reload shader
			shared_ptr<Program> tmp = mandelshader;

			mandelshader = make_shared<Program>();
			mandelshader->setVerbose(true);
			mandelshader->setShaderNames(shaderLoc + "/passthru.vs", shaderLoc + "/IQ_mandelbulb_derivative.fs");
			if (!mandelshader->init())
			{
				std::cerr << "One or more shaders failed to compile... no change made!" << std::endl;
				mandelshader = tmp;
			}
			else
			{
				addShaderAttributes();
			}
		}
  }

  bool aiming = false;

  void mouseCallback(GLFWwindow *window, int button, int action, int mods)
  {
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        ImGui_ImpGlfw_SetMouseJustPressed(button);
        
    if (button == GLFW_MOUSE_BUTTON_RIGHT && !io.WantCaptureMouse)
    {
      aiming = (action == GLFW_PRESS);
      glfwGetCursorPos(windowManager->getHandle(), &prevX, &prevY);
    }
  }

  void resizeCallback(GLFWwindow *window, int width, int height)
  {
    glViewport(0, 0, width, height);
  }
  
  double prevX = -1., prevY = -1.;
  
  void cursorPosCallback(GLFWwindow *window, double x, double y)
  {
    if(!aiming)
      return;
    int w, h;
    double dX, dY;
    
    glfwGetFramebufferSize(windowManager->getHandle(), &w, &h);
    dX = (prevX-x)/w;
    dY = (prevY-y)/h;
    prevX = x;
    prevY = y;
    
    mycam.rotate(dX, dY);
  }

  void init(const std::string& resourceDirectory)
  {
    GLSL::checkVersion();

    shaderLoc = resourceDirectory;
    
    // Set background color.
    glClearColor(0.f, 1.f, 0.f, 1.0f);

    // Enable z-buffer test.
    glEnable(GL_DEPTH_TEST);

    //culling:
    glFrontFace(GL_CCW);

    //transparency
    glEnable(GL_BLEND);
    //next function defines how to mix the background color with the transparent pixel in the foreground. 
    //This is the standard:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    ImGui::CreateContext();
    ImGui_ImplGlfwGL3_Init(windowManager->getHandle(), true);
    
    ccsphere.xres = BOXTEXSIZE;
    ccsphere.yres = BOXTEXSIZE;
    ccsphere.skyFBO.fill(0);
    ccsphere.skyTex = 0;
    createCCSphere();
    
    mat4 look = lookAt(
      vec3(0, 0, 2),// eye
      vec3(0, 0, 0),// target
      vec3(0, 1, 0)// up
      );
    vec4 eyepos = look * vec4(0, 0, 0, 1);
    mycam.pos = vec3(eyepos.x, eyepos.y, eyepos.z);

    mandelshader = make_shared<Program>();
    mandelshader->setVerbose(true);
    //mandelshader->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/showspace.fs");
    //mandelshader->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/IQ_juliabulb_derivative.fs");
    mandelshader->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/IQ_mandelbulb_derivative.fs");
    if (!mandelshader->init())
    {
      std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
      exit(1);
    }
    addShaderAttributes();
    
    ccSphereshader = make_shared<Program>();
    ccSphereshader->setVerbose(true);
    ccSphereshader->setShaderNames(resourceDirectory + "/ccsphere.vs", resourceDirectory + "/ccsphere.fs");
    if (!ccSphereshader->init())
    {
      std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
      exit(1);
    }
    ccSphereshader->addAttribute("vertNor");
    ccSphereshader->addAttribute("vertBinorm");
    ccSphereshader->addAttribute("vertTan");
    ccSphereshader->addAttribute("vertPos");
    ccSphereshader->addAttribute("vertTex");
    ccSphereshader->addUniform("sphereMap");
    ccSphereshader->addUniform("MVP");
  }

  void initGeom(const std::string& resourceDirectory)
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

    skybox_mesh.loadMesh(resourceDirectory + "/skybox2.obj");
    skybox_mesh.resize();
    skybox_mesh.init();
  }
  
  // this is pricey, it redraws the image to the faces of the skybox
  void redrawSkybox()
  {
  }
  
  // maybe call it an "onionbox" later or smthn
  void renderSkybox()
  {
    int width, height;
    // for each direction, bind a frame buffer, set the view matrix appropriately, and render
    mat4 camview = mycam.process();
  
    for(int i = 0; i < NUM_SIDES; i++)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, ccsphere.skyFBO[i]);
      renderBulb(mycam.pos, dirEnumToDirection(i), dirEnumToUp(i), vec2(ccsphere.xres, ccsphere.yres));
    }
    
    ccSphereshader->bind();
    // This binds the main screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ccsphere.skyTex);
    glUniform1i(ccSphereshader->getUniform("sphereMap"), 0);
    
    vec3 camdir = mycam.viewVec();
    
    mat4 camAtOrigin = transpose(lookAt(vec3(0, 0, 0), camdir, vec3(0, 1, 0)) * translate(mat4(), skybox_translate));
    glUniformMatrix4fv(ccSphereshader->getUniform("MVP"), 1, GL_FALSE, value_ptr(camAtOrigin));
    skybox_mesh.draw(ccSphereshader);
    ccSphereshader->unbind();
  }
  
  // bind your framebuffer before calling this function
  void renderBulb(vec3 origin, vec3 direction, vec3 up, vec2 bufferRes)
  {
    glViewport(0, 0, bufferRes.x, bufferRes.y);
    
    mat4 view = transpose(lookAt(origin, origin + direction, up));
    
    // Clear framebuffer.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    mandelshader->bind();
    glUniform2f(mandelshader->getUniform("resolution"), static_cast<float>(bufferRes.x), static_cast<float>(bufferRes.y));
    glUniform1f(mandelshader->getUniform("intersectStepSize"), intersect_step_size*mycam.zoomLevel);
    glUniform1i(mandelshader->getUniform("intersectStepCount"), intersect_step_count);
    glUniform3fv(mandelshader->getUniform("clearColor"), 1, (float*)&clear_color);
    glUniform3fv(mandelshader->getUniform("yColor"), 1, (float*)&y_color);
    glUniform3fv(mandelshader->getUniform("zColor"), 1, (float*)&z_color);
    glUniform3fv(mandelshader->getUniform("wColor"), 1, (float*)&w_color);
    glUniform1f(mandelshader->getUniform("zoomLevel"), mycam.zoomLevel);
    glUniform1f(mandelshader->getUniform("startOffset"), start_offset);
    glUniform1f(mandelshader->getUniform("fle"), fle);
    glUniform1i(mandelshader->getUniform("modulo"), modulo);
    glUniform1f(mandelshader->getUniform("escapeFactor"), escape_factor);
    glUniform1f(mandelshader->getUniform("mapResultFactor"), map_result_factor);
    glUniform1i(mandelshader->getUniform("mapIterCount"), map_iter_count);
    glUniform1f(mandelshader->getUniform("time"), glfwGetTime());
    
    glUniformMatrix4fv(mandelshader->getUniform("view"), 1, GL_FALSE, value_ptr(view));
    
    glBindVertexArray(VertexArrayUnitPlane);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    mandelshader->unbind(); 
  }

  void render()
  {
    int width, height;
    glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
//    renderSkybox();
#if 1
    mycam.process();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if(cubemode)
    {
      renderBulb(mycam.pos, dirEnumToDirection(foo), dirEnumToUp(foo), vec2(ccsphere.xres, ccsphere.yres));
    }
    else
    {
      renderBulb(mycam.pos, mycam.viewVec(), vec3(0, 1, 0), vec2(width, height));
    }
#endif
  }
  
  vec4 dirEnumToDirection(int dir)
  {
    switch(dir)
    {
      default:
        cerr << "Invalid direction enum: " << dir << endl;
      case FRONT:
        return vec4(0, 0, -1, 0);
        break;
      case BACK:
        return vec4(0, 0, +1, 0);
        break;
      case LEFT:
        return vec4(-1, 0, 0, 0);
        break;
      case RIGHT:
        return vec4(+1, 0, 0, 0);
        break;
      case TOP:
        return vec4(0, +1, 0, 0);
        break;
      case BOTTOM:
        return vec4(0, -1, 0, 0);
        break;
    }
  }
  
  // ugh this is a gross hack and I need to think of a more algorithmic/sane thing to do here
  vec4 dirEnumToUp(int dir)
  {
    switch(dir)
    {
      default:
        cerr << "Invalid direction enum: " << dir << endl;
      case FRONT:
      case BACK:
      case LEFT:
      case RIGHT:
        return vec4(0, +1, 0, 0);
        break;
      case TOP:
        return vec4(0, 0, +1, 0);
        break;
      case BOTTOM:
        return vec4(0, 0, -1, 0);
        break;
    }
  }
  
  void createCCSphere()
  {
    // init fbos and other gl structures
    // load in and map the ccsphere model
    glGenTextures(1, &ccsphere.skyTex);
    glGenFramebuffers(NUM_SIDES, ccsphere.skyFBO.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ccsphere.skyTex);
      
    glTextureStorage3D(ccsphere.skyTex, 1, GL_RGBA8, ccsphere.xres, ccsphere.yres, NUM_SIDES);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    for(int i = 0; i < NUM_SIDES; i++)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, ccsphere.skyFBO[i]);
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ccsphere.skyTex, 0, i);
    }
  }

  void CreateFBOandTex(unsigned int width, unsigned int height, GLuint* fbo, GLuint* colorattch) {
    glGenFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

    glGenTextures(1, colorattch);
    glBindTexture(GL_TEXTURE_2D, *colorattch);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *colorattch, 0);
  }
  
  void doImgui()
  {
    ImGui::Begin("Mandelbulb");
    ImGui::Text("Mandelbulb controls");                           // Display some text (you can use a format string too)
    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
    ImGui::ColorEdit3("y color", (float*)&y_color); // Edit 3 floats representing a color
    ImGui::ColorEdit3("z color", (float*)&z_color); // Edit 3 floats representing a color
    ImGui::ColorEdit3("w color", (float*)&w_color); // Edit 3 floats representing a color
    
    ImGui::SliderFloat("mapping start offset", &start_offset, 0.002f, 30.f, "%.3f", 1.2f);
    ImGui::SliderFloat("zoom level", &mycam.zoomLevel, 0.002f, 30.f, "%.3f", 1.2f);
    ImGui::SliderFloat("intersect step size", &intersect_step_size, 1e-20, 1e-1f, "%.3e", 1.5f);
    ImGui::SliderInt("intersect step count", &intersect_step_count, 1, 1024);
    ImGui::SliderInt("Mandelbulb modulo", &modulo, 2, 32);
    ImGui::SliderFloat("Mandelbulb escape factor", &escape_factor, .01, 10., "%.3f", 4.2f);
    ImGui::SliderFloat("Mandelbulb map result factor", &map_result_factor, .01, 10., "%.3f", 4.2f);
    ImGui::SliderInt("Mandelbulb map iter count", &map_iter_count, 1, 32);
    ImGui::SliderFloat("fle", &fle, 0.1f, 15.f);
    
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
    
    if(cubemode)
    {
      ImGui::Begin("Side controls");
      if(ImGui::Button("Top"))
      {
        foo = TOP;
      }
      if(ImGui::Button("Bottom"))
      {
        foo = BOTTOM;
      }
      if(ImGui::ArrowButton("Left", ImGuiDir_Left))
      {
        foo = LEFT;
      }
      if(ImGui::ArrowButton("Right", ImGuiDir_Right))
      {
        foo = RIGHT;
      }
      if(ImGui::ArrowButton("Front", ImGuiDir_Up))
      {
        foo = FRONT;
      }
      if(ImGui::ArrowButton("Back", ImGuiDir_Down))
      {
        foo = BACK;
      }
      ImGui::End();
    }
    
    vec3 viewdir = mycam.viewVec();
    vec3 xforward = mycam.xMovement();
    vec3 zforward = mycam.zMovement();
    
    ImGui::Begin("Camera");
    ImGui::LabelText("View Vector:", "X: %0.2f, Y: %0.2f, Z: %0.2f", viewdir.x, viewdir.y, viewdir.z);
    ImGui::LabelText("View Position:", "X: %0.2f, Y: %0.2f, Z: %0.2f", mycam.pos.x, mycam.pos.y, mycam.pos.z);
    ImGui::LabelText("X Forward:", "X: %0.2f, Y: %0.2f, Z: %0.2f", xforward.x, xforward.y, xforward.z);
    ImGui::LabelText("Z Forward:", "X: %0.2f, Y: %0.2f, Z: %0.2f", zforward.x, zforward.y, zforward.z);
    ImGui::End();
  }
};

struct FPSdata {
  bool dataInit = false;
  chrono::microseconds fpsbuffer[FPSBUFSIZE];
  size_t fpsoff = 0;
  chrono::steady_clock::time_point start, stop;
};

void startFrameCapture(FPSdata &dt)
{
  dt.start = chrono::steady_clock::now(); 
}

void showFPS(FPSdata &dt)
{
    dt.stop = chrono::steady_clock::now();
    auto frame_duration = chrono::duration_cast<chrono::microseconds>(dt.stop - dt.start);
    if(!dt.dataInit)
    {
      for(auto i = 0; i < FPSBUFSIZE; i++)
      {
        dt.fpsbuffer[i] = frame_duration;
      }
    }
    dt.fpsbuffer[(++dt.fpsoff) % FPSBUFSIZE] = frame_duration;
    dt.fpsoff %= FPSBUFSIZE;
    double avgfps = 0.0; for (int i = 0; i < FPSBUFSIZE; i++) { avgfps += 1e6 / dt.fpsbuffer[i].count(); } avgfps /= FPSBUFSIZE;
    cout << "Frame: " << dt.fpsbuffer[dt.fpsoff].count() / 1e3 << "ms, FPS: " << 1e6 / dt.fpsbuffer[dt.fpsoff].count() << ", FPS(avg): " << avgfps << endl;
}

//*********************************************************************************************************
int main(int argc, char **argv)
{
  // Where the resources are loaded from
  std::string resourceDir = "../resources";

  if (argc >= 2)
  {
    resourceDir = argv[1];
  }


  Application *application = new Application();

  // Your main will always include a similar set up to establish your window
  // and GL context, etc.

  WindowManager *windowManager = new WindowManager();
  windowManager->init(FRAMEWIDTH, FRAMEHEIGHT);
  windowManager->setEventCallbacks(application);
  application->windowManager = windowManager;

  // This is the code that will likely change program to program as you
  // may need to initialize or set up different data and state

  application->init(resourceDir);
  application->initGeom(resourceDir);

  FPSdata dt;
  
  // Loop until the user closes the window.
  while (! glfwWindowShouldClose(windowManager->getHandle()))
  {
    //startFrameCapture(dt);
    // Render scene.
    ImGui_ImplGlfwGL3_NewFrame();
    application->render();
    application->doImgui();
    ImGui::Render();
    ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
    
    // Swap front and back buffers.
    glfwSwapBuffers(windowManager->getHandle());
    //showFPS(dt);
    // Poll for and process events.
    glfwPollEvents();
  }

  // Quit program.
  ImGui_ImplGlfwGL3_Shutdown();
  ImGui::DestroyContext();
  windowManager->shutdown();
  return 0;
}



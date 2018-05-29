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
#include <cassert>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DIRECTION_IMPLEMENTATION
#include "directions.h"

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "camera.h"
#include "MarchingLayer.h"
#include "MarchingManager.h"

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

#define BOXTEXSIZE 256

class Application : public EventCallbacks
{

public:

  WindowManager * windowManager = nullptr;

  // Our shader program
  std::shared_ptr<Program> mandelshader;
  
  std::shared_ptr<Program> ccSphereshader;

  //camera
  camera mycam;
  
  MandelRenderer mrender;

  std::shared_ptr<MarchingManager> marcher;
  
  GLuint feedbackBuf, queryObject;
  
  bool showHUD = false;
  bool hud_countdown = false;
  chrono::steady_clock::time_point showHUDTime;
  
  vec3 skybox_translate = vec3(0, 0, 1.);
  bool cubemode = true;
  bool freezeRender = false;
  
  string shaderLoc;
  
  GLuint commonSkyStencil;
  
  void addShaderAttributes()
  {
    mandelshader->addAttribute("vertPos");
    mandelshader->addUniform("resolution");
    mandelshader->addUniform("view");
    mandelshader->addUniform("camOrigin");
    mandelshader->addUniform("clearColor");
    mandelshader->addUniform("yColor");
    mandelshader->addUniform("zColor");
    mandelshader->addUniform("wColor");
    mandelshader->addUniform("intersectThreshold");
    mandelshader->addUniform("intersectStepCount");
    mandelshader->addUniform("zoomLevel");
    mandelshader->addUniform("modulo");
    mandelshader->addUniform("fle");
    mandelshader->addUniform("exhaust");
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
    {
      // probably put stuff in here to handle unsetting movement inputs
      return;
    }
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
      mycam.pos = vec3(0, 0, 2);
      mycam.pitch = mycam.yaw = 0;
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
      cubemode = !cubemode;
    }
    if (key == GLFW_KEY_U && action == GLFW_PRESS)
    {
      freezeRender = !freezeRender;
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
		
		if(mycam.anyButtonPressed())
		{
		  showHUD = true;
		}
		else if(action == GLFW_RELEASE)
		{
		  showHUDTime = chrono::steady_clock::now() + chrono::duration_cast<chrono::steady_clock::duration>(chrono::seconds(1));
		  hud_countdown = true;
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
      // this doesn't work for some reason
      // it SHOULD hide the cursor while you're right clicking so you can freely pan, but, wugh
      glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, aiming ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
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
    

    //transparency
    glEnable(GL_BLEND);

    //culling:
    glFrontFace(GL_CCW);

    //next function defines how to mix the background color with the transparent pixel in the foreground. 
    //This is the standard:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    ImGui::CreateContext();
    ImGui_ImplGlfwGL3_Init(windowManager->getHandle(), true);

    marcher = make_shared<MarchingManager>(BOXTEXSIZE, BOXTEXSIZE);

    mrender.init();
    
    mycam.pos = vec3(0, 0, 2);
    mycam.pitch = mycam.yaw = 0;

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
    glGenTransformFeedbacks(1, &feedbackBuf);
    glGenQueries(1, &queryObject);

    // prep the common mesh
    MarchingLayer::skybox_mesh.loadMesh(resourceDirectory + "/skybox2.obj");
    MarchingLayer::skybox_mesh.resize();
    MarchingLayer::skybox_mesh.init();
  }
  
  // maybe call it an "onionbox" later or smthn
  void renderSkybox()
  {
    int width, height;
    // for each direction, bind a frame buffer, set the view matrix appropriately, and render
    mycam.process();
    
    if(!freezeRender)
    {
      marcher->redraw(mycam, mandelshader, mrender);
    }
    // This binds the main screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Set background color - max green, to stand out, in order to expose errors
    glClearColor(0.f, 1.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
    glViewport(0, 0, width, height);
    
    marcher->draw(mycam, ccSphereshader);
  }

  void render()
  {
    int width, height;
    glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
    if(cubemode)
    {
      renderSkybox();
    }
    else
    {
      mycam.process();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      mrender.render(mandelshader, mycam.pos*mycam.zoomLevel, mycam.getForward(), vec3(0, 1, 0), mycam.zoomLevel, vec2(width, height));
    }
  }
  
  void update()
  {
    // use "zoom level" to determine level of detail
    marcher->setDepth(static_cast<int>(-log(mycam.zoomLevel)));
  }
  
  void createCCStencil(int width, int height)
  {
    // init fbos and other gl structures
    // load in and map the ccsphere model
    glGenTextures(1, &commonSkyStencil);
    
    // Stencil attachment
    glBindTexture(GL_TEXTURE_2D_ARRAY, commonSkyStencil);
    
    glTextureStorage3D(commonSkyStencil, 1, GL_STENCIL_INDEX8, width, height, NUM_SIDES);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  
  void doImgui()
  {
    if(hud_countdown && chrono::steady_clock::now() < showHUDTime)
    {
      showHUD = false;
      hud_countdown = false;
    }

    if(ImGui::Begin("Mandelbulb"))
    {
      ImGui::Text("Mandelbulb controls");                           // Display some text (you can use a format string too)
      ImGui::ColorEdit3("clear color", (float*)&mrender.data.clear_color); // Edit 3 floats representing a color
      ImGui::ColorEdit3("y color", (float*)&mrender.data.y_color); // Edit 3 floats representing a color
      ImGui::ColorEdit3("z color", (float*)&mrender.data.z_color); // Edit 3 floats representing a color
      ImGui::ColorEdit3("w color", (float*)&mrender.data.w_color); // Edit 3 floats representing a color
      
      ImGui::SliderFloat("mapping start offset", &mrender.data.start_offset, 0.002f, 30.f, "%.3f", 1.2f);
      ImGui::SliderFloat("zoom level", &mycam.zoomLevel, 1e-20f, 1.f, "%.3f", 10.f);
      ImGui::SliderFloat("intersect threshold", &mrender.data.intersect_threshold, 1e-20, 1e-1f, "%.3e", 1.5f);
      ImGui::SliderInt("intersect step count", &mrender.data.intersect_step_count, 1, 1024);
      ImGui::SliderInt("Mandelbulb modulo", &mrender.data.modulo, 2, 32);
      ImGui::SliderFloat("Mandelbulb escape factor", &mrender.data.escape_factor, .01, 10., "%.3f", 4.2f);
      ImGui::SliderFloat("Mandelbulb map result factor", &mrender.data.map_result_factor, .01, 10., "%.3f", 4.2f);
      ImGui::SliderInt("Mandelbulb map iter count", &mrender.data.map_iter_count, 1, 32);
      ImGui::SliderFloat("fle", &mrender.data.fle, 0.1f, 15.f);
      
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }
    
    if(ImGui::Begin("Camera"))
    {
      vec3 viewdir = mycam.getForward();
      vec3 xforward = mycam.xMovement();
      vec3 zforward = mycam.zMovement();
      ImGui::LabelText("View Vector:", "X: %0.2f, Y: %0.2f, Z: %0.2f", viewdir.x, viewdir.y, viewdir.z);
      ImGui::LabelText("View Position:", "X: %0.2f, Y: %0.2f, Z: %0.2f", mycam.pos.x, mycam.pos.y, mycam.pos.z);
      ImGui::LabelText("X Forward:", "X: %0.2f, Y: %0.2f, Z: %0.2f", xforward.x, xforward.y, xforward.z);
      ImGui::LabelText("Z Forward:", "X: %0.2f, Y: %0.2f, Z: %0.2f", zforward.x, zforward.y, zforward.z);
      ImGui::LabelText("Theta:", "%0.2f pi", mycam.yaw / pi<float>());
      ImGui::LabelText("Phi:", "%0.2f", mycam.pitch);
      
      ImGui::SliderFloat("Movement Factor", &mycam.velocityFactor, 1e-2f, 1e4f, "%.3f", 10.f);
      
      
      if(ImGui::TreeNode("View Matrix"))
      {
        ImGuiTextBuffer matrix_text;
        mat4 view = mycam.getView();
        for(int i = 0; i < 4; i++)
        {
          matrix_text.appendf("%5.2f %5.2f %5.2f %5.2f\n", view[0][i], view[1][i], view[2][i], view[3][i]);
        }
        ImGui::Text(matrix_text.c_str());
        ImGui::TreePop();
      }
      ImGui::End();
    }
    
    if(ImGui::Begin("Position HUD", &showHUD, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav))
    {
      ImGui::Text("Position: X: %0.2f, Y: %0.2f, Z: %0.2f", mycam.pos.x, mycam.pos.y, mycam.pos.z);
      
      ImGui::Text("Zoom Level:");
      ImGui::SameLine(); ImGui::ProgressBar(-log(mycam.zoomLevel)/1e1);
      
      if(hud_countdown)
      {
        ImGui::Text("Vanish in %ld seconds", chrono::duration_cast<chrono::seconds>(showHUDTime - chrono::steady_clock::now()).count());
      }
      ImGui::End();
    }

    if(ImGui::Begin("Layers Info"))
    {
      ImGui::LabelText("Layers:", "%d", marcher->getDepth());
      for(unsigned int i = 0; i < marcher->layer_display_list.size(); i++)
      {
        ImGui::Checkbox("", (bool*)&marcher->layer_display_list[i]); ImGui::SameLine();
      }
      ImGui::NewLine();
      ImGui::End();
    }

    ImGui::ShowDemoWindow();
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
    application->update();
    application->doImgui();
    ImGui::Render();
    ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
    
    // Swap front and back buffers.
    glfwSwapBuffers(windowManager->getHandle());
    // Poll for and process events.
    glfwPollEvents();
  }

  // Quit program.
  ImGui_ImplGlfwGL3_Shutdown();
  ImGui::DestroyContext();
  windowManager->shutdown();
  return 0;
}



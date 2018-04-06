/* Lab 6 base code - transforms using local matrix functions
	to be written by students -
	based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
	& Ian Dunn, Christian Eckhardt
*/
#include <iostream>
#include <chrono>
#include <algorithm>
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
	std::shared_ptr<Program> pixshader;

	//camera
	camera mycam;
	
	mat4 bulb_xfrm = mat4(1.);
	
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  
  ImVec4 y_color = ImVec4(0.10,0.20,0.30,1.);
  ImVec4 z_color = ImVec4(0.02,0.10,0.30,1.);
  ImVec4 w_color = ImVec4(0.30,0.10,0.02,1.);
	
	GLfloat intersectStepSize = 10.0;
	
	GLfloat zoom_level = 1.0;
	GLfloat start_offset = 1.0;

	GLuint VertexArrayUnitPlane, VertexBufferUnitPlane;

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
	}

  bool aiming = false;

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        ImGui_ImpGlfw_SetMouseJustPressed(button);
        
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
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

		
		// Set background color.
		glClearColor(0.12f, 0.34f, 0.56f, 1.0f);

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		//culling:
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		//transparency
		glEnable(GL_BLEND);
		//next function defines how to mix the background color with the transparent pixel in the foreground. 
		//This is the standard:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
    ImGui::CreateContext();
    ImGui_ImplGlfwGL3_Init(windowManager->getHandle(), true);
		
		mat4 look = lookAt(
		  vec3(0, 0, 2),// eye
		  vec3(0, 0, 0),// target
		  vec3(0, 1, 0)// up
		  );
		vec4 eyepos = look * vec4(0, 0, 0, 1);
		mycam.pos = vec3(eyepos.x, eyepos.y, eyepos.z);

		pixshader = make_shared<Program>();
		pixshader->setVerbose(true);
    //pixshader->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/showspace.fs");
		//pixshader->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/IQ_juliabulb_derivative.fs");
		pixshader->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/IQ_mandelbulb_derivative.fs");
		if (!pixshader->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		pixshader->init();
		pixshader->addAttribute("vertPos");
		pixshader->addUniform("resolution");
		pixshader->addUniform("time");
		pixshader->addUniform("view");
		pixshader->addUniform("clearColor");
		pixshader->addUniform("yColor");
		pixshader->addUniform("zColor");
		pixshader->addUniform("wColor");
		pixshader->addUniform("intersectStepSize");
		pixshader->addUniform("zoomLevel");
		pixshader->addUniform("startOffset");
		pixshader->addUniform("bulbXfrm");
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

	}

	void render()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);
    //ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		
		// love too couple input processing with state polling
		mat4 view = transpose(mycam.process());
		
		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		pixshader->bind();
		glUniform2f(pixshader->getUniform("resolution"), static_cast<float>(width), static_cast<float>(width));
		glUniform1f(pixshader->getUniform("time"), glfwGetTime());
//		glUniform1f(pixshader->getUniform("intersectStepSize"), 0.25/glm::max(1.f, -log10(length(mycam.pos)/500000.f)));
		glUniform1f(pixshader->getUniform("intersectStepSize"), 0.0025);
		glUniform3fv(pixshader->getUniform("clearColor"), 1, (float*)&clear_color);
		glUniform3fv(pixshader->getUniform("yColor"), 1, (float*)&y_color);
		glUniform3fv(pixshader->getUniform("zColor"), 1, (float*)&z_color);
		glUniform3fv(pixshader->getUniform("wColor"), 1, (float*)&w_color);
		glUniform1f(pixshader->getUniform("zoomLevel"), zoom_level);
		glUniform1f(pixshader->getUniform("startOffset"), start_offset);
		glUniformMatrix4fv(pixshader->getUniform("view"), 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(pixshader->getUniform("bulbXfrm"), 1, GL_FALSE, value_ptr(bulb_xfrm));

		glBindVertexArray(VertexArrayUnitPlane);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		pixshader->unbind();
		
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
    ImGui::SliderFloat("zoom level", &zoom_level, 0.002f, 30.f, "%.3f", 1.2f);
    
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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



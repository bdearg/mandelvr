/* Lab 6 base code - transforms using local matrix functions
	to be written by students -
	based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
	& Ian Dunn, Christian Eckhardt
*/
#define OPENVRBUILD
#include <iostream>
#include <chrono>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "camera.h"
#include "VRplayer.hpp"
// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <openvr.h>

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

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;
	vr::IVRSystem* pHMD; 

	// Our shader program
	std::shared_ptr<Program> pixshader;

	//camera
	camera mycam;

	VRplayer* vrviewer = nullptr;
	
	GLfloat intersectStepSize = 10.0;

	GLuint VertexArrayUnitPlane, VertexBufferUnitPlane;

	struct HMD_PoseData {
		mat4 P_left;
		mat4 P_right;
		mat4 Pose_left;
		mat4 Pose_right;
	} HMD_Pose;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
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

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		
		// Set background color.
		glClearColor(0.12f, 0.34f, 0.56f, 1.0f);

		// Enable z-buffer test.
		glDisable(GL_DEPTH_TEST);

		//culling:
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		//transparency
		glDisable(GL_BLEND);
		//next function defines how to mix the background color with the transparent pixel in the foreground. 
		//This is the standard:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
		
		mat4 look = lookAt(
		  vec3(0, 0, 2),// eye
		  vec3(0, 0, 0),// target
		  vec3(0, 1, 0)// up
		  );
		vec4 eyepos = look * vec4(0, 0, 0, 1);
		mycam.pos = vec3(eyepos.x, eyepos.y, eyepos.z);

		pixshader = make_shared<Program>();
		pixshader->setVerbose(true);
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
		pixshader->addUniform("intersectStepSize");
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

	void initOVR() {
		vr::EVRInitError vrErr = vr::VRInitError_None;
		pHMD = vr::VR_Init(&vrErr, vr::VRApplication_Scene);

		if (vrErr != vr::VRInitError_None) {
			pHMD = NULL;
			printf("Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(vrErr));
			exit(1);
		}

		if (!vr::VRCompositor()) {
			printf("Compositor initialization failed. See log file for details\n");
			exit(1);
		}

		// CreateEyeFBO(target_width, target_height, leftEyeDesc);
		// CreateEyeFBO(target_width, target_height, rightEyeDesc);
	}

	void render()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);
		
		// love too couple input processing with state polling
		mat4 view = transpose(mycam.process());
		
		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

		pixshader->bind();
		glUniform2f(pixshader->getUniform("resolution"), static_cast<float>(width), static_cast<float>(width));
		glUniform1f(pixshader->getUniform("time"), glfwGetTime());
		glUniform1f(pixshader->getUniform("intersectStepSize"), 0.25);
		glUniformMatrix4fv(pixshader->getUniform("view"), 1, GL_FALSE, value_ptr(view));

		glBindVertexArray(VertexArrayUnitPlane);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		pixshader->unbind();
		
	}

	void VRrender() {
		// psuedocode to get started
		// int width, height =  Get vr recommended FBO size
		// glViewport(width, height)
		// 
		// bind pixel shader and set basic uniforms
		// 
		// vrviewer->playerWaitGetPoses

		// bindLeftEyeFBO 
		// set uniforms from vrviewer
		// glBindVertexArray(VertexArrayUnitPlane);
		// glDrawArrays(GL_TRIANGLES, 0, 6);
		// 
		// bindRightEyeFBO
		// set uniforms from vrviewer
		// glBindVertexArray(VertexArrayUnitPlane);
		// glDrawArrays(GL_TRIANGLES, 0, 6);

		// submit left and right to compositor

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
#ifdef OPENVRBUILD
	application->initOVR();
#endif
  FPSdata dt;
	
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
	  startFrameCapture(dt);
		// Render scene.
#ifdef OPENVRBUILD
	    application->VRrender();
#else
		application->render();
#endif
		
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		showFPS(dt);
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}



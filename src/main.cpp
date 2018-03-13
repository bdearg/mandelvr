/* Lab 6 base code - transforms using local matrix functions
	to be written by students -
	based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
	& Ian Dunn, Christian Eckhardt
*/
#define OPENVRBUILD
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

#include "VRplayer.hpp"

#include "imgui_impl_glfw_gl3.h"

// used for helper in perspective
#include "glm/glm.hpp"
#include "glm/ext.hpp"
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

static void CreateEyeFBO(UINT width, UINT height, GLuint* fbo, GLuint* colorattch);

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;
	vr::IVRSystem* pHMD;

	// Our shader program
	std::shared_ptr<Program> pixshader;
	std::shared_ptr<Program> passthru;

	//camera
	camera mycam;

	VRplayer* vrviewer = nullptr;
	
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	GLfloat intersectStepSize = 10.0;

	GLuint VertexArrayUnitPlane, VertexBufferUnitPlane;

	struct EyeFbo {
		GLuint FBO;
		GLuint colorattch;
	};

	EyeFbo leftFBO;
	EyeFbo rightFBO;

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

  bool aiming = false;

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
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
    dX = (x - prevX)/w;
    dY = ((prevY-y)/h);
    prevX = x;
    prevY = y;
    
    cout << "dY: " << dY << endl;
    
    mycam.rotate(dX, dY);
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
		pixshader->addUniform("projection");
		pixshader->addUniform("intersectStepSize");
		pixshader->addUniform("viewoffset");
		pixshader->addUniform("viewscale");
		pixshader->addUniform("P");
		pixshader->addUniform("headpose");
		pixshader->addUniform("rotationoffset");

		passthru = make_shared<Program>();
		passthru->setVerbose(true);
		passthru->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/companion_view.fs");
		if (!passthru->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		passthru->init();
		passthru->addAttribute("vertPos");
		passthru->addUniform("leftEye");
		passthru->addUniform("rightEye");
		passthru->addUniform("vr_resolution");
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

		vrviewer = new VRplayer(pHMD);
	}

	void initVRFBO() {
		uint32_t vr_width, vr_height;
		pHMD->GetRecommendedRenderTargetSize(&vr_width, &vr_height);

		CreateEyeFBO(vr_width, vr_height, &(leftFBO.FBO), &(leftFBO.colorattch));
		CreateEyeFBO(vr_width, vr_height, &(rightFBO.FBO), &(rightFBO.colorattch));
	}

	void render()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);
		
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		// love too couple input processing with state polling
		mat4 view = transpose(mycam.process());

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		pixshader->bind();
		glUniform2f(pixshader->getUniform("resolution"), static_cast<float>(width), static_cast<float>(width));
		glUniform1f(pixshader->getUniform("time"), glfwGetTime());
//		glUniform1f(pixshader->getUniform("intersectStepSize"), 0.25/glm::max(1.f, -log10(length(mycam.pos)/500000.f)));
		glUniform1f(pixshader->getUniform("intersectStepSize"), 0.0025);
		glUniformMatrix4fv(pixshader->getUniform("view"), 1, GL_FALSE, value_ptr(view));

		glBindVertexArray(VertexArrayUnitPlane);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		pixshader->unbind();

	}


	void VRrender() {
		uint32_t vr_width, vr_height;
		pHMD->GetRecommendedRenderTargetSize(&vr_width, &vr_height);
		glViewport(0, 0, vr_width, vr_height);

		
		mat4 hmdPose = vrviewer->getHeadsetPose();
		mat4 rotationOffset = vrviewer->getRotationOffset();
		cout << glm::to_string(rotationOffset) << endl;
		float viewerscale = static_cast<float>(vrviewer->getPlayerScale());
		{ // Left eye
			pixshader->bind();
			glUniform2f(pixshader->getUniform("resolution"), static_cast<float>(vr_width), static_cast<float>(vr_height));
			glUniform1f(pixshader->getUniform("time"), glfwGetTime());
			glUniform1f(pixshader->getUniform("intersectStepSize"), 0.25);
			glUniform1f(pixshader->getUniform("viewscale"), viewerscale);
			mat4 view = (vrviewer->getEyeView(vr::Eye_Left));
			mat4 P = vrviewer->getEyeProj(vr::Eye_Left);
			glUniformMatrix4fv(pixshader->getUniform("view"), 1, GL_FALSE, value_ptr(view));
			glUniformMatrix4fv(pixshader->getUniform("projection"), 1, GL_FALSE, value_ptr(P));
			glUniformMatrix4fv(pixshader->getUniform("headpose"), 1, GL_FALSE, value_ptr(hmdPose));
			glUniform3fv(pixshader->getUniform("viewoffset"), 1, value_ptr(vrviewer->getPositionOffset()));
			glUniformMatrix4fv(pixshader->getUniform("rotationoffset"), 1, GL_FALSE, value_ptr(rotationOffset));

			glBindFramebuffer(GL_FRAMEBUFFER, leftFBO.FBO);

			glBindVertexArray(VertexArrayUnitPlane);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			pixshader->unbind();

			vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftFBO.colorattch, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
			vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		}

		{ // Right eye
			pixshader->bind();
			glUniform2f(pixshader->getUniform("resolution"), static_cast<float>(vr_width), static_cast<float>(vr_height));
			glUniform1f(pixshader->getUniform("time"), glfwGetTime());
			glUniform1f(pixshader->getUniform("intersectStepSize"), 0.25);
			glUniform1f(pixshader->getUniform("viewscale"), viewerscale);
			mat4 view = vrviewer->getEyeView(vr::Eye_Right);
			mat4 P = vrviewer->getEyeProj(vr::Eye_Right);
			glUniformMatrix4fv(pixshader->getUniform("view"), 1, GL_FALSE, value_ptr(view));
			glUniformMatrix4fv(pixshader->getUniform("projection"), 1, GL_FALSE, value_ptr(P));
			glUniformMatrix4fv(pixshader->getUniform("headpose"), 1, GL_FALSE, value_ptr(hmdPose));
			glUniform3fv(pixshader->getUniform("viewoffset"), 1, value_ptr(vrviewer->getPositionOffset()));
			glUniformMatrix4fv(pixshader->getUniform("rotationoffset"), 1, GL_FALSE, value_ptr(rotationOffset));

			glBindFramebuffer(GL_FRAMEBUFFER, rightFBO.FBO);

			glBindVertexArray(VertexArrayUnitPlane);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			pixshader->unbind();

			vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightFBO.colorattch, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
			vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		passthru->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, leftFBO.colorattch);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, rightFBO.colorattch);
		glUniform1i(passthru->getUniform("leftEye"), 0);
		glUniform1i(passthru->getUniform("rightEye"), 1);
		glUniform2f(passthru->getUniform("vr_resolution"), static_cast<float>(vr_width), static_cast<float>(vr_height));
		glBindVertexArray(VertexArrayUnitPlane);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		passthru->unbind();

		vrviewer->playerWaitGetPoses();

	}
	
	void doImgui()
	{
    static float f = 0.0f;
    static int counter = 0;
    ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}

};

static void CreateEyeFBO(UINT width, UINT height, GLuint* fbo, GLuint* colorattch) {
	glGenFramebuffers(1, fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

	glGenTextures(1, colorattch);
	glBindTexture(GL_TEXTURE_2D, *colorattch);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *colorattch, 0);
}

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
	if (!dt.dataInit)
	{
		for (auto i = 0; i < FPSBUFSIZE; i++)
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

#ifdef OPENVRBUILD
	application->initOVR();
#endif

	WindowManager *windowManager = new WindowManager();
#ifdef OPENVRBUILD
	uint32_t vr_width, vr_height;
	application->pHMD->GetRecommendedRenderTargetSize(&vr_width, &vr_height);
	windowManager->init(vr_width, vr_height/2);
#else
	windowManager->init(FRAMEWIDTH, FRAMEHEIGHT);
#endif
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state


	application->init(resourceDir);
	application->initGeom(resourceDir);

#ifdef OPENVRBUILD
	application->initVRFBO();
#endif

	FPSdata dt;

	// Loop until the user closes the window.
	double lasttime = glfwGetTime();
	while (!glfwWindowShouldClose(windowManager->getHandle()))
	{
		startFrameCapture(dt);
		// Render scene.
    ImGui_ImplGlfwGL3_NewFrame();
#ifdef OPENVRBUILD
		application->vrviewer->playerControlsTick(windowManager->getHandle(), glfwGetTime() - lasttime);
		lasttime = glfwGetTime();
		application->VRrender();
#else
		application->render();
#endif

		application->doImgui();
    ImGui::Render();
    ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		//showFPS(dt);
		// Poll for and process events.
		glfwPollEvents();
	}

#ifdef OPENVRBUILD
	vr::VR_Shutdown();
#endif
	// Quit program.
  ImGui_ImplGlfwGL3_Shutdown();
  ImGui::DestroyContext();
	windowManager->shutdown();
	return 0;
}



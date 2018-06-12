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
#include "MandelRenderer.h"

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
#define MAXBUTTONS 10

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

	string shaderLoc;

	//camera
	camera mycam;

	unique_ptr<VRplayer> vrviewer;
	
	MandelRenderer mrender;

	GLfloat intersectStepSize = 0.25;
	GLint intersectStepCount = 128;

	GLuint VertexArrayUnitPlane, VertexBufferUnitPlane;

	bool VRButtonJustPressed[MAXBUTTONS];

	struct EyeFbo {
		GLuint FBO;
		GLuint colorattch;
	};

	EyeFbo leftFBO;
	EyeFbo rightFBO;

	void addShaderAttributes()
	{
		pixshader->addAttribute("vertPos");
		pixshader->addUniform("resolution");
		pixshader->addUniform("time");
		pixshader->addUniform("view");
		pixshader->addUniform("clearColor");
		pixshader->addUniform("yColor");
		pixshader->addUniform("zColor");
		pixshader->addUniform("wColor");
		pixshader->addUniform("diffc1");
		pixshader->addUniform("diffc2");
		pixshader->addUniform("diffc3");
		pixshader->addUniform("modulo");
		pixshader->addUniform("juliaFactor");
		pixshader->addUniform("juliaPoint");
		pixshader->addUniform("projection");
		pixshader->addUniform("intersectStepSize");
		pixshader->addUniform("intersectStepCount");
		pixshader->addUniform("mapIterCount");
		pixshader->addUniform("exhaust");
		pixshader->addUniform("unitIPD");
		pixshader->addUniform("viewoffset");
		pixshader->addUniform("viewscale");
		pixshader->addUniform("P");
		pixshader->addUniform("headpose");
		pixshader->addUniform("rotationoffset");
		pixshader->addUniform("modulo");

		pixshader->addUniform("iTest");
	}

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

		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			// reload shader
			shared_ptr<Program> tmp = pixshader;

			pixshader = make_shared<Program>();
			pixshader->setVerbose(true);
			pixshader->setShaderNames(shaderLoc + "/passthru.vs", shaderLoc + "/IQ_mandelbulb_derivative.fs");
			if (!pixshader->init())
			{
				std::cerr << "One or more shaders failed to compile... no change made!" << std::endl;
				pixshader = tmp;
			}
			else
			{
				addShaderAttributes();
			}
		}

		if (key == GLFW_KEY_O && action == GLFW_PRESS)
		{
			vrviewer->resetView();
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

		// im hacker
		shaderLoc = resourceDirectory;

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

		mrender.init();

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
		pixshader->setShaderNames(resourceDirectory + "/passthru.vs", resourceDirectory + "/IQ_mandelbulb_derivative.fs");
		if (!pixshader->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		pixshader->init();
		addShaderAttributes();

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

		vrviewer.reset(new VRplayer(pHMD));
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


		for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; i++)
		{
			if (glfwJoystickPresent(i))
			{
				int count;
				const unsigned char* buttons = glfwGetJoystickButtons(i, &count);
				const char* name = glfwGetJoystickName(i);
				cout << name << ": " << count << " buttons" << endl;
			}
		}
	}

	void joystickCallback(int joy, int event)
	{
		if (event == GLFW_DISCONNECTED && joy > GLFW_JOYSTICK_2)
		{
			cerr << "This program needs 2 joysticks to run!" << endl;
			exit(EXIT_FAILURE);
		}
	}

	void VRrender() {
		uint32_t vr_width, vr_height;
		pHMD->GetRecommendedRenderTargetSize(&vr_width, &vr_height);
		glViewport(0, 0, vr_width, vr_height);

		mat4 worldTransform = glm::mat4(1.f); // glm::rotate(90.f, vec3(0, 0, 1));
		
		mat4 hmdPose = vrviewer->getHeadsetPose();
		mat4 rotationOffset = vrviewer->getRotationOffset();
//		cout << glm::to_string(rotationOffset) << endl;
		float viewerscale = static_cast<float>(vrviewer->getPlayerScale());
		{ // Left eye
			pixshader->bind();
			glUniform1f(pixshader->getUniform("time"), glfwGetTime());
			glUniform1f(pixshader->getUniform("unitIPD"), vrviewer->getFocusMult());
			mat4 view = vrviewer->getEyeView(vr::Eye_Left);
			mat4 viewrot = view*worldTransform;
			mat4 P = vrviewer->getEyeProj(vr::Eye_Left);
			glUniformMatrix4fv(pixshader->getUniform("view"), 1, GL_FALSE, value_ptr(viewrot));
			glUniformMatrix4fv(pixshader->getUniform("projection"), 1, GL_FALSE, value_ptr(P));
			glUniformMatrix4fv(pixshader->getUniform("headpose"), 1, GL_FALSE, value_ptr(hmdPose));
			glUniform3fv(pixshader->getUniform("viewoffset"), 1, value_ptr(vrviewer->getPositionOffset()));
			glUniformMatrix4fv(pixshader->getUniform("rotationoffset"), 1, GL_FALSE, value_ptr(rotationOffset));

			glBindFramebuffer(GL_FRAMEBUFFER, leftFBO.FBO);

			mrender.render(pixshader, viewerscale, vec2(static_cast<float>(vr_width), static_cast<float>(vr_height)), true);

			pixshader->unbind();

			vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftFBO.colorattch, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
			vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		}

		{ // Right eye
			pixshader->bind();
			glUniform1f(pixshader->getUniform("time"), glfwGetTime());
			glUniform1f(pixshader->getUniform("unitIPD"), vrviewer->getFocusMult());
			mat4 view = vrviewer->getEyeView(vr::Eye_Right);
			mat4 viewrot = view*worldTransform;
			mat4 P = vrviewer->getEyeProj(vr::Eye_Right);
			glUniformMatrix4fv(pixshader->getUniform("view"), 1, GL_FALSE, value_ptr(viewrot));
			glUniformMatrix4fv(pixshader->getUniform("projection"), 1, GL_FALSE, value_ptr(P));
			glUniformMatrix4fv(pixshader->getUniform("headpose"), 1, GL_FALSE, value_ptr(hmdPose));
			glUniform3fv(pixshader->getUniform("viewoffset"), 1, value_ptr(vrviewer->getPositionOffset()));
			glUniformMatrix4fv(pixshader->getUniform("rotationoffset"), 1, GL_FALSE, value_ptr(rotationOffset));

			glBindFramebuffer(GL_FRAMEBUFFER, rightFBO.FBO);

			mrender.render(pixshader, viewerscale, vec2(static_cast<float>(vr_width), static_cast<float>(vr_height)), true);

			pixshader->unbind();

			vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightFBO.colorattch, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
			vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		ImGui_ImplGlfwGL3_NewFrame();

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
		doImgui();
		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

		vrviewer->playerWaitGetPoses();

	}

	void doImgui()
	{
		ImGui::Begin("Mandelbulb");
		ImGui::Text("Mandelbulb controls");                           // Display some text (you can use a format string too)                           // Display some text (you can use a format string too)
    ImGui::ColorEdit3("clear color", (float*)&mrender.data.clear_color); // Edit 3 floats representing a color
    ImGui::ColorEdit3("y color", (float*)&mrender.data.y_color); // Edit 3 floats representing a color
    ImGui::ColorEdit3("z color", (float*)&mrender.data.z_color); // Edit 3 floats representing a color
    ImGui::ColorEdit3("w color", (float*)&mrender.data.w_color); // Edit 3 floats representing a color
    ImGui::ColorEdit3("diffuse 1", (float*)&mrender.data.diff1);
    ImGui::ColorEdit3("diffuse 2", (float*)&mrender.data.diff2);
    ImGui::ColorEdit3("diffuse 3", (float*)&mrender.data.diff3);
    
    ImGui::SliderInt("intersect step count", &mrender.data.intersect_step_count, 1, 1024);
    ImGui::SliderFloat("intersect step factor", &mrender.data.intersect_step_factor, 1e-20, 1.f, "%.3e", 1.5f);
    ImGui::SliderInt("Mandelbulb modulo", &mrender.data.modulo, 2, 32);
    ImGui::SliderInt("Mandelbulb map iter count", &mrender.data.map_iter_count, 1, 32);
	ImGui::SliderInt("Int test", &mrender.data.iTest, 1, 32);
	  ImGui::SliderFloat3("Julia Point", (float*)&mrender.data.juliaPoint, -1., 1.);
	  ImGui::SliderFloat("Julia Factor", &mrender.data.juliaFactor, 0.f, 1.f);
		ImGui::SliderFloat("Intersect Step Size", &intersectStepSize, 2.5e-12, 15., "%.3e", 10.f);


		vec3 pos = vrviewer->getPositionOffset();
		ImGui::Text("X: %.3f Y: %.3f Z: %.3f", pos.x, pos.y, pos.z);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		ImGui::Begin("Controls");
		ImGui::Text("KEYBOARD");
		ImGui::Text("R - reload shader");
		ImGui::Text("O - Reset to origin");
		ImGui::Text("Esc - Exit");

		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::Text("OCULUS");
		ImGui::NewLine();
		ImGui::Text("L Hand");
		ImGui::Text("Joystick - 'flat' movement");
		ImGui::Text("A - Shrink");
		ImGui::Text("B - Reduce inter-eye distance");
		ImGui::Text("Squeeze trigger - Boost");
		ImGui::NewLine();
		ImGui::Text("R Hand");
		ImGui::Text("Joystick - U/D movement + rotation");
		ImGui::Text("X - Grow");
		ImGui::Text("Y - Increase inter-eye distance");
		ImGui::Text("Squeeze trigger - Boost");
		ImGui::End();
		
		bool showHUD = true;
    
		if(ImGui::Begin("Position HUD", &showHUD, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav))
		{
			ImGui::Text("Position: X: %0.2f, Y: %0.2f, Z: %0.2f", pos.x, pos.y, pos.z);
      
			ImGui::Text("Zoom Level:");
			ImGui::SameLine(); ImGui::ProgressBar(-log(static_cast<float>(vrviewer->getPlayerScale()))/1e1);
		}
		ImGui::End();
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

	// Loop until the user closes the window.
	double lasttime = glfwGetTime();
	while (!glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
#ifdef OPENVRBUILD
		application->vrviewer->playerControlsTick(windowManager->getHandle(), glfwGetTime() - lasttime);
		lasttime = glfwGetTime();
		application->VRrender();
#else
		application->render();
#endif

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

#ifdef OPENVRBUILD
	vr::VR_Shutdown();
#endif
	ImGui_ImplGlfwGL3_Shutdown();
	ImGui::DestroyContext();
	// Quit program.
	windowManager->shutdown();
	return 0;
}



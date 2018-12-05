/*
ZJ Wood CPE 471 Lab 3 base code
*/

#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <glad/glad.h>
#include <time.h>
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include <glm/gtx/string_cast.hpp>

#include "WindowManager.h"
#include "Shape.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;

float GROUND_OFFSET = -2.0;
float CAR_OFFSET = -15;
float TRANSFORM_FLOOR = 20.0;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct GameStats {
	int top;
	int bottom;
	int left;
	int right;
	bool playing;
	int size;
	int starting;
	int ending;
	bool stuck;
	double carSpeed;
	float isBraking = false;
};
#define M_PI 3.1415926
#define SHADOW_DIM 1024

struct Light
{
    vec3 position;
    vec3 direction;
    vec3 color;
};

struct Building {
    vec3 building;
    bool isVisible;
};

shared_ptr<Shape> shape, shapeLamp;
GameStats gameStats;
mat4 oldM;
vec3 oldCamPos, oldCamRot;
Light primaryLight;
bool show_shadowmap = false;
mat4 lightSpace = mat4(1.0f);

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}
class camera
{
public:
	glm::vec3 pos, rot;
	int w, a, s, d;
	camera()
	{
		w = a = s = d = 0;
		rot = vec3(0, 0, 0);
        pos = vec3(-18, -14, -17);
	}
	void incrementSpeed() {
        if (gameStats.carSpeed < 3.0) {
            gameStats.carSpeed += 0.07;
        }
	}
    void decrementSpeed() {
        if (gameStats.carSpeed > -2.0) {
            gameStats.carSpeed -= 0.07;
        }
    }
	glm::mat4 process(double ftime)
	{
		float speed = 0;
        if (w == 1) {
            incrementSpeed();
            gameStats.isBraking = 0.0;
        } else if (s == 1) {
            decrementSpeed();
            gameStats.isBraking = 1.0;
        } else {
            // bring to zero
            if (gameStats.carSpeed < 0) {
                gameStats.carSpeed += 0.01;
            } else if (gameStats.carSpeed > 0) {
                gameStats.carSpeed -= 0.01;
            }
            gameStats.isBraking = 0.0;
        }

        speed = -3 * ftime * gameStats.carSpeed;

		float yangle=0;

		if (a == 1)
			yangle = -(M_PI/2)*ftime * 1.5;
		else if(d==1)
			yangle = (M_PI/2)*ftime * 1.5;

        double xangle = (M_PI/2);
        rot.x = xangle;
        mat4 RX = rotate(mat4(1), rot.x, vec3(1, 0, 0));


		rot.y += yangle;
		mat4 R = rotate(mat4(1), rot.y, vec3(0, 0, 1));
		vec4 dir = vec4(0, speed, 0,1);
		dir = dir * R * RX;
		pos += vec3(dir.x, dir.y, dir.z);
		mat4 T = translate(mat4(1), pos);
		return R * RX * T;
	}
};

camera mycam;
class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	shared_ptr<Program> progLamp, /*progCityGround,*/ progCityBuilding, progLambo;
	shared_ptr<Program> prog2, shadowProg;

	GLuint VAOBox;
	GLuint VBOBoxPos, VBOBoxColor, VBOBoxIndex;
	GLuint FBOtex, fb, depth_rb, fb_shadowMap, FBOtex_shadowMapDepth;
	GLuint VertexBufferIDBox, VertexArrayIDBox, VertexBufferIDNorm, VertexBufferTex;

	//texture data
	GLuint Texture, mask, Texture2, citytex, streetTex;

    vector<Building> allBuildings;
    Building exitBuilding;
    int exitIndex;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{

		if (gameStats.playing) {
			if (key == GLFW_KEY_Y && action == GLFW_RELEASE)
				{
				show_shadowmap = !show_shadowmap;
				}
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
				glfwSetWindowShouldClose(window, GL_TRUE);
			}
			if (key == GLFW_KEY_W && action == GLFW_PRESS) {
				mycam.w = 1;
			}
			if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
				mycam.w = 0;
			}
			if (key == GLFW_KEY_S && action == GLFW_PRESS) {
				mycam.s = 1;
			}
			if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
				mycam.s = 0;
			}
			if (key == GLFW_KEY_A && action == GLFW_PRESS) {
				mycam.a = 1;
			}
			if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
				mycam.a = 0;
			}
			if (key == GLFW_KEY_D && action == GLFW_PRESS) {
				mycam.d = 1;
			}
			if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
				mycam.d = 0;
			}
		}
	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{ }

    void init_screen_texture_fbo()
    {
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

        glBindTexture(GL_TEXTURE_2D, FBOtex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //NULL means reserve texture memory, but texels are undefined
        //**** Tell OpenGL to reserve level 0
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        //You must reserve memory for other mipmaps levels as well either by making a series of calls to
        //glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
        //Here, we'll use :
        glGenerateMipmap(GL_TEXTURE_2D);
        //-------------------------
        glGenFramebuffers(1, &fb);
        glBindFramebuffer(GL_FRAMEBUFFER, fb);
        //Attach 2D texture to this FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtex, 0);
        //-------------------------
        glGenRenderbuffers(1, &depth_rb);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        //-------------------------
        //Attach depth buffer to FBO
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
        //-------------------------
        //Does the GPU support current FBO configuration?
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        switch (status)
        {
            case GL_FRAMEBUFFER_COMPLETE:
                cout << "status framebuffer: good" << std::endl;
                break;
            default:
                cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		init_screen_texture_fbo();
	}

    void init(const std::string& resourceDirectory)
    {
//	    stbi_set_flip_vertically_on_load(true);
        GLSL::checkVersion();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
       // glEnable(GL_BLEND);
       // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        prog2 = make_shared<Program>();
        prog2->setVerbose(true);
        prog2->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_nolight.glsl");
        if (!prog2->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog2->init();
        prog2->addUniform("P");
        prog2->addUniform("V");
        prog2->addUniform("M");
        prog2->addAttribute("vertPos");
        prog2->addAttribute("vertTex");

        progLambo = std::make_shared<Program>();
        progLambo->setVerbose(true);
        progLambo->setShaderNames(resourceDirectory + "/shader_vertex_lambo.glsl", resourceDirectory + "/shader_fragment_lambo.glsl");
        progLambo->init();
        progLambo->addUniform("P");
        progLambo->addUniform("V");
        progLambo->addUniform("M");
        progLambo->addUniform("isBraking");
        progLambo->addUniform("campos");
        progLambo->addAttribute("vertPos");
        progLambo->addAttribute("vertTex");
        progLambo->addAttribute("vertNor");


        progLamp = std::make_shared<Program>();
        progLamp->setVerbose(true);
        progLamp->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
        progLamp->init();
        progLamp->addUniform("P");
        progLamp->addUniform("V");
        progLamp->addUniform("M");
        progLamp->addUniform("Color");
        progLamp->addAttribute("vertPos");
        progLamp->addUniform("lightSpace");
        progLamp->addUniform("campos");
        progLamp->addUniform("lightpos");
        progLamp->addUniform("lightdir");

//
//        progCityGround = std::make_shared<Program>();
//        progCityGround->setVerbose(true);
//        progCityGround->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
//        if (!progCityGround->init())
//        {
//            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
//            exit(1);
//        }
//        progCityGround->addUniform("P");
//        progCityGround->addUniform("V");
//        progCityGround->addUniform("M");
//        progCityGround->addAttribute("vertPos");
//        progCityGround->addAttribute("vertCol");



        progCityBuilding = std::make_shared<Program>();
        progCityBuilding->setVerbose(true);
        progCityBuilding->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
        if (!progCityBuilding->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        progCityBuilding->addUniform("P");
        progCityBuilding->addUniform("V");
        progCityBuilding->addUniform("M");
        progCityBuilding->addUniform("lightSpace");
        progCityBuilding->addAttribute("vertPos");
        progCityBuilding->addAttribute("vertCol");
		progCityBuilding->addAttribute("vertTex");
        progCityBuilding->addUniform("lamps");
        progCityBuilding->addUniform("isGround");
        progCityBuilding->addUniform("isLamp");


		// Initialize the Shadow Map shader program.
		shadowProg = make_shared<Program>();
		shadowProg->setVerbose(true);
		shadowProg->setShaderNames(resourceDirectory + "/shadow_vert.glsl", resourceDirectory + "/shadow_frag.glsl");
		if (!shadowProg->init())
			{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
			}
		shadowProg->init();
		shadowProg->addUniform("P");
		shadowProg->addUniform("V");
		shadowProg->addUniform("M");
		shadowProg->addAttribute("vertPos");
    }

	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{
		string resourceDirectory = "../resources" ;

        //init rectangle mesh (2 triangles) for the post processing
        glGenVertexArrays(1, &VertexArrayIDBox);
        glBindVertexArray(VertexArrayIDBox);

        //generate vertex buffer to hand off to OGL
        glGenBuffers(1, &VertexBufferIDBox);
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);
        GLfloat *rectangle_positions = new GLfloat[18];
        int verccount = 0;
        rectangle_positions[verccount++] = -1.0, rectangle_positions[verccount++] = -1.0, rectangle_positions[verccount++] = 0.0;
        rectangle_positions[verccount++] = 1.0, rectangle_positions[verccount++] = -1.0, rectangle_positions[verccount++] = 0.0;
        rectangle_positions[verccount++] = -1.0, rectangle_positions[verccount++] = 1.0, rectangle_positions[verccount++] = 0.0;
        rectangle_positions[verccount++] = 1.0, rectangle_positions[verccount++] = -1.0, rectangle_positions[verccount++] = 0.0;
        rectangle_positions[verccount++] = 1.0, rectangle_positions[verccount++] = 1.0, rectangle_positions[verccount++] = 0.0;
        rectangle_positions[verccount++] = -1.0, rectangle_positions[verccount++] = 1.0, rectangle_positions[verccount++] = 0.0;
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_positions, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        //normals
        glGenBuffers(1, &VertexBufferIDNorm);
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDNorm);
        GLfloat *rectangle_normals = new GLfloat[18];
        verccount = 0;
        rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 1.0;
        rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 1.0;
        rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 1.0;
        rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 1.0;
        rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 1.0;
        rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 0.0, rectangle_normals[verccount++] = 1.0;
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_normals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        //texture coords
        glGenBuffers(1, &VertexBufferTex);
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);
        float t = 1. / 100.;
        GLfloat *rectangle_texture_coords = new GLfloat[12];
        int texccount = 0;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
        glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        progLambo->bind();
        shape = make_shared<Shape>();
        shape->loadMesh(resourceDirectory + "/lambo.obj");
        shape->resize();
        shape->init();
        progLambo->unbind();

        int width, height, channels;
        char filepath[1000];

        //texture 1
        string str = resourceDirectory + "/lambo_diffuse.jpg";
        strcpy(filepath, str.c_str());
        unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &Texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

		//texture 1
		str = resourceDirectory + "/headmask.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &mask);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mask);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//texture 1
		str = resourceDirectory + "/daycity.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &citytex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, citytex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
        //texture 2
        str = resourceDirectory + "/lambo_spec.jpg";
        strcpy(filepath, str.c_str());
        data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &Texture2);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Texture2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        // another one
        str = resourceDirectory + "/street.jpg";
        strcpy(filepath, str.c_str());
        data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &streetTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, streetTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        //[TWOTEXTURES]
        GLuint Tex1Location = glGetUniformLocation(progLambo->pid, "tex");
        GLuint Tex2Location = glGetUniformLocation(progLambo->pid, "tex2");
        glUseProgram(progLambo->pid);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);

        glUseProgram(progCityBuilding->pid);
        Tex1Location = glGetUniformLocation(progCityBuilding->pid, "shadowMapTex");
        glUniform1i(Tex1Location, 2);
		GLuint texloc = glGetUniformLocation(progCityBuilding->pid, "tex");
		glUniform1i(texloc, 0);
		GLuint texloc3 = glGetUniformLocation(progCityBuilding->pid, "mask");
		glUniform1i(texloc3, 1);
        GLuint texloc4 = glGetUniformLocation(progCityBuilding->pid, "streetTex");
        glUniform1i(texloc4, 3);

		// cube
        glGenVertexArrays(1, &VAOBox);
        glBindVertexArray(VAOBox);
        glGenBuffers(1, &VBOBoxPos);
        glBindBuffer(GL_ARRAY_BUFFER, VBOBoxPos);
        GLfloat cube_vertices[] = {
                // front
                -1.0, -1.0,  1.0,
                1.0, -1.0,  1.0,
                1.0,  1.0,  1.0,
                -1.0,  1.0,  1.0,
                // back
                -1.0, -1.0, -1.0,
                1.0, -1.0, -1.0,
                1.0,  1.0, -1.0,
                -1.0,  1.0, -1.0,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
        glGenBuffers(1, &VBOBoxColor);
        glBindBuffer(GL_ARRAY_BUFFER, VBOBoxColor);
        GLfloat cube_colors[] = {
                // front
                0.5, 0.5, 0.5,
                0.2, 0.2, 0.2,
                0.1, 0.1, 0.1,
                0.0, 0.0, 0.0,
                // back
                0.5, 0.5, 0.5,
                0.2, 0.2, 0.2,
                0.1, 0.1, 0.1,
                0.0, 0.0, 0.0,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_colors), cube_colors, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
		//tex:
		glGenBuffers(1, &VBOBoxColor);
		glBindBuffer(GL_ARRAY_BUFFER, VBOBoxColor);
		GLfloat cube_tex[] = {
			// front
			0.0, 1.0,
			1.0, 1.0,
			1.0, 0.0, 
			0.0, 0.0, 
			// back
			1.0, 1.0,
			0.0, 1.0,
			0.0, 0.0,
			1.0, 0.0,
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//
        glGenBuffers(1, &VBOBoxIndex);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOBoxIndex);
        GLushort cube_elements[] = {
                // front
                0, 1, 2,
                2, 3, 0,
                // top
                1, 5, 6,
                6, 2, 1,
                // back
                7, 6, 5,
                5, 4, 7,
                // bottom
                4, 0, 3,
                3, 7, 4,
                // left
                4, 5, 1,
                1, 0, 4,
                // right
                3, 2, 6,
                6, 7, 3,
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);
        glBindVertexArray(0);


        progLamp->bind();
        shapeLamp = make_shared<Shape>();
        shapeLamp->loadMesh(resourceDirectory + "/streetLamp.obj");
        shapeLamp->resize();
        shapeLamp->init();
        progLamp->unbind();

        int rowColNum = 20;

		gameStats.top = 16;
		gameStats.left = 0;
		gameStats.right = rowColNum*2;
		gameStats.bottom = 16 + -rowColNum*2;
		gameStats.playing = true;
		gameStats.size = rowColNum;
		gameStats.starting = rowColNum*((rowColNum/2)-1);
		gameStats.stuck = false;
		gameStats.carSpeed = 0.0;

        // populate buildings
        for (int x = 0; x < rowColNum; x++) {
            for (int z = 0; z < rowColNum; z++) {
                int myRand = rand() % 30 + 1;
                vec3 buildingPos = vec3(x*2, (float) myRand / 13, 16 + -z*2);
                Building temp;
                temp.building = buildingPos;
                temp.isVisible = true;
                allBuildings.push_back(temp);
            }
        }

        stack<int> maze;
        vector<Building> saved = allBuildings;
        while (maze.size() < rowColNum*3) {
            allBuildings = saved;
            maze = getMaze(rowColNum);
        }
        gameStats.ending = maze.top();

        // Attach Shadow Map depth texture to Shadow Map FBO
        {
            glGenFramebuffers(1, &fb_shadowMap);
            glBindFramebuffer(GL_FRAMEBUFFER, fb_shadowMap);

            glGenTextures(1, &FBOtex_shadowMapDepth);
            //glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_DIM, SHADOW_DIM, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(vec3(1.0)));

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, FBOtex_shadowMapDepth, 0);

            // We don't want the draw result for a shadow map!
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            GLenum status;
            status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            switch (status)
            {
                case GL_FRAMEBUFFER_COMPLETE:
                    cout << "status framebuffer: good" << std::endl;
                    break;
                default:
                    cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }


        glUseProgram(shadowProg->pid);
        GLuint ShadowTexLocation = glGetUniformLocation(shadowProg->pid, "shadowMapTex");
        glUniform1i(ShadowTexLocation, 0);
		

        //RGBA8 2D texture, 24 bit depth texture, 256x256
        glGenTextures(1, &FBOtex);
        init_screen_texture_fbo();

	}

    void render_to_screen()
    {

        double frametime = get_last_elapsed_time();

        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);

        auto P = std::make_shared<MatrixStack>();
        P->pushMatrix();
        P->perspective(70., width, height, 0.1, 100.0f);
        glm::mat4 M,V,S,T;

        V = glm::mat4(1);

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        prog2->bind();
        glActiveTexture(GL_TEXTURE0);

        // Debug, shows shadow map when 'y' is pressed
        show_shadowmap ? glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth) : glBindTexture(GL_TEXTURE_2D, FBOtex);

        M = scale(glm::mat4(1), vec3(1.2,1,1)) * translate(glm::mat4(1), vec3(-0.5, -0.5, -1));
        glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
        glUniformMatrix4fv(prog2->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(prog2->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        prog2->unbind();

    }

    void get_light_proj_matrix(glm::mat4& lightP)
    {
        // If your scene goes outside these "bounds" (e.g. shadows stop working near boundary),
        // feel free to increase these numbers (or decrease if scene shrinks/light gets closer to
        // scene objects).
        const float left = -15.0f;
        const float right = 15.0f;
        const float bottom = -15.0f;
        const float top = 15.0f;
        const float zNear = 0.1f;
        const float zFar = 50.0f;

        lightP = perspective((float)(3.14159 / 4.), (float)((float)960/ (float)540), 0.1f, 20.0f);
    }

    void get_light_view_matrix(glm::mat4& lightV)
    {
	//	glm::mat4 R = glm::rotate(glm::mat4(1), mycam.rot.y, glm::vec3(0, 1, 0));
        // Change earth_pos (or primaryLight.direction) to change where the light is pointing at.
       // lightV = glm::lookAt(mycam.pos + vec3(0, 0, 0), mycam.pos + vec3(0,0,-1), glm::vec3(0.0f, 1.0f, 0.0f))*R;

		mat4 R = rotate(glm::mat4(1), mycam.rot.y, vec3(0, 1, 0));
		mat4 T = translate(glm::mat4(1), mycam.pos + vec3(0,14.75,0));
		lightV = R  * T;
    }

    void render_to_texture()
    {
        glBindFramebuffer(GL_FRAMEBUFFER,fb );
        glClearColor(0.0, 0.0, 0.0, 0.0);


        double frametime = get_last_elapsed_time();
        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width/(float)height;
        glViewport(0, 0, width, height);

      

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create the matrix stacks - please leave these alone for now

        glm::mat4 V, M, P, S, T, R, T1, R2, R3, T2, T3, R4;
        V = glm::mat4(1);
        M = glm::mat4(1);
        P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 20.0f); //so much type casting... GLM metods are quite funny ones

        V = mycam.process(frametime);

        /******************************* LAMP ******************************/
//        progLamp->bind();
//        vec3 lampPos = vec3(-2.0, 0.12, -1.0);
//        vec3 entranceColor = vec3(1.0, 0.0, 0.0);
//        vec3 exitColor = vec3(0.0, 1.0, 0.0);
//        glUniformMatrix4fv(progLamp->getUniform("P"), 1, GL_FALSE, &P[0][0]);
//        glUniformMatrix4fv(progLamp->getUniform("V"), 1, GL_FALSE, &V[0][0]);
//        glUniform3fv(progLamp->getUniform("Color"), 1, &entranceColor[0]);
//        glUniformMatrix4fv(progLamp->getUniform("lightSpace"), 1, GL_FALSE, &lightSpace[0][0]);
//        glUniform3fv(progLamp->getUniform("campos"), 1, &mycam.pos.x);
//        glUniform3fv(progLamp->getUniform("lightpos"), 1, &primaryLight.position.x);
//        glUniform3fv(progLamp->getUniform("lightdir"), 1, &primaryLight.direction.x);
//        glActiveTexture(GL_TEXTURE1);
//        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
//        // entrance lamps
//        S = scale(mat4(1.0f), vec3(1.5, 1.5, 1.5));
//        T = translate(mat4(1.0f), vec3(17, 0, 17.5));
//        R = rotate(mat4(1.0f), (float) 3.14/2, vec3(0.0f, 1.0f, 0.0f));
//        M = T * S * R;
//        glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        shapeLamp->draw(progLamp, false);
//        T = translate(mat4(1.0f), vec3(19, 0, 17.5));
//        M = T * S * R;
//        glUniform3fv(progLamp->getUniform("Color"), 1, &entranceColor[0]);
//        glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        shapeLamp->draw(progLamp, false);
//        // exit lamps
//        if (getLeftIndex(gameStats.size, exitIndex) < 0) {
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z + 1));
//            M = T * S;
//            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
//            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(progLamp, false);
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z - 1));
//            M = T * S;
//            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
//            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(progLamp, false);
//        } else if (getRightIndex(gameStats.size, exitIndex) < 0) {
//            R = rotate(mat4(1.0f), (float) -3.14, vec3(0.0f, 1.0f, 0.0f));
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z + 1));
//            M = T * S * R;
//            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
//            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(progLamp, false);
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z - 1));
//            M = T * S * R;
//            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
//            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(progLamp, false);
//        } else if (getUpIndex(gameStats.size, exitIndex) < 0) {
//            R = rotate(mat4(1.0f), (float) -3.14/2, vec3(0.0f, 1.0f, 0.0f));
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 1, 0, exitBuilding.building.z - 2));
//            M = T * S * R;
//            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
//            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(progLamp, false);
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 1, 0, exitBuilding.building.z - 2));
//            M = T * S * R;
//            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
//            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(progLamp, false);
//        }
//        progLamp->unbind();

        /************************ City Ground ********************/
//        progCityGround->bind();
//        glUniformMatrix4fv(progCityGround->getUniform("P"), 1, GL_FALSE, &P[0][0]);
//        glUniformMatrix4fv(progCityGround->getUniform("V"), 1, GL_FALSE, &V[0][0]);
//        glUniformMatrix4fv(progCityGround->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        glBindVertexArray(VAOBox);
//        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOBoxIndex);
//        S = scale(mat4(1.0f), vec3(20.0f, 1.0f, 20.0f));
//        T = translate(mat4(1.0f), vec3(0.0, GROUND_OFFSET, 0.0));
//        M = T * S;
//        glUniformMatrix4fv(progCityGround->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
//        progCityGround->unbind();

        /************************ Lambo ********************/
        progLambo->bind();
        glUniformMatrix4fv(progLambo->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(progLambo->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(progLambo->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        S = scale(mat4(1.0f), vec3(0.9f, 0.9f, 0.9f));
        T = translate(mat4(1.0f), vec3(0, CAR_OFFSET, 0.0) - mycam.pos);
        R = rotate(mat4(1.0f), (float) 3.14, vec3(0.0f, 1.0f, 0.0f));
        R2 = rotate(mat4(1.0f), -mycam.rot.y, vec3(0.0, 1.0, 0.0));
        mat4 temp = T * R2 * R * S;

        vec3 offsets[4] = {vec3(0.1, 0.0, 0.4), vec3(-0.1, 0.0, 0.4), vec3(0.1, 0.0, -0.4), vec3(-0.1, 0.0, -0.4)};

        gameStats.stuck = false;
        for (int i = 0; i < 4; i++) {
            mat4 offset = translate(temp, offsets[i]);
            int x = round(offset[3][0]/2);
            int y = 19 - (round(offset[3][2]/2) + 11);
            if (allBuildings[y + x*20].isVisible && (y + x*20+1) != gameStats.starting && (y + x*20-1) != gameStats.ending) {
                gameStats.stuck = true;
                gameStats.carSpeed = 0;
            }
        }
        if (gameStats.stuck) {
            M = oldM;
            mycam.pos = oldCamPos;
            mycam.rot = oldCamRot;
        } else {
            M = temp;
            oldM = temp;
            oldCamPos = mycam.pos;
            oldCamRot = mycam.rot;
        }
        glUniform1f(progLambo->getUniform("isBraking"), gameStats.isBraking);
        glUniformMatrix4fv(progLambo->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniform3fv(progLambo->getUniform("campos"), 1, &mycam.pos[0]);
        shape->draw(progLambo, false);
        progLambo->unbind();

        /************************ City Building ********************/
        progCityBuilding->bind();
        glUniformMatrix4fv(progCityBuilding->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(progCityBuilding->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glBindVertexArray(VAOBox);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOBoxIndex);

		glUniformMatrix4fv(progCityBuilding->getUniform("lightSpace"), 1, GL_FALSE, &lightSpace[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, citytex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mask);
		glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, streetTex);
        glUniform1f(progCityBuilding->getUniform("isGround"), 0.0);
        glUniform1f(progCityBuilding->getUniform("isLamp"), 0.0);
        for (int i = 0; i < allBuildings.size(); i++) {
            if (!allBuildings[i].isVisible) {
                continue;
            }
            S = scale(mat4(1.0f), vec3(1.0f, 2+allBuildings[i].building.y, 1.0f));
            T = translate(mat4(1.0f), allBuildings[i].building);
            M = T * S;
            glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
        }
        /******************************* GROUND **************************/
        glUniform1f(progCityBuilding->getUniform("isGround"), 1.0);
        glUniform1f(progCityBuilding->getUniform("isLamp"), 0.0);
        S = scale(mat4(1.0f), vec3(22.0f, 1.0f, 22.0f));
        T = translate(mat4(1.0f), vec3(TRANSFORM_FLOOR, GROUND_OFFSET, -4.0));
        M = T * S;
        glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);

        /*************************** LAMP ********************************/
        vector<vec3> lamps;
        vec3 entranceColor = vec3(1.0, 0.0, 0.0);
        vec3 exitColor = vec3(0.0, 1.0, 0.0);
        glUniformMatrix4fv(progCityBuilding->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(progCityBuilding->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniform3fv(progCityBuilding->getUniform("Color"), 1, &entranceColor[0]);
        glUniformMatrix4fv(progCityBuilding->getUniform("lightSpace"), 1, GL_FALSE, &lightSpace[0][0]);
        glUniform3fv(progCityBuilding->getUniform("campos"), 1, &mycam.pos.x);
        glUniform3fv(progCityBuilding->getUniform("lightpos"), 1, &primaryLight.position.x);
        glUniform3fv(progCityBuilding->getUniform("lightdir"), 1, &primaryLight.direction.x);
        glUniform3fv(progCityBuilding->getUniform("lamps"), lamps.size(), reinterpret_cast<GLfloat *>(lamps.data()));
        glUniform1f(progCityBuilding->getUniform("isGround"), 0.0);
        glUniform1f(progCityBuilding->getUniform("isLamp"), 1.0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
        // entrance lamps
        S = scale(mat4(1.0f), vec3(1.5, 1.5, 1.5));
        T = translate(mat4(1.0f), vec3(17, 0, 17.5));
        R = rotate(mat4(1.0f), (float) 3.14/2, vec3(0.0f, 1.0f, 0.0f));
        M = T * S * R;
        vec3 lampPos = vec3(M[3][0], 1.3, M[3][2]);
        cout << "lampPos1: " << lampPos.x << " " << lampPos.y << " " << lampPos.z << endl;
        lamps.push_back(lampPos);
        glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniform3fv(progCityBuilding->getUniform("lamps"), lamps.size(), reinterpret_cast<GLfloat *>(lamps.data()));
        shapeLamp->draw(progCityBuilding, false);
        T = translate(mat4(1.0f), vec3(19, 0, 17.5));
        M = T * S * R;
        lampPos = vec3(M[3][0], 1.3, M[3][2]);
        cout << "lampPos2: " << lampPos.x << " " << lampPos.y << " " << lampPos.z << endl;
        lamps.push_back(lampPos);
        glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniform3fv(progCityBuilding->getUniform("lamps"), lamps.size(), reinterpret_cast<GLfloat *>(lamps.data()));
        shapeLamp->draw(progCityBuilding, false);
        // exit lamps
        if (getLeftIndex(gameStats.size, exitIndex) < 0) {
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z + 1));
            M = T * S;
            glUniform3fv(progCityBuilding->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progCityBuilding, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z - 1));
            M = T * S;
            glUniform3fv(progCityBuilding->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progCityBuilding, false);
        } else if (getRightIndex(gameStats.size, exitIndex) < 0) {
            R = rotate(mat4(1.0f), (float) -3.14, vec3(0.0f, 1.0f, 0.0f));
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z + 1));
            M = T * S * R;
            glUniform3fv(progCityBuilding->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progCityBuilding, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z - 1));
            M = T * S * R;
            glUniform3fv(progCityBuilding->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progCityBuilding, false);
        } else if (getUpIndex(gameStats.size, exitIndex) < 0) {
            R = rotate(mat4(1.0f), (float) -3.14/2, vec3(0.0f, 1.0f, 0.0f));
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 1, 0, exitBuilding.building.z - 2));
            M = T * S * R;
            glUniform3fv(progCityBuilding->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progCityBuilding, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 1, 0, exitBuilding.building.z - 2));
            M = T * S * R;
            glUniform3fv(progCityBuilding->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progCityBuilding->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progCityBuilding, false);
        }


        progCityBuilding->unbind();
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, FBOtex);
        glGenerateMipmap(GL_TEXTURE_2D);

    }

    void render_to_shadowmap()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fb_shadowMap);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, SHADOW_DIM, SHADOW_DIM);

        glDisable(GL_BLEND);

        glm::mat4 M, V, S, T, P;

        // Orthographic frustum in light space; encloses the scene, adjust if larger or smaller scene used.
        get_light_proj_matrix(P);

        // "Camera" for rendering shadow map is at light source, looking at the scene.
        get_light_view_matrix(V);
        T = translate(mat4(1.0f), vec3(0,0,0));

        // Bind shadow map shader program and matrix uniforms.
        shadowProg->bind();
        glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(shadowProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);




        double frametime = get_last_elapsed_time();
        // Get current frame buffer size.
        int width, height;
       

        mat4 lightP, lightV;
        get_light_proj_matrix(lightP);
        get_light_view_matrix(lightV);
        lightSpace = P * V;

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create the matrix stacks - please leave these alone for now

        glm::mat4 R, T1, R2, R3, T2, T3, R4;
        M = glm::mat4(1);

//        /******************************* LAMP ******************************/
//        vec3 lampPos = vec3(-2.0, 0.12, -1.0);
//        vec3 entranceColor = vec3(1.0, 0.0, 0.0);
//        vec3 exitColor = vec3(0.0, 1.0, 0.0);
//        glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
//        glUniformMatrix4fv(shadowProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
//        glActiveTexture(GL_TEXTURE1);
//        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
//        // entrance lamps
//        S = scale(mat4(1.0f), vec3(1.5, 1.5, 1.5));
//        T = translate(mat4(1.0f), vec3(17, 0, 17.5));
//        R = rotate(mat4(1.0f), (float) 3.14/2, vec3(0.0f, 1.0f, 0.0f));
//        M = T * S * R;
//        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        shapeLamp->draw(shadowProg, false);
//        T = translate(mat4(1.0f), vec3(19, 0, 17.5));
//        M = T * S * R;
//        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        shapeLamp->draw(shadowProg, false);
//        // exit lamps
//        if (getLeftIndex(gameStats.size, exitIndex) < 0) {
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z + 1));
//            M = T * S;
//            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(shadowProg, false);
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z - 1));
//            M = T * S;
//            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(shadowProg, false);
//        } else if (getRightIndex(gameStats.size, exitIndex) < 0) {
//            R = rotate(mat4(1.0f), (float) -3.14, vec3(0.0f, 1.0f, 0.0f));
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z + 1));
//            M = T * S * R;
//            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(shadowProg, false);
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z - 1));
//            M = T * S * R;
//            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(shadowProg, false);
//        } else if (getUpIndex(gameStats.size, exitIndex) < 0) {
//            R = rotate(mat4(1.0f), (float) -3.14/2, vec3(0.0f, 1.0f, 0.0f));
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 1, 0, exitBuilding.building.z - 2));
//            M = T * S * R;
//            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(shadowProg, false);
//            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 1, 0, exitBuilding.building.z - 2));
//            M = T * S * R;
//            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//            shapeLamp->draw(shadowProg, false);
//        }

        /************************ Lambo ********************/
        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        S = scale(mat4(1.0f), vec3(0.9f, 0.9f, 0.9f));
        T = translate(mat4(1.0f), vec3(0, CAR_OFFSET, 0.0) - mycam.pos);
        R = rotate(mat4(1.0f), (float) 3.14, vec3(0.0f, 1.0f, 0.0f));
        R2 = rotate(mat4(1.0f), -mycam.rot.y, vec3(0.0, 1.0, 0.0));
        mat4 temp = T * R2 * R * S;

        vec3 offsets[4] = {vec3(0.1, 0.0, 0.4), vec3(-0.1, 0.0, 0.4), vec3(0.1, 0.0, -0.4), vec3(-0.1, 0.0, -0.4)};

        gameStats.stuck = false;
        for (int i = 0; i < 4; i++) {
            mat4 offset = translate(temp, offsets[i]);
            int x = round(offset[3][0]/2);
            int y = 19 - (round(offset[3][2]/2) + 11);
            if (allBuildings[y + x*20].isVisible && (y + x*20+1) != gameStats.starting && (y + x*20-1) != gameStats.ending) {
                gameStats.stuck = true;
                gameStats.carSpeed = 0;
            }
        }
        if (gameStats.stuck) {
            M = oldM;
            mycam.pos = oldCamPos;
            mycam.rot = oldCamRot;
        } else {
            M = temp;
            oldM = temp;
            oldCamPos = mycam.pos;
            oldCamRot = mycam.rot;
        }

        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        shape->draw(shadowProg, false);

        /************************ City Building ********************/
        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glBindVertexArray(VAOBox);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOBoxIndex);
        for (int i = 0; i < allBuildings.size(); i++) {
            if (!allBuildings[i].isVisible) {
                continue;
            }
            S = scale(mat4(1.0f), vec3(1.0f, 2+allBuildings[i].building.y, 1.0f));
            T = translate(mat4(1.0f), allBuildings[i].building);
            M = T * S;
            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
        }

        /************************ City Ground ********************/
        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glBindVertexArray(VAOBox);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOBoxIndex);
        S = scale(mat4(1.0f), vec3(22.0f, 1.0f, 22.0f));
        T = translate(mat4(1.0f), vec3(TRANSFORM_FLOOR, GROUND_OFFSET, -4.0));
        M = T * S;
        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);

        /*************************** LAMP ********************************/
        vec3 lampPos = vec3(-2.0, 0.12, -1.0);
        vec3 entranceColor = vec3(1.0, 0.0, 0.0);
        vec3 exitColor = vec3(0.0, 1.0, 0.0);
        glUniform3fv(shadowProg->getUniform("Color"), 1, &entranceColor[0]);
        glUniformMatrix4fv(shadowProg->getUniform("lightSpace"), 1, GL_FALSE, &lightSpace[0][0]);
        glUniform3fv(shadowProg->getUniform("campos"), 1, &mycam.pos.x);
        glUniform3fv(shadowProg->getUniform("lightpos"), 1, &primaryLight.position.x);
        glUniform3fv(shadowProg->getUniform("lightdir"), 1, &primaryLight.direction.x);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
        // entrance lamps
        S = scale(mat4(1.0f), vec3(1.5, 1.5, 1.5));
        T = translate(mat4(1.0f), vec3(17, 0, 17.5));
        R = rotate(mat4(1.0f), (float) 3.14/2, vec3(0.0f, 1.0f, 0.0f));
        M = T * S * R;
        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        shapeLamp->draw(shadowProg, false);
        T = translate(mat4(1.0f), vec3(19, 0, 17.5));
        M = T * S * R;
        glUniform3fv(shadowProg->getUniform("Color"), 1, &entranceColor[0]);
        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        shapeLamp->draw(shadowProg, false);
        // exit lamps
        if (getLeftIndex(gameStats.size, exitIndex) < 0) {
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z + 1));
            M = T * S;
            glUniform3fv(shadowProg->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(shadowProg, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z - 1));
            M = T * S;
            glUniform3fv(shadowProg->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(shadowProg, false);
        } else if (getRightIndex(gameStats.size, exitIndex) < 0) {
            R = rotate(mat4(1.0f), (float) -3.14, vec3(0.0f, 1.0f, 0.0f));
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z + 1));
            M = T * S * R;
            glUniform3fv(shadowProg->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(shadowProg, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z - 1));
            M = T * S * R;
            glUniform3fv(shadowProg->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(shadowProg, false);
        } else if (getUpIndex(gameStats.size, exitIndex) < 0) {
            R = rotate(mat4(1.0f), (float) -3.14/2, vec3(0.0f, 1.0f, 0.0f));
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 1, 0, exitBuilding.building.z - 2));
            M = T * S * R;
            glUniform3fv(shadowProg->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(shadowProg, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 1, 0, exitBuilding.building.z - 2));
            M = T * S * R;
            glUniform3fv(shadowProg->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(shadowProg, false);
        }

        glBindVertexArray(0);

        //done, unbind stuff
        shadowProg->unbind();
        glEnable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

	stack<int> getMaze(int size) {

        srand(time(NULL));

        stack <int> path;

        int currentCell = size*((size/2)-1);
        path.push(currentCell);
        for (int i = 0; i < 4; i++) {
            allBuildings[currentCell].isVisible = false;
            currentCell = getUpIndex(size, currentCell);
            path.push(currentCell);
        }
        allBuildings[currentCell].isVisible = false;
        path.push(currentCell);

        while(getUpIndex(size, currentCell) >= 0 && getLeftIndex(size, currentCell) >= 0 && getRightIndex(size, currentCell) >= 0) {
            int temp = getRandomNeighbor(size, currentCell);
            if (temp > -1 && getNumSurroundingCells(size, temp) > 1 && getDownIndex(size, temp) > -1) {
                currentCell = temp;
                exitBuilding = allBuildings[currentCell];
                exitIndex = currentCell;
                allBuildings[currentCell].isVisible = false;
                path.push(currentCell);
            } else {
                if (!path.empty()) {
                    currentCell = path.top();
                    path.pop();
                } else {
                    break;
                }
            }
        }
        return path;
	}

	int getNumSurroundingCells(int size, int index) {
		int left = getLeftIndex(size, index);
		int right = getRightIndex(size, index);
		int down = getDownIndex(size, index);
		int up = getUpIndex(size, index);
		int count = 0;
		if (left > -1 && allBuildings[left].isVisible) {
			count++;
		}
		if (right > -1 && allBuildings[right].isVisible) {
			count++;
		}
		if (down > -1 && allBuildings[down].isVisible) {
			count++;
		}
		if (up > -1 && allBuildings[up].isVisible) {
			count++;
		}
		return count;
	}

	int getRandomNeighbor(int size, int index) {
		int myRand = rand() % 4;
		switch(myRand) {
			case 0:
				return getUpIndex(size, index);
			case 1:
				return getDownIndex(size, index);
			case 2:
				return getLeftIndex(size, index);
			case 3:
				return getRightIndex(size, index);
			default:
				return -1;
		}
	}

	int getLeftIndex(int size, int index) {
		if (index - size < 0) {
			return -1;
		}
		return index - size;
	}
	int getRightIndex(int size, int index) {
		if (index + size > size * size) {
			return -1;
		}
		return index + size;
	}
	int getDownIndex(int size, int index) {
		if (index % size == 0) {
			return -1;
		}
		return index - 1;
	}
	int getUpIndex(int size, int index) {
		if ((index+1) % size == 0) {
			return -1;
		}
		return index + 1;
	}

};
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(960, 540);
//	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render_to_shadowmap();
        application->render_to_texture();
        application->render_to_screen();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}

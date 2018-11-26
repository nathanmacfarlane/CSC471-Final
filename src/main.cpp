/*
ZJ Wood CPE 471 Lab 3 base code
*/

#include <iostream>
#include <glad/glad.h>

#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"

#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
shared_ptr<Shape> shape, shapeLamp;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct GameStats {
	int top;
	int bottom;
	int left;
	int right;
	bool playing;
	int size;
	bool canGoForward;
};

struct Building {
    vec3 building;
    bool isVisible;
};

GameStats gameStats;


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
		rot = glm::vec3(0, 0, 0);
        pos = glm::vec3(-18, -14, -17);
	}
	glm::mat4 process(double ftime)
	{
		float speed = 0;
		if (w == 1 && gameStats.canGoForward)
			speed = -6*ftime;
		else if (s == 1 && gameStats.canGoForward)
			speed = 6*ftime;

		float yangle=0;

		if (a == 1 && gameStats.canGoForward)
			yangle = -(M_PI/2)*ftime;
		else if(d==1 && gameStats.canGoForward)
			yangle = (M_PI/2)*ftime;

        double xangle = (M_PI/2);
        rot.x = xangle;
        mat4 RX = rotate(mat4(1), rot.x, vec3(1, 0, 0));


		rot.y += yangle;
		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 0, 1));
		glm::vec4 dir = glm::vec4(0, speed, 0,1);
		dir = dir * R * RX;
		pos += glm::vec3(dir.x, dir.y, dir.z);
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		return R * RX * T;
	}
};

camera mycam;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> progLamp, progCityGround, progCityBuilding, progLambo;

	GLuint VAOBox;
	GLuint VBOBoxPos, VBOBoxColor, VBOBoxIndex;

	//texture data
	GLuint Texture, Texture2;

    vector<Building> allBuildings;
    Building exitBuilding;
    int exitIndex;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{

		if (gameStats.playing) {
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

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}

	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{
//	    prog->bind();
		string resourceDirectory = "../resources" ;
//		shape = make_shared<Shape>();
//		shape->loadMesh(resourceDirectory + "/largeCity.obj");
//		shape->resize();
//		shape->init();
//		prog->unbind();

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

        //[TWOTEXTURES]
        GLuint Tex1Location = glGetUniformLocation(progLambo->pid, "tex");
        GLuint Tex2Location = glGetUniformLocation(progLambo->pid, "tex2");
        glUseProgram(progLambo->pid);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);

//        int width, height, channels;
//        char filepath[1000];
//        string str = resourceDirectory + "/cityTexture.jpg";
//        strcpy(filepath, str.c_str());
//        unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
//        glGenTextures(1, &Texture);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, Texture);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
//        glGenerateMipmap(GL_TEXTURE_2D);
//
//		GLuint Tex1Location = glGetUniformLocation(progCityGround->pid, "tex");
//		glUseProgram(progCityGround->pid);
//		glUniform1i(Tex1Location, 0);


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
		gameStats.canGoForward = true;

        // populate buildings
        for (int x = 0; x < rowColNum; x++) {
            for (int z = 0; z < rowColNum; z++) {
                int myRand = rand() % 100 + 1;
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

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
//
//		// Initialize the GLSL program.
//		prog = std::make_shared<Program>();
//		prog->setVerbose(true);
//		prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
//		prog->init();
//		prog->addUniform("P");
//		prog->addUniform("V");
//		prog->addUniform("M");
//		prog->addUniform("campos");
//		prog->addAttribute("vertPos");
//		prog->addAttribute("vertNor");

        progLambo = std::make_shared<Program>();
        progLambo->setVerbose(true);
        progLambo->setShaderNames(resourceDirectory + "/shader_vertex_lambo.glsl", resourceDirectory + "/shader_fragment_lambo.glsl");
        progLambo->init();
        progLambo->addUniform("P");
        progLambo->addUniform("V");
        progLambo->addUniform("M");
        progLambo->addUniform("campos");
        progLambo->addAttribute("vertPos");
        progLambo->addAttribute("vertTex");
        progLambo->addAttribute("vertNor");


        progCityGround = std::make_shared<Program>();
        progCityGround->setVerbose(true);
        progCityGround->setShaderNames(resourceDirectory + "/shader_vertex_ground.glsl", resourceDirectory + "/shader_fragment_ground.glsl");
        if (!progCityGround->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        progCityGround->addUniform("P");
        progCityGround->addUniform("V");
        progCityGround->addUniform("M");
        progCityGround->addAttribute("vertPos");
//		progCityGround->addAttribute("vertTex");
        progCityGround->addAttribute("vertCol");



        progCityBuilding = std::make_shared<Program>();
        progCityBuilding->setVerbose(true);
        progCityBuilding->setShaderNames(resourceDirectory + "/shader_vertex_cube.glsl", resourceDirectory + "/shader_fragment_cube.glsl");
        if (!progCityBuilding->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        progCityBuilding->addUniform("P");
        progCityBuilding->addUniform("V");
        progCityBuilding->addUniform("M");
        progCityBuilding->addAttribute("vertPos");
        progCityBuilding->addAttribute("vertCol");




        progLamp = std::make_shared<Program>();
        progLamp->setVerbose(true);
        progLamp->setShaderNames(resourceDirectory + "/shader_vertex_lamp.glsl", resourceDirectory + "/shader_fragment_lamp.glsl");
        progLamp->init();
        progLamp->addUniform("P");
        progLamp->addUniform("V");
        progLamp->addUniform("M");
        progLamp->addUniform("Color");
        progLamp->addAttribute("vertPos");
	}


	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{
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
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones

		V = mycam.process(frametime);
//		/******************************* CITY ******************************/
//		prog->bind();
//        vec3 temp = vec3(-mycam.pos.x, -mycam.pos.y, -mycam.pos.z);
//		//send the matrices to the shaders
//		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
//		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
//		glUniform3fv(prog->getUniform("campos"), 1, &temp[0]);
//		S = scale(mat4(1.0f), vec3(10.0f, 10.0f, 10.0f));
//		T = translate(mat4(1.0f), vec3(0.0, 1.7, -10.0));
//		M = T * S;
//		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//		shape->draw(prog);
//		prog->unbind();


        /******************************* LAMP ******************************/
        progLamp->bind();
        vec3 lampPos = vec3(-2.0, 0.12, -1.0);
        vec3 entranceColor = vec3(1.0, 0.0, 0.0);
        vec3 exitColor = vec3(0.0, 1.0, 0.0);
        glUniformMatrix4fv(progLamp->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(progLamp->getUniform("V"), 1, GL_FALSE, &V[0][0]);
//        glUniform3fv(progLamp->getUniform("lampPos"), 1, &lampPos[0]);
        glUniform3fv(progLamp->getUniform("Color"), 1, &entranceColor[0]);
        // entrance lamps
        S = scale(mat4(1.0f), vec3(1.5, 1.5, 1.5));
        T = translate(mat4(1.0f), vec3(17, 0, 17.5));
        R = rotate(mat4(1.0f), (float) 3.14/2, vec3(0.0f, 1.0f, 0.0f));
        M = T * S * R;
        glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        shapeLamp->draw(progLamp, false);
        T = translate(mat4(1.0f), vec3(19, 0, 17.5));
        M = T * S * R;
        glUniform3fv(progLamp->getUniform("Color"), 1, &entranceColor[0]);
        glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        shapeLamp->draw(progLamp, false);
        // exit lamps
        if (getLeftIndex(gameStats.size, exitIndex) < 0) {
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z + 1));
            M = T * S;
            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progLamp, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 2, 0, exitBuilding.building.z - 1));
            M = T * S;
            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progLamp, false);
        } else if (getRightIndex(gameStats.size, exitIndex) < 0) {
            R = rotate(mat4(1.0f), (float) -3.14, vec3(0.0f, 1.0f, 0.0f));
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z + 1));
            M = T * S * R;
            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progLamp, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 2, 0, exitBuilding.building.z - 1));
            M = T * S * R;
            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progLamp, false);
        } else if (getUpIndex(gameStats.size, exitIndex) < 0) {
            R = rotate(mat4(1.0f), (float) -3.14/2, vec3(0.0f, 1.0f, 0.0f));
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x + 1, 0, exitBuilding.building.z - 2));
            M = T * S * R;
            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progLamp, false);
            T = translate(mat4(1.0f), vec3(exitBuilding.building.x - 1, 0, exitBuilding.building.z - 2));
            M = T * S * R;
            glUniform3fv(progLamp->getUniform("Color"), 1, &exitColor[0]);
            glUniformMatrix4fv(progLamp->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shapeLamp->draw(progLamp, false);
        }
        progLamp->unbind();

//        /************************ City Ground ********************/
//        progCityGround->bind();
//        glUniformMatrix4fv(progCityGround->getUniform("P"), 1, GL_FALSE, &P[0][0]);
//        glUniformMatrix4fv(progCityGround->getUniform("V"), 1, GL_FALSE, &V[0][0]);
//        glUniformMatrix4fv(progCityGround->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        glBindVertexArray(VAOBox);
//        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOBoxIndex);
//        S = scale(mat4(1.0f), vec3(20.0f, 20.0f, 20.0f));
//        T = translate(mat4(1.0f), vec3(0.0, -22.0, 0.0));
//        M = T * S;
//        glUniformMatrix4fv(progCityGround->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
//        progCityGround->unbind();

		float rotateBuffer = abs(sin(mycam.rot.y/M_PI))/2;
		float rotateBufferlr = abs(cos(mycam.rot.y/M_PI))/2;

//		cout << "lr: " << rotateBufferlr << endl;

		int verticalRow = round(-mycam.pos.x + 0.6)/2;
		int topcol = round(mycam.pos.z + 17 - rotateBuffer)/2;
		int bottomcol = round(mycam.pos.z + 16.1 + rotateBuffer)/2;

		int leftrow = round(-mycam.pos.x + 0 - rotateBuffer)/2;
		int leftcol = round(mycam.pos.z + 17.5 + rotateBuffer)/2;

		int rightrow = round(-mycam.pos.x + 0.7 + rotateBufferlr)/2;
		int rightcol = round(mycam.pos.z + 17.5)/2;

		bool isStuck = false;

		if (allBuildings[topcol+verticalRow*20].isVisible) {
//			isStuck = true;
		}

		if (allBuildings[bottomcol+verticalRow*20].isVisible) {
//			isStuck = true;
		}

		if (allBuildings[rightcol+rightrow*20].isVisible) {
		    cout << "stuck on right" << endl;
//            isStuck = true;
		}

		if (allBuildings[leftcol+leftrow*20].isVisible) {
            cout << "stuck on left" << endl;
//            isStuck = true;
		}

//		cout << "left row: " << leftrow << " leftcol: " << leftcol << endl;

		gameStats.canGoForward = !isStuck;

		/************************ Lambo ********************/
		progLambo->bind();
		glUniformMatrix4fv(progLambo->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progLambo->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(progLambo->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		S = scale(mat4(1.0f), vec3(1, 1, 1));
		T = translate(mat4(1.0f), vec3(0, -40, 0) - mycam.pos);
		R = rotate(mat4(1.0f), (float) 3.14, vec3(0.0f, 1.0f, 0.0f));
		R2 = rotate(mat4(1.0f), -mycam.rot.y, vec3(0.0, 1.0, 0.0));
		M = T * S * R2 * R;
		mat4 test = M;
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
        progCityBuilding->unbind();

        glBindVertexArray(0);

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
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}

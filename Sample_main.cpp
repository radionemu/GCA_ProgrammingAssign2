
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>

#include <GL/glew.h>
#include <GL/glut.h>

#include <lodepng.h>

#include <general/resolution.hpp>

using namespace std;

//Global Variables
int WIDTH = 600; int HEIGHT = 600;
int reswidth, resheight;
//input vector
std::vector<glm::vec4> vertices;

//Buffers
GLuint programID;
GLuint Buffers[1];
GLuint VertexArrayID[2];
bool isCirlce = false;
GLuint gTextureID;
GLuint TextureSampler;

//Uniforms
GLuint tessID;
GLuint modeID;
GLuint threshID;

int tessLv = 10;
int mode = 0;
float thresh = 0.3f;

static void GLClearError(){
    while(glGetError() != GL_NO_ERROR);
}

static void GLCheckError(){
    while(GLenum error = glGetError()){
        std::cout << "[OpenGL Error] : " << error << std::endl;
    }
}

GLuint TextureBind(int width, int height, const void * buffer){
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    printf("Texture WIDTH : %d HEIGHT : %d\n", width, height);
    GLClearError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);//GL_RGBA!!!!!!!!!!!!
    GLCheckError();

    //check it later
    GLClearError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
	GLCheckError();

    GLClearError();
    glGenerateMipmap(GL_TEXTURE_2D);
    GLCheckError();

    return textureID;
}

GLuint LoadPNG(const char * PNGpath){
    //wait what
    std::vector<unsigned char> PNGimage;
    unsigned int width, height;
    unsigned int res = lodepng::decode(PNGimage, width, height, PNGpath);

    if(res != 0){
        printf("PNG open failed.\n");
        return 1;
    }

    printf("PNG file [%s] opened.\n", PNGpath);
    
    return TextureBind(width, height, &PNGimage[0]);
}

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path){
    //create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    GLint Result = GL_FALSE;
    int InfoLogLength;

    //Read the vertex shader code from the file
    string VertexShaderCode;
    ifstream VertexShaderStream(vertex_file_path, ios::in);
    if(!VertexShaderStream.is_open()){
        printf("Vertex Shader : [%s] Open failed\n", vertex_file_path);
    }
    if(VertexShaderStream.is_open())
    {
        printf("Vertex Shader : [%s] Opened\n", vertex_file_path);
        string Line = "";
        while(getline(VertexShaderStream, Line)){
            VertexShaderCode += "\n" + Line;
        }

        VertexShaderStream.close();
    }

    //Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const* VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    //Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if(InfoLogLength != 0){
        vector<char> VertexShaderErrorMessage(InfoLogLength);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);
    }

    //Read the fragment shader code from the file
    string FragmentShaderCode;
    ifstream FragmentShaderStream(fragment_file_path, ios::in);
    if(FragmentShaderStream.is_open())
    {
        printf("FragmentShader : [%s] Opened\n", fragment_file_path);
        string Line = "";
        while(getline(FragmentShaderStream, Line)){
            FragmentShaderCode += "\n" + Line;
        }
        FragmentShaderStream.close();
    }

    //Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const* FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    //Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if(InfoLogLength != 0){
        vector<char> FragmentShaderErrorMessage(InfoLogLength);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);
    }

    //Link the program
    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if(InfoLogLength != 0){
        vector<char> ProgramErrorMessage( std::max(InfoLogLength, int(1)) );
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);
    }

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}


void renderScene(void)
{
    glUniform1i(modeID, mode);
    glUniform1f(threshID, thresh);

	//Clear all pixels
	glClear(GL_COLOR_BUFFER_BIT);
	//bind and buffer data since data is added on callback function
	glBindVertexArray(VertexArrayID[0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureID);
	glUniform1i(TextureSampler, 0);

	//Draw Control Points
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	//Double buffer
	glutSwapBuffers();
	//glutPostRedisplay();
}

void MouseEvent(int button, int state, int x, int y){
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
        mode = (mode+1)%4;
    }else if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
        mode = (4+((mode-1)%4))%4;
    }
    if(state == GLUT_DOWN)
        printf("mode changed into [%d]\n", mode);
    glutPostRedisplay();
}

void KeyboardEvent(int key, int x, int y){
    if(key == GLUT_KEY_RIGHT){
        thresh += 0.1f;
    }else if(key == GLUT_KEY_LEFT){
        thresh -= 0.1f;
    }
    thresh = glm::clamp(thresh,0.0f,1.0f);
    printf("current Threshold : [%f]\n", thresh);
    glutPostRedisplay();
}

void init()
{
    //initilize the glew and check the errors.
    
    GLenum res = glewInit();
    if(res != GLEW_OK)
    {
        fprintf(stderr, "Error: '%s' \n", glewGetErrorString(res));
    }
	//select the background color
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

}


int main(int argc, char **argv)
{
	//init GLUT and create Window
	//initialize the GLUT
	glutInit(&argc, argv);
	//GLUT_DOUBLE enables double buffering (drawing to a background buffer while the other buffer is displayed)
#ifdef WINDOWS  // 윈도우즈에서 컴파일 할때는 아래를 포함
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
#else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
#endif
	//These two functions are used to define the position and size of the window. 

    GL_Resolution gl_resolution = GL_Resolution::getResInstance();
    gl_resolution.SetWindowResolution(WIDTH, HEIGHT);
    glm::vec2 dispRes = gl_resolution.GetDisplayResolution();
    glm::vec2 windRes = gl_resolution.GetWindowResolution();

	glutInitWindowPosition((dispRes.x-windRes.x)/2, (dispRes.y-windRes.y)/2);
	glutInitWindowSize(windRes.x, windRes.y);
	//This is used to define the name of the window.
	glutCreateWindow("Computer Graphics Advance : PA2");

	//call initization function
	init();

	//1.
	//Generate VAO
	glGenVertexArrays(2, VertexArrayID);

	//2.useProgramID 
	programID = LoadShaders("VertexShader.txt", "FragmentShader.txt");
	glUseProgram(programID);

	//posx posy uvx uvy
	vertices.push_back({-1.0,-1.0, 0.0, 0.0});
	vertices.push_back({-1.0,1.0, 0.0, 1.0});
	vertices.push_back({1.0,-1.0, 1.0, 0.0});
	vertices.push_back({-1.0,1.0, 0.0, 1.0});
	vertices.push_back({1.0,-1.0, 1.0, 0.0});
	vertices.push_back({1.0,1.0, 1.0, 1.0});

	//3.Generate VBO & Bind into VAO
	glBindVertexArray(VertexArrayID[0]);
	glGenBuffers(1, &Buffers[0]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec4), &vertices[0], GL_STATIC_DRAW);
	//Link Attrib
	GLuint vertexAttr = glGetAttribLocation(programID, "pos");
	GLuint UVAttr = glGetAttribLocation(programID, "vtxUV");
	GLuint TextureSampler = glGetUniformLocation(programID, "TextureSampler");
	glVertexAttribPointer(vertexAttr, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4,((void *)0));
	glEnableVertexAttribArray(vertexAttr);
	glVertexAttribPointer(UVAttr, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4,((void *)8));
	glEnableVertexAttribArray(UVAttr);
	//Link Uniform
    modeID = glGetUniformLocation(programID, "rendermode");
    threshID = glGetUniformLocation(programID, "threshold");

	//Texture Load
	gTextureID = LoadPNG("Lenna.png");

	//callback Function
	glutDisplayFunc(renderScene);
	glutMouseFunc(MouseEvent);
	glutSpecialFunc(KeyboardEvent);

	//enter GLUT event processing cycle
	glutMainLoop();

	glDeleteVertexArrays(1, VertexArrayID);
	
	return 1;
}


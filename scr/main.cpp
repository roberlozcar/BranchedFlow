#define _USE_MATH_DEFINES

#include "auxiliar.h"
#include "PLANE.h"

#include <gl/glew.h>
#define SOLVE_FGLUT_WARNING
#include <gl/freeglut.h> 

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>


//Variables de la ventana
int WIDTH =1024;
int HEIGHT = 1024;
int sizescreen = WIDTH * HEIGHT;

//////////////////////////////////////////////////////////////
// Variables que nos dan acceso a Objetos OpenGL
//////////////////////////////////////////////////////////////

//Shaders y program
unsigned int vshader; 
unsigned int fshader;
unsigned int program;

unsigned int cshader;
unsigned int program2;

unsigned int usize;

//VAO 
unsigned int vaoparticles;
unsigned int vaopotential;

//VBOs que forman parte del objeto 
unsigned int pospotVBO;
unsigned int potBuffer;
unsigned int posparticleVBO;
unsigned int velparticleBuffer;
unsigned int potcolorBuffer;
unsigned int pariclecolorBuffer;

int numparticles;
int wgs = 1024;
glm::ivec2 size= glm::ivec2(WIDTH, HEIGHT);
glm::vec2 sstart= glm::vec2(-1., -1.);
glm::vec2 send= glm::vec2(1.,1.);
glm::vec2 startaux(-1., -1.);
glm::vec2 endaux(1., 1.);
float gamma = 0.f;

unsigned int utrans;
glm::mat4 trans = glm::mat4(1.f);
float angle = 0.f;

//////////////////////////////////////////////////////////////
// Funciones auxiliares
//////////////////////////////////////////////////////////////

//Declaración de CB
void renderFunc();
void resizeFunc(int width, int height);
void idleFunc();
void keyboardFunc(unsigned char key, int x, int y);
void mouseFunc(int button, int state, int x, int y);
void mouseMotionFunc(int, int);

//Funciones de inicialización y destrucción
void initContext(int argc, char** argv);
void initOGL();
void initShader(const char *vname, const char *fname);
void initShader2(const char* name);
void initObj();
void initObj2();
void initObj2disk();
void destroy();

//Carga el shader indicado, devuele el ID del shader
//!Por implementar
GLuint loadShader(const char *fileName, GLenum type);

//Crea una textura, la configura, la sube a OpenGL, 
//y devuelve el identificador de la textura 
unsigned int loadTex(const char *fileName);

int main(int argc, char** argv)
{
	printf(" How many paths do you want to simulate?\n");
	scanf("%d", &numparticles);
	numparticles *= 2;

	//printf("\n Directional (0) or Radial (1) configuration?\n");
	char answer[20]="1";
	//scanf("%s", &answer);

	initContext(argc, argv);
	initOGL();
	initShader("../shaders/shader.v0.vert", "../shaders/shader.v0.frag");
	initShader2("../shaders/shader.v0.comp");

	initObj();

	if (stricmp(answer, "1")==1 || stricmp(answer, "Radial")==1 || stricmp(answer, "radial") == 1) {
		initObj2();
	}
	else {
		initObj2disk();
	}

	glutMainLoop();		// bucle de eventos

	destroy();

	return 0;
}
	
//////////////////////////////////////////
// Funciones auxiliares 
void initContext(int argc, char** argv){

	// Definimos el contexto
	glutInit(&argc, argv);							// Inicializa Glut
	glutInitContextVersion(4, 3);					// Indicamos la version de OpenGL
	glutInitContextProfile(GLUT_CORE_PROFILE);		// Queremos un contexto sin compabilidad hacia atras

	// Definimos el Frame Buffer y creamos la ventana
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);	// Frame Buffer por defecto
	glutInitWindowSize(WIDTH, HEIGHT);								// Tamaño de la ventana
	glutInitWindowPosition(0, 0);								// Posicion de la ventana
	glutCreateWindow("BranchedFlow");							// Crea la ventana

	// Inicializamos las extensiones de OpenGL
	glewExperimental = GL_TRUE;
	GLenum err = glewInit(); 
	if (GLEW_OK != err) 
	{ 
		std::cout << "Error: " << glewGetErrorString(err) << std::endl; 
		exit(-1); 
	} 

	// Comprobamos la version de OpenGL
	const GLubyte* oglVersion = glGetString(GL_VERSION);  
	std::cout << "This system supports OpenGL Version: " << oglVersion << std::endl;

	// Le indicamos a Glut que funciones hay que ejecutar cuando se dan ciertos eventos
	glutReshapeFunc(resizeFunc);		// redimension de la ventana
	glutDisplayFunc(renderFunc);		// renderizado
	//glutIdleFunc(idleFunc);				// cuando el procesador esta ocioso
	glutKeyboardFunc(keyboardFunc);		// evento de teclado
	glutMouseFunc(mouseFunc);			// evento de raton
	glutMotionFunc(mouseMotionFunc);	// control de la camara con el raton
}

void initOGL()
{
	glDisable(GL_DEPTH_TEST);					
	glClearColor(0.f, 0.f, 0.f, 0.f);		

	glPolygonMode(GL_FRONT, GL_FILL);
	glDisable(GL_CULL_FACE);	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);

}

void destroy()
{
	// LIBERAMOS RECURSOS

	// buffers
	glDeleteVertexArrays(1, &vaoparticles);
	glDeleteVertexArrays(1, &vaopotential);

	// shaders y programa 1
	glDetachShader(program, vshader); 
	glDetachShader(program, fshader);
	glDeleteShader(vshader); 
	glDeleteShader(fshader);
	glDeleteProgram(program);

	//shaders y program 2
	glDetachShader(program2, cshader);
	glDeleteProgram(program2);
}

void initShader(const char *vname, const char *fname)
{
	// Cargamos los shaders de vertices y fragmentos
	vshader = loadShader(vname, GL_VERTEX_SHADER); 
	fshader = loadShader(fname, GL_FRAGMENT_SHADER);

	// Enlazamos los shaders en un programa
	program = glCreateProgram();			// crea el programa
	glAttachShader(program, vshader);		// asigna el shader de vertices
	glAttachShader(program, fshader);		// asigna el shader de fragmentos

	glLinkProgram(program);					// enlaza

	// Comprobamos si hay error al enlazar
	int linked; 
	glGetProgramiv(program, GL_LINK_STATUS, &linked);	// comprueba el status de linkado
	if (!linked) 
	{ 
		//Calculamos una cadena de error 
		GLint logLen; 
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
		char *logString = new char[logLen]; 
		glGetProgramInfoLog(program, logLen, NULL, logString); 
		std::cout << "Error: " << logString << std::endl; 
		delete[] logString; 
		exit (-1); 
	}

}

void initShader2(const char* name) {
	// Cargamos los shaders de vertices y fragmentos
	cshader = loadShader(name, GL_COMPUTE_SHADER);

	//Enlazamos los shaders en un programa
	program2 = glCreateProgram();
	glAttachShader(program2, cshader);
	glLinkProgram(program2);

	// Comprobamos si hay error al enlazar
	int linked;
	glGetProgramiv(program2, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		//Calculamos una cadena de error
		GLint logLen;
		glGetProgramiv(program2, GL_INFO_LOG_LENGTH, &logLen);
		char* logString = new char[logLen];
		glGetProgramInfoLog(program2, logLen, NULL, logString);
		std::cout << "Error: " << logString << std::endl;
		delete[] logString;
		exit(-1);
	}
	usize= glGetUniformLocation(program2, "size");
}

void initObj()
{
	glm::vec2* pospot = new glm::vec2[sizescreen];
	float* potential = new float[sizescreen];
	glm::vec4* potcolor = new glm::vec4[sizescreen];
	for (int i = 0; i < WIDTH; ++i) {
		for (int j = 0; j < HEIGHT; ++j) {
			float rand = glm::linearRand(0.f, .5f);
			potential[i * WIDTH + j] = rand;
			potcolor[i * WIDTH + j] = glm::vec4(rand,rand,rand,1.f);
			pospot[i * WIDTH + j] = glm::vec2(i,j)/glm::vec2(size)*2.f-1.f;
		}
	}

	glGenVertexArrays(1, &vaopotential);			// crea el VAO
	glBindVertexArray(vaopotential);				// activa el VAO

	glGenBuffers(1, &pospotVBO);
	
	glBindBuffer(GL_ARRAY_BUFFER, pospotVBO);
	glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec2)* sizescreen, pospot, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &potBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, potBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * sizescreen, potential, GL_STATIC_DRAW);

	glGenBuffers(1, &potcolorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, potcolorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * sizescreen, potcolor, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);


	glUseProgram(program);

	glDrawArrays(GL_POINTS, 0, sizescreen);

}

void initObj2() {

	using namespace glm;

	vec4* parpos = new vec4[numparticles];
	vec2* parvel = new vec2[numparticles];
	vec4* parcol = new vec4[numparticles];

	for (int i = 0; i < numparticles; ++i) {
		parpos[i] = vec4(-1.f, glm::linearRand(-.1f,.1f),0.f,0.f);
		parcol[i] = vec4(1.f, 1.f, 0.f,.5f);
		parvel[i] = vec2(.01f, 0.f);
	}

	glGenVertexArrays(1, &vaoparticles);
	glBindVertexArray(vaoparticles);

	glGenBuffers(1, &posparticleVBO);
	glBindBuffer(GL_ARRAY_BUFFER, posparticleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*numparticles, parpos, GL_STATIC_READ);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &pariclecolorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, pariclecolorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * numparticles, parcol, GL_STATIC_READ);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &velparticleBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velparticleBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec2) * numparticles, parvel, GL_STATIC_DRAW);

}

void initObj2disk() {

	using namespace glm;

	vec4* parpos = new vec4[numparticles];
	vec2* parvel = new vec2[numparticles];
	vec4* parcol = new vec4[numparticles];

	for (int i = 0; i < numparticles; ++i) {
		parpos[i] = vec4(diskRand(0.1f), 0.f, 0.f);
		parcol[i] = vec4(1.f, 1.f, 0.f, .5f);
		parvel[i] = vec2(normalize(parpos[i]))*0.01f;
	}

	glGenVertexArrays(1, &vaoparticles);
	glBindVertexArray(vaoparticles);

	glGenBuffers(1, &posparticleVBO);
	glBindBuffer(GL_ARRAY_BUFFER, posparticleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * numparticles, parpos, GL_STATIC_READ);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &pariclecolorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, pariclecolorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * numparticles, parcol, GL_STATIC_READ);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &velparticleBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velparticleBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec2) * numparticles, parvel, GL_STATIC_DRAW);

}


GLuint loadShader(const char *fileName, GLenum type)
{ 
	unsigned int fileLen; 
	char* source = loadStringFromFile(fileName, fileLen);		// carga el codigo del shader

	////////////////////////////////////////////// 
	//Creación y compilación del Shader 
	GLuint shader;												 
	shader = glCreateShader(type);								// crea un ID para el shader
	glShaderSource(shader, 1, 
		(const GLchar **)&source, (const GLint *)&fileLen);		// le asigna el codigo del shader
	glCompileShader(shader);									// compila el shader
	delete[] source;											// libera espacio

	//Comprobamos que se compiló bien 
	GLint compiled; 
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled); 
	if (!compiled) { 
		//Calculamos una cadena de error 
		GLint logLen; 
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen); 
		char *logString = new char[logLen]; 
		glGetShaderInfoLog(shader, logLen, NULL, logString); 
		std::cout << "Error: " << logString << std::endl; 
		delete[] logString; 
		exit (-1); 
	}

	return shader; 
}

unsigned int loadTex(const char *fileName)
{ 
	unsigned char* map; 
	unsigned int w, h; 

	// cargamos la textura en map
	map = loadTexture(fileName, w, h); 

	// comprobacion de que ha cargado
	if (!map) 
	{ 
		std::cout << "Error cargando el fichero: " << fileName << std::endl; 
		exit(-1); 
	}

	// creamos el id de la textura, la activamos y la subimos a la tarjeta grafica
	unsigned int texId; 
	glGenTextures(1, &texId);										// crea
	glBindTexture(GL_TEXTURE_2D, texId);							// activa	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA,		// sube a la grafica
		GL_UNSIGNED_BYTE, (GLvoid*)map);

	// generamos el resto de niveles
	glGenerateMipmap(GL_TEXTURE_2D);

	GLfloat fLargest;
	if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
	}

	// configuramos el acceso
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // tengo menos texeles
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);				// tengo mas texeles
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);			// me paso de la coordenada X
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);			// me paso de la coordenada Y

	// liberamos memoria
	delete[] map;

	return texId;	// devolvemos el ID de la textura
}

void renderFunc()
{

	glUseProgram(program);

	// Se renderiza el sistema de particulas
	glDrawArrays(GL_LINES, 0, numparticles);

	glutSwapBuffers();

	glUseProgram(program2);	
	glActiveTexture(GL_TEXTURE0);
	if(usize!=-1)
		glUniform2iv(usize, 1,&size[0]);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, posparticleVBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, velparticleBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, potBuffer);

	// Se lanzan los compute shaders
	glDispatchCompute(numparticles/wgs+1, 1, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);



	std::cout << "Calculations done" << std::endl;

}

void resizeFunc(int width, int height)
{
	if (width != WIDTH || height != HEIGHT) {
		WIDTH = width;
		HEIGHT = height;

		glViewport(0, 0, width, height);
	}
	
}


void idleFunc()
{
	//glutPostRedisplay();
}

void keyboardFunc(unsigned char key, int x, int y){

	if (key == 'n' || key == 'N') {
		glutPostRedisplay();
	}
}

void mouseFunc(int button, int state, int x, int y){

}

void mouseMotionFunc(int x, int y)
{
	
}

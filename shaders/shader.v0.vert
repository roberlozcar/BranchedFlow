#version 430 core

layout (location=0) in vec2 inPos;

layout (location=1) in vec4 inColor;

//uniform mat4 trans;

out vec4 fragColor;

void main()
{
	//gl_Position =trans*vec4(inPos,0.,1.);
	gl_Position =vec4(inPos,0.,1.);
	fragColor=inColor;
}

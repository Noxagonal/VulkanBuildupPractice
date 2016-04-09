#version 410

layout(location=0) in vec3 Vertex_Location;

void main()
{
	gl_Position = vec4(Vertex_Location, 1.0);
}

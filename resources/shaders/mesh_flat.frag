#version 450

layout (location = 0) in PerVertexData
{
  vec3 color;
} fragIn;

layout (location = 0) out vec3 fs_Color;

void main()
{
	fs_Color = fragIn.color;
}
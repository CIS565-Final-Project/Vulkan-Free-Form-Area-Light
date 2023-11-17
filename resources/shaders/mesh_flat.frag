#version 450

#extension GL_KHR_vulkan_glsl:enable

layout (location = 0) in PerVertexData
{
  vec3 color;
} fragIn;

layout (location = 0) out vec3 fs_Color;

layout(set = 3, binding = 0) uniform sampler2D u_Texture;

void main()
{
	fs_Color = texture(u_Texture, vec2(0.5f, 0.5f)).rgb;
}
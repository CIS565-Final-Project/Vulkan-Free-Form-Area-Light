#version 450

#extension GL_KHR_vulkan_glsl:enable

layout(location = 0) in vec3 vs_Color;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 color = 1.f - vs_Color;
	outColor = vec4(color, 1.f);
}
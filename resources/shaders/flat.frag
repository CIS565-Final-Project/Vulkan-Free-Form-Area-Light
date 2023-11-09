#version 450

#extension GL_KHR_vulkan_glsl:enable

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 vs_Color;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
	//vec3 color = 1.f - vs_Color;
	//outColor = vec4(color, 1.f);

	outColor = texture(texSampler, fragTexCoord);
}
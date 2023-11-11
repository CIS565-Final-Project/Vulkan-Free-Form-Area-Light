#version 450

#extension GL_KHR_vulkan_glsl:enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	vec3 eye;
} camera;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 vs_Color;
layout(location = 1) in vec2 vs_TexCoord;
layout(location = 2) in vec3 vs_Normal;
layout(location = 3) in vec3 vs_WorldPos;

layout(location = 0) out vec4 outColor;

void main()
{
	//vec3 color = 1.f - vs_Color;
	//outColor = vec4(color, 1.f);

	float d = dot(normalize(camera.eye - vs_WorldPos), vs_Normal);
	outColor = texture(texSampler, vs_TexCoord) * smoothstep(d, 0.035f, 0.13f);
}
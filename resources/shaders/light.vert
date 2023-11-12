#version 450

#extension GL_KHR_vulkan_glsl:enable

const vec2 position[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5,  0.5),
	vec2(-0.5, 0.5)
);

const vec3 color[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	vec3 eye;
} camera;

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 vs_Color;
layout(location = 1) out vec2 vs_TexCoord;
layout(location = 2) out vec3 vs_Normal;

void main()
{
	// gl_Position = vec4(position[gl_VertexIndex], 0.0, 1.0);
	// vs_Color = color[gl_VertexIndex];
	// 

	gl_Position = camera.proj * camera.view  * model * vec4(inPosition,  1.0);
	// vs_Color = inColor;
	vs_TexCoord = inTexCoord;
	vs_Normal = inNormal;
}
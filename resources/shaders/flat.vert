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

layout(location = 0) out vec3 vs_Color;

void main()
{
	gl_Position = vec4(position[gl_VertexIndex], 0.0, 1.0);
	vs_Color = color[gl_VertexIndex];

}
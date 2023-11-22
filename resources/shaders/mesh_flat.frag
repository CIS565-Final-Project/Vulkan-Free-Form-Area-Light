#version 450

#extension GL_KHR_vulkan_glsl:enable

layout (location = 0) in PerVertexData
{
	vec2 uv;
	vec3 color;
} fragIn;

layout (location = 0) out vec3 fs_Color;

// layout(set = 1, binding = 5) uniform sampler2D u_Texture;

void main()
{
	// fs_Color = texture(u_Texture, fragIn.uv).rgb;
	//fs_Color = fragIn.color;

	fs_Color = vec3(1.0, 1.0, 1.0);
}
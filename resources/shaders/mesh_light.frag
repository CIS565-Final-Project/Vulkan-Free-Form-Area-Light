#version 450

#extension GL_KHR_vulkan_glsl:enable


layout (location = 0) in PerVertexData
{
	float polygon;
	vec2 uv;
} fragIn;


layout (location = 0) out vec3 fs_Color;

layout(set = 1, binding = 5) uniform sampler2DArray u_Texture;

void main()
{
	fs_Color = texture(u_Texture, vec3(fragIn.uv, 0.f)).rgb;
	//fs_Color = vec3(1.0);
	//  fs_Color = fragIn.color;

	// fs_Color = vec3(fragIn.uv, 0.0);
}
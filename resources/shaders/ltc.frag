#version 450

//#extension GL_KHR_vulkan_glsl:require

#define INV_PI 0.31830988618f
#define LUT_SIZE 64.f
#define MAX_LIGHT_VERTEX 5

layout(set = 1, binding = 1) uniform sampler2D texSampler;
//layout(set = 1, binding = 2) uniform sampler2D ltcSampler;
//layout(set = 2, binding = 0) uniform vec3 cameraPos;

// layout(set = 2, binding = 1) uniform LightBufferObject{
// 	vec3 vertex[MAX_LIGHT_VERTEX];//alignment is 4!!!
// 	float size;
// }lights;

layout(location = 0) in vec3 vs_Color;
layout(location = 1) in vec2 fragTexCoord;
//layout(location = 2) in float fs_roughness;
layout(location = 2) in vec3 fs_Pos;
//layout(location = 4) in vec3 fs_norm;
layout(location = 0) out vec4 outColor;

mat3 LTCMatrix(vec3 V, vec3 N, float roughness){
	float theta = acos(max(dot(V,N),0));
	vec2 uv = vec2(roughness, 2 * theta * INV_PI);
	//reproject uv to 64x64 texture eg. for roughness = 1, u should be 63.5/64;
	uv = uv * (LUT_SIZE - 1)/LUT_SIZE  + 0.5 / LUT_SIZE;
	vec4 ltcVal = texture(texSampler, uv);
	//vec4 ltcVal = vec4(1.f);	
	
	mat3 res = mat3(
		vec3(1,0,ltcVal.w),
		vec3(0,ltcVal.z,0),
		vec3(ltcVal.y,0,ltcVal.x)
	);
	//ltc fit中的invM是转置的版本
	return transpose(res);
}

void ClipVertex(in vec3 lightVertex[MAX_LIGHT_VERTEX],int arrSize, 
				out vec3 clipedPos[MAX_LIGHT_VERTEX], out int clipedSize){
	int readIdx = 0;
	clipedSize = 0;//= writeIdx;
	for(;readIdx<arrSize;++readIdx){
		if(readIdx>0 && (lightVertex[readIdx - 1].z * lightVertex[readIdx].z) < 0){
			//current node is above plane and previous node is below plane
			//or current node is below plane and previous node is above plane
			vec3 prevPos = lightVertex[readIdx - 1];
			vec3 curPos = lightVertex[readIdx];
			//the actual result: 
			// note that we can also just use prevPos.z * curPos - curPos.z * prevPos because the clipedPos will be nomalized anyway
			float u = prevPos.z / (prevPos.z - curPos.z);
			clipedPos[clipedSize] = mix(prevPos,curPos,u);
			++clipedSize;
		}
		if(lightVertex[readIdx].z >= 0){
			//current node is above plane
			clipedPos[clipedSize] = lightVertex[readIdx];
			++clipedSize;
		}
	}
	//handle last edge
	if(readIdx>0 && lightVertex[readIdx - 1].z * lightVertex[0].z < 0){
		//current node is above plane and previous node is below plane
		//or current node is below plane and previous node is above plane
		vec3 prevPos = lightVertex[readIdx - 1];
		vec3 curPos = lightVertex[0];
		float u = prevPos.z / (prevPos.z - curPos.z);
		clipedPos[clipedSize] = mix(prevPos,curPos,u);
		++clipedSize;
	}
}

float IntegrateD(mat3 LTCMat, vec3 V, vec3 N, vec3 shadePos, vec3 lightVertex[MAX_LIGHT_VERTEX], int arrSize){
	//to tangent space
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up,N));
	//vec3 tangent = normalize(V - N * dot(V,N));
	vec3 bitangent = cross(N,tangent);
	LTCMat = LTCMat * transpose(mat3(tangent,bitangent,N)); //inverse rotation matrix
	for(int i = 0;i<arrSize;++i){
		lightVertex[i] = LTCMat * (lightVertex[i] - shadePos);
	}
	//clip lightVertex
	vec3 clipedPos[MAX_LIGHT_VERTEX];
	ClipVertex(lightVertex,arrSize,clipedPos,arrSize);
	//reproject light onto hemisphere
	for(int i = 0;i<arrSize;++i){
		clipedPos[i] = normalize(clipedPos[i]);
	}
	//integrate diffuse light
	float res = 0;
	for(int i = 1;i<arrSize;++i){
		vec3 p_i = clipedPos[i-1];
		vec3 p_j = clipedPos[i]; 
		float theta = acos(dot(p_i,p_j));
		float proj = dot(normalize(cross(p_i,p_j)) , vec3(0,0,1));
		res += (theta * proj);
	}
	if(arrSize>0){
		vec3 p_i = clipedPos[arrSize-1];
		vec3 p_j = clipedPos[0]; 
		float theta = acos(dot(p_i,p_j));
		float proj = dot(normalize(cross(p_i,p_j)) , vec3(0,0,1));
		res += (theta * proj);
	}
	return res * 0.5 * INV_PI;
}
float halfWidth = 3.5f;
vec3 lights[5] = vec3[](
	vec3(-halfWidth, halfWidth + 2.0f, -5.0f),
	vec3(halfWidth, halfWidth + 2.0f, -5.0f ),
	vec3(halfWidth, -halfWidth + 2.0f, -5.0f ),
	vec3(-halfWidth, -halfWidth + 2.0f, -5.0f),
	vec3(0.f)
);

void main()
{
	//vec3 color = 1.f - vs_Color;
	//outColor = vec4(color, 1.f);
	vec3 cameraPos = vec3(0,0,-10);
	vec3 fs_norm = vec3(0,1,0);
	vec3 V = normalize(cameraPos - fs_Pos);
	vec3 N = normalize(fs_norm);
	//float roughness = fs_roughness;
	mat3 LTCMat = LTCMatrix(V,N,0.1);
	//outColor = vec3(IntegrateD(LTCMat,V,N,fs_Pos,lights.vertex,lights.size));
	//outColor = texture(texSampler, fragTexCoord);
	outColor = vec4(vec3(IntegrateD(LTCMat,V,N,fs_Pos,lights,4)),1);
}
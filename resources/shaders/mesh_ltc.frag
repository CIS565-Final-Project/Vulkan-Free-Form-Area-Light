#version 450

#extension GL_KHR_vulkan_glsl:enable

#define INV_PI 0.31830988618f
#define LUT_SIZE 64.f
#define MAX_LIGHT_VERTEX 5
#define MAX_BEZIER_CURVE 5
//uniform
layout(set = 0, binding = 0) uniform CameraUBO {
	vec4 pos;
    mat4 viewProjMat;
} u_CamUBO;
layout(set = 1, binding = 5) uniform sampler2D texSampler;
layout(set = 1, binding = 6) uniform sampler2DArray ltSampler;

layout(set = 2, binding = 0) uniform Roughness{
	float u_Roughness;
};

//in
layout (location = 0) in PerVertexData{
	vec2 uv;
	vec3 color;
	vec3 pos; // worldPos
} fragIn;

//out
layout (location = 0) out vec3 fs_Color;

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
	//ltc
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

//v: bounding quad of the light vertices in LTC space (not normalized, i.e. not on hemisphere yet)
void FetchLight(in vec3 v[4], vec3 lookup, out vec2 uv, out float lod){
	//project shading point onto light plane
	vec3 v1 = v[1] - v[0];
	vec3 v2 = v[2] - v[0];
	vec3 N = normalize(cross(v1,v2));//Normal of the plane
	float dist = dot(v[0],N)/dot(lookup,N);
	vec3 pt = dist * lookup;

	//calculate A for LOD: i.e.pow(dist2/A,0.5)
	vec3 e01 = v[1] - v[0];
	vec3 e12 = v[2] - v[1];
	vec3 e23 = v[3] - v[2];
	vec3 e30 = v[0] - v[3];
	float A = length(0.5 * (cross(e01,e12) + cross(e23,e30)));	

	//barycentric interpolation
	//https://cgvr.informatik.uni-bremen.de/teaching/cg_literatur/barycentric_floater.pdf
	float lenPv0 = length(v[0] - pt);
	float lenPv1 = length(v[1] - pt);
	float lenPv2 = length(v[2] - pt);
	float lenPv3 = length(v[3] - pt);
	float tan0 = tan(acos(dot(v[0] - pt, v[1] - pt)/(lenPv0 * lenPv1))*0.5);
	float tan1 = tan(acos(dot(v[1] - pt, v[2] - pt)/(lenPv1 * lenPv2))*0.5);
	float tan2 = tan(acos(dot(v[2] - pt, v[3] - pt)/(lenPv2 * lenPv3))*0.5);
	float tan3 = tan(acos(dot(v[3] - pt, v[0] - pt)/(lenPv3 * lenPv0))*0.5);
	vec4 barycentric = vec4(
		(tan3 + tan0)/lenPv0,
		(tan0 + tan1)/lenPv1,
		(tan1 + tan2)/lenPv2,
		(tan2 + tan3)/lenPv3
	);
	barycentric /= (barycentric.x + barycentric.y + barycentric.z + barycentric.w);

	//OUTPUT
	lod = log(2048.0 * dist / pow(A, 0.5)) / log(3.0);//magic
	uv = vec2(0, 1) * barycentric.x + vec2(1,1) * barycentric.y + vec2(1,0) * barycentric.z + vec2(0,0) * barycentric.w;
}

vec3 IntegrateEdge(vec3 p_i, vec3 p_j){
	float theta = acos(dot(p_i,p_j));
	vec3 res = normalize(cross(p_i,p_j));
	//float res = dot(normalize(cross(p_i,p_j)), vec3(0,0,1))
	return res * theta;
}

float IntegrateD(mat3 LTCMat, vec3 V, vec3 N, vec3 shadePos, vec3 lightVertex[MAX_LIGHT_VERTEX], int arrSize, out vec2 uv, out float lod){
	//to tangent space
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(V - N * dot(V,N));
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
	vec3 lookup = vec3(0);
	for(int i = 1;i<arrSize;++i){
		vec3 p_i = clipedPos[i-1];
		vec3 p_j = clipedPos[i]; 
		lookup += IntegrateEdge(p_i,p_j);
	}
	if(arrSize>0){
		vec3 p_i = clipedPos[arrSize-1];
		vec3 p_j = clipedPos[0]; 
		lookup += IntegrateEdge(p_i,p_j);
	}
	float res = lookup.z;
	
	//calculate fetch uv, lod
	//******************
	//Assuming it's a quad light!!!!!
	vec3 lightBounds[4] = vec3[](
		lightVertex[0],
		lightVertex[1],
		lightVertex[2],
		lightVertex[3]
	);
	FetchLight(lightBounds,normalize(lookup),uv,lod);
	//****************************
	return res * 0.5 * INV_PI;
}

float halfWidth = 1.5f;
vec3 lights[5] = vec3[](
	vec3(-halfWidth, -halfWidth + 1.0f, 5.0f),
	vec3(halfWidth, -halfWidth + 1.0f, 5.0f ),
	vec3(halfWidth, halfWidth + 1.0f, 5.0f ),
	vec3(-halfWidth, halfWidth + 1.0f, 5.0f),
	vec3(0.f)
);

void main(){
	vec3 pos = fragIn.pos;
	vec3 cameraPos = u_CamUBO.pos.xyz;
	vec3 fs_norm = vec3(0,1,0);
	vec3 V = normalize(cameraPos - pos);
	vec3 N = normalize(fs_norm);
	float roughness = u_Roughness;
	mat3 LTCMat = LTCMatrix(V, N, roughness);
	float lod;
	vec2 ltuv; 
	float d = IntegrateD(LTCMat,V,N,pos,lights,4, ltuv, lod);
	fs_Color =  d * mix(texture(ltSampler,vec3(ltuv,ceil(lod))).xyz, texture(ltSampler,vec3(ltuv,floor(lod))).xyz, ceil(lod) - lod);
}
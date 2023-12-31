#version 450

#extension GL_KHR_vulkan_glsl:enable

#define INV_PI 0.31830988618f
#define LUT_SIZE 64.f
#define MAX_LIGHT_VERTEX 10
#define MAX_BEZIER_CURVE 5
#define MAX_STACK_SIZE 12
#define EPS 1.0e-5
#define MIN_THRESHOLD 0.01
#define CLIP 1
//struct
struct LightInfo {
	vec2 boundUV[4]; // use for texture
	vec3 boundPositions[4]; //use for texture
	vec4 boundSphere;
	vec3 lightVertex[MAX_LIGHT_VERTEX];	
	int arraySize;
	int lightType;//0: polygon 1: bezier
	float amplitude;
	float padding;
};

//uniform
layout(set = 0, binding = 0) uniform CameraUBO {
	vec4 pos;
    mat4 viewProjMat;
} u_CamUBO;

layout(set = 1, binding = 5) uniform sampler2D LTCSampler;
layout(set = 1, binding = 6) uniform sampler2D LTCAmpSampler;
layout(set = 1, binding = 7) uniform sampler2DArray compressedSampler;
layout(set = 2, binding = 0) uniform LightCount{
	uint lightCount;
};
layout(set = 2, binding = 1) buffer lightBuffer{
	LightInfo lightInfos[];
};

layout(set = 2, binding = 2) uniform sampler2DArray lightAtlasTexture;

layout(set = 3, binding = 0) uniform MaterialParam{
	vec4 materialParam;
};

//in

layout (location = 0) in PerVertexData{
	vec2 uv;
	vec3 color;
	vec3 pos; // worldPos
	vec3 normal;
} fragIn;

//out
layout (location = 0) out vec4 fs_Color;

//Tool function
vec2 GetFrenselTerm(vec3 V, vec3 N, float roughness){
	float theta = acos(max(dot(V,N),0));
	vec2 uv = vec2(roughness, 2 * theta * INV_PI);
	//reproject uv to 64x64 texture eg. for roughness = 1, u should be 63.5/64;
	uv = uv * (LUT_SIZE - 1)/LUT_SIZE  + 0.5 / LUT_SIZE;

	return texture(LTCAmpSampler, uv).xy;
}
mat3 LTCMatrix(vec3 V, vec3 N, float roughness){
	float theta = acos(max(dot(V,N),0));
	vec2 uv = vec2(roughness, 2 * theta * INV_PI);
	//reproject uv to 64x64 texture eg. for roughness = 1, u should be 63.5/64;
	uv = uv * (LUT_SIZE - 1)/LUT_SIZE  + 0.5 / LUT_SIZE;
	vec4 ltcVal = texture(LTCSampler, uv);

	mat3 res = mat3(
		vec3(ltcVal.x,0,ltcVal.z),
		vec3(0,1,0),
		vec3(ltcVal.y,0,ltcVal.w)
	);
	//ltc
	return res;
}
mat3 BitMatrix(vec3 V, vec3 N){
	vec3 tangent = normalize(V - N * dot(V,N));
	vec3 bitangent = cross(N,tangent);
	return transpose(mat3(tangent,bitangent,N));
}
float Fresnel(vec3 V, vec3 N, float metallic, float roughness){
	float R = mix((0.04f), (1.f), metallic);
    float cos = max(dot(V,N),0);
    return R + (max((1.0-roughness),R)-R)*pow(1-cos,5);
}



//v: bounding quad of the light vertices in LTC space (not normalized, i.e. not on hemisphere yet)
void FetchLight(vec3 v[4],  vec2 uvs[4], vec3 lookup, out vec2 uv, out float lod){
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
	uv = uvs[0] * barycentric.x + uvs[1] * barycentric.y + uvs[2] * barycentric.z + uvs[3] * barycentric.w;
}


// ----------------------------------------------
// Clipping
// ----------------------------------------------
void ClipVertex(in vec3 lightVertex[MAX_LIGHT_VERTEX],int arrSize, out vec3 clipedPos[MAX_LIGHT_VERTEX], out int clipedSize){
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
//https://github.com/Paul180297/BezierLightLTC/blob/master/shaders/floorLTC.frag
//sort so v.x < v.y < v.z
vec3 sort3(vec3 v){
	if(v.x > v.y){
		float tmp = v.y;
		v.y = v.x;
		v.x = tmp;
	}
	if(v.y > v.z){
		float tmp = v.z;
		v.z = v.y;
		v.y = tmp;
	}
	if(v.x > v.y){
		float tmp = v.y;
		v.y = v.x;
		v.x = tmp;
	}
	return v;
}
bool between01(const float t){
	return 0.0 <= t && t <= 1.0;
}
void SolveLinear(const float a, const float b, out int realSolCnt, out float sol[3]){
	float sol0 = -b/a;
	realSolCnt = 0;
	if(between01(sol0))sol[realSolCnt++] = sol0;
}
void SolveQuadratic(const float a, const float b, const float c, out int realSolCnt, out float sol[3]){
	float delta = b*b - 4 *a*c;
	realSolCnt = 0;
	if(delta == 0){
		float sol0 = -b/(2*a);
		if(between01(sol0))sol[realSolCnt++] = sol0;
	}else if(delta > 0){
		float sqrt_delta = sqrt(delta);
		float sol0 = (-b - sqrt_delta) / (2 * a);
		float sol1 = (-b + sqrt_delta) / (2 * a);
		if(sol0 < sol1){
			float tmp = sol0;
			sol0 = sol1;
			sol1 = tmp;
		}
		if(between01(sol0))sol[realSolCnt++] = sol0;
		if(between01(sol1))sol[realSolCnt++] = sol1;
	}
}
void SolveCubic(const float a, const float b, const float c, const float d, out int realSolCnt, out float sol[3]){
	vec4 Coefficient = vec4(d,c,b,a);
	Coefficient.xyz /= Coefficient.w;
	Coefficient.yz /= 3.0;
	vec3 Delta = vec3(
		(Coefficient.z * (-Coefficient.z) + Coefficient.y),
		(Coefficient.z * (-Coefficient.y) + Coefficient.x),
		dot(vec2(Coefficient.z, -Coefficient.y),Coefficient.xy)
	);
	float Discriminant = dot(vec2(4.0f * Delta.x,-Delta.y),Delta.zy);
	vec2 Depressed = vec2(
		(Delta.x * (-2.0 * Coefficient.z) + Delta.y),
		Delta.x
	);
	realSolCnt = 0;
	if(Discriminant > 0.0){
		float Theta = atan(sqrt(Discriminant),-Depressed.x)/3.f;
		vec2 CubicRoot = vec2(cos(Theta),sin(Theta));
		vec3 Root = vec3(
			CubicRoot.x,
			dot(vec2(-0.5, -0.5 * sqrt(3.f)), CubicRoot),
			dot(vec2(-0.5,  0.5 * sqrt(3.f)), CubicRoot)
		);
		vec3 tt = sort3(Root * 2.0 * sqrt(-Depressed.y) - Coefficient.z);
		if(between01(tt.z)){sol[realSolCnt++] = tt.z;}
		if(between01(tt.y)){sol[realSolCnt++] = tt.y;}
		if(between01(tt.x)){sol[realSolCnt++] = tt.x;}
	}else{
		vec2 tmp = 0.5 * (-Depressed.x + vec2(-1.0,1.0) * sqrt(-Discriminant));
		vec2 pq = sign(tmp) * pow(abs(tmp),vec2(0.3333));
		float t0 = pq.x + pq.y - Coefficient.z;
		if(between01(t0)){sol[realSolCnt++] = t0;}
	}
}
void SolveEquation(const float a, const float b, const float c, const float d, out int realSolCnt, out float sol[3]){
	if(abs(a)>EPS){
		SolveCubic(a,b,c,d,realSolCnt,sol);
	}
	else if(abs(b)>EPS){
		SolveQuadratic(b,c,d,realSolCnt,sol);
	}
	else{
		SolveLinear(c,d,realSolCnt,sol);
	}
}
vec3 BezierCurve(const vec3 ctrlPts[4], float t){
	vec3 u01 = mix(ctrlPts[0], ctrlPts[1], t);
	vec3 u12 = mix(ctrlPts[1], ctrlPts[2], t);
	vec3 u23 = mix(ctrlPts[2], ctrlPts[3], t);
	u01 = mix(u01, u12, t);
	u12 = mix(u12, u23, t);
	return mix(u01, u12, t);
}
//@return need integrate
bool ClipBezier(vec3 controlPts[4], out int intersectCnt, out float intersectTs[3]){
	int numCtrlUnder = 0;
	intersectCnt = 0;
	for(int i = 0; i<4;++i){
		numCtrlUnder += controlPts[i].z < 0.0 ? 1 : 0;
	}
	if(numCtrlUnder == 0)return true;
	if(numCtrlUnder == 4)return false;//all control points under plane
	//return true;
	const float p0z = controlPts[0].z;
	const float p1z = controlPts[1].z;
	const float p2z = controlPts[2].z;
	const float p3z = controlPts[3].z;

	const float a = -p0z + 3 * p1z - 3 * p2z + p3z;//could be 0!!!
	const float b = 3 * (p0z - 2 * p1z + p2z);
	const float c = 3 * (-p0z + p1z);
	const float d = p0z;

	SolveEquation(a,b,c,d, intersectCnt, intersectTs);
	
	if(intersectCnt>0)return true;
	if(
		BezierCurve(controlPts,1.0).z > 0.0  // fix clip bug
		//&& BezierCurve(controlPts,0.0).z > 0.0 
		//&& BezierCurve(controlPts,0.5).z > 0.0
	){
		return true;
	}
	return false;
}

// ----------------------------------------------
// Integration
// ----------------------------------------------
vec3 IntegrateEdge(vec3 p_i, vec3 p_j){
	float theta = acos(dot(p_i,p_j));
	vec3 res = normalize(cross(p_i,p_j));
	//float res = dot(normalize(cross(p_i,p_j)), vec3(0,0,1))
	return res * theta;
}
vec2 tStack[MAX_STACK_SIZE];
vec3 IntegrateBezier(vec3 ctrlPts[4], float tStart, float tEnd, float threshold){
	vec3 res = vec3(0.f);
	int stackTop = 0;
	tStack[stackTop++] = vec2(tStart,tEnd);
	while(stackTop!=0){
		vec2 tmp = tStack[--stackTop];
		float tMin = tmp.x;
		float tMax = tmp.y;
		float tMid = (tMin + tMax) / 2.f;

		vec3 v0 = BezierCurve(ctrlPts,tMin);
		vec3 v1 = BezierCurve(ctrlPts,tMid);
		vec3 v2 = BezierCurve(ctrlPts,tMax);
		
		//if curve is nearly a segment
		bool isLine = false;
		if(dot(cross(v0-v1,v0-v2),cross(v0-v1,v0-v2)) < 0.001){
			isLine = true;
		}
		v0 = normalize(v0);
		v1 = normalize(v1);
		v2 = normalize(v2);

		vec3 I01 = IntegrateEdge(v0,v1);
		vec3 I12 = IntegrateEdge(v1,v2);
		vec3 I20 = IntegrateEdge(v2,v0);

		if(!isLine || (abs(I01.z + I12.z + I20.z) >= threshold && stackTop<(MAX_STACK_SIZE -2))){
			tStack[stackTop++] = vec2(tMin, tMid);
			tStack[stackTop++] = vec2(tMid, tMax);
		}else{
			res += (I01 + I12);
		}
	}
	return res;
}
float IntegrateD(mat3 LTCMat, vec3 V, vec3 N, vec3 shadePos,const in LightInfo lightInfo, bool twoSided, out vec2 uv, out float lod){

	vec3 lightVertex[MAX_LIGHT_VERTEX] = lightInfo.lightVertex;
	int arrSize = lightInfo.arraySize;
	//to tangent space
	mat3 worldToLTC = LTCMat * BitMatrix(V,N); //inverse rotation matrix
	for(int i = 0;i<arrSize;++i){
		lightVertex[i] = worldToLTC * (lightVertex[i] - shadePos);
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

	//two-sided
	float res = lookup.z;
	if(twoSided){
		if(res < 0){
			res = -res;
			lookup = -lookup;
		}
	}else{
		res = max(0.0,res);
	}
	//calculate fetch uv, lod
	//transform boundary to LTC
	vec3 ltcBoundary[4];
	for(int i = 0;i<4;++i){
		ltcBoundary[i] = worldToLTC * (lightInfo.boundPositions[i] - shadePos);
	}

	FetchLight(ltcBoundary,lightInfo.boundUV, normalize(lookup),uv,lod);
	return res * 0.5 * INV_PI;
}
float IntegrateBezierD(mat3 LTCMat, vec3 V, vec3 N, vec3 shadePos, float roughness,const in LightInfo lightInfo, bool twoSided, out vec2 uv, out float lod){
	vec3 lightVertex[MAX_LIGHT_VERTEX] = lightInfo.lightVertex;
	int arrSize = lightInfo.arraySize;
	// to LTC Space
	mat3 worldToLTC = LTCMat * BitMatrix(V,N);//transpose(mat3(tangent,bitangent,N)); //inverse rotation matrix
	int bezierNum = arrSize/4;
	for(int i = 0;i<bezierNum;++i){
		int offset = i << 2;
		lightVertex[offset + 0] = worldToLTC * (lightVertex[offset + 0] - shadePos);
		lightVertex[offset + 1] = worldToLTC * (lightVertex[offset + 1] - shadePos);
		lightVertex[offset + 2] = worldToLTC * (lightVertex[offset + 2] - shadePos);
		lightVertex[offset + 3] = worldToLTC * (lightVertex[offset + 3] - shadePos);
	}
	//Integrate
	// float res = 0.0;
	vec3 lookup = vec3(0.f);
	float threshold = roughness * roughness;
	threshold = max(threshold * threshold * 0.05, MIN_THRESHOLD);
	bool hasBegin = false;
	vec3 vBegin = vec3(0.f);
	bool hasEnd = false;
	vec3 vEnd = vec3(0.f);
	for(int i = 0;i<bezierNum;++i){
		int offset = i << 2;
		vec3 tmpCtrlPts[4] = vec3[](
			lightVertex[offset + 0],
			lightVertex[offset + 1],
			lightVertex[offset + 2],
			lightVertex[offset + 3]
		);
#if CLIP
		float ts[3];
		int tCnt = 0;
		//Clip
		if(ClipBezier(tmpCtrlPts, tCnt, ts)){
			//Integrate
			//res += tCnt/2.0;
			//continue;
			float t0,t1,t2;
			switch(tCnt){
				case 0:
				{
					lookup += IntegrateBezier(tmpCtrlPts,0,1,threshold);
					break;
				}
				case 1:
				{	
					t0 = ts[0];
					if(BezierCurve(tmpCtrlPts,0.5 * t0).z > 0){
						//start point above plane
						lookup += IntegrateBezier(tmpCtrlPts,0,t0,threshold);
						hasEnd = true;
						vEnd = BezierCurve(tmpCtrlPts,t0);
					}else{
						//start point below plane
						lookup += IntegrateBezier(tmpCtrlPts,t0,1,threshold);
						if(hasEnd){
							vec3 v0 = BezierCurve(tmpCtrlPts,t0);
							hasEnd = false;
							lookup += IntegrateEdge(normalize(vEnd),normalize(v0));
						}else{
							vBegin = BezierCurve(tmpCtrlPts,t0);
							hasBegin = true;									
						}
					}
					break;
				}
				case 2:
				{
					t0 = ts[1];
					t1 = ts[0];
					if(BezierCurve(tmpCtrlPts,(t0+t1)/2.f).z > 0){
						//mid point above plane
						//0->t0
						if(hasEnd){
							vec3 v0 = BezierCurve(tmpCtrlPts,t0);
							lookup += IntegrateEdge(normalize(vEnd),normalize(v0));
							hasEnd = false;
						}else{
							hasBegin = true;
							vBegin = BezierCurve(tmpCtrlPts,t0);
						}
						//t0->t1
						lookup += IntegrateBezier(tmpCtrlPts,t0,t1,threshold);
						//t1->1
						hasEnd = true;
						vEnd = BezierCurve(tmpCtrlPts,t1);
					}else{
						//mid point below plane
						//0->t0
						lookup += IntegrateBezier(tmpCtrlPts,0,t0,threshold);
						//t0->t1
						vec3 v0 = BezierCurve(tmpCtrlPts,t0);
						vec3 v1 = BezierCurve(tmpCtrlPts,t1);
						lookup += IntegrateEdge(normalize(v0),normalize(v1));
						//t1->1
						lookup += IntegrateBezier(tmpCtrlPts,t1,1,threshold);
					}
					break;
				}
				case 3:
				{
					t0 = ts[2];
					t1 = ts[1];
					t2 = ts[0];
					if(BezierCurve(tmpCtrlPts,(t0 + t1)/2.f).z > 0.f){
						//0->t0
						if(hasEnd){
							vec3 v0 = BezierCurve(tmpCtrlPts,t0);
							lookup += IntegrateEdge(normalize(vEnd),normalize(v0));
							hasEnd = false;
						}else{
							hasBegin = true;
							vBegin = BezierCurve(tmpCtrlPts,t0);
						}
						//t0->t1
						lookup += IntegrateBezier(tmpCtrlPts,t0,t1,threshold);
						//t1->t2
						vec3 v1 = BezierCurve(tmpCtrlPts,t1);
						vec3 v2 = BezierCurve(tmpCtrlPts,t2);
						lookup += IntegrateEdge(normalize(v1),normalize(v2));
						//t2->1
						lookup += IntegrateBezier(tmpCtrlPts,t2,1,threshold);
					}else{
						//0->t0
						lookup += IntegrateBezier(tmpCtrlPts,0,t0,threshold);
						//t0->t1
						vec3 v0 = BezierCurve(tmpCtrlPts,t0);
						vec3 v1 = BezierCurve(tmpCtrlPts,t1);
						lookup += IntegrateEdge(normalize(v0),normalize(v1));
						//t1->t2
						lookup += IntegrateBezier(tmpCtrlPts,t1,t2,threshold);
						//t2->1
						hasEnd = true;
						vEnd = BezierCurve(tmpCtrlPts,t2);
					}
					break;
				}
				default:
				{
					break;
				}
			}
		}else{
			//entire curve below plane, do nothing
		}
#else
		lookup += IntegrateBezier(tmpCtrlPts,0,1,threshold);
#endif
	}
	if(hasBegin && hasEnd){
		lookup += IntegrateEdge(normalize(vEnd),normalize(vBegin));
	}

	//two sided
	float res = lookup.z;
	if(twoSided){
		if(lookup.z < 0){
			res = -res;
			lookup = - lookup;
		}
	}else{
		res = max(0.0,res);
	}

	vec3 ltcBoundary[4];
	for(int i = 0;i<4;++i){
		ltcBoundary[i] = worldToLTC * (lightInfo.boundPositions[i] - shadePos);
	}

	FetchLight(ltcBoundary,lightInfo.boundUV, normalize(lookup),uv,lod);

	return res* 0.5 * INV_PI;
}


void CoordinateSystem(in vec3 normal, out vec3 tangent, out vec3 bitangent)
{
	vec3 up = abs(normal.z) < 0.999f ? vec3(0.f, 0.f, 1.f) : vec3(1.f, 0.f, 0.f);
	tangent = normalize(cross(up, normal));
	bitangent = cross(normal, tangent);
}

vec3 GetNormal()
{	
	vec3 normal = normalize(fragIn.normal);
	vec3 nor = texture(compressedSampler, vec3(fragIn.uv, 1.0)).rgb;
	nor = 2 * nor - vec3(1.0f);
	
	if(dot(nor, nor) > 0.f)
	{
		vec3 tangent;
		vec3 bitangent;
		CoordinateSystem(normal, tangent, bitangent);
	
		normal = normalize(nor.x * tangent + nor.y * bitangent + nor.z * normal);
	}

	return normal;
	
}

void main(){

	vec4 albedo = texture(compressedSampler, vec3(fragIn.uv, 0.0)).rgba;
	fs_Color = vec4(0.f);

	//if(albedo.a < 0.01f) discard;
	//fs_Color = albedo;
	//return;

	if(albedo.a < 0.01f) discard;

	vec3 pos = fragIn.pos;
	vec3 cameraPos = u_CamUBO.pos.xyz;
	vec3 fs_norm = GetNormal();

	vec3 V = normalize(cameraPos - pos);
	vec3 N = normalize(fs_norm);

	float roughness = texture(compressedSampler, vec3(fragIn.uv, 2.0)).r;
	//roughness += 
	//roughness = clamp(roughness , 0.1f, 0.99f);//fix visual artifact when roughness is 1.0
	//roughness = materialParam.x;
	float metallic = texture(compressedSampler, vec3(fragIn.uv, 3.0)).r;

	mat3 LTCMat = LTCMatrix(V, N, roughness);
	vec2 fresnelWeight = GetFrenselTerm(V,N,roughness);
	float F0 = Fresnel(V,N,metallic,roughness);
	F0 = 1;
	mat3 I = mat3(
		1,0,0,
		0,1,0,
		0,0,1
	);
	
	bool doubleSide = false;

	// Multilight Integration
	for(int i = 0;i< lightCount; ++i){
		LightInfo lightInfo = lightInfos[i];
		bool clipBySphere = (lightInfo.boundSphere.w > 0) && (dot(pos - lightInfo.boundSphere.xyz, pos - lightInfo.boundSphere.xyz) > (lightInfo.boundSphere.w * lightInfo.boundSphere.w));
		if(clipBySphere)continue;
		if(lightInfo.lightType == 0){
			//polygon
			float lod;
		    vec2 ltuv;

			float d = IntegrateD(LTCMat, V, N, pos,lightInfo, doubleSide, ltuv, lod) * lightInfo.amplitude;
			//float F = max(fresnelWeight.x, 0.001);
			//vec3 spec = vec3(d * (F + fresnelWeight.y/F - fresnelWeight.y));
			vec3 spec = vec3(F0 * fresnelWeight.x + (1 - F0) * fresnelWeight.y) * d;
			//apply light texture
			spec *= mix(texture(lightAtlasTexture,vec3(ltuv,ceil(lod))).xyz, texture(lightAtlasTexture,vec3(ltuv,floor(lod))).xyz, ceil(lod) - lod);
			spec = max(spec, 0.f);

			d = IntegrateD(I, V, N, pos, lightInfo, doubleSide, ltuv, lod) * lightInfo.amplitude;
			//apply light texture
			vec3 diffuse = d * mix(texture(lightAtlasTexture,vec3(ltuv,ceil(lod))).xyz, texture(lightAtlasTexture,vec3(ltuv,floor(lod))).xyz, ceil(lod) - lod);
			diffuse = max(diffuse, vec3(0.f)) * (1.f - metallic);

			fs_Color += vec4(mix(diffuse, spec, F0), 0.f);
			//fs_Color += vec4(spec,0.f);
		}else{
			float lod;
		    vec2 ltuv;
			
			float d = IntegrateBezierD(LTCMat, V, N, pos, roughness, lightInfo, doubleSide, ltuv, lod) * lightInfo.amplitude;
			vec3 spec = vec3(F0 * fresnelWeight.x + (1 - F0) * fresnelWeight.y) * d;
			//apply texture
			spec *= mix(texture(lightAtlasTexture,vec3(ltuv,ceil(lod))).xyz, texture(lightAtlasTexture,vec3(ltuv,floor(lod))).xyz, ceil(lod) - lod);
			spec = max(spec, 0.f);

			d = IntegrateBezierD(I, V, N, pos, roughness, lightInfo, doubleSide, ltuv, lod) * lightInfo.amplitude;
			//apply light texture
			vec3 diffuse = d * mix(texture(lightAtlasTexture,vec3(ltuv,ceil(lod))).xyz, texture(lightAtlasTexture,vec3(ltuv,floor(lod))).xyz, ceil(lod) - lod);
			diffuse = max(diffuse, vec3(0.f)) * (1.f - metallic);

			fs_Color += vec4(mix(diffuse, spec, F0), 0.f);
		}
	}
	 // fs_Color.xyz = N;
	// fs_Color.xyz = vec3(dot(V, N));
	fs_Color += 0.1;
	fs_Color *= albedo;
	fs_Color = (fs_Color) / (fs_Color + 1.f);

	fs_Color.a = albedo.a;
	//fs_Color * 1.1f;
	// fs_Color = clamp(fs_Color, vec4(0.f), vec4(1.f));
}
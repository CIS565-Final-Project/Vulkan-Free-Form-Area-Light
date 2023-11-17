#version 450

//#extension GL_KHR_vulkan_glsl:require

#define INV_PI 0.31830988618f
#define LUT_SIZE 64.f
#define MAX_LIGHT_VERTEX 5
#define MAX_BEZIER_CURVE 5

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
	
	//in ltc fit the result is transposed
	mat3 res = mat3(
		vec3(1,0,ltcVal.y),
		vec3(0,ltcVal.z,0),
		vec3(ltcVal.w,0,ltcVal.x)
	);
	return res;
}
//to tangent space
mat3 BitMatrix(vec3 V, vec3 N){
	vec3 tangent = normalize(V - N * dot(V,N));
	vec3 bitangent = cross(N,tangent);
	return transpose(mat3(tangent,bitangent,N));
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
void SolveCubic(const float a, const float b, const float c, const float d
, out int realSolCnt, out float sol[3]){
	vec4 Coefficient = vec4(d,c,b,a);
	Coefficient.xyz /= Coefficient.w;
	Coefficient.yz /= 3.f;
	vec3 Delta = vec3(
		(Coefficient.z * (-Coefficient.z) + Coefficient.y),
		(Coefficient.z * (-Coefficient.y) + Coefficient.x),
		dot(vec2(Coefficient.z, -Coefficient.y),Coefficient.xy)
	);
	float Discriminant = dot(vec2(4.f * Delta.x,-Delta.y),Delta.zy);
	vec2 Depressed = vec2(
		(Delta.x * (-2.f * Coefficient.z) + Delta.y),
		Delta.x
	);
	realSolCnt = 0;
	if(Discriminant > 0.f){
		float Theta = atan(sqrt(Discriminant),-Depressed.x)/3.f;
		vec2 CubicRoot = vec2(cos(Theta),sin(Theta));
		vec3 Root = vec3(
			CubicRoot.x,
			dot(vec2(-0.5, -0.5 * sqrt(3.f)), CubicRoot),
			dot(vec2(-0.5,  0.5 * sqrt(3.f)), CubicRoot)
		);
		vec3 tt = sort3(Root * 2.f * sqrt(-Depressed.y) - Coefficient.z);
		if(between01(tt.z)){sol[realSolCnt++] = tt.z;}
		if(between01(tt.y)){sol[realSolCnt++] = tt.y;}
		if(between01(tt.x)){sol[realSolCnt++] = tt.x;}
	}else{
		vec2 tmp = 0.5 * (-Depressed.x + vec2(-1.0,1.0) * sqrt(-Discriminant));
		vec2 pq = sign(tmp) * pow(abs(tmp),vec2(0.33333));
		float t0 = pq.x + pq.y - Coefficient.z;
		if(between01(t0)){sol[realSolCnt++] = t0;}
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
	for(int i = 0; i<4;++i){
		numCtrlUnder += controlPts[i].z < 0.0? 1 : 0;
	}
	intersectCnt = 0;
	if(numCtrlUnder == 0)return true;
	if(numCtrlUnder == 4)return false;//all control points under plane

	const float p0z = controlPts[0].z;
	const float p1z = controlPts[1].z;
	const float p2z = controlPts[2].z;
	const float p3z = controlPts[3].z;

	const float a = -p0z + 3 * p1z - 3 * p2z + p3z;
	const float b = 3 * (p0z - 2 * p1z + p2z);
	const float c = 3 * (-p0z + p1z);
	const float d = p0z;

	SolveCubic(a,b,c,d, intersectCnt, intersectTs);
	if(intersectCnt>0)return true;
	if(BezierCurve(controlPts,0.5).z > 0){
		return true;
	}
	return false;
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

float IntegrateOnHemisphere(vec3 p_i,vec3 p_j){
	float theta = acos(dot(p_i,p_j));
	float proj = normalize(cross(p_i,p_j)).z;
	return theta * proj;
}
float IntegrateEdge(vec3 p_i, vec3 p_j){
	return IntegrateOnHemisphere(normalize(p_i),normalize(p_j));
}
vec2 tStack[12];
float IntegrateBezier(vec3 ctrlPts[4], float tStart, float tEnd, float threshold){
	
	// //if curve is nearly a segment
	// vec3 v01 = ctrlPts[1] - ctrlPts[0];
	// vec3 v02 = ctrlPts[2] - ctrlPts[0];
	// vec3 v03 = ctrlPts[3] - ctrlPts[0];
	// if(abs(dot(v01,v02)) < EPS && abs(dot(v01,v03)) < EPS){
	// 	vec3 v0 = normalize(BezierCurve(ctrlPts, tStart));
	// 	vec3 v1 = normalize(bezierCurve(ctrlPts, tEnd));
	// 	return IntegrateOnHemisphere(v0,v1);
	// }
	float res = 0.0;
	int stackTop = 0;
	tStack[stackTop++] = vec2(tStart,tEnd);
	while(stackTop!=0){
		vec2 tmp = tStack[--stackTop];
		float tMin = tmp.x;
		float tMax = tmp.y;
		float tMid = (tMin + tMax) / 2.f;

		vec3 v0 = normalize(BezierCurve(ctrlPts,tMin));
		vec3 v1 = normalize(BezierCurve(ctrlPts,tMid));
		vec3 v2 = normalize(BezierCurve(ctrlPts,tMax));

		float I01 = IntegrateOnHemisphere(v0,v1);
		float I12 = IntegrateOnHemisphere(v1,v2);
		float I20 = IntegrateOnHemisphere(v2,v0);

		if(abs(I01 + I12 + I20) >= threshold){
			tStack[stackTop++] = vec2(tMin, tMid);
			tStack[stackTop++] = vec2(tMid, tMax);
		}else{
			res += (I01 + I12);
		}
	}
	return res;
}
float IntegrateBezierD(vec3 V, vec3 N, vec3 shadePos, float roughness
	, vec3 allCtrlPts[MAX_BEZIER_CURVE * 4], int bezierNum){
	// to LTC Space
	mat3 worldToLTC = LTCMatrix(V,N,roughness) * BitMatrix(V,N);
	for(int i = 0;i<bezierNum;++i){
		int offset = i << 2;
		allCtrlPts[offset + 0] = worldToLTC * (allCtrlPts[offset + 0] - shadePos);
		allCtrlPts[offset + 1] = worldToLTC * (allCtrlPts[offset + 1] - shadePos);
		allCtrlPts[offset + 2] = worldToLTC * (allCtrlPts[offset + 2] - shadePos);
		allCtrlPts[offset + 3] = worldToLTC * (allCtrlPts[offset + 3] - shadePos);
	}
	//Integrate
	float res = 0.0;
	float threshold = roughness * roughness;
	threshold = threshold * threshold * 0.1;
	bool hasBegin = false;
	vec3 vBegin = vec3(0.f);
	bool hasEnd = false;
	vec3 vEnd = vec3(0.f);
	for(int i = 0;i<bezierNum;++i){
		int offset = i << 2;
		vec3 tmpCtrlPts[4] = vec3[](
			allCtrlPts[offset + 0],
			allCtrlPts[offset + 1],
			allCtrlPts[offset + 2],
			allCtrlPts[offset + 3]
		);
		float ts[3];
		int tCnt = 0;
		//Clip
		if(ClipBezier(tmpCtrlPts, tCnt, ts)){
			//Integrate
			if(tCnt==0){
				res += IntegrateBezier(tmpCtrlPts,0,1,threshold);
			}else{
				float t0,t1,t2;
				switch(tCnt){
					case 1:
					{	
						t0 = ts[0];
						if(BezierCurve(tmpCtrlPts,0.5 * t0).z > 0){
							//start point above plane
							res += IntegrateBezier(tmpCtrlPts,0,t0,threshold);
							hasEnd = true;
							vEnd = BezierCurve(tmpCtrlPts,t0);
						}else{
							//start point below plane
							res += IntegrateBezier(tmpCtrlPts,t0,1,threshold);
							if(hasEnd){
								vec3 v0 = BezierCurve(tmpCtrlPts,t0);
								hasEnd = false;
								res += IntegrateEdge(vEnd,v0);
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
						if(BezierCurve(tmpCtrlPts,(t0+t1)/2.f).z > 0.f){
							//mid point above plane
							//0->t0
							if(hasEnd){
								vec3 v0 = BezierCurve(tmpCtrlPts,t0);
								res += IntegrateEdge(vEnd,v0);
								hasEnd = false;
							}else{
								hasBegin = true;
								vBegin = BezierCurve(tmpCtrlPts,t0);
							}
							//t0->t1
							res += IntegrateBezier(tmpCtrlPts,t0,t1,threshold);
							//t1->1
							hasEnd = true;
							vEnd = BezierCurve(tmpCtrlPts,t1);
						}else{
							//mid point below plane
							//0->t0
							res += IntegrateBezier(tmpCtrlPts,0,t0,threshold);
							//t0->t1
							vec3 v0 = BezierCurve(tmpCtrlPts,t0);
							vec3 v1 = BezierCurve(tmpCtrlPts,t1);
							res += IntegrateEdge(v0,v1);
							//t1->1
							res += IntegrateBezier(tmpCtrlPts,t1,1,threshold);
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
								res += IntegrateEdge(vEnd,v0);
								hasEnd = false;
							}else{
								hasBegin = true;
								vBegin = BezierCurve(tmpCtrlPts,t0);
							}
							//t0->t1
							res += IntegrateBezier(tmpCtrlPts,t0,t1,threshold);
							//t1->t2
							vec3 v1 = BezierCurve(tmpCtrlPts,t1);
							vec3 v2 = BezierCurve(tmpCtrlPts,t2);
							res += IntegrateEdge(v1,v2);
							//t2->1
							res += IntegrateBezier(tmpCtrlPts,t2,1,threshold);
						}else{
							//0->t0
							res += IntegrateBezier(tmpCtrlPts,0,t1,threshold);
							//t0->t1
							vec3 v0 = BezierCurve(tmpCtrlPts,t0);
							vec3 v1 = BezierCurve(tmpCtrlPts,t1);
							res += IntegrateEdge(v0,v1);
							//t1->t2
							res += IntegrateBezier(tmpCtrlPts,t1,t2,threshold);
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
			}
		}else{
			//entire curve below plane, do nothing
		}
	}
	if(hasBegin && hasEnd){
		res += IntegrateEdge(vEnd,vBegin);
	}
	return max(0.0,res);
}

float IntegrateD(vec3 V, vec3 N, vec3 shadePos, float roughness
	, vec3 lightVertex[MAX_LIGHT_VERTEX], int arrSize){
	mat3 worldToLTC = LTCMatrix(V,N,roughness) * BitMatrix(V,N);
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

float halfWidth = 1.5f;
vec3 lights[5] = vec3[](
	vec3(-halfWidth, halfWidth + 2.0f, -5.0f),
	vec3(halfWidth, halfWidth + 2.0f, -5.0f ),
	vec3(halfWidth, -halfWidth + 2.0f, -5.0f ),
	vec3(-halfWidth, -halfWidth + 2.0f, -5.0f),
	vec3(0.f)
);

void main()
{
	vec3 cameraPos = vec3(0,1,10);
	vec3 fs_norm = vec3(0,1,0);
	vec3 V = normalize(cameraPos - fs_Pos);
	vec3 N = normalize(fs_norm);
	//float roughness = fs_roughness;
	float roughness = 0;
	float ItgD = IntegrateD(V, N, fs_Pos, roughness, lights, 4);
	outColor = vec4(vec3(ItgD),1);
}
#pragma once
#define N 64
// -----------------------------------------------------------
// scene.h
// Simple test scene for ray tracing experiments. Goals:
// - Super-fast scene intersection
// - Easy interface: scene.FindNearest / IsOccluded
// - With normals and albedo: GetNormal / GetAlbedo
// - Area light source (animated), for light transport
// - Primitives can be hit from inside - for dielectrics
// - Can be extended with other primitives and/or a BVH
// - Optionally animated - for temporal experiments
// - Not everything is axis aligned - for cache experiments
// - Can be evaluated at arbitrary time - for motion blur
// - Has some high-frequency details - for filtering
// Some speed tricks that severely affect maintainability
// are enclosed in #ifdef SPEEDTRIX / #endif. Mind these
// if you plan to alter the scene in any way.
// -----------------------------------------------------------

#define SPEEDTRIX

#define PLANE_X(o,i) {if((t=-(ray.O.x+o)*ray.rD.x)<ray.t)ray.t=t,ray.objIdx=i;}
#define PLANE_Y(o,i) {if((t=-(ray.O.y+o)*ray.rD.y)<ray.t)ray.t=t,ray.objIdx=i;}
#define PLANE_Z(o,i) {if((t=-(ray.O.z+o)*ray.rD.z)<ray.t)ray.t=t,ray.objIdx=i;}

namespace Tmpl8 {
	// -----------------------------------------------------------
	// Scene class
	// We intersect this. The query is internally forwarded to the
	// list of primitives, so that the nearest hit can be returned.
	// For this hit (distance, obj id), we can query the normal and
	// albedo.
	// -----------------------------------------------------------
	class TriScene
	{
	public:
		TriScene()
		{
			triangle = Triangle(10, new float3[3]{
				float3(-0.5f, -0.5f, 0),
				float3(0, 0.5f, 0),
				float3(0.5, -0.5f, 0)
				});
			SetTime(0);
		}
		void SetTime(float t)
		{
		}
		float3 GetLightPos() const
		{
			return float3(0, 0, 0);
		}
		float3 GetRandomPointOnLight() const
		{
			return float3(0, 0, 0);
			/*
			float randX = Rand(1.0f) - 0.5f;
			float randY = Rand(1.0f) - 0.5f;
			return TransformPosition(float3(randX, 0, randY), quad.T) - float3(0, 0.01f, 0);
			*/
		}
		float3 GetLightColor() const
		{
			return float3(2, 2, 1.6);
		}
		void FindNearest(Ray& ray) const
		{
		}
		bool IsOccluded(Ray& ray) const
		{
		}
		float3 GetNormal(int objIdx, float3 I, float3 wo) const
		{

		}
		Material GetMaterial(int objIdx) const
		{

		}

		float3 GetAlbedo(int objIdx, float3 I) const
		{

		}
		__declspec(align(64)) // start a new cacheline here
			float animTime = 0;
	};

	Triangle[44]
}
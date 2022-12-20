#pragma once
namespace Tmpl8 {
	struct BVHNode
	{
		float3 aabbMin, aabbMax;
		uint leftNode, firstPrimIdx, primCount;
	};

	struct AABB
	{
		float3 bmin = 1e30f, bmax = -1e30f;
		void grow(float3 p) { bmin = fminf(bmin, p), bmax = fmaxf(bmax, p); }
		float area()
		{
			float3 e = bmax - bmin; // box extent
			return e.x * e.y + e.y * e.z + e.z * e.x;
		}
	};
}
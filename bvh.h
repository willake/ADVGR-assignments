#pragma once
namespace Tmpl8 {
	struct BVHNode
	{
		float3 aabbMin, aabbMax;
		uint leftNode, firstTriIdx, triCount;
		bool isLeaf() { return triCount > 0; }
	};
}
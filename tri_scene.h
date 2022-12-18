#pragma once
#define TriN 64

namespace Tmpl8 {
	struct Tri { float3 vertex0, vertex1, vertex2; float3 centroid; };

	class TriScene
	{
	public:
		TriScene()
		{
			for (int i = 0; i < TriN; i++)
			{
				float3 r0(RandomFloat(), RandomFloat(), RandomFloat());
				float3 r1(RandomFloat(), RandomFloat(), RandomFloat());
				float3 r2(RandomFloat(), RandomFloat(), RandomFloat());
				tri[i].vertex0 = r0 * 9 - float3(5);
				tri[i].vertex1 = tri[i].vertex0 + r1;
				tri[i].vertex2 = tri[i].vertex0 + r2;
			}
			SetTime(0);
			BuildBVH();

			whiteMaterial = Material();
		}
		void SetTime(float t)
		{
		}

		void BuildBVH()
		{
			// populate triangle index array
			for (int i = 0; i < TriN; i++) triIdx[i] = i;

			for (int i = 0; i < TriN; i++) tri[i].centroid =
				(tri[i].vertex0 + tri[i].vertex1 + tri[i].vertex2) * 0.3333f;
			// assign all triangles to root node
			BVHNode& root = bvhNode[rootNodeIdx];
			root.leftNode = 0;
			root.firstTriIdx = 0, root.triCount = TriN;
			UpdateNodeBounds(rootNodeIdx);
			// subdivide recursively
			Subdivide(rootNodeIdx);
		}

		void UpdateNodeBounds(uint nodeIdx)
		{
			BVHNode& node = bvhNode[nodeIdx];
			node.aabbMin = float3(1e30f);
			node.aabbMax = float3(-1e30f);
			for (uint first = node.firstTriIdx, i = 0; i < node.triCount; i++)
			{
				uint leafTriIdx = triIdx[first + i];
				Tri& leafTri = tri[leafTriIdx];
				node.aabbMin = fminf(node.aabbMin, leafTri.vertex0),
					node.aabbMin = fminf(node.aabbMin, leafTri.vertex1),
					node.aabbMin = fminf(node.aabbMin, leafTri.vertex2),
					node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex0),
					node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex1),
					node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex2);
			}
		}

		void Subdivide(uint nodeIdx)
		{
			// terminate recursion
			BVHNode& node = bvhNode[nodeIdx];
			if (node.triCount <= 2) return;
			// determine split axis and position
			float3 extent = node.aabbMax - node.aabbMin;
			int axis = 0;
			if (extent.y > extent.x) axis = 1;
			if (extent.z > extent[axis]) axis = 2;
			float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
			// in-place partition
			int i = node.firstTriIdx;
			int j = i + node.triCount - 1;
			while (i <= j)
			{
				if (tri[triIdx[i]].centroid[axis] < splitPos)
					i++;
				else
					swap(triIdx[i], triIdx[j--]);
			}
			// abort split if one of the sides is empty
			int leftCount = i - node.firstTriIdx;
			if (leftCount == 0 || leftCount == node.triCount) return;
			// create child nodes
			int leftChildIdx = nodesUsed++;
			int rightChildIdx = nodesUsed++;
			node.leftNode = leftChildIdx;
			bvhNode[leftChildIdx].firstTriIdx = node.firstTriIdx;
			bvhNode[leftChildIdx].triCount = leftCount;
			bvhNode[rightChildIdx].firstTriIdx = i;
			bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
			node.triCount = 0;
			UpdateNodeBounds(leftChildIdx);
			UpdateNodeBounds(rightChildIdx);
			// recurse
			Subdivide(leftChildIdx);
			Subdivide(rightChildIdx);
		}

		float3 GetNormalTri(const Tri& tri) const
		{
			const float3 edge1 = tri.vertex1 - tri.vertex0;
			const float3 edge2 = tri.vertex2 - tri.vertex0;
			float3 n = cross(edge1, edge2);  //this is the triangle's normal 
			return normalize(n);
		}

		void FindNearest(Ray& ray)
		{
			IntersectBVH(ray, rootNodeIdx);
		}

		void IntersectBVH(Ray& ray, const uint nodeIdx)
		{
			BVHNode& node = bvhNode[nodeIdx];
			if (!IntersectAABB(ray, node.aabbMin, node.aabbMax)) return;
			if (node.isLeaf())
			{
				for (uint i = 0; i < node.triCount; i++)
				{
					if (IntersectTri(ray, tri[triIdx[node.firstTriIdx + i]]))
						ray.objIdx = triIdx[node.firstTriIdx + i];
				}
			}
			else
			{
				IntersectBVH(ray, node.leftNode);
				IntersectBVH(ray, node.leftNode + 1);
			}
		}

		bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax)
		{
			float tx1 = (bmin.x - ray.O.x) / ray.D.x, tx2 = (bmax.x - ray.O.x) / ray.D.x;
			float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
			float ty1 = (bmin.y - ray.O.y) / ray.D.y, ty2 = (bmax.y - ray.O.y) / ray.D.y;
			tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
			float tz1 = (bmin.z - ray.O.z) / ray.D.z, tz2 = (bmax.z - ray.O.z) / ray.D.z;
			tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
			return tmax >= tmin && tmin < ray.t&& tmax > 0;
		}

		bool IntersectTri(Ray& ray, const Tri& tri)
		{
			const float3 edge1 = tri.vertex1 - tri.vertex0;
			const float3 edge2 = tri.vertex2 - tri.vertex0;
			const float3 h = cross(ray.D, edge2);
			const float a = dot(edge1, h);
			if (a > -0.0001f && a < 0.0001f) return false; // ray parallel to triangle
			const float f = 1 / a;
			const float3 s = ray.O - tri.vertex0;
			const float u = f * dot(s, h);
			if (u < 0 || u > 1) return false;
			const float3 q = cross(s, edge1);
			const float v = f * dot(ray.D, q);
			if (v < 0 || u + v > 1) return false;
			const float t = f * dot(edge2, q);
			if (t < FLT_EPSILON) return false;

			ray.t = min(ray.t, t);
			return true;
			
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

		bool IsOccluded(Ray& ray) const
		{
		}
		float3 GetNormal(int objIdx, float3 I, float3 wo) const
		{
			return GetNormalTri(tri[objIdx]);
		}
		Material GetMaterial(int objIdx) const
		{
			return whiteMaterial;
		}

		float3 GetAlbedo(int objIdx, float3 I) const
		{
			return whiteMaterial.GetAlbedo(I);
		}
		__declspec(align(64)) // start a new cacheline here
			float animTime = 0;
		
		Tri tri[TriN];
		uint triIdx[TriN];
		BVHNode bvhNode[TriN * 2 - 1];
		uint rootNodeIdx = 0, nodesUsed = 1;
		Material whiteMaterial;
};
}
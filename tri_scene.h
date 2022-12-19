#pragma once
#define TriN 100

namespace Tmpl8 {
	struct Tri { float3 vertex0, vertex1, vertex2; float3 centroid; };

	class TriScene
	{
	public:
		TriScene()
		{
			FILE* file = fopen("assets/unity.tri", "r");
			float a, b, c, d, e, f, g, h, i;
			for (int t = 0; t < TriN; t++)
			{
				fscanf(file, "%f %f %f %f %f %f %f %f %f\n",
					&a, &b, &c, &d, &e, &f, &g, &h, &i);
				tri[t].vertex0 = float3(a, b, c);
				tri[t].vertex1 = float3(d, e, f);
				tri[t].vertex2 = float3(g, h, i);
			}
			fclose(file);
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
			// determine split axis using SAH
			int bestAxis = -1;
			float bestPos = 0, bestCost = 1e30f;
			for (int axis = 0; axis < 3; axis++) for (uint i = 0; i < node.triCount; i++)
			{
				Tri& triangle = tri[triIdx[node.firstTriIdx + i]];
				float candidatePos = triangle.centroid[axis];
				float cost = EvaluateSAH(node, axis, candidatePos);
				if (cost < bestCost)
					bestPos = candidatePos, bestAxis = axis, bestCost = cost;
			}
			int axis = bestAxis;
			float splitPos = bestPos;
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

		float EvaluateSAH(BVHNode& node, int axis, float pos)
		{
			// determine triangle counts and bounds for this split candidate
			aabb leftBox, rightBox;
			int leftCount = 0, rightCount = 0;
			for (uint i = 0; i < node.triCount; i++)
			{
				Tri& triangle = tri[triIdx[node.firstTriIdx + i]];
				if (triangle.centroid[axis] < pos)
				{
					leftCount++;
					leftBox.grow(triangle.vertex0);
					leftBox.grow(triangle.vertex1);
					leftBox.grow(triangle.vertex2);
				}
				else
				{
					rightCount++;
					rightBox.grow(triangle.vertex0);
					rightBox.grow(triangle.vertex1);
					rightBox.grow(triangle.vertex2);
				}
			}
			float cost = leftCount * leftBox.area() + rightCount * rightBox.area();
			return cost > 0 ? cost : 1e30f;
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

		void IntersectBVH(Ray& ray)
		{
			BVHNode* node = &bvhNode[rootNodeIdx], * stack[64];
			uint stackPtr = 0;
			while (1)
			{
				if (node->isLeaf())
				{
					for (uint i = 0; i < node->triCount; i++)
					{
						if (IntersectTri(ray, tri[triIdx[node->firstTriIdx + i]]))
							ray.objIdx = triIdx[node->firstTriIdx + i];
					}
					if (stackPtr == 0) break; else node = stack[--stackPtr];
					continue;
				}
				BVHNode* child1 = &bvhNode[node->firstTriIdx];
				BVHNode* child2 = &bvhNode[node->firstTriIdx + 1];
				float dist1 = IntersectAABB(ray, child1->aabbMin, child1->aabbMax);
				float dist2 = IntersectAABB(ray, child2->aabbMin, child2->aabbMax);
				if (dist1 > dist2) { swap(dist1, dist2); swap(child1, child2); }
				if (dist1 == 1e30f)
				{
					if (stackPtr == 0) break; else node = stack[--stackPtr];
				}
				else
				{
					node = child1;
					if (dist2 != 1e30f) stack[stackPtr++] = child2;
				}
			}
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

		float IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax)
		{
			float tx1 = (bmin.x - ray.O.x) / ray.D.x, tx2 = (bmax.x - ray.O.x) / ray.D.x;
			float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
			float ty1 = (bmin.y - ray.O.y) / ray.D.y, ty2 = (bmax.y - ray.O.y) / ray.D.y;
			tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
			float tz1 = (bmin.z - ray.O.z) / ray.D.z, tz2 = (bmax.z - ray.O.z) / ray.D.z;
			tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
			if (tmax >= tmin && tmin < ray.t && tmax > 0) return tmin; else return 1e30f;
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
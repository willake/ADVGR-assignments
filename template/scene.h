#pragma once

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
class Scene
{
public:
	Scene()
	{
		// we store all primitives in one continuous buffer
		gameObjects[0] = PrimitiveFactory::GenerateQuad(0, 1, 1); // 0: light source
		gameObjects[1] = PrimitiveFactory::GenerateSphere(1, 3, 0.5f); // 1: bouncing ball
		gameObjects[2] = PrimitiveFactory::GenerateSphere(2, 3, 1);
		//gameObjects[2] = PrimitiveFactory::GenerateCube(2, 4, float3(0), float3(1.15f)); // 3: cube
		gameObjects[3] = PrimitiveFactory::GenerateQuad(3, 5, 20, mat4::Translate(-7, 0, 1) * mat4::RotateZ(PI / 2)); // left wall
		gameObjects[4] = PrimitiveFactory::GenerateQuad(4, 5, 20, mat4::Translate(7, 0, 0) * mat4::RotateZ(-PI / 2)); // right wall
		gameObjects[5] = PrimitiveFactory::GenerateQuad(5, 6, 20, mat4::Translate(0, -1, 0)); // floor
		gameObjects[6] = PrimitiveFactory::GenerateQuad(6, 5, 20, mat4::Translate(0, 4, 0)); // roof
		gameObjects[7] = PrimitiveFactory::GenerateQuad(7, 5, 20, mat4::Translate(0, 0, -7) * mat4::RotateX(PI / 2)); // back wall
		gameObjects[8] = PrimitiveFactory::GenerateQuad(8, 5, 20, mat4::Translate(0, 0, 7) * mat4::RotateX(-PI / 2)); // front wall
		for (int i = 9; i < 9 + 20; i++)
		{
			mat4 T = mat4::Translate(float3(
				RandomFloat() * 10 - 5, RandomFloat() * 3, RandomFloat() * 10 - 5
			)) * mat4::RotateX(RandomFloat() * PI) * mat4::RotateY(RandomFloat() * PI) * mat4::RotateZ(RandomFloat() * PI);
			gameObjects[i] = PrimitiveFactory::GenerateTriangle(i, 2, float3(-0.2f, -0.2f, 0), float3(0, 0.2f, 0), float3(0.5, -0.2f, 0), T);
		}
		for (int i = 29; i < 29 + 10; i++)
		{
			mat4 T = mat4::Translate(float3(
				RandomFloat() * 10 - 5, RandomFloat() * 3, RandomFloat() * 10 - 5
			));
			int matIdx = 8 + (int)floor(RandomFloat() * 3.99);
			gameObjects[i] = PrimitiveFactory::GenerateSphere(i, matIdx, 0.2f, T);
		}
		SetTime( 0 );
		// error material
		materials[0].color = float3(240 / 255.f, 98 / 255.f, 146 / 255.f);
		// light material
		materials[1].isLight = true; // light material
		// white material
		materials[2].color = float3(1);
		// ball material
		materials[3].isMirror = true;
		materials[3].color = float3(233 / 255.f, 199 / 255.f, 199 / 255.f);
		// cube material
		materials[4].isGlass = true;
		materials[4].color = float3(103 / 255.f, 137 / 255.f, 131 / 255.f);
		// plane material
		materials[5].color = float3(.8f);
		// floor material
		materials[6].solverId = 1;
		materials[6].isMirror = true;
		materials[6].reflectivity = 0.3f;
		// back wall material
		materials[7].solverId = 2;
		// white material
		materials[8].color = float3(255 / 255.0f, 212 / 255.0f, 178 / 255.0f);
		// white material
		materials[9].color = float3(255 / 255.0f, 246 / 255.0f, 189 / 255.0f);
		// white material
		materials[10].color = float3(206 / 255.0f, 237 / 255.0f, 199 / 255.0f);
		// white material
		materials[11].color = float3(134 / 255.0f, 200 / 255.0f, 188 / 255.0f);
		// Note: once we have triangle support we should get rid of the class
		// hierarchy: virtuals reduce performance somewhat.

		for (int i = 0; i < size(gameObjects); i++) gameObjectsIdx[i] = i;

		BuildBVH();
	}

	void SetTime( float t )
	{
		// default time for the scene is simply 0. Updating/ the time per frame 
		// enables animation. Updating it per ray can be used for motion blur.
		animTime = t;
		// light source animation: swing
		mat4 M1base = mat4::Translate( float3( 0, 4.6f, 2 ) );
		mat4 M1 = M1base * mat4::RotateZ( sinf( animTime * 0.6f ) * 0.1f ) * mat4::Translate( float3( 0, -0.9, 0 ) );
		gameObjects[0].T = M1, gameObjects[0].invT = M1.FastInvertedTransformNoScale();
		// cube animation: spin
		//mat4 M2base = mat4::RotateX( PI / 4 ) * mat4::RotateZ( PI / 4 );
		//mat4 M2 = mat4::Translate( float3( 1.4f, 0, 2 ) ) * mat4::RotateY( animTime * 0.5f ) * M2base;
		mat4 M2 = mat4::Translate(float3(1.4f, 0, 2));
		gameObjects[2].T = M2, gameObjects[2].invT = M2.FastInvertedTransformNoScale();
		// sphere animation: bounce
		float tm = 1 - sqrf( fmodf( animTime, 2.0f ) - 1 );
		mat4 M3base = mat4::Translate(float3(-1.4f, -0.5f, 2) );
		mat4 M3 = M3base * mat4::Translate(0, tm, 0);
		gameObjects[1].T = M3, gameObjects[1].invT = M3.FastInvertedTransformNoScale();
	}

	void BuildBVH()
	{
		BVHNode& root = bvhNode[rootNodeIdx];
		root.leftNode = 0;
		root.firstPrimIdx = 0, root.primCount = size(gameObjects);
		UpdateNodeBounds(rootNodeIdx);
		// subdivide recursively
		Subdivide(rootNodeIdx);
		
	}

	void Subdivide(uint nodeIdx)
	{
		// terminate recursion
		BVHNode& node = bvhNode[nodeIdx];
		if (node.primCount <= 2) return;

		// determine split axis using SAH
		int bestAxis = -1;
		float bestPos = 0, bestCost = 1e30f;
		for (int axis = 0; axis < 3; axis++) for (uint i = 0; i < node.primCount; i++)
		{
			Primitive& primitive = gameObjects[gameObjectsIdx[node.firstPrimIdx + i]];
			float candidatePos = TransformPosition(float3(0), primitive.T).cell[axis];
			float cost = EvaluateSAH(node, axis, candidatePos);
			if (cost < bestCost)
				bestPos = candidatePos, bestAxis = axis, bestCost = cost;
		}

		float3 e = node.aabbMax - node.aabbMin; // extent of parent
		float parentArea = e.x * e.y + e.y * e.z + e.z * e.x;
		float parentCost = node.primCount * parentArea;

		if (bestCost >= parentCost) return;

		int axis = bestAxis;
		float splitPos = bestPos;
		// in-place partition
		int i = node.firstPrimIdx;
		int j = i + node.primCount - 1;
		while (i <= j)
		{
			float3 pos = TransformPosition(float3(0), gameObjects[gameObjectsIdx[i]].T);
			if (pos[axis] < splitPos)
				i++;
			else
				swap(gameObjectsIdx[i], gameObjectsIdx[j--]);
		}
		// abort split if one of the sides is empty
		int leftCount = i - node.firstPrimIdx;
		if (leftCount == 0 || leftCount == node.primCount) return;
		// create child nodes
		int leftChildIdx = nodesUsed++;
		int rightChildIdx = nodesUsed++;
		node.leftNode = leftChildIdx;
		bvhNode[leftChildIdx].firstPrimIdx = node.firstPrimIdx;
		bvhNode[leftChildIdx].primCount = leftCount;
		bvhNode[rightChildIdx].firstPrimIdx = i;
		bvhNode[rightChildIdx].primCount = node.primCount - leftCount;
		node.primCount = 0;
		UpdateNodeBounds(leftChildIdx);
		UpdateNodeBounds(rightChildIdx);
		// recurse
		Subdivide(leftChildIdx);
		Subdivide(rightChildIdx);
	}

	float EvaluateSAH(BVHNode& node, int axis, float pos)
	{
		// determine triangle counts and bounds for this split candidate
		AABB leftBox, rightBox;
		int leftCount = 0, rightCount = 0;
		for (uint i = 0; i < node.primCount; i++)
		{
			Primitive& primitive = gameObjects[gameObjectsIdx[node.firstPrimIdx + i]];
			float3 candidatePos = TransformPosition(float3(0), primitive.T);
			if (candidatePos.cell[axis] < pos)
			{
				leftCount++;
				AABB bounds = PrimitiveUtils::GetBounds(primitive);
				leftBox.grow(bounds.bmin);
				leftBox.grow(bounds.bmax);
			}
			else
			{
				rightCount++;
				AABB bounds = PrimitiveUtils::GetBounds(primitive);
				rightBox.grow(bounds.bmin);
				rightBox.grow(bounds.bmax);
			}
		}
		float cost = leftCount * leftBox.area() + rightCount * rightBox.area();
		return cost > 0 ? cost : 1e30f;
	}

	void UpdateNodeBounds(uint nodeIdx)
	{
		BVHNode& node = bvhNode[nodeIdx];
		node.aabbMin = float3(1e30f);
		node.aabbMax = float3(-1e30f);
		for (uint first = node.firstPrimIdx, i = 0; i < node.primCount; i++)
		{
			uint leafTriIdx = gameObjectsIdx[first + i];
			Primitive& leafPrim = gameObjects[leafTriIdx];
			AABB bounds = PrimitiveUtils::GetBounds(leafPrim);
			node.aabbMin = fminf(node.aabbMin, bounds.bmin);
			node.aabbMax = fmaxf(node.aabbMax, bounds.bmax);
		}
	}

	void FindNearest(Ray& ray)
	{
		float t;
		
		/*
		for (int i = 0; i < size(gameObjects); i++)
		{
			PrimitiveUtils::Intersect(gameObjects[i], ray);
		}*/

		IntersectBVH(ray, rootNodeIdx);
	}

	bool IsOccluded(Ray& ray)
	{
		float rayLength = ray.t;
		/*
		for (int i = 0; i < size(gameObjects); i++)
		{
			PrimitiveUtils::Intersect(gameObjects[i], ray);
		}*/
		IntersectBVH(ray, rootNodeIdx, true);
		return ray.t < rayLength;
	}

	void IntersectBVH(Ray& ray, const uint nodeIdx, bool shadowRay = false)
	{
		BVHNode& node = bvhNode[nodeIdx];
		if (!IntersectAABB(ray, node.aabbMin, node.aabbMax)) return;
		if (node.primCount > 0)
		{
			if (shadowRay) 
			{
				ray.t = 0;  
				return;
			}

			for (uint i = 0; i < node.primCount; i++)
			{
				Primitive& p = gameObjects[gameObjectsIdx[node.firstPrimIdx + i]];
				PrimitiveUtils::Intersect(p, ray);
			}
		}
		else
		{
			if (node.leftNode == 0) return;
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

	void IntersectLight(Ray& ray)
	{
		PrimitiveUtils::Intersect(gameObjects[0], ray);
	}

	float3 GetLightPos() const
	{
		// light point position is the middle of the swinging quad
		float3 corner1 = TransformPosition( float3( -0.5f, 0, -0.5f ), gameObjects[0].T);
		float3 corner2 = TransformPosition( float3( 0.5f, 0, 0.5f ), gameObjects[0].T );
		return (corner1 + corner2) * 0.5f - float3( 0, 0.01f, 0 );
	}

	float3 GetRandomPointOnLight() const
	{
		float randX = Rand(1.0f) - 0.5f;
		float randY = Rand(1.0f) - 0.5f;
		return TransformPosition(float3(randX, 0, randY), gameObjects[0].T) - float3(0, 0.01f, 0);
	}

	float3 GetLightColor() const
	{
		return float3( 16, 16, 12.4 );
	}

	float3 GetNormal( int objIdx, float3 I, float3 wo )
	{
		// we get the normal after finding the nearest intersection:
		// this way we prevent calculating it multiple times.
		
		if (objIdx == -1) return float3(0);
		float3 N = PrimitiveUtils::GetNormal(gameObjects[objIdx], I);
		if (dot( N, wo ) > 0) N = -N; // hit backside / inside
		return N;
	}
	Material GetMaterial( int objIdx ) const
	{
		if (objIdx == -1) return materials[0]; // or perhaps we should just crash
		
		int matIdx = gameObjects[objIdx].matIdx;
		return materials[matIdx];
	}

	float3 GetAlbedo( int objIdx, float3 I ) const
	{
		Material mat = materials[0];
		if (objIdx == -1) return MaterialUtils::GetAlbedo(mat, I); // or perhaps we should just crash
		int matIdx = gameObjects[objIdx].matIdx;
		mat = materials[matIdx];

		return MaterialUtils::GetAlbedo(mat, I);
		// once we have triangle support, we should pass objIdx and the bary-
		// centric coordinates of the hit, instead of the intersection location.
	}
	__declspec(align(64)) // start a new cacheline here
	float animTime = 0;
	Primitive gameObjects[39];
	Material materials[12];
	BVHNode bvhNode[39 * 2 -1];
	uint gameObjectsIdx[39];
	uint rootNodeIdx = 0, nodesUsed = 1;
};

}
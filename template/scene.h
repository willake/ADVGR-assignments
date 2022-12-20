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
		gameObjects[0] = PrimitiveFactory::GenerateQuad(0, 1); // 0: light source
		gameObjects[1] = PrimitiveFactory::GenerateSphere(1, float3(0), 0.5f); // 1: bouncing ball
		gameObjects[2] = PrimitiveFactory::GenerateSphere(2, float3(0, 2.5f, -3.07f), 8); // 2: rounded corners
		gameObjects[3] = PrimitiveFactory::GenerateCube(3, float3(0), float3(1.15f)); // 3: cube
		gameObjects[4] = PrimitiveFactory::GeneratePlane(4, float3(1, 0, 0), 3);
		gameObjects[5] = PrimitiveFactory::GeneratePlane(5, float3(-1, 0, 0), 2.99f);
		gameObjects[6] = PrimitiveFactory::GeneratePlane(6, float3(0, 1, 0), 1);
		gameObjects[7] = PrimitiveFactory::GeneratePlane(7, float3(0, -1, 0), 2);
		gameObjects[8] = PrimitiveFactory::GeneratePlane(8, float3(0, 0, 1), 3);
		gameObjects[9] = PrimitiveFactory::GeneratePlane(9, float3(0, 0, -1), 3.99f);
		gameObjects[10] = PrimitiveFactory::GenerateTriangle(10,
			float3(-0.5f, -0.5f, 0), float3(0, 0.5f, 0), float3(0.5, -0.5f, 0)
		);
		SetTime( 0 );

		lightMaterial = Material();
		lightMaterial.isLight = true;

		errorMaterial = Material(); // for error
		errorMaterial.color = float3(240 / 255.f, 98 / 255.f, 146 / 255.f);

		whiteMaterial = Material();

		ballMaterial = Material();
		ballMaterial.isMirror = true;
		ballMaterial.color = float3(233 / 255.f, 199 / 255.f, 199 / 255.f);

		cubeMaterial = Material();
		cubeMaterial.isGlass = true;
		cubeMaterial.color = float3(103 / 255.f, 137 / 255.f, 131 / 255.f);

		planeMaterial = Material();
		planeMaterial.color = float3(.8f);

		floorMaterial = FloorMaterial();
		floorMaterial.isMirror = true;
		floorMaterial.reflectivity = 0.3f;

		backWallMaterial = BackWallMaterial();
		// Note: once we have triangle support we should get rid of the class
		// hierarchy: virtuals reduce performance somewhat.
	}
	void SetTime( float t )
	{
		// default time for the scene is simply 0. Updating/ the time per frame 
		// enables animation. Updating it per ray can be used for motion blur.
		animTime = t;
		// light source animation: swing
		mat4 M1base = mat4::Translate( float3( 0, 2.6f, 2 ) );
		mat4 M1 = M1base * mat4::RotateZ( sinf( animTime * 0.6f ) * 0.1f ) * mat4::Translate( float3( 0, -0.9, 0 ) );
		gameObjects[0].T = M1, gameObjects[0].invT = M1.FastInvertedTransformNoScale();
		// cube animation: spin
		mat4 M2base = mat4::RotateX( PI / 4 ) * mat4::RotateZ( PI / 4 );
		mat4 M2 = mat4::Translate( float3( 1.4f, 0, 2 ) ) * mat4::RotateY( animTime * 0.5f ) * M2base;
		gameObjects[3].T = M2, gameObjects[3].invT = M2.FastInvertedTransformNoScale();
		// sphere animation: bounce
		float tm = 1 - sqrf( fmodf( animTime, 2.0f ) - 1 );
		gameObjects[1].tri.vertex0 = float3(-1.4f, -0.5f + tm, 2);
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
		return float3( 2, 2, 1.6 );
	}
	void FindNearest( Ray& ray )
	{
		// room walls - ugly shortcut for more speed
		float t;
		if (ray.D.x < 0) PLANE_X( 3, 4 ) else PLANE_X( -2.99f, 5 );
		if (ray.D.y < 0) PLANE_Y( 1, 6 ) else PLANE_Y( -2, 7 );
		if (ray.D.z < 0) PLANE_Z( 3, 8 ) else PLANE_Z( -3.99f, 9 );

		for (int i = 0; i < size(gameObjects); i++)
		{
			PrimitiveUtils::Intersect(gameObjects[i], ray);
		}
	}
	bool IsOccluded( Ray& ray )
	{
		float rayLength = ray.t;
		for (int i = 0; i < size(gameObjects); i++)
		{
			PrimitiveUtils::Intersect(gameObjects[i], ray);
		}
		return ray.t < rayLength;
		// technically this is wasteful: 
		// - we potentially search beyond rayLength
		// - we store objIdx and t when we just need a yes/no
		// - we don't 'early out' after the first occlusion
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
		if (objIdx == -1) return errorMaterial; // or perhaps we should just crash
		if (objIdx == 0) return lightMaterial; // light source
		if (objIdx == 1) return ballMaterial; // bouncing ball
		if (objIdx == 2) return whiteMaterial; // corner
		if (objIdx == 3) return cubeMaterial; // cube
		if (objIdx == 6) return floorMaterial;
		if (objIdx == 9) return backWallMaterial;
		if (objIdx == 10) return whiteMaterial;
		return planeMaterial;
	}

	float3 GetAlbedo( int objIdx, float3 I ) const
	{
		if (objIdx == -1) return errorMaterial.GetAlbedo(I); // or perhaps we should just crash
		if (objIdx == 0) return lightMaterial.GetAlbedo(I); // light source
		if (objIdx == 1) return ballMaterial.GetAlbedo(I); // bouncing ball
		if (objIdx == 2) return whiteMaterial.GetAlbedo(I); // corner
		if (objIdx == 3) return cubeMaterial.GetAlbedo(I); // cube
		if (objIdx == 6) return floorMaterial.GetAlbedo(I);
		if (objIdx == 9) return backWallMaterial.GetAlbedo(I);
		if (objIdx == 10) return whiteMaterial.GetAlbedo(I);
		return planeMaterial.GetAlbedo(I);
		// once we have triangle support, we should pass objIdx and the bary-
		// centric coordinates of the hit, instead of the intersection location.
	}
	__declspec(align(64)) // start a new cacheline here
	float animTime = 0;
	Primitive gameObjects[11];
	Material lightMaterial;
	Material errorMaterial; // for error
	Material whiteMaterial;
	Material ballMaterial;
	Material cubeMaterial;
	Material planeMaterial;
	FloorMaterial floorMaterial;
	BackWallMaterial backWallMaterial;
};

}
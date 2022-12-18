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
		quad = Quad( 0, 1 );									// 0: light source
		sphere = Sphere( 1, float3( 0 ), 0.5f );				// 1: bouncing ball
		sphere2 = Sphere( 2, float3( 0, 2.5f, -3.07f ), 8 );	// 2: rounded corners
		cube = Cube( 3,  float3( 0 ), float3( 1.15f ) );			// 3: cube
		plane[0] = Plane( 4, float3( 1, 0, 0 ), 3 );			// 4: left wall
		plane[1] = Plane( 5, float3( -1, 0, 0 ), 2.99f );		// 5: right wall
		plane[2] = Plane( 6, float3( 0, 1, 0 ), 1 );			// 6: floor
		plane[3] = Plane( 7, float3( 0, -1, 0 ), 2 );			// 7: ceiling
		plane[4] = Plane( 8, float3( 0, 0, 1 ), 3 );			// 8: front wall
		plane[5] = Plane( 9, float3( 0, 0, -1 ), 3.99f );		// 9: back wall
		triangle = Triangle(10, new float3[3]{
			float3(-0.5f, -0.5f, 0),
			float3(0, 0.5f, 0),
			float3(0.5, -0.5f, 0)
		});
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
		cubeMaterial.color = float3(203 / 255.f, 237 / 255.f, 213 / 255.f);

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
		quad.T = M1, quad.invT = M1.FastInvertedTransformNoScale();
		// cube animation: spin
		mat4 M2base = mat4::RotateX( PI / 4 ) * mat4::RotateZ( PI / 4 );
		mat4 M2 = mat4::Translate( float3( 1.4f, 0, 2 ) ) * mat4::RotateY( animTime * 0.5f ) * M2base;
		cube.M = M2, cube.invM = M2.FastInvertedTransformNoScale();
		// sphere animation: bounce
		float tm = 1 - sqrf( fmodf( animTime, 2.0f ) - 1 );
		sphere.pos = float3( -1.4f, -0.5f + tm, 2 );
	}
	float3 GetLightPos() const
	{
		// light point position is the middle of the swinging quad
		float3 corner1 = TransformPosition( float3( -0.5f, 0, -0.5f ), quad.T );
		float3 corner2 = TransformPosition( float3( 0.5f, 0, 0.5f ), quad.T );
		return (corner1 + corner2) * 0.5f - float3( 0, 0.01f, 0 );
	}
	float3 GetRandomPointOnLight() const
	{
		float randX = Rand(1.0f) - 0.5f;
		float randY = Rand(1.0f) - 0.5f;
		return TransformPosition(float3(randX, 0, randY), quad.T) - float3(0, 0.01f, 0);
	}
	float3 GetLightColor() const
	{
		return float3( 4, 4, 3.2 );
	}
	void FindNearest( Ray& ray ) const
	{
		// room walls - ugly shortcut for more speed
		float t;
		if (ray.D.x < 0) PLANE_X( 3, 4 ) else PLANE_X( -2.99f, 5 );
		if (ray.D.y < 0) PLANE_Y( 1, 6 ) else PLANE_Y( -2, 7 );
		if (ray.D.z < 0) PLANE_Z( 3, 8 ) else PLANE_Z( -3.99f, 9 );
		quad.Intersect( ray );
		sphere.Intersect( ray );
		sphere2.Intersect( ray );
		cube.Intersect( ray );
		triangle.Intersect(ray);
	}
	bool IsOccluded( Ray& ray ) const
	{
		float rayLength = ray.t;
		// skip planes: it is not possible for the walls to occlude anything
		quad.Intersect( ray );
		sphere.Intersect( ray );
		sphere2.Intersect( ray );
		cube.Intersect( ray );
		triangle.Intersect(ray);
		return ray.t < rayLength;
		// technically this is wasteful: 
		// - we potentially search beyond rayLength
		// - we store objIdx and t when we just need a yes/no
		// - we don't 'early out' after the first occlusion
	}
	float3 GetNormal( int objIdx, float3 I, float3 wo ) const
	{
		// we get the normal after finding the nearest intersection:
		// this way we prevent calculating it multiple times.
		if (objIdx == -1) return float3( 0 ); // or perhaps we should just crash
		float3 N;
		if (objIdx == 0) N = quad.GetNormal( I );
		else if (objIdx == 1) N = sphere.GetNormal( I );
		else if (objIdx == 2) N = sphere2.GetNormal( I );
		else if (objIdx == 3) N = cube.GetNormal( I );
		else if (objIdx == 10) N = triangle.GetNormal(I);
		else 
		{
			// faster to handle the 6 planes without a call to GetNormal
			N = float3( 0 );
			N[(objIdx - 4) / 2] = 1 - 2 * (float)(objIdx & 1);
		}
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
	Quad quad;
	Sphere sphere;
	Sphere sphere2;
	Cube cube;
	Plane plane[6];
	Triangle triangle;
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
#pragma once
namespace Tmpl8 {
	// -----------------------------------------------------------
// Sphere primitive
// Basic sphere, with explicit support for rays that start
// inside it. Good candidate for a dielectric material.
// -----------------------------------------------------------
	class Sphere
	{
	public:
		Sphere() = default;
		Sphere(int idx, float3 p, float r) :
			pos(p), r2(r* r), invr(1 / r), objIdx(idx) {}
		void Intersect(Ray& ray) const
		{
			float3 oc = ray.O - this->pos;
			float b = dot(oc, ray.D);
			float c = dot(oc, oc) - this->r2;
			float t, d = b * b - c;
			if (d <= 0) return;
			d = sqrtf(d), t = -b - d;
			if (t < ray.t && t > 0)
			{
				ray.t = t, ray.objIdx = objIdx;
				return;
			}
			t = d - b;
			if (t < ray.t && t > 0)
			{
				ray.t = t, ray.objIdx = objIdx;
				return;
			}
		}
		float3 GetNormal(const float3 I) const
		{
			return (I - this->pos) * invr;
		}
		float3 pos = 0;
		float r2 = 0, invr = 0;
		int objIdx = -1;
	};

	// -----------------------------------------------------------
	// Plane primitive
	// Basic infinite plane, defined by a normal and a distance
	// from the origin (in the direction of the normal).
	// -----------------------------------------------------------
	class Plane
	{
	public:
		Plane() = default;
		Plane(int idx, float3 normal, float dist) : N(normal), d(dist), objIdx(idx) {}
		void Intersect(Ray& ray) const
		{
			float t = -(dot(ray.O, this->N) + this->d) / (dot(ray.D, this->N));
			if (t < ray.t && t > 0) ray.t = t, ray.objIdx = objIdx;
		}
		float3 GetNormal(const float3 I) const
		{
			return N;
		}
		float3 N;
		float d;
		int objIdx = -1;
	};

	// -----------------------------------------------------------
	// Cube primitive
	// Oriented cube. Unsure if this will also work for rays that
	// start inside it; maybe not the best candidate for testing
	// dielectrics.
	// -----------------------------------------------------------
	class Cube
	{
	public:
		Cube() = default;
		Cube(int idx, float3 pos, float3 size, mat4 transform = mat4::Identity())
		{
			objIdx = idx;
			b[0] = pos - 0.5f * size, b[1] = pos + 0.5f * size;
			M = transform, invM = transform.FastInvertedTransformNoScale();
		}
		void Intersect(Ray& ray) const
		{
			// 'rotate' the cube by transforming the ray into object space
			// using the inverse of the cube transform.
			float3 O = TransformPosition(ray.O, invM);
			float3 D = TransformVector(ray.D, invM);
			float rDx = 1 / D.x, rDy = 1 / D.y, rDz = 1 / D.z;
			int signx = D.x < 0, signy = D.y < 0, signz = D.z < 0;
			float tmin = (b[signx].x - O.x) * rDx;
			float tmax = (b[1 - signx].x - O.x) * rDx;
			float tymin = (b[signy].y - O.y) * rDy;
			float tymax = (b[1 - signy].y - O.y) * rDy;
			if (tmin > tymax || tymin > tmax) return;
			tmin = max(tmin, tymin), tmax = min(tmax, tymax);
			float tzmin = (b[signz].z - O.z) * rDz;
			float tzmax = (b[1 - signz].z - O.z) * rDz;
			if (tmin > tzmax || tzmin > tmax) return;
			tmin = max(tmin, tzmin), tmax = min(tmax, tzmax);
			if (tmin > 0)
			{
				if (tmin < ray.t) ray.t = tmin, ray.objIdx = objIdx;
			}
			else if (tmax > 0)
			{
				if (tmax < ray.t) ray.t = tmax, ray.objIdx = objIdx;
			}
		}
		float3 GetNormal(const float3 I) const
		{
			// transform intersection point to object space
			float3 objI = TransformPosition(I, invM);
			// determine normal in object space
			float3 N = float3(-1, 0, 0);
			float d0 = fabs(objI.x - b[0].x), d1 = fabs(objI.x - b[1].x);
			float d2 = fabs(objI.y - b[0].y), d3 = fabs(objI.y - b[1].y);
			float d4 = fabs(objI.z - b[0].z), d5 = fabs(objI.z - b[1].z);
			float minDist = d0;
			if (d1 < minDist) minDist = d1, N.x = 1;
			if (d2 < minDist) minDist = d2, N = float3(0, -1, 0);
			if (d3 < minDist) minDist = d3, N = float3(0, 1, 0);
			if (d4 < minDist) minDist = d4, N = float3(0, 0, -1);
			if (d5 < minDist) minDist = d5, N = float3(0, 0, 1);
			// return normal in world space
			return TransformVector(N, M);
		}
		float3 b[2];
		mat4 M, invM;
		int objIdx = -1;
	};

	// -----------------------------------------------------------
	// Quad primitive
	// Oriented quad, intended to be used as a light source.
	// -----------------------------------------------------------
	class Quad
	{
	public:
		Quad() = default;
		Quad(int idx, float s, mat4 transform = mat4::Identity())
		{
			objIdx = idx;
			size = s * 0.5f;
			T = transform, invT = transform.FastInvertedTransformNoScale();
		}
		void Intersect(Ray& ray) const
		{
			const float3 O = TransformPosition(ray.O, invT);
			const float3 D = TransformVector(ray.D, invT);
			const float t = O.y / -D.y;
			if (t < ray.t && t > 0)
			{
				float3 I = O + t * D;
				if (I.x > -size && I.x < size && I.z > -size && I.z < size)
					ray.t = t, ray.objIdx = objIdx;
			}
		}
		float3 GetNormal(const float3 I) const
		{
			// TransformVector( float3( 0, -1, 0 ), T ) 
			return float3(-T.cell[1], -T.cell[5], -T.cell[9]);
		}
		float size;
		mat4 T, invT;
		int objIdx = -1;
	};
	// -----------------------------------------------------------
	// Triangle primitive
	// reference: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
	// -----------------------------------------------------------
	class Triangle
	{
	public:
		Triangle() = default;
		Triangle(int idx, float3 vertices[3], mat4 transform = mat4::Identity())
		{
			objIdx = idx;
			v[0] = vertices[0];
			v[1] = vertices[1];
			v[2] = vertices[2];
			M = transform, invM = transform.FastInvertedTransformNoScale();
		}
		void Intersect(Ray& ray) const
		{
			float3 v0v1 = v[1] - v[0];  //edge 0 
			float3 v0v2 = v[2] - v[0];  //edge 1 
			float3 pvec = cross(ray.D, v0v2);
			float det = dot(v0v1, pvec);

			// check if ray and plane are parallel ?
			if (fabs(det) < FLT_EPSILON) return;

			float invDet = 1 / det;

			float3 tvec = ray.O - v[0];
			float u = dot(tvec, pvec) * invDet;
			if (u < 0 || u > 1) return;

			float3 qvec = cross(tvec, v0v1);
			float v = dot(ray.D, qvec) * invDet;
			if (v < 0 || u + v > 1) return;

			float t = dot(v0v2, qvec) * invDet;
			if (t > FLT_EPSILON)
			{
				ray.t = min(ray.t, t);
				ray.objIdx = objIdx;
			}
		}
		float3 GetNormal(const float3 I) const
		{
			float3 v0v1 = v[1] - v[0];  //edge 0 
			float3 v0v2 = v[2] - v[0];  //edge 1 
			float3 N = cross(v0v1, v0v2);  //this is the triangle's normal 
			return normalize(N);
		}
		float3 v[3];
		mat4 M, invM;
		int objIdx = -1;
	};
}
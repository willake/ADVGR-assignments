#pragma once
namespace Tmpl8 {
	struct Tri { float3 vertex0, vertex1, vertex2; float3 centroid; };

	struct Primitive {
	public:
		int objIdx;
		int matIdx;
		int type; // 0 triangle, 1 shpere, 2 plane, 3 cube, 4 quad
		Tri tri;
		mat4 T, invT;
	};

	class PrimitiveUtils {
	public:
		static AABB GetBounds(Primitive& p)
		{
			switch (p.type)
			{
				case 0:
				default:
					return GetBoundsTriangle(p);
				case 1:
					return GetBoundsSphere(p);
				case 2:
					return GetBoundsPlane(p);
				case 3:
					return GetBoundsCube(p);
				case 4:
					return GetBoundsQuad(p);
			}
		}

		static float3 GetNormal(Primitive& p, float3 I) 
		{
			switch (p.type)
			{
				case 0:
				default:
					return GetNormalTriangle(p);
				case 1:
					return GetNormalSphere(p, I);
				case 2:
					return GetNormalPlane(p);
				case 3:
					return GetNormalCube(p, I);
				case 4:
					return GetNormalQuad(p);
			}
		}

		static void Intersect(Primitive& p, Ray& ray) 
		{
			switch (p.type)
			{
				case 0:
				default:
					IntersectTriangle(p, ray);
					break;
				case 1:
					IntersectSphere(p, ray);
					break;
				case 2:
					IntersectPlane(p, ray);
					break;
				case 3:
					IntersectCube(p, ray);
					break;
				case 4:
					IntersectQuad(p, ray);
					break;
			}
		}

		static inline AABB GetBoundsTriangle(Primitive& p)
		{
			AABB aabb;
			Tri& tri = p.tri;
			aabb.grow(tri.vertex0);
			aabb.grow(tri.vertex1);
			aabb.grow(tri.vertex2);

			return aabb;
		}

		static inline AABB GetBoundsSphere(Primitive& p)
		{
			AABB aabb;
			Tri& tri = p.tri;
			float3 min = tri.vertex0 - float3(tri.vertex1.x);
			float3 max = tri.vertex0 + float3(tri.vertex1.x);
			aabb.grow(min);
			aabb.grow(max);

			return aabb;
		}

		static inline AABB GetBoundsPlane(Primitive& p)
		{
			AABB aabb;

			return aabb;
		}

		static inline AABB GetBoundsCube(Primitive& p)
		{
			AABB aabb;
			Tri& tri = p.tri;

			float3 min = TransformPosition(tri.vertex0, p.T);
			float3 max = TransformPosition(tri.vertex1, p.T);
			aabb.grow(min);
			aabb.grow(max);

			return aabb;
		}

		static inline AABB GetBoundsQuad(Primitive& p)
		{
			AABB aabb;
			Tri& tri = p.tri;
			float s = tri.vertex0.x;
			float3 min = TransformPosition(float3(-s, 0, -s), p.T);
			float3 max = TransformPosition(float3(s, 0, s), p.T);
			aabb.grow(min);
			aabb.grow(max);

			return aabb;
		}

		static inline void IntersectTriangle(Primitive& p, Ray& ray)
		{
			Tri& tri = p.tri;
			float3 v0v1 = tri.vertex1 - tri.vertex0;  //edge 0 
			float3 v0v2 = tri.vertex2 - tri.vertex0;  //edge 1 
			float3 pvec = cross(ray.D, v0v2);
			float det = dot(v0v1, pvec);

			// check if ray and plane are parallel ?
			if (fabs(det) < FLT_EPSILON) return;

			float invDet = 1 / det;

			float3 tvec = ray.O - tri.vertex0;
			float u = dot(tvec, pvec) * invDet;
			if (u < 0 || u > 1) return;

			float3 qvec = cross(tvec, v0v1);
			float v = dot(ray.D, qvec) * invDet;
			if (v < 0 || u + v > 1) return;

			float t = dot(v0v2, qvec) * invDet;
			if (t > FLT_EPSILON)
			{
				ray.t = min(ray.t, t);
				ray.objIdx = p.objIdx;
			}
		}

		static inline void IntersectSphere(Primitive& p, Ray& ray)
		{
			Tri& tri = p.tri;
			float3 pos = TransformPosition(float3(0), p.T);
			float3 oc = ray.O - pos;
			float r2 = tri.vertex0.y;
			float b = dot(oc, ray.D);
			float c = dot(oc, oc) - r2;
			float t, d = b * b - c;
			if (d <= 0) return;
			d = sqrtf(d), t = -b - d;
			if (t < ray.t && t > 0)
			{
				ray.t = t, ray.objIdx = p.objIdx;
				return;
			}
			t = d - b;
			if (t < ray.t && t > 0)
			{
				ray.t = t, ray.objIdx = p.objIdx;
				return;
			}
		}

		static inline void IntersectPlane(Primitive& p, Ray& ray) 
		{
			Tri& tri = p.tri;
			float3 N = tri.vertex0;
			float d = tri.vertex1.x;
			float t = -(dot(ray.O, N) + d) / (dot(ray.D, N));
			if (t < ray.t && t > 0) ray.t = t, ray.objIdx = p.objIdx;
		}

		static inline void IntersectCube(Primitive& p, Ray& ray) 
		{
			Tri& tri = p.tri;
			// 'rotate' the cube by transforming the ray into object space
			// using the inverse of the cube transform.
			float3 O = TransformPosition(ray.O, p.invT);
			float3 D = TransformVector(ray.D, p.invT);
			float3 b[2] = { tri.vertex0, tri.vertex1 };
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
				if (tmin < ray.t) ray.t = tmin, ray.objIdx = p.objIdx;
			}
			else if (tmax > 0)
			{
				if (tmax < ray.t) ray.t = tmax, ray.objIdx = p.objIdx;
			}
		}

		static inline void IntersectQuad(Primitive& p, Ray& ray) 
		{
			Tri& tri = p.tri;
			const float3 O = TransformPosition(ray.O, p.invT);
			const float3 D = TransformVector(ray.D, p.invT);
			const float t = O.y / -D.y;
			const float size = tri.vertex0.x;
			if (t < ray.t && t > 0)
			{
				float3 I = O + t * D;
				if (I.x > -size && I.x < size && I.z > -size && I.z < size)
					ray.t = t, ray.objIdx = p.objIdx;
			}
		}

		static inline float3 GetNormalTriangle(Primitive& p)
		{
			Tri& tri = p.tri;
			float3 v0v1 = tri.vertex1 - tri.vertex0;  //edge 0 
			float3 v0v2 = tri.vertex2 - tri.vertex0;  //edge 1 
			float3 N = cross(v0v1, v0v2);  //this is the triangle's normal 
			return normalize(N);
		}

		static inline float3 GetNormalSphere(Primitive& p, float3 I)
		{
			float3 pos = TransformPosition(float3(0), p.T);
			return (I - pos) * p.tri.vertex0.z;
		}

		static inline float3 GetNormalPlane(Primitive& p)
		{
			return p.tri.vertex0;
		}

		static inline float3 GetNormalCube(Primitive& p, float3 I)
		{
			// transform intersection point to object space
			float3 objI = TransformPosition(I, p.invT);
			// determine normal in object space
			float3 N = float3(-1, 0, 0);
			float3 b[2] = { p.tri.vertex0, p.tri.vertex1 };
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
			return TransformVector(N, p.T);
		}

		static inline float3 GetNormalQuad(Primitive& p)
		{
			return float3(-p.T.cell[1], -p.T.cell[5], -p.T.cell[9]);
		}
	};

	class PrimitiveFactory {
	public:
		static Primitive GenerateTriangle(int objId, int matIdx, float3 vertex0, float3 vertex1, float3 vertex2, mat4 transform = mat4::Identity()) 
		{
			Primitive primitive;
			primitive.objIdx = objId;
			primitive.matIdx = matIdx;
			primitive.type = 0;
			primitive.tri.vertex0 = vertex0;
			primitive.tri.vertex1 = vertex1;
			primitive.tri.vertex2 = vertex2;
			primitive.T = transform, primitive.invT = transform.FastInvertedTransformNoScale();
			return primitive;
		}

		static Primitive GenerateSphere(int objId, int matIdx, float r, mat4 transform = mat4::Identity())
		{
			Primitive primitive;
			primitive.objIdx = objId;
			primitive.matIdx = matIdx;
			primitive.type = 1;
			primitive.tri.vertex0.x = r;
			primitive.tri.vertex0.y = r * r; // r2
			primitive.tri.vertex0.z = 1 / r; // invr
			primitive.T = transform, primitive.invT = transform.FastInvertedTransformNoScale();
			return primitive;
		}

		static Primitive GeneratePlane(int objId, int matIdx, float3 normal, float dist, mat4 transform = mat4::Identity())
		{
			Primitive primitive;
			primitive.objIdx = objId;
			primitive.matIdx = matIdx;
			primitive.type = 2;
			primitive.tri.vertex0 = normal;
			primitive.tri.vertex1.x = dist;
			primitive.T = transform, primitive.invT = transform.FastInvertedTransformNoScale();
			return primitive;
		}

		static Primitive GenerateCube(int objId, int matIdx, float3 pos, float3 size, mat4 transform = mat4::Identity())
		{
			Primitive primitive;
			primitive.objIdx = objId;
			primitive.matIdx = matIdx;
			primitive.type = 3;
			primitive.tri.vertex0 = pos - 0.5f * size;
			primitive.tri.vertex1 = pos + 0.5f * size;
			primitive.T = transform, primitive.invT = transform.FastInvertedTransformNoScale();
			return primitive;
		}

		static Primitive GenerateQuad(int objId, int matIdx, float s, mat4 transform = mat4::Identity())
		{
			Primitive primitive;
			primitive.objIdx = objId;
			primitive.matIdx = matIdx;
			primitive.type = 4;
			primitive.tri.vertex0.x = s * 0.5f;
			primitive.T = transform, primitive.invT = transform.FastInvertedTransformNoScale();
			return primitive;
		}
	};
}
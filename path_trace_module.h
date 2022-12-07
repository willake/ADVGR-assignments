#pragma once
class PathTraceModule
{
public:
	void Init(Scene& scene)
	{
		scene = scene;
		isInitialized = true;
	}

	float3 Trace(Ray& ray, int iterated)
	{
		scene.FindNearest(ray);
		if (ray.objIdx == -1) return 0; // or a fancy sky color
		float3 I = ray.O + ray.t * ray.D;
		float3 N = scene.GetNormal(ray.objIdx, I, ray.D);
		Material material = scene.GetMaterial(ray.objIdx);
		float3 albedo = scene.GetAlbedo(ray.objIdx, I); // very bad

		float3 R = DiffuseReflection(N);
		Ray rayToHemisphere = Ray(I + R * 0.001f, R);
		scene.FindNearest(rayToHemisphere);
		Material rMaterial = scene.GetMaterial(rayToHemisphere.objIdx);

		if (rMaterial.isLight)
		{
			float3 BRDF = albedo * INVPI;
			float cosI = dot(R, N);
			return 2.0f * PI * BRDF * rMaterial.color * cosI;
		}

		return float3(0);
	}

	float3 DiffuseReflection(float3 N)
	{
		// could be better but I do not have idea for now xD
		mat4 rotateX = mat4::RotateX(Rand(2 * PI));
		mat4 rotateZ = mat4::RotateY(Rand(2 * PI));
		mat4 rotateY = mat4::RotateZ(Rand(2 * PI));

		mat4 rotation = rotateX * rotateY * rotateZ;

		float3 direction = float4(1, 1, 1, 1) * rotation;

		float angle = acosf(dot(direction, N));

		if (angle > PI) return direction * -1;

		return direction;
	}

	bool isInitialized = false;
	Scene scene;
};
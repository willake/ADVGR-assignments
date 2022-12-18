#pragma once
class SimpleTriangleRayTraceModule
{
public:
	void Init(TriScene& scene)
	{
		scene = scene;
		isInitialized = true;
	}

	float3 Trace(Ray& ray)
	{
		scene.FindNearest(ray);
		if (ray.objIdx == -1) return 0; // or a fancy sky color
		float3 I = ray.O + ray.t * ray.D;
		float3 N = scene.GetNormal(ray.objIdx, I, ray.D);
		Material material = scene.GetMaterial(ray.objIdx);
		float3 albedo = scene.GetAlbedo(ray.objIdx, I); // very bad

		return albedo;
	}

	int depthLimit = 5;
	bool isInitialized = false;
	TriScene scene;
};
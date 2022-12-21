#pragma once
class WhittedStyleRayTraceModule
{
public:
	void Init(Scene& scene)
	{
		scene = scene;
		isInitialized = true;
	}

	float3 Trace(Ray& ray, int depth)
	{
		scene.FindNearest(ray);
		if (ray.objIdx == -1) return float3(195 / 255.0f, 251 / 255.0f, 249 / 255.0f); // or a fancy sky color
		float3 I = ray.O + ray.t * ray.D;
		float3 N = scene.GetNormal(ray.objIdx, I, ray.D);
		Material material = scene.GetMaterial(ray.objIdx);
		float3 albedo = scene.GetAlbedo(ray.objIdx, I); // very bad

		/* visualize normal */ // return (N + 1) * 0.5f;
		/* visualize distance */ // return 0.1f * float3( ray.t, ray.t, ray.t );
		//return albedo;
		/* refraction of glass: 1.52 */
		float n1 = 1;
		float n2 = 1.52f;
		float n1DividedByn2 = n1 / n2;
		float cosI = dot(N, -ray.D);
		float k = 1 - (((n1 / n2) * (n1 / n2)) * (1 - (cosI * cosI)));

		if (depth > depthLimit)
		{
			return albedo * DirectIllumination(I, N);
		}

		// glass
		if (material.isGlass && !(k < 0))
		{
			float ThetaI = acos(cosI);
			float sinI = sin(ThetaI);

			float cosT = sqrt(1 - (n1DividedByn2 * sinI));
			float Rs = ((n1 * cosI) - (n2 * cosT)) / ((n1 * cosI) + (n2 * cosT));
			float Rp = ((n1 * cosI) - (n2 * cosT)) / ((n1 * cosI) + (n2 * cosT));

			float Fr = ((Rs * Rs) + (Rp * Rp)) / 2;
			float Ft = 1 - Fr;

			float p = Rand(1);

			float3 reflectDirection = reflect(ray.D, N);
			float3 reflection = albedo * Trace(Ray(I + reflectDirection * 0.001f, reflectDirection), depth + 1);

			float3 refractDirection = (n1DividedByn2 * ray.D) + (N * ((n1DividedByn2 * cosI) - sqrt(k)));
			float3 refraction = albedo * Trace(Ray(I + (refractDirection * 0.001f), refractDirection), depth + 1);

			return Fr * reflection + Ft * refraction;
		}

		// reflection
		if (material.isMirror || (material.isGlass && k < 0))
		{
			float3 reflectDirection = reflect(ray.D, N);
			float3 reflection = albedo * Trace(Ray(I + (reflectDirection * 0.001f), reflectDirection), depth + 1);
			return (material.reflectivity * reflection) + ((1 - material.reflectivity) * albedo * DirectIllumination(I, N));
		}

		// diffuse
		return albedo * DirectIllumination(I, N);
	}

	float3 DirectIllumination(float3 I, float3 N)
	{
		float3 lightColor = scene.GetLightColor();
		float3 lightPos = scene.GetLightPos();
		float3 L = normalize(lightPos - I);
		Ray shadowRay = Ray(I + (L * 0.001f), L);

		//scene.quad.Intersect(shadowRay);
		scene.IntersectLight(shadowRay);

		if (scene.IsOccluded(shadowRay)) return float3(0);

		float d = length(lightPos - I);
		float distF = 1 / (d * d);
		float angleF = dot(L, N);
		lightColor.x = lightColor.x * distF * angleF;
		lightColor.y = lightColor.y * distF * angleF;
		lightColor.z = lightColor.z * distF * angleF;

		return lightColor;
	}

	int depthLimit = 5;
	bool isInitialized = false;
	Scene scene;
};
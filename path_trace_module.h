#pragma once
class PathTraceModule
{
public:
	void Init(Scene& scene)
	{
		scene = scene;
		isInitialized = true;
	}

	float3 Trace(Ray& ray)
	{
		float3 result;
		for (int i = 0; i < sampleCount; i++)
		{
			result += Sample(ray, 1);
		}

		result *= 2 * PI / sampleCount;
		return result;
	}

	float3 Sample(Ray& ray, int depth)
	{
		if (depth > depthLimit) return 0;
		scene.FindNearest(ray);
		if (ray.objIdx == -1) return 0; // or a fancy sky color
		float3 I = ray.O + ray.t * ray.D;
		float3 N = scene.GetNormal(ray.objIdx, I, ray.D);
		Material material = scene.GetMaterial(ray.objIdx);

		if (material.isLight) return scene.GetLightColor();

		float3 albedo = scene.GetAlbedo(ray.objIdx, I); // very bad

		//refraction of glass: 1.52 
		float n1 = 1;
		float n2 = 1.52f;
		float n1DividedByn2 = n1 / n2;
		float cosI = dot(N, -ray.D);
		float k = 1 - (((n1 / n2) * (n1 / n2)) * (1 - (cosI * cosI)));

		// glass
		if (material.isGlass && !(k < 0))
		{
			float ThetaI = acos(cosI);
			float sinI = sin(ThetaI);

			float cosT = sqrt(1 - (n1DividedByn2 * sinI));
			float Rs = ((n1 * cosI) - (n2 * cosT)) / ((n1 * cosI) + (n2 * cosT));
			float Rp = ((n1 * cosI) - (n2 * cosT)) / ((n1 * cosI) + (n2 * cosT));

			float Fr = ((Rs * Rs) + (Rp * Rp)) / 2;
			//float Ft = 1 - Fr;

			float p = Rand(1);

			if(p > Fr)
			{
				float3 reflectDirection = reflect(ray.D, N);
				return albedo * Sample(Ray(I + reflectDirection * 0.001f, reflectDirection), depth + 1);
			}
			else
			{
				float3 refractDirection = (n1DividedByn2 * ray.D) + (N * ((n1DividedByn2 * cosI) - sqrt(k)));
				return albedo * Sample(Ray(I + (refractDirection * 0.001f), refractDirection), depth + 1);
			}
		}

		if (material.isMirror || (material.isGlass && k < 0))
		{
			float3 reflectDirection = reflect(ray.D, N);
			return albedo * Sample(Ray(I + N * FLT_EPSILON, reflectDirection), depth + 1);
		}

		float3 R = DiffuseReflection(N);
		Ray rayToHemisphere = Ray(I + R * FLT_EPSILON, R);

		float3 BRDF = albedo * INVPI;
		float3 Ei = Sample(rayToHemisphere, depth + 1) * dot(N, R);

		return PI * 2.0f * BRDF * Ei;
	}

	float3 DiffuseReflection(float3 N)
	{
		// reference https://math.stackexchange.com/questions/1585975/how-to-generate-random-points-on-a-sphere
		// I am still figuring it out
		float u1 = Rand(1.f);
		float u2 = Rand(1.f);

		float theta1 = acosf(2 * u1 - 1) - (PI / 2);
		float theta2 = 2 * PI * u2;

		float3 direction = normalize(float3(
			cos(theta1) * cos(theta2),
			cos(theta1) * sin(theta2),
			sin(theta1)
		));

		if (dot(direction, N) < 0) return direction * -1;

		return direction;
	}

	int depthLimit = 5;
	int sampleCount = 5;
	bool isInitialized = false;
	Scene scene;
};
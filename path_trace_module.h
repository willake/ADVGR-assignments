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

		if (material.isLight) return albedo * 0.3f;

		/* refraction of glass: 1.52 */
		float n1 = 1;
		float n2 = 1.52f;
		float n1DividedByn2 = n1 / n2;
		float cosI = dot(N, -ray.D);
		float k = 1 - (((n1 / n2) * (n1 / n2)) * (1 - (cosI * cosI)));

		// glass
		if (material.isGlass && iterated < 5 && !(k < 0))
		{
			float ThetaI = acos(cosI);
			float sinI = sin(ThetaI);
			float3 refractDirection = (n1DividedByn2 * ray.D) + (N * ((n1DividedByn2 * cosI) - sqrt(k)));
			float3 refraction = albedo * Trace(Ray(I + (refractDirection * 0.001f), refractDirection), iterated + 1);

			float cosT = sqrt(1 - (n1DividedByn2 * sinI));
			float Rs = ((n1 * cosI) - (n2 * cosT)) / ((n1 * cosI) + (n2 * cosT));
			float Rp = ((n1 * cosI) - (n2 * cosT)) / ((n1 * cosI) + (n2 * cosT));

			float Fr = ((Rs * Rs) + (Rp * Rp)) / 2;
			float Ft = 1 - Fr;

			float3 reflectDirection = reflect(ray.D, N);
			float3 reflection = albedo * Trace(Ray(I + reflectDirection * 0.001f, reflectDirection), iterated + 1);
			return (Fr * reflection) + (Ft * refraction);
		}

		if ((material.isMirror || (material.isGlass && k < 0)) && iterated < 5)
		{
			float3 reflectDirection = reflect(ray.D, N);
			return albedo * Trace(Ray(I + reflectDirection * 0.001f, reflectDirection), iterated + 1);
		}

		float3 R = DiffuseReflection(N);

		float3 BRDF = albedo / PI;
		
		float3 Ei = Trace(Ray(I + R * 0.001f, R), iterated + 1) * dot(R, N);
		
		return 2.0f * PI * BRDF * Ei;
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

	bool isInitialized = false;
	Scene scene;
};
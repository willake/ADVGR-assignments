#include "precomp.h"

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------
void Renderer::Init()
{
	// create fp32 rgb pixel buffer to render to
	accumulator = (float4*)MALLOC64( SCRWIDTH * SCRHEIGHT * 16 );
	memset( accumulator, 0, SCRWIDTH * SCRHEIGHT * 16 );
}

// -----------------------------------------------------------
// Evaluate light transport
// -----------------------------------------------------------
float3 Renderer::Trace( Ray& ray, int iterated )
{
	scene.FindNearest( ray );
	if (ray.objIdx == -1) return 0; // or a fancy sky color
	float3 I = ray.O + ray.t * ray.D;
	float3 N = scene.GetNormal( ray.objIdx, I, ray.D );
	Material material = scene.GetMaterial( ray.objIdx );
	float3 albedo = scene.GetAlbedo( ray.objIdx, I ); // very bad
	
	/* visualize normal */ // return (N + 1) * 0.5f;
	/* visualize distance */ // return 0.1f * float3( ray.t, ray.t, ray.t );
	
	/* refraction of glass: 1.52 */
	float n1 = 1;
	float n2 = 1.52f;
	float n1DividedByn2 = n1 / n2;
	float cosI = dot(N, -ray.D);
	float k = 1 - ((n1DividedByn2  * n1DividedByn2) * (1 - (cosI * cosI)));

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
		float3 reflection = albedo * Trace(Ray(I + (reflectDirection * 0.001f), reflectDirection), iterated + 1);
		return (Fr * reflection) + (Ft * refraction);
	}

	// reflection
	if ((material.isMirror || (material.isGlass && k < 0))&& iterated < 5) 
	{ 
		float3 reflectDirection = reflect(ray.D, N);
		float3 reflection = albedo * Trace(Ray(I + (reflectDirection * 0.001f), reflectDirection), iterated + 1);
		return (material.reflectivity * reflection) + ((1 - material.reflectivity) * albedo * DirectIllumination(I, N));
	}
	return albedo * DirectIllumination(I, N);
}

float3 Renderer::DirectIllumination(float3 I, float3 N)
{
	float3 lightColor = scene.GetLightColor();
	float3 lightPos = scene.GetLightPos();
	float3 L = normalize(lightPos - I);
	Ray shadowRay = Ray(I + (L * 0.001f), L);

	scene.quad.Intersect(shadowRay);

	if (scene.IsOccluded(shadowRay)) return float3(0);

	float d = length(lightPos - I);
	float distF = 1 / (d * d);
	float angleF = dot(L, N);
	lightColor.x = lightColor.x * distF * angleF;
	lightColor.y = lightColor.y * distF * angleF;
	lightColor.z = lightColor.z * distF * angleF;

	return lightColor * 0.3f;
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void Renderer::Tick( float deltaTime )
{
	// animation
	static float animTime = 0;
	//scene.SetTime( animTime += deltaTime * 0.002f );
	// pixel loop
	Timer t;

	if (isMouseButtonRightDown)
	{
		//camera.Rotate(offsetMouseX, offsetMouseY);
		//camera.UpdateView();
	}

	if (verticalInput != 0 || horizontalInput != 0)
	{
		camera.Move(verticalInput, horizontalInput);
		camera.UpdateView();
	}

	// lines are executed as OpenMP parallel tasks (disabled in DEBUG)
	#pragma omp parallel for schedule(dynamic)
	for (int y = 0; y < SCRHEIGHT; y++)
	{
		// trace a primary ray for each pixel on the line
		for (int x = 0; x < SCRWIDTH; x++)
		{
			if (isAntiAlisingOn)
			{
				float sampleMatrix[4 * 2] = {
					-Rand(2.f) / 4.f,  Rand(2.f) / 4.f,
					-Rand(2.f) / 4.f, -Rand(2.f) / 4.f,
					 Rand(2.f) / 4.f,  Rand(2.f) / 4.f,
					 Rand(2.f) / 4.f, -Rand(2.f) / 4.f,
				};
				for (int sample = 0; sample < 4; sample++)
				{
					accumulator[x + y * SCRWIDTH] += float4(
						Trace(camera.GetPrimaryRay((float)x + sampleMatrix[2 * sample], (float)y + sampleMatrix[2 * sample + 1]), 1), 0);
				}
				// take average
				accumulator[x + y * SCRWIDTH] /= 4.0f;
			}
			else
			{
				accumulator[x + y * SCRWIDTH] =
					float4(Trace(camera.GetPrimaryRay(x, y), 1), 0);
			}
		}
		// translate accumulator contents to rgb32 pixels
		for (int dest = y * SCRWIDTH, x = 0; x < SCRWIDTH; x++)
			screen->pixels[dest + x] = 
				RGBF32_to_RGB8( &accumulator[x + y * SCRWIDTH] );
	}
	// performance report - running average - ms, MRays/s
	static float avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
	if (alpha > 0.05f) alpha *= 0.5f;
	float fps = 1000 / avg, rps = (SCRWIDTH * SCRHEIGHT) * fps;
	printf( "%5.2fms (%.1fps) - %.1fMrays/s\n", avg, fps, rps / 1000000 );
}
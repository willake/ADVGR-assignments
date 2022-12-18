#include "precomp.h"

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------
void Renderer::Init()
{
	// create fp32 rgb pixel buffer to render to
	accumulator = (float4*)MALLOC64( SCRWIDTH * SCRHEIGHT * 16 );
	memset( accumulator, 0, SCRWIDTH * SCRHEIGHT * 16 );
	
	switch (rendererModuleType)
	{
		case RendererModuleType::Triangle:
			triangleTracerModule = SimpleTriangleRayTraceModule();
			break;
		case RendererModuleType::WhittedStyle:
		default:
			whittedStyleRayTraceModule = WhittedStyleRayTraceModule();
			break;
		case RendererModuleType::PathTrace:
			pathTracerModule = PathTraceModule();
			break;
	}
}

// -----------------------------------------------------------
// Evaluate light transport
// -----------------------------------------------------------
float3 Renderer::Trace( Ray& ray)
{
	switch (rendererModuleType)
	{
		case RendererModuleType::WhittedStyle:
		default:
			if (whittedStyleRayTraceModule.isInitialized == false)
			{
				whittedStyleRayTraceModule.Init(scene);
			}
			return whittedStyleRayTraceModule.Trace(ray, 1);
		case RendererModuleType::PathTrace:
			if (pathTracerModule.isInitialized == false)
			{
				pathTracerModule.Init(scene);
			}
			return pathTracerModule.Trace(ray);
		case RendererModuleType::Triangle:
			if (triangleTracerModule.isInitialized == false)
			{
				triangleTracerModule.Init(triScene);
			}
			return triangleTracerModule.Trace(ray);
	}

	
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
		camera.Rotate(mouseOffset.x, mouseOffset.y);
	}

	if (verticalInput != 0 || horizontalInput != 0)
	{
		camera.Move(verticalInput, horizontalInput, deltaTime);
	}

	if (camera.isUpdated)
	{
		camera.UpdateView();
		samepleCount = 0;
	}

	if (samepleCount == maxSampleCount)
	{
		samepleCount = 0;
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
						Trace(camera.GetPrimaryRay((float)x + sampleMatrix[2 * sample], (float)y + sampleMatrix[2 * sample + 1])), 0);
				}
				// take average
				accumulator[x + y * SCRWIDTH] /= 4.0f;
			}
			else
			{
				if (samepleCount == 0)
				{
					accumulator[x + y * SCRWIDTH] = float4(Trace(camera.GetPrimaryRay(x, y)), 0);
				}
				else
				{
					float4 color = float4(Trace(camera.GetPrimaryRay(x, y)), 0);
					float4 last = accumulator[x + y * SCRWIDTH];
					accumulator[x + y * SCRWIDTH] = last + ((color - last) / samepleCount);
				}
			}
		}
		 
		// translate accumulator contents to rgb32 pixels
		for (int dest = y * SCRWIDTH, x = 0; x < SCRWIDTH; x++)
		{
			//for (auto it = frameCaches.begin(); it != frameCaches.end(); it++)
			//{
				//float4* temp = *it;
				//accumulator[x + y * SCRWIDTH] += temp[x + y * SCRWIDTH];
			//}
			//accumulator[x + y * SCRWIDTH] /= (float)(frameCaches.size() + 1);

			screen->pixels[dest + x] =
				RGBF32_to_RGB8(&accumulator[x + y * SCRWIDTH]);
		}
	}
	// performance report - running average - ms, MRays/s
	static float avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
	if (alpha > 0.05f) alpha *= 0.5f;
	float fps = 1000 / avg, rps = (SCRWIDTH * SCRHEIGHT) * fps;
	printf( "%5.2fms (%.1fps) - %.1fMrays/s\n", avg, fps, rps / 1000000 );

	samepleCount++;
}
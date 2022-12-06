#pragma once

// default screen resolution
#define SCRWIDTH	1280
#define SCRHEIGHT	720
// #define FULLSCREEN
// #define DOUBLESIZE

namespace Tmpl8 {

class Camera
{
public:
	Camera()
	{
		// setup a basic view frustum
		camPos = float3( 0, 0, -2 );
		offsetTL = float3(-aspect, 1, 2);
		offsetTR = float3(aspect, 1, 2);
		offsetBL = float3(-aspect, -1, 2);
		yaw = 0.0f;
		pitch = 0.0f;
		Rotate(0, 0);
		Move(0, 0, 0);
	}
	Ray GetPrimaryRay( const int x, const int y )
	{
		// calculate pixel position on virtual screen plane
		const float u = (float)x * (1.0f / SCRWIDTH);
		const float v = (float)y * (1.0f / SCRHEIGHT);
		const float fovFactor = tan(fov / 2 * PI / 180) * 2;
		const float3 origin = imagePlaneTL * fovFactor;
		const float3 width = (imagePlaneTR - imagePlaneTL) * fovFactor;
		const float3 height = (imagePlaneBL - imagePlaneTL) * fovFactor;
		const float3 P = origin + u * width + v * height;

		return Ray( camPos, normalize( P - camPos ) );
	}
	Ray GetPrimaryRay(const float x, const float y)
	{
		// calculate pixel position on virtual screen plane
		const float u = x * (1.0f / SCRWIDTH);
		const float v = y * (1.0f / SCRHEIGHT);
		const float fovFactor = tan(fov / 2 * PI / 180) * 2;
		const float3 origin = imagePlaneTL * fovFactor;
		const float3 width = (imagePlaneTR - imagePlaneTL) * fovFactor;
		const float3 height = (imagePlaneBL - imagePlaneTL) * fovFactor;
		const float3 P = origin + u * width + v * height;

		return Ray(camPos, normalize(P - camPos));
	}
	void Rotate(const int offsetX, const int offsetY)
	{
		yaw += (float)offsetX;
		pitch += (float)-offsetY;

		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;

		orientation = mat4::RotateY(yaw * PI / 180) * mat4::RotateX(pitch * PI / 180);

		camRight = normalize(float4(1, 0, 0, 1) * orientation);
		camUp = normalize(float4(0, 1, 0, 1) * orientation);
		camFront = normalize(float4(0, 0, 1, 1) * orientation);
		isUpdated = true;
	}
	void Move(const int vertical, const int horizontal, const float deltaTime)
	{
		camPos += camFront * (float)vertical * moveSpeed * deltaTime;
		camPos += camRight * (float)horizontal * moveSpeed * deltaTime;

		translation = mat4::Translate(camPos);
		isUpdated = true;
	}
	void UpdateView()
	{
		imagePlaneTL = float4(-aspect, 1, 2, 1) * orientation * translation;
		imagePlaneTR = float4(aspect, 1, 2, 1) * orientation * translation;
		imagePlaneBL = float4(-aspect, -1, 2, 1) * orientation * translation;
		isUpdated = false;

		printf("camPos %f %f %f \n", camPos.x, camPos.y, camPos.z);
		printf("imagePlaneTL %f %f %f \n", imagePlaneTL.x, imagePlaneTL.y, imagePlaneTL.z);
		printf("imagePlaneTR %f %f %f \n", imagePlaneTR.x, imagePlaneTR.y, imagePlaneTR.z);
		printf("imagePlaneBL %f %f %f \n", imagePlaneBL.x, imagePlaneBL.y, imagePlaneBL.z);
	}
	float aspect = (float)SCRWIDTH / (float)SCRHEIGHT;
	float fov = 60.0f;
	float moveSpeed = 0.01f;
	float yaw, pitch;
	bool isUpdated = false;
	float3 camPos;
	float3 imagePlaneTL, imagePlaneTR, imagePlaneBL;
	float3 camFront, camUp, camRight;
	float3 offsetTL, offsetTR, offsetBL;
	mat4 translation, orientation;
};

}
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
		topLeft = float3( -aspect, 1, 0 );
		topRight = float3( aspect, 1, 0 );
		bottomLeft = float3( -aspect, -1, 0 );
		topLeftTranslation = mat4::Translate(topLeft);
		topRightTranslation = mat4::Translate(topRight);
		bottomLeftTranslation = mat4::Translate(bottomLeft);
		//yaw = -90.0f;
		//pitch = 0.0f;
		//cameraFront = float3(0, 0, -1);
		//cameraUp = float3(0, 1, 0);
	}
	Ray GetPrimaryRay( const int x, const int y )
	{
		// calculate pixel position on virtual screen plane
		const float u = (float)x * (1.0f / SCRWIDTH);
		const float v = (float)y * (1.0f / SCRHEIGHT);
		const float fovFactor = tan(fov / 2 * PI / 180) * 2;
		const float3 origin = topLeft * fovFactor;
		const float3 width = (topRight - topLeft) * fovFactor;
		const float3 height = (bottomLeft - topLeft) * fovFactor;
		const float3 P = origin + u * width + v * height;

		return Ray( camPos, normalize( P - camPos ) );
	}
	void Rotate(const float offsetX, const float offsetY)
	{
		//yaw += offsetX;
		//pitch += offsetY;

		//if (pitch > 89.0f) pitch = 89.0f;
		//if (pitch < -89.0f) pitch = -89.0f;

		//float3 direction = float3(0.0f);
		//direction.x = cos(yaw * PI / 180) * cos(pitch * PI / 180);
		//direction.y = sin(pitch * PI / 180);
		//direction.z = sin(yaw * PI / 180) * cos(pitch * PI / 180);
		
		//float3 up = float3(0.0f, 1.0f, 0.0f);
		//float3 cameraRight = normalize(cross(up, direction));

		//cameraUp = cross(direction, cameraRight);
	}
	void Move(const int vertical, const int horizontal)
	{
		const mat4 translation = mat4::Translate((float)horizontal * movingSpeed, 0, (float)vertical * movingSpeed);
		camPos = TransformPosition(camPos, translation);

		topLeft = TransformPosition(camPos, topLeftTranslation);
		topRight = TransformPosition(camPos, topRightTranslation);
		bottomLeft = TransformPosition(camPos, bottomLeftTranslation);
	}
	void UpdateView()
	{
		//mat4 rotation = mat4::LookAt(camPos, camPos + cameraFront, cameraUp);

		//topLeft = TransformPosition(camPos, topLeftTranslation);
		//topRight = TransformPosition(camPos, topRightTranslation);
		//bottomLeft = TransformPosition(camPos, bottomLeftTranslation);
	}
	float aspect = (float)SCRWIDTH / (float)SCRHEIGHT;
	float fov = 60.0f;
	float movingSpeed = 0.01f;
	//float yaw, pitch;
	float3 camPos;
	float3 topLeft, topRight, bottomLeft;
	//float3 cameraFront, cameraUp;
	mat4 topLeftTranslation, topRightTranslation, bottomLeftTranslation;
};

}
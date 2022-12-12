#pragma once
class Material
{
public:
	Material() = default;
	Material(bool isLight, bool isMirror, bool isGlass, float3 color = float3(1), float reflectivity = 0.0f, float refractivity = 0.0f) :
		isLight(isLight), isMirror(isMirror), isGlass(isGlass), color(color), reflectivity(reflectivity), refractivity(refractivity) {}
	virtual float3 GetAlbedo(const float3 I) const { return color; } // not the best choice
	bool isLight = false;
	bool isMirror = false;
	bool isGlass = false;
	float3 color = float3(1);
	float reflectivity = 1.0f;
	float refractivity = 1.0f;
};

class FloorMaterial : public Material
{
public:
	FloorMaterial() = default;
	FloorMaterial(bool isLight, bool isMirror, bool isGlass, float3 color = float3(1), float reflectivity = 0.0f, float refractivity = 0.0f) :
		Material(isLight, isMirror, isGlass, color, reflectivity, refractivity) {}
	float3 GetAlbedo(const float3 I) const 
	{ 			
		// floor albedo: checkerboard
		int ix = (int)(I.x * 2 + 96.01f);
		int iz = (int)(I.z * 2 + 96.01f);
		// add deliberate aliasing to two tile
		if (ix == 98 && iz == 98) ix = (int)(I.x * 32.01f), iz = (int)(I.z * 32.01f);
		if (ix == 94 && iz == 98) ix = (int)(I.x * 64.01f), iz = (int)(I.z * 64.01f);
		return color * float3(((ix + iz) & 1) ? 1 : 0.3f);
	}
};

class BackWallMaterial : public Material
{
public:
	BackWallMaterial() = default;
	BackWallMaterial(bool isLight, bool isMirror, bool isGlass, float3 color = float3(1), float reflectivity = 0.0f, float refractivity = 0.0f) :
		Material(isLight, isMirror, isGlass, color, reflectivity, refractivity) {}
	float3 GetAlbedo(const float3 I) const
	{ 			
		// floor albedo: checkerboard
		// back wall: logo
		static Surface logo("assets/logo.png");
		int ix = (int)((I.x + 4) * (128.0f / 8));
		int iy = (int)((2 - I.y) * (64.0f / 3));
		uint p = logo.pixels[(ix & 127) + (iy & 63) * 128];
		uint3 i3((p >> 16) & 255, (p >> 8) & 255, p & 255);
		return float3(i3) * (1.0f / 255.0f);
	}
};
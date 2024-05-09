#pragma once

struct vec4 {
	float x, y, z, w;
};

struct vec3 {
	float x, y, z;
};

struct vec2 {
	float x, y;
};

bool w2s(const vec3& world, vec2& screen, float m[16])
{
	vec4 clipCoords;

	return true;
}
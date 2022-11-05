#pragma once

#include <memory>
#include <vector>
#include <intrin.h>

struct Occluder
{
	static std::unique_ptr<Occluder> bake(const std::vector<__m128>& vertices, __m128 refMin, __m128 refMax);

	__m128 m_center;

	__m128 m_refMin;
	__m128 m_refMax;

	__m128 m_boundsMin;
	__m128 m_boundsMax;

	__m256i* m_vertexData;
	uint32_t m_packetCount;
};



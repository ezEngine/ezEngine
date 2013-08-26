
#pragma once

/// \brief Operator to serialize ezIAllocator::Stats objects.
void EZ_FOUNDATION_DLL operator<< (ezIBinaryStreamWriter& Stream, const ezIAllocator::Stats& rhs);

/// \brief Operator to serialize ezIAllocator::Stats objects.
void EZ_FOUNDATION_DLL operator>> (ezIBinaryStreamReader& Stream, ezIAllocator::Stats& rhs);
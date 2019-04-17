#pragma once

/// \brief Operator to serialize ezIAllocator::Stats objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& Stream, const ezAllocatorBase::Stats& rhs);

/// \brief Operator to serialize ezIAllocator::Stats objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& Stream, ezAllocatorBase::Stats& rhs);

struct ezTime;

/// \brief Operator to serialize ezTime objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& Stream, ezTime Value);

/// \brief Operator to serialize ezTime objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& Stream, ezTime& Value);


class ezUuid;

/// \brief Operator to serialize ezUuid objects. [tested]
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& Stream, const ezUuid& Value);

/// \brief Operator to serialize ezUuid objects. [tested]
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& Stream, ezUuid& Value);

class ezHashedString;

/// \brief Operator to serialize ezHashedString objects. [tested]
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& Stream, const ezHashedString& Value);

/// \brief Operator to serialize ezHashedString objects. [tested]
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& Stream, ezHashedString& Value);

class ezTempHashedString;

/// \brief Operator to serialize ezHashedString objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& Stream, const ezTempHashedString& Value);

/// \brief Operator to serialize ezHashedString objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& Stream, ezTempHashedString& Value);

class ezVariant;

/// \brief Operator to serialize ezVariant objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& Stream, const ezVariant& Value);

/// \brief Operator to serialize ezVariant objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& Stream, ezVariant& Value);


#pragma once

/// \brief Operator to serialize ezIAllocator::Stats objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezAllocator::Stats& rhs);

/// \brief Operator to serialize ezIAllocator::Stats objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezAllocator::Stats& rhs);

struct ezTime;

/// \brief Operator to serialize ezTime objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, ezTime value);

/// \brief Operator to serialize ezTime objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezTime& ref_value);


class ezUuid;

/// \brief Operator to serialize ezUuid objects. [tested]
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezUuid& value);

/// \brief Operator to serialize ezUuid objects. [tested]
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezUuid& ref_value);

class ezHashedString;

/// \brief Operator to serialize ezHashedString objects. [tested]
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezHashedString& sValue);

/// \brief Operator to serialize ezHashedString objects. [tested]
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezHashedString& ref_sValue);

class ezTempHashedString;

/// \brief Operator to serialize ezHashedString objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezTempHashedString& sValue);

/// \brief Operator to serialize ezHashedString objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezTempHashedString& ref_sValue);

class ezVariant;

/// \brief Operator to serialize ezVariant objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezVariant& value);

/// \brief Operator to serialize ezVariant objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezVariant& ref_value);

class ezTimestamp;

/// \brief Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, ezTimestamp value);

/// \brief Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezTimestamp& ref_value);

struct ezVarianceTypeFloat;

/// \brief Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeFloat& value);

/// \brief Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezVarianceTypeFloat& ref_value);

struct ezVarianceTypeTime;

/// \brief Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeTime& value);

/// \brief Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezVarianceTypeTime& ref_value);

struct ezVarianceTypeAngle;

/// \brief Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeAngle& value);

/// \brief Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezVarianceTypeAngle& ref_value);

#pragma once

/// Operator to serialize ezIAllocator::Stats objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezAllocator::Stats& rhs);

/// Operator to serialize ezIAllocator::Stats objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezAllocator::Stats& rhs);

struct ezTime;

/// Operator to serialize ezTime objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, ezTime value);

/// Operator to serialize ezTime objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezTime& ref_value);


class ezUuid;

/// Operator to serialize ezUuid objects. [tested]
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezUuid& value);

/// Operator to serialize ezUuid objects. [tested]
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezUuid& ref_value);

class ezHashedString;

/// Operator to serialize ezHashedString objects. [tested]
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezHashedString& sValue);

/// Operator to serialize ezHashedString objects. [tested]
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezHashedString& ref_sValue);

class ezTempHashedString;

/// Operator to serialize ezHashedString objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezTempHashedString& sValue);

/// Operator to serialize ezHashedString objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezTempHashedString& ref_sValue);

class ezVariant;

/// Operator to serialize ezVariant objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezVariant& value);

/// Operator to serialize ezVariant objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezVariant& ref_value);

class ezTimestamp;

/// Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, ezTimestamp value);

/// Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezTimestamp& ref_value);

struct ezVarianceTypeFloat;

/// Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeFloat& value);

/// Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezVarianceTypeFloat& ref_value);

struct ezVarianceTypeTime;

/// Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeTime& value);

/// Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezVarianceTypeTime& ref_value);

struct ezVarianceTypeAngle;

/// Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeAngle& value);

/// Operator to serialize ezTimestamp objects.
EZ_FOUNDATION_DLL void operator>>(ezStreamReader& inout_stream, ezVarianceTypeAngle& ref_value);

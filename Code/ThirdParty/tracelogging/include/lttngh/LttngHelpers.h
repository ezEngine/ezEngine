// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#ifndef _lttnghelpers_h
#define _lttnghelpers_h

#include <lttng/ust-version.h>

#if LTTNG_UST_MAJOR_VERSION != 2
#error "Please use LTTNG 2.x (2.7+)"
#elif LTTNG_UST_MINOR_VERSION >= 13
#define lttngh_UST_VER 213
#elif LTTNG_UST_MINOR_VERSION >= 8
#define lttngh_UST_VER 208
#elif LTTNG_UST_MINOR_VERSION >= 7
#define lttngh_UST_VER 207
#else
#error "Please use LTTNG 2.x (2.7+)"
#endif

#include <lttng/tracepoint-types.h> // lttng_ust_tracepoint
#include <lttng/ust-compiler.h>     // lttng_ust_notrace
#include <lttng/ust-events.h>       // lttng_event, etc.
#include <endian.h>                 // __BYTE_ORDER, __FLOAT_WORD_ORDER
#include <string.h>                 // strlen
#include <uchar.h>                  // char16_t

// Use lttngh_IP_PARAM to get the address of the caller.
// (For use as the pCallerIp parameter of lttngh_EventProbe.)
#undef lttngh_IP_PARAM
#ifdef TP_IP_PARAM
#define lttngh_IP_PARAM (TP_IP_PARAM)
#elif defined(__PPC__) && !defined(__PPC64__)
#define lttngh_IP_PARAM NULL
#else
#define lttngh_IP_PARAM __builtin_return_address(0)
#endif

#define lttngh_IS_SIGNED_TYPE(CTYPE) ((CTYPE)(-1) < (CTYPE)1)

// Abstraction layer to reduce differences between LTTNG versions.
#if lttngh_UST_VER >= 213

typedef struct lttng_ust_probe_desc lttngh_ust_probe_desc;
typedef struct lttng_ust_event_desc lttngh_ust_event_desc;
typedef struct lttng_ust_event_field lttngh_ust_event_field;
typedef struct lttng_ust_type_integer lttngh_ust_type_integer;
typedef struct lttng_ust_enum_desc lttngh_ust_enum_desc;

#define lttngh_ALIGNOF(type) (__alignof__(type) - __alignof__(type) + lttng_ust_rb_alignof(type)) // Flag error for invalid type even on x86/x64.
#define lttngh_UST_SYM_NAME_LEN LTTNG_UST_ABI_SYM_NAME_LEN
#define lttngh_PROVIDER_NAME(probeDesc) ((probeDesc).provider_name)

#if defined(LTTNG_UST_RING_BUFFER_NATURAL_ALIGN)
#define lttngh_UST_RING_BUFFER_NATURAL_ALIGN 1
#else
#define lttngh_UST_RING_BUFFER_NATURAL_ALIGN 0
#endif

// Initializer for lttng_ust_probe_desc (lttngh_ust_probe_desc)
#define lttngh_INIT_PROBE_DESC(NAME) \
    { sizeof(struct lttng_ust_probe_desc), NAME, NULL, 0, LTTNG_UST_PROVIDER_MAJOR, LTTNG_UST_PROVIDER_MINOR }

// Initializer for lttng_ust_tracepoint
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TRACEPOINT(PROVIDER_NAME, EVENT_NAME) \
    { sizeof(struct lttng_ust_tracepoint), PROVIDER_NAME, EVENT_NAME, 0, NULL, NULL, "" }

// Initializer for lttng_ust_tracepoint_class
// (Only for 2.13+)
#define lttngh_INIT_TRACEPOINT_CLASS(PROBE_DESC, CALLBACK, FIELD_PTRS, FIELD_COUNT) \
    { sizeof(struct lttng_ust_tracepoint_class), FIELD_PTRS, FIELD_COUNT, CALLBACK, "", PROBE_DESC }

// Initializer for lttng_ust_event_desc (lttngh_ust_event_desc)
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_EVENT_DESC(PROBE_DESC, EVENT_NAME, TRACEPOINT_CLASS, LEVEL_PTR) \
    { sizeof(struct lttng_ust_event_desc), EVENT_NAME, PROBE_DESC, TRACEPOINT_CLASS, LEVEL_PTR, NULL }

// Initializer for lttng_ust_event_field (lttngh_ust_event_field)
#define lttngh_INIT_EVENT_FIELD(NAME, TYPE) \
    { sizeof(struct lttng_ust_event_field), NAME, &((TYPE).parent), 0, 0 }

// Initializer for lttng_ust_enum_desc (lttngh_ust_enum_desc)
#define lttngh_INIT_ENUM_DESC(PROBE_DESC, NAME, ENTRIES, ENTRY_COUNT) \
    { sizeof(struct lttng_ust_enum_desc), NAME, ENTRIES, ENTRY_COUNT, PROBE_DESC }

// Initializer for lttng_ust_enum_entry with unsigned values
#define lttngh_INIT_ENUM_ENTRY_UNSIGNED(NAME, START, END) \
    { sizeof(struct lttng_ust_enum_entry), {START, 0}, {END, 0}, NAME, 0 }

// Initializer for lttng_ust_type_integer
#define lttngh_INIT_TYPE_INTEGER(CTYPE, RADIX, BYTESWAP) { \
    { lttng_ust_type_integer }, sizeof(struct lttng_ust_type_integer), \
    8u * sizeof(CTYPE), 8u * lttngh_ALIGNOF(CTYPE), lttngh_IS_SIGNED_TYPE(CTYPE), BYTESWAP, RADIX \
    }

// Initializer for lttng_ust_type_float
#define lttngh_INIT_TYPE_FLOAT(CTYPE) { \
    { lttng_ust_type_float }, sizeof(struct lttng_ust_type_float), \
    sizeof(CTYPE) == 4 ? 8 : 11, sizeof(CTYPE) == 4 ? 24 : 53, 8u * lttngh_ALIGNOF(CTYPE), 0 \
    }

// Initializer for lttng_ust_type_sequence
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TYPE_SEQUENCE(ELEMENT_TYPE, COUNT_NAME, ALIGNMENT) { \
    { lttng_ust_type_sequence }, sizeof(struct lttng_ust_type_sequence), \
    COUNT_NAME, &((ELEMENT_TYPE).parent), ALIGNMENT, lttng_ust_string_encoding_none \
    }

// Initializer for lttng_ust_type_array
#define lttngh_INIT_TYPE_ARRAY(ELEMENT_TYPE, COUNT, ALIGNMENT) { \
    { lttng_ust_type_array }, sizeof(struct lttng_ust_type_array), \
    &((ELEMENT_TYPE).parent), COUNT, ALIGNMENT, lttng_ust_string_encoding_none \
    }

// Initializer for lttng_ust_type_string of CHAR8 (nul-terminated string)
#define lttngh_INIT_TYPE_CHAR8_STRING(ENCODE) { \
    { lttng_ust_type_string }, sizeof(struct lttng_ust_type_string), \
    lttng_ust_string_encoding_##ENCODE \
    }

// Initializer for lttng_ust_type_sequence of CHAR8 (counted string)
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TYPE_CHAR8_SEQUENCE(ENCODE, COUNT_NAME) { \
    { lttng_ust_type_sequence }, sizeof(struct lttng_ust_type_sequence), \
    COUNT_NAME, &lttngh_TypeUInt8.parent, lttngh_ALIGNOF(char), lttng_ust_string_encoding_##ENCODE \
    }

// Initializer for lttng_ust_type_array of CHAR8 (fixed-length string)
#define lttngh_INIT_TYPE_CHAR8_ARRAY(ENCODE, COUNT) { \
    { lttng_ust_type_array }, sizeof(struct lttng_ust_type_array), \
    &lttngh_TypeUInt8.parent, COUNT, lttngh_ALIGNOF(char), lttng_ust_string_encoding_##ENCODE \
    }

// Initializer for lttng_ust_type_enum
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TYPE_ENUM(UNDERLYING_TYPE, ENUM_DESC) { \
    { lttng_ust_type_enum }, sizeof(struct lttng_ust_type_enum), \
    &ENUM_DESC, &((UNDERLYING_TYPE).parent) \
    }

// lttngh_Type definitions for use with lttngh_INIT_EVENT_FIELD:

extern const struct lttng_ust_type_integer  lttngh_TypeInt8;
extern const struct lttng_ust_type_integer  lttngh_TypeUInt8;
extern const struct lttng_ust_type_integer  lttngh_TypeHexInt8;

extern const struct lttng_ust_type_integer  lttngh_TypeInt16;
extern const struct lttng_ust_type_integer  lttngh_TypeUInt16;
extern const struct lttng_ust_type_integer  lttngh_TypeHexInt16;
extern const struct lttng_ust_type_integer  lttngh_TypeUInt16BE; // IP PORT

extern const struct lttng_ust_type_integer  lttngh_TypeInt32;
extern const struct lttng_ust_type_integer  lttngh_TypeUInt32;
extern const struct lttng_ust_type_integer  lttngh_TypeHexInt32;

extern const struct lttng_ust_type_integer  lttngh_TypeLong;
extern const struct lttng_ust_type_integer  lttngh_TypeULong;
extern const struct lttng_ust_type_integer  lttngh_TypeHexLong;

extern const struct lttng_ust_type_integer  lttngh_TypeIntPtr;
extern const struct lttng_ust_type_integer  lttngh_TypeUIntPtr;
extern const struct lttng_ust_type_integer  lttngh_TypeHexIntPtr;

extern const struct lttng_ust_type_integer  lttngh_TypeInt64;
extern const struct lttng_ust_type_integer  lttngh_TypeUInt64;
extern const struct lttng_ust_type_integer  lttngh_TypeHexInt64;

extern const struct lttng_ust_type_float    lttngh_TypeFloat32;
extern const struct lttng_ust_type_float    lttngh_TypeFloat64;

extern const struct lttng_ust_type_enum     lttngh_TypeBool8;  // bool8 = enum : uint8_t
extern const struct lttng_ust_type_enum     lttngh_TypeBool32; // bool32 = enum : int32_t
#define lttngh_TypeBool8ForArray            lttngh_TypeUInt8   // LTTNG array of enum is broken. Using UInt8 instead.
#define lttngh_TypeBool32ForArray           lttngh_TypeUInt32  // LTTNG array of enum is broken. Using Int32 instead.

extern const struct lttng_ust_type_sequence lttngh_TypeInt8Sequence;
extern const struct lttng_ust_type_sequence lttngh_TypeUInt8Sequence;
extern const struct lttng_ust_type_sequence lttngh_TypeHexInt8Sequence;

extern const struct lttng_ust_type_sequence lttngh_TypeInt16Sequence;
extern const struct lttng_ust_type_sequence lttngh_TypeUInt16Sequence;
extern const struct lttng_ust_type_sequence lttngh_TypeHexInt16Sequence;

extern const struct lttng_ust_type_sequence lttngh_TypeInt32Sequence;
extern const struct lttng_ust_type_sequence lttngh_TypeUInt32Sequence;
extern const struct lttng_ust_type_sequence lttngh_TypeHexInt32Sequence;

extern const struct lttng_ust_type_sequence lttngh_TypeLongSequence;
extern const struct lttng_ust_type_sequence lttngh_TypeULongSequence;
extern const struct lttng_ust_type_sequence lttngh_TypeHexLongSequence;

extern const struct lttng_ust_type_sequence lttngh_TypeIntPtrSequence;
extern const struct lttng_ust_type_sequence lttngh_TypeUIntPtrSequence;
extern const struct lttng_ust_type_sequence lttngh_TypeHexIntPtrSequence;

extern const struct lttng_ust_type_sequence lttngh_TypeInt64Sequence;
extern const struct lttng_ust_type_sequence lttngh_TypeUInt64Sequence;
extern const struct lttng_ust_type_sequence lttngh_TypeHexInt64Sequence;

//extern const struct lttng_ust_type_sequence lttngh_TypeFloat32Sequence; // LTTNG array of float is broken.
//extern const struct lttng_ust_type_sequence lttngh_TypeFloat64Sequence; // LTTNG array of float is broken.

#define lttngh_TypeBool8Sequence            lttngh_TypeUInt8Sequence   // LTTNG sequence of enum is broken. Using UInt8[] instead.
#define lttngh_TypeBool32Sequence           lttngh_TypeInt32Sequence   // LTTNG sequence of enum is broken. Using Int32[] instead.

extern const struct lttng_ust_type_string   lttngh_TypeUtf8String;
extern const struct lttng_ust_type_sequence lttngh_TypeUtf8Sequence;

extern const struct lttng_ust_type_array    lttngh_TypeUtf8Char;   // CHAR = utf8char[1]
extern const struct lttng_ust_type_array    lttngh_TypeGuid;       // GUID = hexint8[16]
extern const struct lttng_ust_type_array    lttngh_TypeSystemTime; // SYSTEMTIME = uint16[8]
extern const struct lttng_ust_type_array    lttngh_TypeFileTime;   // FILETIME = uint64[1]
extern const struct lttng_ust_type_sequence lttngh_TypeActivityId; // Nullable<GUID> = hexint8[N] (where N is uint8_t).

typedef struct lttngh_registration
{
    struct lttng_ust_registered_probe* registered_probe;
    struct lttng_ust_tracepoint* const* tracepoint_start;
    volatile int is_registered;
} lttngh_registration;
#define lttngh_REGISTRATION_INIT { NULL, NULL, 0 }

// Returns a pointer to volatile int with registration state.
// 0 = unregistered.
// 1 = registered.
// other = unspecified.
#define lttngh_REGISTRATION_STATE(registration) (&(registration).is_registered)

#else // lttngh_UST_VER

typedef struct lttng_probe_desc lttngh_ust_probe_desc;
typedef struct lttng_event_desc lttngh_ust_event_desc;
typedef struct lttng_event_field lttngh_ust_event_field;
typedef struct lttng_type lttngh_ust_type_integer;
#if lttngh_UST_VER >= 208
typedef struct lttng_enum_desc lttngh_ust_enum_desc;
#endif // lttngh_UST_VER

#define lttngh_ALIGNOF(type) (__alignof__(type) - __alignof__(type) + lttng_alignof(type)) // Flag error for invalid type even on x86/x64.
#define lttngh_UST_SYM_NAME_LEN LTTNG_UST_SYM_NAME_LEN
#define lttngh_PROVIDER_NAME(probeDesc) ((probeDesc).provider)

#if defined(RING_BUFFER_ALIGN)
#define lttngh_UST_RING_BUFFER_NATURAL_ALIGN 1
#else
#define lttngh_UST_RING_BUFFER_NATURAL_ALIGN 0
#endif

// Initializer for lttng_probe_desc (lttngh_ust_probe_desc)
#define lttngh_INIT_PROBE_DESC(NAME) \
    { NAME, NULL, 0, {NULL,NULL}, {NULL,NULL}, 0, LTTNG_UST_PROVIDER_MAJOR, LTTNG_UST_PROVIDER_MINOR, {}}

// Initializer for lttng_ust_tracepoint
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TRACEPOINT(FULL_NAME) \
    { FULL_NAME, 0, NULL, NULL, "", {} }

// Initializer for lttng_ust_event_desc (lttngh_ust_event_desc).
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_EVENT_DESC(FULL_NAME, CALLBACK, FIELDS, FIELD_COUNT, LEVEL_PTR) \
    { FULL_NAME, CALLBACK, NULL, FIELDS, FIELD_COUNT, LEVEL_PTR, "", {} }

// Initializer for lttng_event_field (lttngh_ust_event_field)
#define lttngh_INIT_EVENT_FIELD(NAME, TYPE) \
    { .name = NAME, .type = TYPE, .nowrite = 0, .padding = {} }

// Initializer for lttng_enum_desc (lttngh_ust_event_field)
#define lttngh_INIT_ENUM_DESC(PROBE_DESC_IGNORED, NAME, ENTRIES, ENTRY_COUNT) \
    { NAME, ENTRIES, ENTRY_COUNT, {} }

// Initializer for lttng_enum_entry with unsigned values
#define lttngh_INIT_ENUM_ENTRY_UNSIGNED(NAME, START, END) \
    { {START, 0}, {END, 0}, NAME, {} }

// Initializer for lttng_type of integer
#define lttngh_INIT_TYPE_INTEGER(CTYPE, RADIX, BYTESWAP) \
    {atype_integer, {.basic = {.integer = { \
    8u * sizeof(CTYPE), 8u * lttngh_ALIGNOF(CTYPE), lttngh_IS_SIGNED_TYPE(CTYPE), BYTESWAP, RADIX, lttng_encode_none, {} \
    }}}}

// Initializer for lttng_type of float
#define lttngh_INIT_TYPE_FLOAT(CTYPE) \
    {atype_float, {.basic = {._float = { \
    sizeof(CTYPE) == 4 ? 8 : 11, sizeof(CTYPE) == 4 ? 24 : 53, 8u * lttngh_ALIGNOF(CTYPE), 0, {} \
    }}}}

// Initializer for lttng_type of sequence
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TYPE_SEQUENCE(ELEMENT_TYPE, COUNT_CTYPE) \
    {atype_sequence, {.sequence = { {atype_integer, {.basic = {.integer = { \
    8u * sizeof(COUNT_CTYPE), 8u * lttngh_ALIGNOF(COUNT_CTYPE), 0, 0, 10, lttng_encode_none, {} \
    }}}}, ELEMENT_TYPE}}}

// Initializer for lttng_type of array
#define lttngh_INIT_TYPE_ARRAY(ELEMENT_TYPE, COUNT, ALIGNMENT_IGNORED) \
    {atype_array, {.array = { \
    ELEMENT_TYPE, COUNT \
    }}}

// Initializer for lttng_type of string of CHAR8 (nul-terminated string)
#define lttngh_INIT_TYPE_CHAR8_STRING(ENCODE) \
    {atype_string, {.basic = {.string = { \
    lttng_encode_##ENCODE \
    }}}}

// Initializer for lttng_type of sequence of CHAR8 (counted string)
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TYPE_CHAR8_SEQUENCE(ENCODE, COUNT_CTYPE) \
    {atype_sequence, {.sequence = { {atype_integer, {.basic = {.integer = { \
    8u * sizeof(COUNT_CTYPE), 8u * lttngh_ALIGNOF(COUNT_CTYPE), 0, 0, 10, lttng_encode_none, {} \
    }}}}, {atype_integer, {.basic = {.integer = { \
    8u * sizeof(char), 8u * lttngh_ALIGNOF(char), 0, 0, 10, lttng_encode_##ENCODE, {} \
    }}}} }}}

// Initializer for lttng_type of array of CHAR8 (fixed-length string)
#define lttngh_INIT_TYPE_CHAR8_ARRAY(ENCODE, COUNT) \
    {atype_array, {.array = { {atype_integer, {.basic = {.integer = { \
    8u * sizeof(char), 8u * lttngh_ALIGNOF(char), 0, 0, 10, lttng_encode_##ENCODE, {} \
    }}}}, COUNT}}}

#if lttngh_UST_VER >= 208
// Initializer for lttng_type of enum of decimal integer
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TYPE_ENUM(CTYPE, ENUM_DESC) \
    {atype_enum, {.basic = {.enumeration = {&ENUM_DESC, { \
    8u * sizeof(CTYPE), 8u * lttngh_ALIGNOF(CTYPE), lttngh_IS_SIGNED_TYPE(CTYPE), 0, 10, lttng_encode_none, {} \
    }}}}}
#else // lttngh_UST_VER
// Initializer for lttng_type of decimal integer
// IMPORTANT: Parameters differ by version!
#define lttngh_INIT_TYPE_ENUM(CTYPE, ENUM_DESC_IGNORED) \
    lttngh_INIT_TYPE_INTEGER(CTYPE, 10, 0)
#endif // lttngh_UST_VER

// lttngh_Type definitions for use with lttngh_INIT_EVENT_FIELD:

#define lttngh_TypeInt8             lttngh_INIT_TYPE_INTEGER(  int8_t, 10, 0)
#define lttngh_TypeUInt8            lttngh_INIT_TYPE_INTEGER( uint8_t, 10, 0)
#define lttngh_TypeHexInt8          lttngh_INIT_TYPE_INTEGER( uint8_t, 16, 0)

#define lttngh_TypeInt16            lttngh_INIT_TYPE_INTEGER( int16_t, 10, 0)
#define lttngh_TypeUInt16           lttngh_INIT_TYPE_INTEGER(uint16_t, 10, 0)
#define lttngh_TypeHexInt16         lttngh_INIT_TYPE_INTEGER(uint16_t, 16, 0)
#define lttngh_TypeUInt16BE         lttngh_INIT_TYPE_INTEGER(uint16_t, 10, __BYTE_ORDER == __LITTLE_ENDIAN) // IP PORT

#define lttngh_TypeInt32            lttngh_INIT_TYPE_INTEGER( int32_t, 10, 0)
#define lttngh_TypeUInt32           lttngh_INIT_TYPE_INTEGER(uint32_t, 10, 0)
#define lttngh_TypeHexInt32         lttngh_INIT_TYPE_INTEGER(uint32_t, 16, 0)

#define lttngh_TypeLong             lttngh_INIT_TYPE_INTEGER(  signed long, 10, 0)
#define lttngh_TypeULong            lttngh_INIT_TYPE_INTEGER(unsigned long, 10, 0)
#define lttngh_TypeHexLong          lttngh_INIT_TYPE_INTEGER(unsigned long, 16, 0)

#define lttngh_TypeIntPtr           lttngh_INIT_TYPE_INTEGER( intptr_t, 10, 0)
#define lttngh_TypeUIntPtr          lttngh_INIT_TYPE_INTEGER(uintptr_t, 10, 0)
#define lttngh_TypeHexIntPtr        lttngh_INIT_TYPE_INTEGER(uintptr_t, 16, 0)

#define lttngh_TypeInt64            lttngh_INIT_TYPE_INTEGER( int64_t, 10, 0)
#define lttngh_TypeUInt64           lttngh_INIT_TYPE_INTEGER(uint64_t, 10, 0)
#define lttngh_TypeHexInt64         lttngh_INIT_TYPE_INTEGER(uint64_t, 16, 0)

#define lttngh_TypeFloat32          lttngh_INIT_TYPE_FLOAT(float)
#define lttngh_TypeFloat64          lttngh_INIT_TYPE_FLOAT(double)

#define lttngh_TypeBool8            lttngh_INIT_TYPE_ENUM(uint8_t, lttngh_BoolEnumDesc) // BOOL8 = enum : uint8_t
#define lttngh_TypeBool32           lttngh_INIT_TYPE_ENUM(int32_t, lttngh_BoolEnumDesc) // BOOL32 = enum : int32_t
#define lttngh_TypeBool8ForArray    lttngh_INIT_TYPE_INTEGER( uint8_t, 10, 0) // LTTNG array of enum is broken.
#define lttngh_TypeBool32ForArray   lttngh_INIT_TYPE_INTEGER( int32_t, 10, 0) // LTTNG array of enum is broken.

#define lttngh_TypeInt8Sequence     lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeInt8, uint16_t)
#define lttngh_TypeUInt8Sequence    lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUInt8, uint16_t)
#define lttngh_TypeHexInt8Sequence  lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt8, uint16_t)

#define lttngh_TypeInt16Sequence    lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeInt16, uint16_t)
#define lttngh_TypeUInt16Sequence   lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUInt16, uint16_t)
#define lttngh_TypeHexInt16Sequence lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt16, uint16_t)

#define lttngh_TypeInt32Sequence    lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeInt32, uint16_t)
#define lttngh_TypeUInt32Sequence   lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUInt32, uint16_t)
#define lttngh_TypeHexInt32Sequence lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt32, uint16_t)

#define lttngh_TypeLongSequence     lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeLong, uint16_t)
#define lttngh_TypeULongSequence    lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeULong, uint16_t)
#define lttngh_TypeHexLongSequence  lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexLong, uint16_t)

#define lttngh_TypeIntPtrSequence    lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeIntPtr, uint16_t)
#define lttngh_TypeUIntPtrSequence   lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUIntPtr, uint16_t)
#define lttngh_TypeHexIntPtrSequence lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexIntPtr, uint16_t)

#define lttngh_TypeInt64Sequence    lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeInt64, uint16_t)
#define lttngh_TypeUInt64Sequence   lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUInt64, uint16_t)
#define lttngh_TypeHexInt64Sequence lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt64, uint16_t)

//#define lttngh_TypeFloat32Sequence  lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeFloat32, uint16_t) // Array of float is broken.
//#define lttngh_TypeFloat64Sequence  lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeFloat64, uint16_t) // Array of float is broken.

#define lttngh_TypeBool8Sequence    lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeBool8ForArray, uint16_t)
#define lttngh_TypeBool32Sequence   lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeBool32ForArray, uint16_t)

#define lttngh_TypeUtf8String       lttngh_INIT_TYPE_CHAR8_STRING(UTF8)
#define lttngh_TypeUtf8Sequence     lttngh_INIT_TYPE_CHAR8_SEQUENCE(UTF8, uint16_t)

#define lttngh_TypeUtf8Char         lttngh_INIT_TYPE_CHAR8_ARRAY(UTF8, 1) // CHAR = utf8char[1]
#define lttngh_TypeGuid             lttngh_INIT_TYPE_ARRAY(lttngh_TypeHexInt8, 16, lttngh_ALIGNOF(uint8_t)) // GUID = hexint8[16]
#define lttngh_TypeSystemTime       lttngh_INIT_TYPE_ARRAY(lttngh_TypeUInt16, 8, lttngh_ALIGNOF(uint16_t))  // SYSTEMTIME = uint16[8]
#define lttngh_TypeFileTime         lttngh_INIT_TYPE_ARRAY(lttngh_TypeUInt64, 1, lttngh_ALIGNOF(uint64_t))  // FILETIME = uint64[1]
#define lttngh_TypeActivityId       lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt8, uint8_t) // Nullable<GUID> = hexint8[N] (where N is uint8_t).

typedef volatile int lttngh_registration;
#define lttngh_REGISTRATION_INIT 0

// Returns a pointer to volatile int with registration state.
// 0 = unregistered.
// 1 = registered.
// other = unspecified.
#define lttngh_REGISTRATION_STATE(registration) (&(registration))

#endif // lttngh_UST_VER

enum lttngh_Level {
  lttngh_Level_EMERG = 0,
  lttngh_Level_ALERT = 1,
  lttngh_Level_CRIT = 2,
  lttngh_Level_ERR = 3,
  lttngh_Level_WARNING = 4,
  lttngh_Level_NOTICE = 5,
  lttngh_Level_INFO = 6,
  lttngh_Level_DEBUG_SYSTEM = 7,
  lttngh_Level_DEBUG_PROGRAM = 8,
  lttngh_Level_DEBUG_PROCESS = 9,
  lttngh_Level_DEBUG_MODULE = 10,
  lttngh_Level_DEBUG_UNIT = 11,
  lttngh_Level_DEBUG_FUNCTION = 12,
  lttngh_Level_DEBUG_LINE = 13,
  lttngh_Level_DEBUG = 14
};

// Level values from tracepoint.h:
#define TRACE_EMERG lttngh_Level_EMERG
#define TRACE_ALERT lttngh_Level_ALERT
#define TRACE_CRIT lttngh_Level_CRIT
#define TRACE_ERR lttngh_Level_ERR
#define TRACE_WARNING lttngh_Level_WARNING
#define TRACE_NOTICE lttngh_Level_NOTICE
#define TRACE_INFO lttngh_Level_INFO
#define TRACE_DEBUG_SYSTEM lttngh_Level_DEBUG_SYSTEM
#define TRACE_DEBUG_PROGRAM lttngh_Level_DEBUG_PROGRAM
#define TRACE_DEBUG_PROCESS lttngh_Level_DEBUG_PROCESS
#define TRACE_DEBUG_MODULE lttngh_Level_DEBUG_MODULE
#define TRACE_DEBUG_UNIT lttngh_Level_DEBUG_UNIT
#define TRACE_DEBUG_FUNCTION lttngh_Level_DEBUG_FUNCTION
#define TRACE_DEBUG_LINE lttngh_Level_DEBUG_LINE
#define TRACE_DEBUG lttngh_Level_DEBUG

// Windows trace compatibility macros (adapted from Win32\evntrace.h):
#define TRACE_LEVEL_CRITICAL lttngh_Level_CRIT // Abnormal exit or termination
#define TRACE_LEVEL_FATAL                                                      \
  lttngh_Level_CRIT // Deprecated name for Abnormal exit or termination
#define TRACE_LEVEL_ERROR lttngh_Level_ERR // Severe errors that need logging
#define TRACE_LEVEL_WARNING                                                    \
  lttngh_Level_WARNING // Warnings such as allocation failure
#define TRACE_LEVEL_INFORMATION                                                \
  lttngh_Level_INFO // Includes non-error cases(e.g. Entry-Exit)
#define TRACE_LEVEL_VERBOSE                                                    \
  lttngh_Level_DEBUG                // Detailed traces from intermediate steps
#define EVENT_TRACE_TYPE_INFO 0x00  // Info or point event
#define EVENT_TRACE_TYPE_START 0x01 // Start event
#define EVENT_TRACE_TYPE_END 0x02   // End event
#define EVENT_TRACE_TYPE_STOP 0x02  // Stop event (WinEvent compatible)
#define EVENT_TRACE_TYPE_DC_START 0x03    // Collection start marker
#define EVENT_TRACE_TYPE_DC_END 0x04      // Collection end marker
#define EVENT_TRACE_TYPE_EXTENSION 0x05   // Extension/continuation
#define EVENT_TRACE_TYPE_REPLY 0x06       // Reply event
#define EVENT_TRACE_TYPE_DEQUEUE 0x07     // De-queue event
#define EVENT_TRACE_TYPE_RESUME 0x07      // Resume event (WinEvent compatible)
#define EVENT_TRACE_TYPE_CHECKPOINT 0x08  // Generic checkpoint event
#define EVENT_TRACE_TYPE_SUSPEND 0x08     // Suspend event (WinEvent compatible)
#define EVENT_TRACE_TYPE_WINEVT_SEND 0x09 // Send Event (WinEvent compatible)
#define EVENT_TRACE_TYPE_WINEVT_RECEIVE                                        \
  0XF0 // Receive Event (WinEvent compatible)

// Windows event compatibility macros (adapted from Win32\winmeta.h):
#define WINEVENT_LEVEL_LOG_ALWAYS lttngh_Level_EMERG
#define WINEVENT_LEVEL_CRITICAL lttngh_Level_CRIT
#define WINEVENT_LEVEL_ERROR lttngh_Level_ERR
#define WINEVENT_LEVEL_WARNING lttngh_Level_WARNING
#define WINEVENT_LEVEL_INFO lttngh_Level_NOTICE
#define WINEVENT_LEVEL_VERBOSE lttngh_Level_DEBUG
#define WINEVT_KEYWORD_ANY 0x0
#define WINEVENT_OPCODE_INFO 0x00      // Info or point event
#define WINEVENT_OPCODE_START 0x01     // Start event
#define WINEVENT_OPCODE_STOP 0x02      // Stop event
#define WINEVENT_OPCODE_DC_START 0x03  // Collection start marker
#define WINEVENT_OPCODE_DC_STOP 0x04   // Collection end marker
#define WINEVENT_OPCODE_EXTENSION 0x05 // Extension/continuation
#define WINEVENT_OPCODE_REPLY 0x06     // Reply event
#define WINEVENT_OPCODE_RESUME 0x07    // Resume event
#define WINEVENT_OPCODE_SUSPEND 0x08   // Suspend event
#define WINEVENT_OPCODE_SEND 0x09      // Send Event
#define WINEVENT_OPCODE_RECEIVE 0XF0   // Receive Event

/*
Type of data provided in a lttngh_DataDesc.
*/
enum lttngh_DataType {

  // Payload data that is not exposed to the bytecode filter, e.g.:
  // - The length data for an atype_sequence field.
  // - The content data for atype_dynamic or atype_struct fields.
  lttngh_DataType_None,

  // Signed integer (little-endian): atype_integer, atype_enum.
  lttngh_DataType_SignedLE,

  // Signed integer (big-endian): atype_integer, atype_enum.
  lttngh_DataType_SignedBE,

  // Unsigned integer (little-endian): atype_integer, atype_enum.
  lttngh_DataType_UnsignedLE,

  // Unsigned integer (big-endian): atype_integer, atype_enum.
  lttngh_DataType_UnsignedBE,

  // Floating-point (little-endian): atype_float.
  lttngh_DataType_FloatLE,

  // Floating-point (big-endian): atype_float.
  lttngh_DataType_FloatBE,

  // NUL-terminated string of 8-bit chars: atype_string.
  lttngh_DataType_String8,

  // Arrays and sequences: atype_array, atype_sequence.
  // - For atype_array, the array length (number of elements) is specified
  //   in the field's lttng_type.u.array.length value and must also be
  //   provided in the DataDesc.Length value.
  // - For atype_sequence, a sequence is expressed by two DataDesc items.
  //   The first DataDesc is created via DataDescCreate with Type = SequenceLength
  //   and contains the sequence's length (number of elements). The second
  //   DataDesc is created via DataDescCreateCounted and contains the data.
  lttngh_DataType_Counted,

  // UTF-16 (host-endian) NUL-terminated string that will be transcoded into
  // a UTF-8 string before storage into the log. The field's lttng_type must
  // be defined as atype_string with UTF8 encoding.
  // Note that when Type = StringUtf16Transcoded, lttngh_EventProbe will
  // use the DataDesc's Length field as scratch space during transcoding.
  lttngh_DataType_StringUtf16Transcoded,

  // UTF-16 (host-endian) string data that will be transcoded into a UTF-8
  // sequence before storage into the log. Unlike a normal sequence (where
  // length is given in a DataDesc with Type = SequenceLength and content is given in
  // a separate DataDesc with Type = Counted), this DataType requires a
  // single DataDesc, which will be transcoded into payload corresponding to
  // a UTF-8 sequence (including both the length and content).
  // In LTTNG-UST 2.12 and before, the field's lttng_type must be defined as
  // follows:
  // - atype = sequence.
  // - u.sequence.length_type = uint16, host-endian.
  // - u.sequence.elem_type = uint8, UTF8 encoding.
  // In LTTNG-UST 2.13 and later, this is encoded as a UINT16 length field
  // followed by a sequence with element = uint8, length_name = NULL, and
  // encoding = UTF8.
  // Note that when Type = SequenceUtf16Transcoded, lttngh_EventProbe will
  // use the DataDesc's Length field as scratch space during transcoding.
  lttngh_DataType_SequenceUtf16Transcoded,

  // UTF-32 (host-endian) NUL-terminated string that will be transcoded into
  // a UTF-8 string before storage into the log. The field's lttng_type must
  // be defined as atype_string with UTF8 encoding.
  // Note that when Type = StringUtf32Transcoded, lttngh_EventProbe will
  // use the DataDesc's Length field as scratch space during transcoding.
  lttngh_DataType_StringUtf32Transcoded,

  // UTF-32 (host-endian) string data that will be transcoded into a UTF-8
  // sequence before storage into the log. Unlike a normal sequence (where
  // length is given in a DataDesc with Type = SequenceLength and content is given in
  // a separate DataDesc with Type = Counted), this DataType requires a
  // single DataDesc, which will be transcoded into payload corresponding to
  // a UTF-8 sequence (including both the length and content).
  // In LTTNG-UST 2.12 and before, the field's lttng_type must be defined as
  // follows:
  // - atype = sequence.
  // - u.sequence.length_type = uint16, host-endian.
  // - u.sequence.elem_type = uint8, UTF8 encoding.
  // In LTTNG-UST 2.13 and later, this is encoded as a UINT16 length field
  // followed by a sequence with element = uint8, length_name = NULL, and
  // encoding = UTF8.
  // Note that when Type = SequenceUtf16Transcoded, lttngh_EventProbe will
  // use the DataDesc's Length field as scratch space during transcoding.
  lttngh_DataType_SequenceUtf32Transcoded,

  // Signed integer (host-endian): atype_integer, atype_enum.
  lttngh_DataType_Signed = __BYTE_ORDER == __LITTLE_ENDIAN
                               ? lttngh_DataType_SignedLE
                               : lttngh_DataType_SignedBE,

  // Unsigned integer (host-endian): atype_integer, atype_enum.
  lttngh_DataType_Unsigned = __BYTE_ORDER == __LITTLE_ENDIAN
                                 ? lttngh_DataType_UnsignedLE
                                 : lttngh_DataType_UnsignedBE,

  // Floating-point (host-endian): atype_float.
  lttngh_DataType_Float = __FLOAT_WORD_ORDER == __LITTLE_ENDIAN
                              ? lttngh_DataType_FloatLE
                              : lttngh_DataType_FloatBE,

  // Use for the length field of a sequence.
  lttngh_DataType_SequenceLength = lttngh_UST_VER >= 213
                                    ? lttngh_DataType_None
                                    : lttngh_DataType_Unsigned
};

/*
The payload for an event is described by an array of DataDesc objects.
Use a DataDescCreate helper to fill in a DataDesc object.

In general, the event payload is simply the concatenation of the data bytes
described by Data and Size, with padding based on Alignment. Exceptions
to this rule (i.e. the cases where Type is used):
- If Type is String8, the string may be padded with '#' if the string's actual
  length is smaller than the length indicated by the Size field.
- If Type is SequenceUtf16Transcoded or SequenceUtf32Transcoded, the event
  payload will be a transcoding of Data, not a copy of Data.

Length is only used by the bytecode filter.
*/
struct lttngh_DataDesc {
  void const *Data;
  uint32_t Size;     // = sizeof(element) * number of elements
  uint8_t Alignment; // = lttngh_ALIGNOF(element)

  // The following fields are mainly for bytecode filtering:
  uint8_t Type;    // Use values from enum lttngh_DataType.
  uint16_t Length; // = number of elements; only used if Type == Counted.
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if lttngh_UST_VER >= 208
/*
Predefined enumeration descriptor for use with bool/BOOL/BOOLEAN.
Note: Generally not used directly. Instead, use lttngh_TypeBool*.
*/
extern const lttngh_ust_enum_desc lttngh_BoolEnumDesc;
#endif // lttngh_UST_VER

/*
Registers a provider. The pRegistration pointer must be zero-filled at
startup and will be atomically updated with registration state.

LTTNG tracks providers by name, and does not support registering multiple
providers with the same name within a process. If this is attempted,
RegisterProvider will return an error code.

It is an error to invoke RegisterProvider when the provider is already
registered (as tracked by pRegistration). If this happens,
RegisterProvider will call abort().
*/
int lttngh_RegisterProvider(
    lttngh_registration *pRegistration,
    lttngh_ust_probe_desc *pProbeDesc,
    struct lttng_ust_tracepoint **pTracepointStart,
    struct lttng_ust_tracepoint **pTracepointStop,
    lttngh_ust_event_desc const **pEventDescStart,
    lttngh_ust_event_desc const **pEventDescStop) lttng_ust_notrace;

/*
Unregisters a provider. The pRegistration state will be atomically updated.

It is not an error to invoke UnregisterProvider when the provider is already
unregistered (as tracked by pRegistration). If this happens,
UnregisterProvider is a no-op and immediately returns.
*/
int lttngh_UnregisterProvider(
    lttngh_registration *pRegistration
#if lttngh_UST_VER < 213
    ,
    lttngh_ust_probe_desc* pProbeDesc,
    struct lttng_ust_tracepoint* const* pTracepointStart
#endif // lttngh_UST_VER
    ) lttng_ust_notrace;

/*
Writes an event with the data from the array of DataDesc objects.
If pCallerIp is NULL, EventProbe will use the immediate caller's address.
Note: lttngh_EventProbe may use pDataDesc[n].Length as scratch space,
which is why pDataDesc is not defined as "const".
*/
int lttngh_EventProbe(struct lttng_ust_tracepoint *pTracepoint,
                      struct lttngh_DataDesc *pDataDesc, unsigned cDataDesc,
                      void *pCallerIp) // usually NULL or lttngh_IP_PARAM
    __attribute__((noinline)) lttng_ust_notrace;

/*
Formats a 4-byte IPv4 address as a nul-terminated string.
Output buffer is assumed to be at least 16 chars.
*/
void lttngh_FormatIPv4(const void* pIPv4, char* buf16) lttng_ust_notrace;
#define LTTNGH_FORMAT_IPV4_LEN 16u // Buffer length for lttngh_FormatIPv4.

/*
Formats a 16-byte IPv6 address as a nul-terminated string.
Output buffer is assumed to be at least LTTNGH_FORMAT_IPV6_LEN chars.
*/
void lttngh_FormatIPv6(const void* pIPv6, char* buf48) lttng_ust_notrace;
#define LTTNGH_FORMAT_IPV6_LEN 48u // Buffer length for lttngh_FormatIPv6.

/*
Formats a sockaddr as a nul-terminated string.
Output buffer is assumed to be at least 65 chars.
*/
void lttngh_FormatSockaddr(const void* pSockaddr, unsigned cbSockaddr,
                           char* buf65) lttng_ust_notrace;
#define LTTNGH_FORMAT_SOCKADDR_LEN 65u // Buffer length for lttngh_FormatSockaddr.

/*
For use by code that is starting an activity.
Generates a new 16-byte activity ID and copies it to pActivityId.
Note that while the generated ID has the structure of a GUID, it is not
guaranteed to be globally-unique. Instead, activity ID generation is
optimized for speed. The generated ID is intended to be locally-unique
(within the current boot session) but may not be globally unique.

Activity types:
- Manually-controlled activities: Activity lifetime can flow across threads
  (or even across processes) as needed, but activity ID must be tracked by
  the developer and directly specified for each event.
- Thread-local (scoped) activities: Activity ID is automatically applied to
  each event (via a thread-local variable), but activity lifetime must
  correspond to a local scope on a single thread, e.g. activity starts at
  function entry and ends at function exit.
  Note that the previous activity ID should be restored at scope exit even
  if the exit is abnormal, i.e. even if scope exit occurs via C++ exception
  (otherwise you're clobbering another scope's activity ID with your own).

Usage pattern for manually-controlled activities:
- Determine the value of parentId, which is the ID of the higher-level
  activity (or NULL if no higher-level activity).
- Call lttngh_ActivityIdCreate to obtain a value for newId.
- Write an activity-start event using Opcode=START, ActivityId=newId, and
  RelatedActivityId=parentId.
- Write info events using ActivityId=newId.
- Write an activity-stop event using Opcode=STOP, ActivityId=newId.

Usage pattern for thread-local (scoped) activities:
- The following pattern assumes that your event generation system (the
  layer between the user and the lttngh_EventProbe function) uses the
  lttngh_ActivityIdFilter function to obtain the activity ID value to use
  when no activity ID is provided by the user.
- Call lttngh_ActivityIdGet to obtain the value of parentId.
- Call lttngh_ActivityIdCreate to obtain a value for newId.
- Call lttngh_ActivityIdSet to change the current thread's ID to newId.
- Write an activity-start event using Opcode=START, ActivityId=NULL, and
  RelatedActivityId=parentId.
- Write info events.
- Write an activity-end event using Opcode=STOP.
- Call lttngh_ActivityIdSet to restore the current thread's ID to parentId.
*/
void lttngh_ActivityIdCreate(void *pNewActivityId) lttng_ust_notrace;

/*
For use by code that is starting a thread-local activity.
Copies the current thread-local 16-byte activity ID to
pCurrentThreadActivityId. Note that when a new thread is created, its
activity ID is initialized to all-zero.

To start a thread-local activity:
- Call lttngh_ActivityIdGet to obtain the value of parentId.
- Call lttngh_ActivityIdCreate to obtain a value for newId.
- Call lttngh_ActivityIdSet to change the current thread's ID to newId.
- Write an activity-start event using Opcode=START, ActivityId=NULL, and
  RelatedActivityId=parentId.
*/
void lttngh_ActivityIdGet(void *pCurrentThreadActivityId) lttng_ust_notrace;

/*
For use by code that is starting or ending a thread-local activity.
Copies the specified 16-byte activity ID to the current thread-local
activity ID.

To start a thread-local activity:
- Call lttngh_ActivityIdGet to obtain the value of parentId.
- Call lttngh_ActivityIdCreate to obtain a value for newId.
- Call lttngh_ActivityIdSet to change the current thread's ID to newId.
- Write an activity-start event using Opcode=START, ActivityId=NULL, and
  RelatedActivityId=parentId.

To end a thread-local activity:
- Write an activity-end event using Opcode=STOP.
- Call lttngh_ActivityIdSet to restore the current thread's ID to parentId.
*/
void lttngh_ActivityIdSet(void const *pNewThreadActivityId) lttng_ust_notrace;

/*
Implementation detail of lttngh_ActivityIdFilter. Do not call directly.
- If current thread-local activity ID is non-zero, return a pointer to the
  current thread-local activity ID.
- Otherwise, return NULL.
*/
void const *lttngh_ActivityIdPeek(void) lttng_ust_notrace;

/*
For use by code that is generating events.
Returns a pointer to the activity ID that should be used for an event.
- If pUserProvidedActivityId != NULL, return pUserProvidedActivityId.
- Otherwise, if current thread-local activity ID is non-zero, return a
  pointer to the current thread-local activity ID.
- Otherwise, return NULL.
*/
static inline void const *
lttngh_ActivityIdFilter(void const *pUserProvidedActivityId) lttng_ust_notrace;
static inline void const *
lttngh_ActivityIdFilter(void const *pUserProvidedActivityId) {
  return pUserProvidedActivityId ? pUserProvidedActivityId
                                 : lttngh_ActivityIdPeek();
}

/*
Constructs a DataDesc object for scalar data.
Use this for types other than array or sequence.
*/
static inline struct lttngh_DataDesc lttngh_DataDescCreate(
    const void *data,          // = &value
    unsigned size,             // = sizeof(value)
    unsigned char alignment,   // = lttngh_ALIGNOF(value)
    enum lttngh_DataType type) // = None, Signed, Unsigned, Float
    lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreate(const void *data, unsigned size, unsigned char alignment,
                      enum lttngh_DataType type) {
  assert(
      type == lttngh_DataType_None || type == lttngh_DataType_SignedLE ||
      type == lttngh_DataType_SignedBE || type == lttngh_DataType_UnsignedLE ||
      type == lttngh_DataType_UnsignedBE || type == lttngh_DataType_FloatLE ||
      type == lttngh_DataType_FloatBE || type == lttngh_DataType_String8);
  struct lttngh_DataDesc dd = {data, size, alignment, (uint8_t)type, 0};
  return dd;
}

/*
Constructs a DataDesc object for a nul-terminated string of 8-bit chars.
The returned DataDesc will have Type = String8.
Note that str must not be NULL and must be NUL-terminated.
*/
static inline struct lttngh_DataDesc
lttngh_DataDescCreateString8(const char *str) lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreateString8(const char *str) {
  struct lttngh_DataDesc dd = {str, (unsigned)strlen(str) + 1,
                               lttngh_ALIGNOF(str[0]), lttngh_DataType_String8,
                               0};
  return dd;
}

/*
Constructs a DataDesc object for the data of an array or sequence.
The returned DataDesc will have Type = Counted.
*/
static inline struct lttngh_DataDesc lttngh_DataDescCreateCounted(
    const void *data,        // = &value
    unsigned size,           // = sizeof(element) * length
    unsigned char alignment, // = lttngh_ALIGNOF(element)
    unsigned length)         // = Number of elements in value
    lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreateCounted(const void *data, unsigned size,
                             unsigned char alignment, unsigned length) {
  struct lttngh_DataDesc dd = {
      data, size, alignment, lttngh_DataType_Counted,
      // Length is only used by bytecode filters. Truncation probably ok.
      (uint16_t)(length > 65535u ? 65535u : length)};
  return dd;
}

/*
Constructs a DataDesc object for a nul-terminated string of UTF-16 chars.
The returned DataDesc will have Type = StringUtf16Transcoded.
Note that str must not be NULL and must be NUL-terminated.
*/
static inline struct lttngh_DataDesc
lttngh_DataDescCreateStringUtf16(const char16_t *str) lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreateStringUtf16(const char16_t *str) {
  const char16_t *strEnd = str;
  while (*strEnd++ != 0) {
  }
  struct lttngh_DataDesc dd = {
      str, (unsigned)(strEnd - str) * (unsigned)sizeof(char16_t),
      lttngh_ALIGNOF(char), lttngh_DataType_StringUtf16Transcoded, 0};
  return dd;
}

/*
Constructs a DataDesc object for a counted string of UTF-16 chars.
The returned DataDesc will have Type = SequenceUtf16Transcoded.
Note that str may be NULL only if length is 0.
*/
static inline struct lttngh_DataDesc lttngh_DataDescCreateSequenceUtf16(
    const char16_t *str, // = &char_array
    uint16_t length)     // = Number of code points in string
    lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreateSequenceUtf16(const char16_t *str, uint16_t length) {
  struct lttngh_DataDesc dd = {str, length * (unsigned)sizeof(char16_t),
                               lttngh_ALIGNOF(uint16_t),
                               lttngh_DataType_SequenceUtf16Transcoded, 0};
  return dd;
}

/*
Constructs a DataDesc object for a nul-terminated string of UTF-32 chars.
The returned DataDesc will have Type = StringUtf32Transcoded.
Note that str must not be NULL and must be NUL-terminated.
*/
static inline struct lttngh_DataDesc
lttngh_DataDescCreateStringUtf32(const char32_t *str) lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreateStringUtf32(const char32_t *str) {
  const char32_t *strEnd = str;
  while (*strEnd++ != 0) {
  }
  struct lttngh_DataDesc dd = {
      str, (unsigned)(strEnd - str) * (unsigned)sizeof(char32_t),
      lttngh_ALIGNOF(char), lttngh_DataType_StringUtf32Transcoded, 0};
  return dd;
}

/*
Constructs a DataDesc object for a counted string of UTF-32 chars.
The returned DataDesc will have Type = SequenceUtf32Transcoded.
Note that str may be NULL only if length is 0.
*/
static inline struct lttngh_DataDesc lttngh_DataDescCreateSequenceUtf32(
    const char32_t *str, // = &char_array
    uint16_t length)     // = Number of code points in string
    lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreateSequenceUtf32(const char32_t *str, uint16_t length) {
  struct lttngh_DataDesc dd = {str, length * (unsigned)sizeof(char32_t),
                               lttngh_ALIGNOF(uint16_t),
                               lttngh_DataType_SequenceUtf32Transcoded, 0};
  return dd;
}

/*
Constructs a DataDesc object for a nul-terminated string of wchar_t chars.
The returned DataDesc will have Type = SequenceUtf16Transcoded or
SequenceUtf32Transcoded depending on the encoding used by wchar_t.
Note that str must not be NULL and must be NUL-terminated.
*/
static inline struct lttngh_DataDesc
lttngh_DataDescCreateStringWchar(const wchar_t *str) lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreateStringWchar(const wchar_t *str) {
#if (__WCHAR_MAX == 0x7fffffff) || (__WCHAR_MAX == 0xffffffff)
  return lttngh_DataDescCreateStringUtf32((char32_t const *)str);
#elif (__WCHAR_MAX == 0x7fff) || (__WCHAR_MAX == 0xffff)
  return lttngh_DataDescCreateStringUtf16((char16_t const *)str);
#else
#error Unsupported wchar_t type.
#endif // __WCHAR_MAX
}

/*
Constructs a DataDesc object for a counted string of wchar_t chars.
The returned DataDesc will have Type = SequenceUtf16Transcoded or
SequenceUtf32Transcoded depending on the encoding used by wchar_t.
Note that str may be NULL only if length is 0.
*/
static inline struct lttngh_DataDesc lttngh_DataDescCreateSequenceWchar(
    const wchar_t *str, // = &char_array
    uint16_t length)    // = Number of code points in string
    lttng_ust_notrace;
static inline struct lttngh_DataDesc
lttngh_DataDescCreateSequenceWchar(const wchar_t *str, uint16_t length) {
#if (__WCHAR_MAX == 0x7fffffff) || (__WCHAR_MAX == 0xffffffff)
  return lttngh_DataDescCreateSequenceUtf32((char32_t const *)str, length);
#elif (__WCHAR_MAX == 0x7fff) || (__WCHAR_MAX == 0xffff)
  return lttngh_DataDescCreateSequenceUtf16((char16_t const *)str, length);
#else
#error Unsupported wchar_t type.
#endif // __WCHAR_MAX
}

#ifdef __cplusplus
} // extern "C"

template<unsigned size, bool is_signed> struct lttngh_UstTypeInt;
template<unsigned size> struct lttngh_UstTypeHexInt;
template<unsigned size> struct lttngh_UstTypeFloat;
template<unsigned size> struct lttngh_UstTypeBool;
template<unsigned size> struct lttngh_UstTypeUtf8Char;

#if lttngh_UST_VER >= 213

template<> struct lttngh_UstTypeInt<1, true> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeInt8; };
template<> struct lttngh_UstTypeInt<2, true> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeInt16; };
template<> struct lttngh_UstTypeInt<4, true> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeInt32; };
template<> struct lttngh_UstTypeInt<8, true> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeInt64; };

template<> struct lttngh_UstTypeInt<1, false> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeUInt8; };
template<> struct lttngh_UstTypeInt<2, false> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeUInt16; };
template<> struct lttngh_UstTypeInt<4, false> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeUInt32; };
template<> struct lttngh_UstTypeInt<8, false> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeUInt64; };

template<> struct lttngh_UstTypeHexInt<1> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeHexInt8; };
template<> struct lttngh_UstTypeHexInt<2> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeHexInt16; };
template<> struct lttngh_UstTypeHexInt<4> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeHexInt32; };
template<> struct lttngh_UstTypeHexInt<8> { static constexpr lttngh_ust_type_integer const& ust_type = lttngh_TypeHexInt64; };

template<> struct lttngh_UstTypeFloat<4> { static constexpr struct lttng_ust_type_float const& ust_type = lttngh_TypeFloat32; };
template<> struct lttngh_UstTypeFloat<8> { static constexpr struct lttng_ust_type_float const& ust_type = lttngh_TypeFloat64; };

template<> struct lttngh_UstTypeBool<1> { static constexpr struct lttng_ust_type_enum const& ust_type = lttngh_TypeBool8; };
template<> struct lttngh_UstTypeBool<4> { static constexpr struct lttng_ust_type_enum const& ust_type = lttngh_TypeBool32; };

template<> struct lttngh_UstTypeUtf8Char<1> { static constexpr struct lttng_ust_type_array const& ust_type = lttngh_TypeUtf8Char; };

#else // lttngh_UST_VER

template<> struct lttngh_UstTypeInt<1, true> { static constexpr struct lttng_type ust_type = lttngh_TypeInt8; };
template<> struct lttngh_UstTypeInt<2, true> { static constexpr struct lttng_type ust_type = lttngh_TypeInt16; };
template<> struct lttngh_UstTypeInt<4, true> { static constexpr struct lttng_type ust_type = lttngh_TypeInt32; };
template<> struct lttngh_UstTypeInt<8, true> { static constexpr struct lttng_type ust_type = lttngh_TypeInt64; };

template<> struct lttngh_UstTypeInt<1, false> { static constexpr struct lttng_type ust_type = lttngh_TypeUInt8; };
template<> struct lttngh_UstTypeInt<2, false> { static constexpr struct lttng_type ust_type = lttngh_TypeUInt16; };
template<> struct lttngh_UstTypeInt<4, false> { static constexpr struct lttng_type ust_type = lttngh_TypeUInt32; };
template<> struct lttngh_UstTypeInt<8, false> { static constexpr struct lttng_type ust_type = lttngh_TypeUInt64; };

template<> struct lttngh_UstTypeHexInt<1> { static constexpr struct lttng_type ust_type = lttngh_TypeHexInt8; };
template<> struct lttngh_UstTypeHexInt<2> { static constexpr struct lttng_type ust_type = lttngh_TypeHexInt16; };
template<> struct lttngh_UstTypeHexInt<4> { static constexpr struct lttng_type ust_type = lttngh_TypeHexInt32; };
template<> struct lttngh_UstTypeHexInt<8> { static constexpr struct lttng_type ust_type = lttngh_TypeHexInt64; };

template<> struct lttngh_UstTypeFloat<4> { static constexpr struct lttng_type ust_type = lttngh_TypeFloat32; };
template<> struct lttngh_UstTypeFloat<8> { static constexpr struct lttng_type ust_type = lttngh_TypeFloat64; };

template<> struct lttngh_UstTypeBool<1> { static constexpr struct lttng_type ust_type = lttngh_TypeBool8; };
template<> struct lttngh_UstTypeBool<4> { static constexpr struct lttng_type ust_type = lttngh_TypeBool32; };

template<> struct lttngh_UstTypeUtf8Char<1> { static constexpr struct lttng_type ust_type = lttngh_TypeUtf8Char; };

#endif // lttngh_UST_VER

#endif // __cplusplus

#endif // _lttnghelpers_h

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <lttngh/LttngHelpers.h>
#include <byteswap.h>
#include <urcu/compiler.h>           // caa_unlikely
#include <urcu/system.h>             // CMM_LOAD_SHARED

#if lttngh_UST_VER >= 213

#include <lttng/ust-ringbuffer-context.h> // lttng_ust_lib_ring_buffer_ctx
#include <lttng/urcu/urcu-ust.h>      // lttng_ust_urcu_read_lock, etc.

typedef struct lttng_ust_event_common lttngh_ust_event_common;

static const lttngh_ust_probe_desc DummyProbeDesc = lttngh_INIT_PROBE_DESC("");

typedef struct lttng_ust_enum_entry lttngh_ust_enum_entry;
static const lttngh_ust_enum_entry BoolEnumEntry0 = lttngh_INIT_ENUM_ENTRY_UNSIGNED("false", 0, 0);
static const lttngh_ust_enum_entry BoolEnumEntry1 = lttngh_INIT_ENUM_ENTRY_UNSIGNED("true", 1, 1);
static const lttngh_ust_enum_entry* BoolEnumEntries[] = {
    &BoolEnumEntry0,
    &BoolEnumEntry1 };
const lttngh_ust_enum_desc lttngh_BoolEnumDesc = lttngh_INIT_ENUM_DESC(&DummyProbeDesc, "bool", BoolEnumEntries, 2);

const struct lttng_ust_type_integer  lttngh_TypeInt8     = lttngh_INIT_TYPE_INTEGER( int8_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeUInt8    = lttngh_INIT_TYPE_INTEGER(uint8_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeHexInt8  = lttngh_INIT_TYPE_INTEGER(uint8_t, 16, 0);

const struct lttng_ust_type_integer  lttngh_TypeInt16    = lttngh_INIT_TYPE_INTEGER( int16_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeUInt16   = lttngh_INIT_TYPE_INTEGER(uint16_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeHexInt16 = lttngh_INIT_TYPE_INTEGER(uint16_t, 16, 0);
const struct lttng_ust_type_integer  lttngh_TypeUInt16BE = lttngh_INIT_TYPE_INTEGER(uint16_t, 10, __BYTE_ORDER == __LITTLE_ENDIAN); // IP PORT

const struct lttng_ust_type_integer  lttngh_TypeInt32    = lttngh_INIT_TYPE_INTEGER( int32_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeUInt32   = lttngh_INIT_TYPE_INTEGER(uint32_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeHexInt32 = lttngh_INIT_TYPE_INTEGER(uint32_t, 16, 0);

const struct lttng_ust_type_integer  lttngh_TypeLong     = lttngh_INIT_TYPE_INTEGER(  signed long, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeULong    = lttngh_INIT_TYPE_INTEGER(unsigned long, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeHexLong  = lttngh_INIT_TYPE_INTEGER(unsigned long, 16, 0);

const struct lttng_ust_type_integer  lttngh_TypeIntPtr   = lttngh_INIT_TYPE_INTEGER( intptr_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeUIntPtr  = lttngh_INIT_TYPE_INTEGER(uintptr_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeHexIntPtr= lttngh_INIT_TYPE_INTEGER(uintptr_t, 16, 0);

const struct lttng_ust_type_integer  lttngh_TypeInt64    = lttngh_INIT_TYPE_INTEGER( int64_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeUInt64   = lttngh_INIT_TYPE_INTEGER(uint64_t, 10, 0);
const struct lttng_ust_type_integer  lttngh_TypeHexInt64 = lttngh_INIT_TYPE_INTEGER(uint64_t, 16, 0);

const struct lttng_ust_type_float    lttngh_TypeFloat32  = lttngh_INIT_TYPE_FLOAT(float);
const struct lttng_ust_type_float    lttngh_TypeFloat64  = lttngh_INIT_TYPE_FLOAT(double);

const struct lttng_ust_type_enum     lttngh_TypeBool8    = lttngh_INIT_TYPE_ENUM(lttngh_TypeUInt8, lttngh_BoolEnumDesc); // bool8 = enum : uint8_t
const struct lttng_ust_type_enum     lttngh_TypeBool32   = lttngh_INIT_TYPE_ENUM(lttngh_TypeInt32, lttngh_BoolEnumDesc); // bool32 = enum : int32_t

const struct lttng_ust_type_sequence lttngh_TypeInt8Sequence     = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeInt8, NULL, lttngh_ALIGNOF(int8_t));
const struct lttng_ust_type_sequence lttngh_TypeUInt8Sequence    = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUInt8, NULL, lttngh_ALIGNOF(int8_t));
const struct lttng_ust_type_sequence lttngh_TypeHexInt8Sequence  = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt8, NULL, lttngh_ALIGNOF(int8_t));

const struct lttng_ust_type_sequence lttngh_TypeInt16Sequence    = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeInt16, NULL, lttngh_ALIGNOF(int16_t));
const struct lttng_ust_type_sequence lttngh_TypeUInt16Sequence   = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUInt16, NULL, lttngh_ALIGNOF(int16_t));
const struct lttng_ust_type_sequence lttngh_TypeHexInt16Sequence = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt16, NULL, lttngh_ALIGNOF(int16_t));

const struct lttng_ust_type_sequence lttngh_TypeInt32Sequence    = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeInt32, NULL, lttngh_ALIGNOF(int32_t));
const struct lttng_ust_type_sequence lttngh_TypeUInt32Sequence   = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUInt32, NULL, lttngh_ALIGNOF(int32_t));
const struct lttng_ust_type_sequence lttngh_TypeHexInt32Sequence = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt32, NULL, lttngh_ALIGNOF(int32_t));

const struct lttng_ust_type_sequence lttngh_TypeLongSequence     = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeLong, NULL, lttngh_ALIGNOF(long));
const struct lttng_ust_type_sequence lttngh_TypeULongSequence    = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeULong, NULL, lttngh_ALIGNOF(long));
const struct lttng_ust_type_sequence lttngh_TypeHexLongSequence  = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexLong, NULL, lttngh_ALIGNOF(long));

const struct lttng_ust_type_sequence lttngh_TypeIntPtrSequence   = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeIntPtr, NULL, lttngh_ALIGNOF(intptr_t));
const struct lttng_ust_type_sequence lttngh_TypeUIntPtrSequence  = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUIntPtr, NULL, lttngh_ALIGNOF(intptr_t));
const struct lttng_ust_type_sequence lttngh_TypeHexIntPtrSequence= lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexIntPtr, NULL, lttngh_ALIGNOF(intptr_t));

const struct lttng_ust_type_sequence lttngh_TypeInt64Sequence    = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeInt64, NULL, lttngh_ALIGNOF(int64_t));
const struct lttng_ust_type_sequence lttngh_TypeUInt64Sequence   = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeUInt64, NULL, lttngh_ALIGNOF(int64_t));
const struct lttng_ust_type_sequence lttngh_TypeHexInt64Sequence = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt64, NULL, lttngh_ALIGNOF(int64_t));

//const struct lttng_ust_type_sequence lttngh_TypeFloat32Sequence = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeFloat32, NULL, lttngh_ALIGNOF(float));
//const struct lttng_ust_type_sequence lttngh_TypeFloat64Sequence = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeFloat64, NULL, lttngh_ALIGNOF(double));

//const struct lttng_ust_type_sequence lttngh_TypeBool8Sequence  = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeBool8ForArray, NULL, lttngh_ALIGNOF(int8_t));
//const struct lttng_ust_type_sequence lttngh_TypeBool32Sequence = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeBool32ForArray, NULL, lttngh_ALIGNOF(int32_t));

const struct lttng_ust_type_string   lttngh_TypeUtf8String   = lttngh_INIT_TYPE_CHAR8_STRING(UTF8);
const struct lttng_ust_type_sequence lttngh_TypeUtf8Sequence = lttngh_INIT_TYPE_CHAR8_SEQUENCE(UTF8, NULL);

const struct lttng_ust_type_array    lttngh_TypeUtf8Char   = lttngh_INIT_TYPE_CHAR8_ARRAY(UTF8, 1);
const struct lttng_ust_type_array    lttngh_TypeGuid       = lttngh_INIT_TYPE_ARRAY(lttngh_TypeHexInt8, 16, lttngh_ALIGNOF(int8_t));
const struct lttng_ust_type_array    lttngh_TypeSystemTime = lttngh_INIT_TYPE_ARRAY(lttngh_TypeUInt16, 8, lttngh_ALIGNOF(int16_t));
const struct lttng_ust_type_array    lttngh_TypeFileTime   = lttngh_INIT_TYPE_ARRAY(lttngh_TypeUInt64, 1, lttngh_ALIGNOF(int64_t));
const struct lttng_ust_type_sequence lttngh_TypeActivityId = lttngh_INIT_TYPE_SEQUENCE(lttngh_TypeHexInt8, NULL, lttngh_ALIGNOF(int8_t));

extern int lttng_ust_tracepoint_module_register(struct lttng_ust_tracepoint* const* tracepoints_start, int tracepoints_count);
extern int lttng_ust_tracepoint_module_unregister(struct lttng_ust_tracepoint* const* tracepoints_start);
#define lttngh_ust_tracepoint_module_register   lttng_ust_tracepoint_module_register
#define lttngh_ust_tracepoint_module_unregister lttng_ust_tracepoint_module_unregister

#define lttngh_ust_ring_buffer_align lttng_ust_ring_buffer_align

#else // lttngh_UST_VER

#include <lttng/ringbuffer-config.h> // lttng_ust_lib_ring_buffer_ctx

typedef struct lttng_event lttngh_ust_event_common;

#if lttngh_UST_VER >= 208
typedef struct lttng_enum_entry lttngh_ust_enum_entry;
static const lttngh_ust_enum_entry BoolEnumEntries[] = {
    lttngh_INIT_ENUM_ENTRY_UNSIGNED("false", 0, 0),
    lttngh_INIT_ENUM_ENTRY_UNSIGNED("true", 1, 1) };
const lttngh_ust_enum_desc lttngh_BoolEnumDesc = lttngh_INIT_ENUM_DESC(NULL, "bool", BoolEnumEntries, 2);
#endif // lttngh_UST_VER

extern void tp_rcu_read_lock_bp(void);
extern void tp_rcu_read_unlock_bp(void);
extern void *tp_rcu_dereference_sym_bp(void *);
#define lttng_ust_urcu_read_lock() tp_rcu_read_lock_bp()
#define lttng_ust_urcu_read_unlock() tp_rcu_read_unlock_bp()
#define lttng_ust_rcu_dereference(p) URCU_FORCE_CAST( \
    __typeof__(p),                                 \
    tp_rcu_dereference_sym_bp(URCU_FORCE_CAST(void *, p)))

extern int tracepoint_register_lib(struct lttng_ust_tracepoint* const* tracepoints_start, int tracepoints_count);
extern int tracepoint_unregister_lib(struct lttng_ust_tracepoint* const* tracepoints_start);
#define lttngh_ust_tracepoint_module_register   tracepoint_register_lib
#define lttngh_ust_tracepoint_module_unregister tracepoint_unregister_lib

#define lttngh_ust_ring_buffer_align lib_ring_buffer_align
#define lttngh_ust_ring_buffer_align_ctx lib_ring_buffer_align_ctx

#endif // lttngh_UST_VER

static unsigned Utf16ToUtf8Size(
    char16_t const *pch16,
    unsigned cch16)
    lttng_ust_notrace;
static unsigned Utf16ToUtf8Size(
    char16_t const *pch16,
    unsigned cch16)
{
    unsigned ich8 = 0;
    unsigned ich16 = 0;

    // Since we never get cch16 > 65535, we can safely skip testing for overflow of ich8.
    assert(cch16 <= (0xFFFFFFFF / 3));

    while (ich16 != cch16)
    {
        // Note that this algorithm accepts unmatched surrogate pairs.
        // That's probably the right decision for logging - we want to preserve
        // them so they can be noticed and fixed.
        unsigned val16 = pch16[ich16];
        ich16 += 1;
        if (caa_likely(val16 < 0x80))
        {
            ich8 += 1;
        }
        else if (caa_likely(val16 < 0x800))
        {
            ich8 += 2;
        }
        else if (
            0xd800 <= val16 && val16 < 0xdc00 &&
            ich16 != cch16 &&
            0xdc00 <= pch16[ich16] && pch16[ich16] < 0xe000)
        {
            // Valid surrogate pair.
            ich16 += 1;
            ich8 += 4;
        }
        else
        {
            ich8 += 3;
        }
    }
    return ich8;
}

static unsigned Utf16ToUtf8(
    char16_t const *pch16,
    unsigned cch16,
    unsigned char *pch8,
    unsigned cch8)
    lttng_ust_notrace;
static unsigned Utf16ToUtf8(
    char16_t const *pch16,
    unsigned cch16,
    unsigned char *pch8,
    unsigned cch8)
{
    unsigned ich8 = 0;
    unsigned ich16 = 0;

    // Since we never get cch16 > 65535, we can safely skip testing for overflow of ich8.
    assert(cch16 <= (0xFFFFFFFF / 3));

    while (ich16 != cch16)
    {
        // Note that this algorithm accepts unmatched surrogate pairs.
        // That's probably the right decision for logging - we want to preserve
        // them so they can be noticed and fixed.
        unsigned val16 = pch16[ich16];
        ich16 += 1;
        if (caa_likely(val16 < 0x80))
        {
            if (caa_unlikely(ich8 == cch8))
                break;
            pch8[ich8++] = (unsigned char)val16;
        }
        else if (caa_likely(val16 < 0x800))
        {
            if (caa_unlikely(ich8 + 1 >= cch8))
                break;
            pch8[ich8++] = (unsigned char)(((val16 >> 6)) | 0xc0);
            pch8[ich8++] = (unsigned char)(((val16)&0x3f) | 0x80);
        }
        else if (
            0xd800 <= val16 && val16 < 0xdc00 &&
            ich16 != cch16 &&
            0xdc00 <= pch16[ich16] && pch16[ich16] < 0xe000)
        {
            // Valid surrogate pair.
            if (caa_unlikely(ich8 + 3 >= cch8))
                break;
            val16 = 0x010000u + (((val16 - 0xd800u) << 10) | (pch16[ich16] - 0xdc00u));
            ich16 += 1;
            pch8[ich8++] = (unsigned char)(((val16 >> 18)) | 0xf0);
            pch8[ich8++] = (unsigned char)(((val16 >> 12) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val16 >> 6) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val16)&0x3f) | 0x80);
        }
        else
        {
            if (caa_unlikely(ich8 + 2 >= cch8))
                break;
            pch8[ich8++] = (unsigned char)(((val16 >> 12)) | 0xe0);
            pch8[ich8++] = (unsigned char)(((val16 >> 6) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val16)&0x3f) | 0x80);
        }
    }

    return ich8;
}

static unsigned Utf32ToUtf8Size(
    char32_t const *pch32,
    unsigned cch32)
    lttng_ust_notrace;
static unsigned Utf32ToUtf8Size(
    char32_t const *pch32,
    unsigned cch32)
{
    unsigned ich8 = 0;
    unsigned ich32 = 0;

    // Since we never get cch32 > 65535, we can safely skip testing for overflow of ich8.
    assert(cch32 <= (0xFFFFFFFF / 7));

    while (ich32 != cch32)
    {
        // Note that this algorithm accepts non-Unicode values (above 0x10FFFF).
        // That's probably the right decision for logging - we want to preserve
        // them so they can be noticed and fixed.
        unsigned val32 = pch32[ich32];
        ich32 += 1;
        if (caa_likely(val32 < 0x80))
        {
            ich8 += 1;
        }
        else if (caa_likely(val32 < 0x800))
        {
            ich8 += 2;
        }
        else if (caa_likely(val32 < 0x10000))
        {
            ich8 += 3;
        }
        else if (caa_likely(val32 < 0x200000))
        {
            ich8 += 4;
        }
        else if (caa_likely(val32 < 0x4000000))
        {
            ich8 += 5;
        }
        else if (caa_likely(val32 < 0x80000000))
        {
            ich8 += 6;
        }
        else
        {
            ich8 += 7;
        }
    }
    return ich8;
}

static unsigned Utf32ToUtf8(
    char32_t const *pch32,
    unsigned cch32,
    unsigned char *pch8,
    unsigned cch8)
    lttng_ust_notrace;
static unsigned Utf32ToUtf8(
    char32_t const *pch32,
    unsigned cch32,
    unsigned char *pch8,
    unsigned cch8)
{
    unsigned ich8 = 0;
    unsigned ich32 = 0;

    // Since we never get cch32 > 65535, we can safely skip testing for overflow of ich8.
    assert(cch32 <= (0xFFFFFFFF / 7));

    while (ich32 != cch32)
    {
        // Note that this algorithm accepts unmatched surrogate pairs.
        // That's probably the right decision for logging - we want to preserve
        // them so they can be noticed and fixed.
        unsigned val32 = pch32[ich32];
        ich32 += 1;
        if (caa_likely(val32 < 0x80))
        {
            if (caa_unlikely(ich8 == cch8))
                break;
            pch8[ich8++] = (unsigned char)val32;
        }
        else if (caa_likely(val32 < 0x800))
        {
            if (caa_unlikely(ich8 + 1 >= cch8))
                break;
            pch8[ich8++] = (unsigned char)(((val32 >> 6)) | 0xc0);
            pch8[ich8++] = (unsigned char)(((val32)&0x3f) | 0x80);
        }
        else if (caa_likely(val32 < 0x10000))
        {
            if (caa_unlikely(ich8 + 2 >= cch8))
                break;
            pch8[ich8++] = (unsigned char)(((val32 >> 12)) | 0xe0);
            pch8[ich8++] = (unsigned char)(((val32 >> 6) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32)&0x3f) | 0x80);
        }
        else if (caa_likely(val32 < 0x200000))
        {
            if (caa_unlikely(ich8 + 3 >= cch8))
                break;
            pch8[ich8++] = (unsigned char)(((val32 >> 18)) | 0xf0);
            pch8[ich8++] = (unsigned char)(((val32 >> 12) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 6) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32)&0x3f) | 0x80);
        }
        else if (caa_likely(val32 < 0x4000000))
        {
            if (caa_unlikely(ich8 + 4 >= cch8))
                break;
            pch8[ich8++] = (unsigned char)(((val32 >> 24)) | 0xf8);
            pch8[ich8++] = (unsigned char)(((val32 >> 18) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 12) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 6) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32)&0x3f) | 0x80);
        }
        else if (caa_likely(val32 < 0x80000000))
        {
            if (caa_unlikely(ich8 + 5 >= cch8))
                break;
            pch8[ich8++] = (unsigned char)(((val32 >> 30)) | 0xfc);
            pch8[ich8++] = (unsigned char)(((val32 >> 24) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 18) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 12) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 6) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32)&0x3f) | 0x80);
        }
        else
        {
            if (caa_unlikely(ich8 + 6 >= cch8))
                break;
            pch8[ich8++] = (unsigned char)0xfe;
            pch8[ich8++] = (unsigned char)(((val32 >> 30) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 24) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 18) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 12) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32 >> 6) & 0x3f) | 0x80);
            pch8[ich8++] = (unsigned char)(((val32)&0x3f) | 0x80);
        }
    }

    return ich8;
}

static void ProviderError(
    lttngh_ust_probe_desc *pProbeDesc,
    int err,
    const char *msg)
    __attribute__((noreturn)) lttng_ust_notrace;
static void ProviderError(
    lttngh_ust_probe_desc *pProbeDesc,
    int err,
    const char *msg)
{
    fprintf(stderr, "LTTng-UST: provider \"%s\" error %d: %s\n",
        lttngh_PROVIDER_NAME(*pProbeDesc),
        err, msg);
    abort();
}

static int FixArrayCompare(
    void const *p1, void const *p2)
    lttng_ust_notrace;
static int FixArrayCompare(
    void const *p1, void const *p2)
{
    // Reverse sort so that NULL goes at end.
    void const *v1 = *(void const *const *)p1;
    void const *v2 = *(void const *const *)p2;
    return v1 < v2 ? 1 : v1 == v2 ? 0 : -1;
}

// Remove duplicates and NULLs from an array of pointers.
static void *FixArray(
    void const **ppStart,
    void const **ppEnd)
    lttng_ust_notrace;
static void *FixArray(
    void const **ppStart,
    void const **ppEnd)
{
    void const **ppGood = ppStart;

    if (ppStart != ppEnd)
    {
        // Sort.
        qsort(ppStart, (size_t)(ppEnd - ppStart), sizeof(void *), FixArrayCompare);

        // Remove adjacent repeated elements.
        for (; ppGood + 1 != ppEnd; ppGood += 1)
        {
            if (*ppGood == *(ppGood + 1))
            {
                void const** ppNext;
                for (ppNext = ppGood + 2; ppNext != ppEnd; ppNext += 1)
                {
                    if (*ppGood != *ppNext)
                    {
                        ppGood += 1;
                        *ppGood = *ppNext;
                    }
                }
                break;
            }
        }

        if (*ppGood != NULL)
        {
            ppGood += 1;
        }
    }

    return ppGood;
}

// ust-tracepoint-event.h(991) __lttng_events_init
int lttngh_RegisterProvider(
    lttngh_registration* pRegistration,
    lttngh_ust_probe_desc *pProbeDesc,
    struct lttng_ust_tracepoint **pTracepointStart,
    struct lttng_ust_tracepoint **pTracepointStop,
    lttngh_ust_event_desc const **pEventDescStart,
    lttngh_ust_event_desc const **pEventDescStop)
{
    int err;
    int volatile* const pIsRegistered = lttngh_REGISTRATION_STATE(*pRegistration);

    // Note: not intended to support multithreaded or ref-counted registration.
    // pRegistration is used to make it safe to call Unregister on an
    // unregistered lib and to help detect misuse, not to make this thread-safe.
    if (__atomic_exchange_n(pIsRegistered, 1, __ATOMIC_RELAXED) != 0)
    {
        err = EEXIST;
        ProviderError(pProbeDesc, err,
                      "provider already registered.");
    }
    else
    {
        lttngh_ust_event_desc const **const pEventDescLast =
            (lttngh_ust_event_desc const **)FixArray(
                (void const **)pEventDescStart, (void const **)pEventDescStop);
        pProbeDesc->event_desc = pEventDescStart;
        pProbeDesc->nr_events = (unsigned)(pEventDescLast - pEventDescStart);
#if lttngh_UST_VER >= 213
        pRegistration->registered_probe = lttng_ust_probe_register(pProbeDesc);
        pRegistration->tracepoint_start = pTracepointStart;
        err = pRegistration->registered_probe ? 0 : ENOMEM;
#else // lttngh_UST_VER
        err = lttng_probe_register(pProbeDesc);
#endif // lttngh_UST_VER
        if (err != 0)
        {
            ProviderError(pProbeDesc, err,
                          "lttng_probe_register failed."
                          " (Registration of multiple providers having the same name is not supported.)");
            __atomic_exchange_n(pIsRegistered, 0, __ATOMIC_RELAXED);
        }
        else
        {
            struct lttng_ust_tracepoint *const *const pTracepointLast =
                (struct lttng_ust_tracepoint *const *)FixArray(
                    (void const **)pTracepointStart, (void const **)pTracepointStop);

            // May fail for out-of-memory. Continue anyway.
            err = lttngh_ust_tracepoint_module_register(pTracepointStart, (int)(pTracepointLast - pTracepointStart));
            if (err != 0)
            {
#if lttngh_UST_VER >= 213
                lttng_ust_probe_unregister(pRegistration->registered_probe);
#else // lttngh_UST_VER
                lttng_probe_unregister(pProbeDesc);
#endif // lttngh_UST_VER

                __atomic_exchange_n(pIsRegistered, 0, __ATOMIC_RELAXED);
            }
        }
    }

    return err;
}

// ust-tracepoint-event.h(1018) __lttng_events_exit
int lttngh_UnregisterProvider(
    lttngh_registration* pRegistration
#if lttngh_UST_VER < 213
    ,
    lttngh_ust_probe_desc* pProbeDesc,
    struct lttng_ust_tracepoint* const* pTracepointStart
#endif // lttngh_UST_VER
    )
{
    int err = 0;
    int volatile* const pIsRegistered = lttngh_REGISTRATION_STATE(*pRegistration);

    // Calling Unregister on an unregistered lib is a safe no-op as long as it
    // doesn't race with a Register (which is a bug in the caller).
    if (__atomic_exchange_n(pIsRegistered, 0, __ATOMIC_RELAXED) != 0)
    {

#if lttngh_UST_VER >= 213
        err = lttngh_ust_tracepoint_module_unregister(pRegistration->tracepoint_start);
        lttng_ust_probe_unregister(pRegistration->registered_probe);
#else // lttngh_UST_VER
        err = lttngh_ust_tracepoint_module_unregister(pTracepointStart);
        lttng_probe_unregister(pProbeDesc);
#endif // lttngh_UST_VER
    }

    return err;
}

static int EventProbeFilter(
    lttngh_ust_event_common const *pEvent,
#if lttngh_UST_VER >= 213
    void* pCallerIp,
#endif
    struct lttngh_DataDesc const *pDataDesc,
    unsigned cDataDesc)
    lttng_ust_notrace;
static int EventProbeFilter(
    lttngh_ust_event_common const *pEvent,
#if lttngh_UST_VER >= 213
    void* pCallerIp,
#endif
    struct lttngh_DataDesc const *pDataDesc,
    unsigned cDataDesc)
{
    int enableRecorder;
    unsigned cbBuffer;
    unsigned i;

    cbBuffer = 0;
    for (i = 0; i != cDataDesc; i += 1)
    {
        switch (pDataDesc[i].Type)
        {
        case lttngh_DataType_None:
            break;
        case lttngh_DataType_SignedLE:
        case lttngh_DataType_SignedBE:
            cbBuffer += (unsigned)sizeof(int64_t);
            break;
        case lttngh_DataType_UnsignedLE:
        case lttngh_DataType_UnsignedBE:
            cbBuffer += (unsigned)sizeof(uint64_t);
            break;
        case lttngh_DataType_FloatLE:
        case lttngh_DataType_FloatBE:
            cbBuffer += (unsigned)sizeof(double);
            break;
        case lttngh_DataType_String8:
        case lttngh_DataType_StringUtf16Transcoded:
        case lttngh_DataType_StringUtf32Transcoded:
#if lttngh_UST_VER >= 213
        case lttngh_DataType_Counted:
#endif
            cbBuffer += (unsigned)sizeof(void *);
            break;
#if lttngh_UST_VER < 213
        case lttngh_DataType_Counted:
#endif
        case lttngh_DataType_SequenceUtf16Transcoded:
        case lttngh_DataType_SequenceUtf32Transcoded:
            cbBuffer += (unsigned)(sizeof(unsigned long) + sizeof(void *));
            break;
        default:
            abort();
            break;
        }
    }

    // Scope for VLA
    {
        char stackData[cbBuffer] __attribute__((aligned(sizeof(size_t)))); // VLA
        char *pStackData = stackData;

        for (i = 0; i != cDataDesc; i += 1)
        {
            switch (pDataDesc[i].Type)
            {
            case lttngh_DataType_None:
                break;
            case lttngh_DataType_Signed:
            {
                switch (pDataDesc[i].Size)
                {
                case 1:
                {
                    int64_t val;
                    val = *(int8_t const *)pDataDesc[i].Data; // Sign-extend
                    memcpy(pStackData, &val, sizeof(int64_t));
                    break;
                }
                case 2:
                {
                    int64_t val;
                    int16_t tempVal;
                    memcpy(&tempVal, pDataDesc[i].Data, sizeof(tempVal));
                    val = tempVal; // Sign-extend
                    memcpy(pStackData, &val, sizeof(int64_t));
                    break;
                }
                case 4:
                {
                    int64_t val;
                    int32_t tempVal;
                    memcpy(&tempVal, pDataDesc[i].Data, sizeof(tempVal));
                    val = tempVal; // Sign-extend
                    memcpy(pStackData, &val, sizeof(int64_t));
                    break;
                }
                case 8:
                {
                    memcpy(pStackData, pDataDesc[i].Data, sizeof(int64_t));
                    break;
                }
                default:
                    abort();
                    break;
                }
                pStackData += sizeof(int64_t);
                break;
            }
            case (lttngh_DataType_Signed == lttngh_DataType_SignedLE
                ? lttngh_DataType_SignedBE
                : lttngh_DataType_SignedLE):
                {
                    switch (pDataDesc[i].Size)
                    {
                    case 1:
                    {
                        int64_t val;
                        val = *(int8_t const *)pDataDesc[i].Data; // Sign-extend
                        memcpy(pStackData, &val, sizeof(int64_t));
                        break;
                    }
                    case 2:
                    {
                        int64_t val;
                        int16_t tempVal;
                        uint16_t tempValSwap;
                        memcpy(&tempValSwap, pDataDesc[i].Data, sizeof(tempValSwap));
                        tempValSwap = bswap_16(tempValSwap);
                        memcpy(&tempVal, &tempValSwap, sizeof(tempVal));
                        val = tempVal; // Sign-extend
                        memcpy(pStackData, &val, sizeof(int64_t));
                        break;
                    }
                    case 4:
                    {
                        int64_t val;
                        int32_t tempVal;
                        uint32_t tempValSwap;
                        memcpy(&tempValSwap, pDataDesc[i].Data, sizeof(tempValSwap));
                        tempValSwap = bswap_32(tempValSwap);
                        memcpy(&tempVal, &tempValSwap, sizeof(tempVal));
                        val = tempVal; // Sign-extend
                        memcpy(pStackData, &val, sizeof(int64_t));
                        break;
                    }
                    case 8:
                    {
                        uint64_t tempValSwap;
                        memcpy(&tempValSwap, pDataDesc[i].Data, sizeof(tempValSwap));
                        tempValSwap = bswap_64(tempValSwap);
                        memcpy(pStackData, &tempValSwap, sizeof(int64_t));
                        break;
                    }
                    default:
                        abort();
                        break;
                    }
                    pStackData += sizeof(int64_t);
                    break;
                }
            case lttngh_DataType_Unsigned:
            {
                switch (pDataDesc[i].Size)
                {
                case 1:
                {
                    uint64_t val;
                    val = *(uint8_t const *)pDataDesc[i].Data; // Zero-extend
                    memcpy(pStackData, &val, sizeof(uint64_t));
                    break;
                }
                case 2:
                {
                    uint64_t val;
                    uint16_t tempVal;
                    memcpy(&tempVal, pDataDesc[i].Data, sizeof(tempVal));
                    val = tempVal; // Zero-extend
                    memcpy(pStackData, &val, sizeof(uint64_t));
                    break;
                }
                case 4:
                {
                    uint64_t val;
                    uint32_t tempVal;
                    memcpy(&tempVal, pDataDesc[i].Data, sizeof(tempVal));
                    val = tempVal; // Zero-extend
                    memcpy(pStackData, &val, sizeof(uint64_t));
                    break;
                }
                case 8:
                {
                    memcpy(pStackData, pDataDesc[i].Data, sizeof(uint64_t));
                    break;
                }
                default:
                    abort();
                    break;
                }
                pStackData += sizeof(uint64_t);
                break;
            }
            case (lttngh_DataType_Unsigned == lttngh_DataType_UnsignedLE
                ? lttngh_DataType_UnsignedBE
                : lttngh_DataType_UnsignedLE):
                {
                    switch (pDataDesc[i].Size)
                    {
                    case 1:
                    {
                        uint64_t val;
                        val = *(uint8_t const *)pDataDesc[i].Data; // Zero-extend
                        memcpy(pStackData, &val, sizeof(uint64_t));
                        break;
                    }
                    case 2:
                    {
                        uint64_t val;
                        uint16_t tempValSwap;
                        memcpy(&tempValSwap, pDataDesc[i].Data, sizeof(tempValSwap));
                        tempValSwap = bswap_16(tempValSwap);
                        val = tempValSwap; // Zero-extend
                        memcpy(pStackData, &val, sizeof(uint64_t));
                        break;
                    }
                    case 4:
                    {
                        uint64_t val;
                        uint32_t tempValSwap;
                        memcpy(&tempValSwap, pDataDesc[i].Data, sizeof(tempValSwap));
                        tempValSwap = bswap_32(tempValSwap);
                        val = tempValSwap; // Zero-extend
                        memcpy(pStackData, &val, sizeof(uint64_t));
                        break;
                    }
                    case 8:
                    {
                        uint64_t tempValSwap;
                        memcpy(&tempValSwap, pDataDesc[i].Data, sizeof(tempValSwap));
                        tempValSwap = bswap_64(tempValSwap);
                        memcpy(pStackData, &tempValSwap, sizeof(uint64_t));
                        break;
                    }
                    default:
                        abort();
                        break;
                    }
                    pStackData += sizeof(uint64_t);
                    break;
                }
            case lttngh_DataType_Float:
            {
                if (pDataDesc[i].Size == sizeof(float))
                {
                    double val;
                    float tempVal;
                    memcpy(&tempVal, pDataDesc[i].Data, sizeof(tempVal));
                    val = tempVal; // Convert
                    memcpy(pStackData, &val, sizeof(double));
                }
                else if (pDataDesc[i].Size == sizeof(double))
                {
                    memcpy(pStackData, pDataDesc[i].Data, sizeof(double));
                }
                else if (pDataDesc[i].Size == sizeof(long double))
                {
                    double val;
                    long double tempVal;
                    memcpy(&tempVal, pDataDesc[i].Data, sizeof(tempVal));
                    val = (double)tempVal; // Convert
                    memcpy(pStackData, &val, sizeof(double));
                }
                else
                {
                    abort();
                }
                pStackData += sizeof(double);
                break;
            }
            case (lttngh_DataType_Float == lttngh_DataType_FloatLE
                ? lttngh_DataType_FloatBE
                : lttngh_DataType_FloatLE):
                {
                    if (pDataDesc[i].Size == sizeof(float))
                    {
                        double val;
                        float tempVal;
                        uint32_t tempValSwap;
                        memcpy(&tempValSwap, pDataDesc[i].Data, sizeof(tempValSwap));
                        tempValSwap = bswap_32(tempValSwap);
                        memcpy(&tempVal, &tempValSwap, sizeof(tempVal));
                        val = tempVal; // Convert
                        memcpy(pStackData, &val, sizeof(double));
                    }
                    else if (pDataDesc[i].Size == sizeof(double))
                    {
                        uint64_t tempValSwap;
                        memcpy(&tempValSwap, pDataDesc[i].Data, sizeof(tempValSwap));
                        tempValSwap = bswap_64(tempValSwap);
                        memcpy(pStackData, &tempValSwap, sizeof(double));
                    }
                    else if (pDataDesc[i].Size == sizeof(long double))
                    {
                        double val;
                        long double tempVal;
                        char tempValSwap[sizeof(long double)];
                        char const *p = (char const*)pDataDesc[i].Data;
                        unsigned iSwap = sizeof(long double);
                        do
                        {
                            iSwap -= 1;
                            tempValSwap[iSwap] = *p;
                            p += 1;
                        } while (iSwap != 0);
                        memcpy(&tempVal, &tempValSwap, sizeof(tempVal));
                        val = (double)tempVal; // Convert
                        memcpy(pStackData, &val, sizeof(double));
                    }
                    else
                    {
                        abort();
                    }
                    pStackData += sizeof(double);
                    break;
                }
            case lttngh_DataType_String8:
            case lttngh_DataType_StringUtf16Transcoded:
            case lttngh_DataType_StringUtf32Transcoded:
#if lttngh_UST_VER >= 213
            case lttngh_DataType_Counted:
#endif
            {
                // TODO - convert to utf8 for filtering?
                memcpy(pStackData, &pDataDesc[i].Data, sizeof(void *));
                pStackData += sizeof(void *);
                break;
            }
#if lttngh_UST_VER < 213
            case lttngh_DataType_Counted:
            {
                unsigned long len = pDataDesc[i].Length;
                memcpy(pStackData, &len, sizeof(unsigned long));
                pStackData += sizeof(unsigned long);
                memcpy(pStackData, &pDataDesc[i].Data, sizeof(void *));
                pStackData += sizeof(void *);
                break;
            }
#endif // lttngh_UST_VER
            case lttngh_DataType_SequenceUtf16Transcoded:
            case lttngh_DataType_SequenceUtf32Transcoded:
            {
                // TODO - convert to utf8 for filtering?
                unsigned long len = pDataDesc[i].Size; // Type says "count of utf-8 bytes", so use size.
                memcpy(pStackData, &len, sizeof(unsigned long));
                pStackData += sizeof(unsigned long);
                memcpy(pStackData, &pDataDesc[i].Data, sizeof(void *));
                pStackData += sizeof(void *);
                break;
            }
            default:
                abort();
                break;
            }
        }

#if lttngh_UST_VER >= 213

        struct lttng_ust_probe_ctx probeCtx = {
            .struct_size = sizeof(probeCtx),
            .ip = pCallerIp };
        enableRecorder =
            !pEvent->eval_filter ||
            pEvent->run_filter(pEvent, stackData, &probeCtx, NULL) == LTTNG_UST_EVENT_FILTER_ACCEPT;
        if (enableRecorder &&
            pEvent->type == LTTNG_UST_EVENT_TYPE_NOTIFIER)
        {
            struct lttng_ust_event_notifier* pEventNotifier = (struct lttng_ust_event_notifier*)pEvent->child;
            struct lttng_ust_notification_ctx context = {
                .struct_size = sizeof(struct lttng_ust_notification_ctx),
                .eval_capture = CMM_ACCESS_ONCE(pEventNotifier->eval_capture) };
            pEventNotifier->notification_send(pEventNotifier, stackData, &probeCtx, &context);
        }

#else // lttngh_UST_VER

        enableRecorder = pEvent->has_enablers_without_bytecode;
        struct lttng_bytecode_runtime* pRuntime;
        for (pRuntime = cds_list_entry(
                 lttng_ust_rcu_dereference(pEvent->bytecode_runtime_head.next), __typeof__(*pRuntime), node);
             &pRuntime->node != &pEvent->bytecode_runtime_head;
             pRuntime = cds_list_entry(
                 lttng_ust_rcu_dereference(pRuntime->node.next), __typeof__(*pRuntime), node))
        {
            if (caa_unlikely(pRuntime->filter(pRuntime, stackData) & LTTNG_FILTER_RECORD_FLAG))
            {
                enableRecorder = 1;
            }
        }

#endif // lttngh_UST_VER
    }

    return enableRecorder;
}

struct lttngh_EventProbeContext
{
    unsigned maxAlign;
    unsigned cbData;
    unsigned char* pbTranscodeScratch;
    unsigned cbTranscodeScratch;
};

static int EventProbeComputeSizes(
    struct lttngh_EventProbeContext* pContext,
    struct lttngh_DataDesc* pDataDesc,
    unsigned cDataDesc)
    lttng_ust_notrace;
static int EventProbeComputeSizes(
    struct lttngh_EventProbeContext* pContext,
    struct lttngh_DataDesc* pDataDesc,
    unsigned cDataDesc)
{
    unsigned cbTranscodeWanted = 0;

    pContext->maxAlign = 1;
    assert(pContext->cbData == 0);

    unsigned i;
    for (i = 0; i != cDataDesc; i += 1)
    {
        switch (pDataDesc[i].Type)
        {
        case lttngh_DataType_StringUtf16Transcoded:
        case lttngh_DataType_StringUtf32Transcoded:
        {
            unsigned cbUtf8;

            if (pDataDesc[i].Type == lttngh_DataType_StringUtf16Transcoded)
            {
                unsigned cch16 = (unsigned)(pDataDesc[i].Size / sizeof(char16_t));
                assert(cch16 != 0); // Error in caller - Size should have included NUL.
                cch16 -= 1;         // Do not count NUL.

                if (caa_unlikely(cch16 > 65535))
                {
                    if (cch16 == ~0u)
                    {
                        cch16 = 0; // Error in caller - Size was 0.
                    }
                    else
                    {
                        cch16 = 65535;
                    }
                }

                cbUtf8 = Utf16ToUtf8Size((char16_t const*)pDataDesc[i].Data, cch16);
            }
            else
            {
                unsigned cch32 = (unsigned)(pDataDesc[i].Size / sizeof(char32_t));
                assert(cch32 != 0); // Error in caller - Size should have included NUL.
                cch32 -= 1;         // Do not count NUL.

                if (caa_unlikely(cch32 > 65535))
                {
                    if (cch32 == ~0u)
                    {
                        cch32 = 0; // Error in caller - Size was 0.
                    }
                    else
                    {
                        cch32 = 65535;
                    }
                }

                cbUtf8 = Utf32ToUtf8Size((char32_t const*)pDataDesc[i].Data, cch32);
            }

            if (caa_unlikely(cbUtf8 > 65535))
            {
                cbUtf8 = 65535;
            }

            // Use DataDesc.Length as scratch space to store utf-8 content size.
            pDataDesc[i].Length = (uint16_t)cbUtf8; // Does not count NUL.

            cbUtf8 += 1; // Include room for 8-bit NUL.
            if (caa_unlikely(cbTranscodeWanted < cbUtf8))
            {
                cbTranscodeWanted = cbUtf8;
            }

            pContext->cbData += cbUtf8;
            if (caa_unlikely(pContext->cbData < cbUtf8))
            {
                return EOVERFLOW;
            }
            break;
        }
        case lttngh_DataType_SequenceUtf16Transcoded:
        case lttngh_DataType_SequenceUtf32Transcoded:
        {
#if lttngh_UST_RING_BUFFER_NATURAL_ALIGN
            if (__alignof__(uint16_t) > pContext->maxAlign)
            {
                pContext->maxAlign = __alignof__(uint16_t);
            }

            unsigned const alignAdjust =
                lttngh_ust_ring_buffer_align(pContext->cbData, __alignof__(uint16_t));
            pContext->cbData += alignAdjust;
            if (caa_unlikely(pContext->cbData < alignAdjust))
            {
                return EOVERFLOW;
            }
#endif // lttngh_UST_RING_BUFFER_NATURAL_ALIGN

            unsigned cbUtf8;

            if (pDataDesc[i].Type == lttngh_DataType_SequenceUtf16Transcoded)
            {
                unsigned cch16 = (unsigned)(pDataDesc[i].Size / sizeof(char16_t));
                if (caa_unlikely(cch16 > 65535))
                {
                    cch16 = 65535;
                }

                cbUtf8 = Utf16ToUtf8Size((char16_t const*)pDataDesc[i].Data, cch16);
            }
            else
            {
                unsigned cch32 = (unsigned)(pDataDesc[i].Size / sizeof(char32_t));
                if (caa_unlikely(cch32 > 65535))
                {
                    cch32 = 65535;
                }

                cbUtf8 = Utf32ToUtf8Size((char32_t const*)pDataDesc[i].Data, cch32);
            }

            if (caa_unlikely(cbUtf8 > 65535))
            {
                cbUtf8 = 65535;
            }

            // Use DataDesc.Length as scratch space to store utf-8 content size.
            pDataDesc[i].Length = (uint16_t)cbUtf8;

            cbUtf8 += 2; // Include room for 16-bit length.
            if (caa_unlikely(cbTranscodeWanted < cbUtf8))
            {
                cbTranscodeWanted = cbUtf8;
            }

            pContext->cbData += cbUtf8;
            if (caa_unlikely(pContext->cbData < cbUtf8))
            {
                return EOVERFLOW;
            }
            break;
        }
        default:
        {
#if lttngh_UST_RING_BUFFER_NATURAL_ALIGN
            if (pDataDesc[i].Alignment > pContext->maxAlign)
            {
                pContext->maxAlign = pDataDesc[i].Alignment;
            }

            unsigned const alignAdjust =
                lttngh_ust_ring_buffer_align(pContext->cbData, pDataDesc[i].Alignment);
            pContext->cbData += alignAdjust;
            if (caa_unlikely(pContext->cbData < alignAdjust))
            {
                return EOVERFLOW;
            }
#endif // lttngh_UST_RING_BUFFER_NATURAL_ALIGN

            pContext->cbData += pDataDesc[i].Size;
            if (pContext->cbData < pDataDesc[i].Size)
            {
                return EOVERFLOW;
            }
            break;
        }
        }
    }

    // If our scratch buffer is too small, try to heap allocate.
    if (caa_unlikely(pContext->cbTranscodeScratch < cbTranscodeWanted))
    {
        unsigned char* pbTranscodeWanted = (unsigned char*)malloc(cbTranscodeWanted);
        if (caa_unlikely(pbTranscodeWanted == NULL))
        {
            return ENOMEM;
        }

        pContext->cbTranscodeScratch = cbTranscodeWanted;
        pContext->pbTranscodeScratch = pbTranscodeWanted;
    }

    return 0;
}

static uint16_t EventProbeTranscodeString(
    struct lttngh_EventProbeContext* pContext,
    struct lttngh_DataDesc const* pDataDesc)
    lttng_ust_notrace;
static uint16_t EventProbeTranscodeString(
    struct lttngh_EventProbeContext* pContext,
    struct lttngh_DataDesc const* pDataDesc)
{
    assert(pDataDesc->Length <= pContext->cbTranscodeScratch - 1);

    uint16_t cbUtf8Written;
    if (pDataDesc->Type == lttngh_DataType_StringUtf16Transcoded)
    {
        cbUtf8Written = (uint16_t)Utf16ToUtf8(
            (char16_t const*)pDataDesc->Data, (unsigned)(pDataDesc->Size / sizeof(char16_t)),
            pContext->pbTranscodeScratch, pDataDesc->Length);
    }
    else
    {
        cbUtf8Written = (uint16_t)Utf32ToUtf8(
            (char32_t const*)pDataDesc->Data, (unsigned)(pDataDesc->Size / sizeof(char32_t)),
            pContext->pbTranscodeScratch, pDataDesc->Length);
    }

    pContext->pbTranscodeScratch[cbUtf8Written] = 0;
    size_t iNul = strlen((char*)pContext->pbTranscodeScratch);
    if (caa_unlikely(iNul != pDataDesc->Length))
    {
        // The data was changed on another thread or was truncated at a multi-byte char, so append #s.
        assert(iNul <= cbUtf8Written);
        assert(cbUtf8Written < pDataDesc->Length);
        memset(pContext->pbTranscodeScratch + iNul,
            '#', pDataDesc->Length - iNul);
        pContext->pbTranscodeScratch[pDataDesc->Length] = 0;
    }

    return cbUtf8Written;
}

static uint16_t EventProbeTranscodeSequence(
    struct lttngh_EventProbeContext* pContext,
    struct lttngh_DataDesc const* pDataDesc)
    lttng_ust_notrace;
static uint16_t EventProbeTranscodeSequence(
    struct lttngh_EventProbeContext* pContext,
    struct lttngh_DataDesc const* pDataDesc)
{
    assert(pDataDesc->Length <= pContext->cbTranscodeScratch - sizeof(uint16_t));

    uint16_t cbUtf8Written;
    if (pDataDesc->Type == lttngh_DataType_SequenceUtf16Transcoded)
    {
        cbUtf8Written = (uint16_t)Utf16ToUtf8(
            (char16_t const*)pDataDesc->Data, (unsigned)(pDataDesc->Size / sizeof(char16_t)),
            pContext->pbTranscodeScratch + sizeof(uint16_t), pDataDesc->Length);
    }
    else
    {
        cbUtf8Written = (uint16_t)Utf32ToUtf8(
            (char32_t const*)pDataDesc->Data, (unsigned)(pDataDesc->Size / sizeof(char32_t)),
            pContext->pbTranscodeScratch + sizeof(uint16_t), pDataDesc->Length);
    }

    if (caa_unlikely(cbUtf8Written != pDataDesc->Length))
    {
        // The data was changed on another thread or was truncated at a multi-byte char, so append #s.
        assert(cbUtf8Written < pDataDesc->Length);
        memset(pContext->pbTranscodeScratch + sizeof(uint16_t) + cbUtf8Written,
            '#', pDataDesc->Length - (unsigned)cbUtf8Written);
        cbUtf8Written = pDataDesc->Length;
    }

    memcpy(pContext->pbTranscodeScratch, &cbUtf8Written, sizeof(uint16_t)); // Fill in length.
    return cbUtf8Written;
}

int lttngh_EventProbe(
    struct lttng_ust_tracepoint *pTracepoint,
    struct lttngh_DataDesc *pDataDesc,
    unsigned cDataDesc,
    void *pCallerIp)
{
    int err = 0;
    struct lttng_ust_tracepoint_probe *pTpProbe;
    unsigned char transcodeScratchOnStack[256] __attribute__((aligned(2)));

    struct lttngh_EventProbeContext context = {
        .maxAlign = 0, // 0 means we haven't computed event size yet.
        .cbData = 0,
        .pbTranscodeScratch = transcodeScratchOnStack,
        .cbTranscodeScratch = sizeof(transcodeScratchOnStack) };

    if (pCallerIp == NULL)
    {
        pCallerIp = lttngh_IP_PARAM;
    }

    lttng_ust_urcu_read_lock();
    pTpProbe = lttng_ust_rcu_dereference(pTracepoint->probes);
    if (caa_likely(pTpProbe))
    {
        do
        {
            lttngh_ust_event_common* const pEvent = (lttngh_ust_event_common* const)pTpProbe->data;

#if lttngh_UST_VER >= 213

            if (caa_likely(pEvent->type == LTTNG_UST_EVENT_TYPE_RECORDER))
            {
                struct lttng_ust_event_recorder* pEventRecorder = (struct lttng_ust_event_recorder*)pEvent->child;
                struct lttng_ust_channel_common* pChannelCommon = pEventRecorder->chan->parent;

                if (
#ifdef TP_SESSION_CHECK // Are we building statedump?
                    session != pChannelCommon->session ||
#endif // TP_SESSION_CHECK
                    caa_unlikely(!CMM_ACCESS_ONCE(pChannelCommon->session->active)) ||
                    caa_unlikely(!CMM_ACCESS_ONCE(pChannelCommon->enabled)))
                {
                    continue;
                }

                if (caa_unlikely(!CMM_ACCESS_ONCE(pEvent->enabled)))
                {
                    continue;
                }

                if (caa_unlikely(CMM_ACCESS_ONCE(pEvent->eval_filter)) &&
                    !caa_unlikely(EventProbeFilter(pEvent, pCallerIp, pDataDesc, cDataDesc)))
                {
                    continue;
                }

                // Compute event size the first time through.
                if (caa_likely(context.maxAlign == 0))
                {
                    err = EventProbeComputeSizes(&context, pDataDesc, cDataDesc);
                    if (err != 0)
                    {
                        goto Done;
                    }
                }

                struct lttng_ust_channel_buffer* pChannel = pEventRecorder->chan;

                struct lttng_ust_ring_buffer_ctx bufferContext;
                lttng_ust_ring_buffer_ctx_init(&bufferContext, pEventRecorder, context.cbData, context.maxAlign, pCallerIp);

                int const reserveResult = pChannel->ops->event_reserve(&bufferContext);
                if (caa_unlikely(reserveResult < 0))
                {
                    err = reserveResult;
                }
                else
                {
                    unsigned i;
                    for (i = 0; i != cDataDesc; i += 1)
                    {
                        uint16_t cbUtf8Written;
                        switch (pDataDesc[i].Type)
                        {
                        case lttngh_DataType_String8:
                            pChannel->ops->event_strcpy(&bufferContext, pDataDesc[i].Data, pDataDesc[i].Size);
                            break;

                        case lttngh_DataType_StringUtf16Transcoded:
                        case lttngh_DataType_StringUtf32Transcoded:
                            cbUtf8Written = EventProbeTranscodeString(&context, &pDataDesc[i]);
                            pChannel->ops->event_write(&bufferContext, context.pbTranscodeScratch, cbUtf8Written + 1u, pDataDesc[i].Alignment);
                            break;

                        case lttngh_DataType_SequenceUtf16Transcoded:
                        case lttngh_DataType_SequenceUtf32Transcoded:
                            cbUtf8Written = EventProbeTranscodeSequence(&context, &pDataDesc[i]);
                            pChannel->ops->event_write(&bufferContext, context.pbTranscodeScratch, sizeof(uint16_t) + cbUtf8Written, pDataDesc[i].Alignment);
                            break;

                        default:
                            pChannel->ops->event_write(&bufferContext, pDataDesc[i].Data, pDataDesc[i].Size, pDataDesc[i].Alignment);
                            break;
                        }
                    }

                    pChannel->ops->event_commit(&bufferContext);
                }
            }
            else if (caa_likely(CMM_ACCESS_ONCE(pEvent->enabled)))
            {
                if (caa_likely(pEvent->type == LTTNG_UST_EVENT_TYPE_NOTIFIER) ||
                    caa_unlikely(CMM_ACCESS_ONCE(pEvent->eval_filter)))
                {
                    (void)EventProbeFilter(pEvent, pCallerIp, pDataDesc, cDataDesc);
                }
            }

#else // lttngh_UST_VER

            struct lttng_channel const *const pChannel = pEvent->chan;

            if (caa_likely(CMM_ACCESS_ONCE(pChannel->session->active)) &&
                caa_likely(CMM_ACCESS_ONCE(pChannel->enabled)) &&
                caa_likely(CMM_ACCESS_ONCE(pEvent->enabled)) &&
#ifdef TP_SESSION_CHECK // Are we building statedump?
                session == pChannel->session &&
#endif // TP_SESSION_CHECK
                (caa_likely(cds_list_empty(&pEvent->bytecode_runtime_head)) ||
                 caa_unlikely(EventProbeFilter(pEvent, pDataDesc, cDataDesc))))
            {
                // Compute event size the first time through.
                if (caa_likely(context.maxAlign == 0))
                {
                    err = EventProbeComputeSizes(&context, pDataDesc, cDataDesc);
                    if (err != 0)
                    {
                        goto Done;
                    }
                }

#if lttngh_UST_VER >= 208
                struct lttng_stack_ctx stackContext;
                memset(&stackContext, 0, sizeof(stackContext));

                stackContext.event = pEvent;
                stackContext.chan_ctx = lttng_ust_rcu_dereference(pChannel->ctx);
                stackContext.event_ctx = lttng_ust_rcu_dereference(pEvent->ctx);
#endif // lttngh_UST_VER >= 208

                struct lttng_ust_lib_ring_buffer_ctx bufferContext;
                lib_ring_buffer_ctx_init(&bufferContext, pChannel->chan, pEvent,
                    context.cbData, (int)context.maxAlign, -1, pChannel->handle
#if lttngh_UST_VER >= 208
                                         ,
                                         &stackContext
#endif // lttngh_UST_VER >= 208
                );
                bufferContext.ip = pCallerIp;

                int const reserveResult = pChannel->ops->event_reserve(&bufferContext, pEvent->id);
                if (caa_unlikely(reserveResult < 0))
                {
                    err = reserveResult;
                }
                else
                {
                    unsigned i;
                    for (i = 0; i != cDataDesc; i += 1)
                    {
                        uint16_t cbUtf8Written;
#if lttngh_UST_RING_BUFFER_NATURAL_ALIGN
                        lttngh_ust_ring_buffer_align_ctx(&bufferContext, pDataDesc[i].Alignment);
#endif // lttngh_UST_RING_BUFFER_NATURAL_ALIGN
                        switch (pDataDesc[i].Type)
                        {
                        case lttngh_DataType_String8:
                            if (pChannel->ops->u.has_strcpy)
                            {
                                pChannel->ops->event_strcpy(&bufferContext, (char const*)pDataDesc[i].Data, pDataDesc[i].Size);
                            }
                            else
                            {
                                pChannel->ops->event_write(&bufferContext, pDataDesc[i].Data, pDataDesc[i].Size);
                            }
                            break;

                        case lttngh_DataType_StringUtf16Transcoded:
                        case lttngh_DataType_StringUtf32Transcoded:
                            cbUtf8Written = EventProbeTranscodeString(&context, &pDataDesc[i]);
                            pChannel->ops->event_write(&bufferContext, context.pbTranscodeScratch, cbUtf8Written + 1u);
                            break;

                        case lttngh_DataType_SequenceUtf16Transcoded:
                        case lttngh_DataType_SequenceUtf32Transcoded:
                            cbUtf8Written = EventProbeTranscodeSequence(&context, &pDataDesc[i]);
                            pChannel->ops->event_write(&bufferContext, context.pbTranscodeScratch, sizeof(uint16_t) + cbUtf8Written);
                            break;

                        default:
                            pChannel->ops->event_write(&bufferContext, pDataDesc[i].Data, pDataDesc[i].Size);
                            break;
                        }
                    }

                    pChannel->ops->event_commit(&bufferContext);
                }
            }
#endif // lttngh_UST_VER
        } while ((++pTpProbe)->data);
    }

Done:

    lttng_ust_urcu_read_unlock();

    if (caa_unlikely(context.pbTranscodeScratch != transcodeScratchOnStack))
    {
        free(context.pbTranscodeScratch);
    }

    return err;
}

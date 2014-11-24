#pragma once

/// \file

#include <Foundation/Basics.h>

/// \brief Declares an id type, see generic id below how to use this
#define EZ_DECLARE_ID_TYPE(name, instanceIndexBits, generationBits) \
  static const StorageType MAX_INSTANCES = (1ULL << instanceIndexBits); \
  static const StorageType INVALID_INSTANCE_INDEX = MAX_INSTANCES - 1; \
  static const StorageType INDEX_AND_GENERATION_MASK = (1ULL << (instanceIndexBits + generationBits)) - 1; \
  EZ_DECLARE_POD_TYPE(); \
  EZ_FORCE_INLINE name() { m_Data = INVALID_INSTANCE_INDEX; } \
  EZ_FORCE_INLINE bool operator==(const name other) const { return m_Data == other.m_Data; } \
  EZ_FORCE_INLINE bool operator!=(const name other) const { return m_Data != other.m_Data; } \
  EZ_FORCE_INLINE bool operator<(const name other) const { return m_Data < other.m_Data; } \
  EZ_FORCE_INLINE void Invalidate() { m_Data = INVALID_INSTANCE_INDEX; } \
  EZ_FORCE_INLINE bool IsIndexAndGenerationEqual(const name other) { \
    return (m_Data & INDEX_AND_GENERATION_MASK) == (other.m_Data & INDEX_AND_GENERATION_MASK); }


/// \brief A generic id class that holds an id combined of an instance index and a generation counter. 
///
/// \todo Document this better.
template <ezUInt32 InstanceIndexBits, ezUInt32 GenerationBits>
struct ezGenericId
{
  enum { STORAGE_SIZE = ((InstanceIndexBits + GenerationBits - 1) / 8) + 1 };
  typedef typename ezSizeToType<STORAGE_SIZE>::Type StorageType;

  EZ_DECLARE_ID_TYPE(ezGenericId, InstanceIndexBits, GenerationBits);

  EZ_FORCE_INLINE ezGenericId(StorageType instanceIndex, StorageType generation)
  {
    m_Data = 0;
    m_InstanceIndex = instanceIndex;
    m_Generation = generation;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : InstanceIndexBits;
      StorageType m_Generation : GenerationBits;
    };
  };
};

#define EZ_DECLARE_HANDLE_TYPE(name, idType) \
  public: \
    EZ_DECLARE_POD_TYPE(); \
    EZ_FORCE_INLINE name() { } \
    EZ_FORCE_INLINE bool operator==(const name other) const { return m_InternalId == other.m_InternalId; } \
    EZ_FORCE_INLINE bool operator!=(const name other) const { return m_InternalId != other.m_InternalId; } \
    EZ_FORCE_INLINE bool operator<(const name other) const { return m_InternalId < other.m_InternalId; } \
    EZ_FORCE_INLINE void Invalidate() { m_InternalId.Invalidate(); } \
    EZ_FORCE_INLINE bool IsInvalidated() const { return m_InternalId == idType(); } \
    typedef idType IdType; \
  protected: \
    EZ_FORCE_INLINE name(idType internalId) : m_InternalId(internalId) { } \
    idType m_InternalId; \
    operator idType () { return m_InternalId; } \
    operator const idType () const { return m_InternalId; }


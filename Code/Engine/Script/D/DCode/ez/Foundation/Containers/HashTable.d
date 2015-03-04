module ez.Foundation.Containers.HashTable;
import ez.Foundation.Basics;
import ez.Foundation.Math.Math;
import ez.Foundation.Memory.MemoryUtils;
import std.algorithm : max;
import core.exception;

struct ezHashTableBase(KeyType, ValueType, Hasher)
{
public:
  @disable this();
  @disable this(this); // TODO implement

  this(ezAllocatorBase allocator)
  {
    m_allocator = allocator;
  }

  ~this()
  {
    Clear();
    m_allocator.Deallocate(m_pEntries); m_pEntries = null;
    m_allocator.Deallocate(m_pEntryFlags); m_pEntryFlags = null;
    m_uiCapacity = 0;
  }

  void Reserve(ezUInt32 uiCapacity)
  {
    ezUInt32 uiNewCapacity = uiCapacity + (uiCapacity / 3) * 2; // ensure a maximum load of 60%
    if (m_uiCapacity >= uiNewCapacity)
      return;

    uiNewCapacity = max(PowerOfTwo_Ceil(uiNewCapacity), CAPACITY_ALIGNMENT);
    SetCapacity(uiNewCapacity);
  }

  void Compact()
  {
    if (IsEmpty())
    {
      // completely deallocate all data, if the table is empty.
      m_allocator.Deallocate(m_pEntries); m_pEntries = null;
      m_allocator.Deallocate(m_pEntryFlags); m_pEntryFlags = null;
      m_uiCapacity = 0;
    }
    else
    {
      const ezUInt32 uiNewCapacity = (m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
      if (m_uiCapacity != uiNewCapacity)
        SetCapacity(uiNewCapacity);
    }
  }

  ezUInt32 GetCount() const
  {
    return m_uiCount;
  }

  bool IsEmpty() const
  {
    return m_uiCount == 0;
  }

  void Clear()
  {
    for (ezUInt32 i = 0; i < m_uiCapacity; ++i)
    {
      if (IsValidEntry(i))
      {
        ezMemoryUtils.Destruct(&m_pEntries[i].key, 1);
        ezMemoryUtils.Destruct(&m_pEntries[i].value, 1);
      }
    }

    ezMemoryUtils.ZeroFill(m_pEntryFlags, GetFlagsCapacity());
    m_uiCount = 0;
  }

  bool Insert(Key key, Value value, Value* out_oldValue = null)
  {
    Reserve(m_uiCount + 1);

    ezUInt32 uiIndex = Hasher.Hash(key) & (m_uiCapacity - 1);
    ezUInt32 uiDeletedIndex = ezInvalidIndex;

    ezUInt32 uiCounter = 0;
    while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
    {
      if (IsDeletedEntry(uiIndex))
      {
        if (uiDeletedIndex == ezInvalidIndex)
          uiDeletedIndex = uiIndex;
      }
      else if (Hasher.Equal(m_pEntries[uiIndex].key, key))
      {
        if (out_oldValue != nullptr)
          move(m_pEntries[uiIndex].value, *out_oldValue); // move the old value out as we don't need it anymore

        move(value, m_pEntries[uiIndex].value); // move the new value in
        return true;
      }
      ++uiIndex;
      if (uiIndex == m_uiCapacity)
        uiIndex = 0;

      ++uiCounter;
    }

    // new entry
    uiIndex = uiDeletedIndex != ezInvalidIndex ? uiDeletedIndex : uiIndex;

    ezMemoryUtils.MoveConstruct(&m_pEntries[uiIndex].key, key, 1);
    ezMemoryUtils.MoveConstruct(&m_pEntries[uiIndex].value, value, 1);
    MarkEntryAsValid(uiIndex);
    ++m_uiCount;

    return false;
  }

  bool Remove(T)(auto ref T key, V* out_oldValue = null)
  {
    ezUInt32 uiIndex = FindEntry(key);
    if (uiIndex != ezInvalidIndex)
    {
      if (out_oldValue != null)
        move(m_pEntries[uiIndex].value, *out_oldValue);

      ezMemoryUtils.Destruct(&m_pEntries[uiIndex].key);
      ezMemoryUtils.Destruct(&m_pEntries[uiIndex].value);

      ezUInt32 uiNextIndex = uiIndex + 1;
      if (uiNextIndex == m_uiCapacity)
        uiNextIndex = 0;

      // if the next entry is free we are at the end of a chain and
      // can immediately mark this entry as free as well
      if (IsFreeEntry(uiNextIndex))
      {
        MarkEntryAsFree(uiIndex);

        // run backwards and free all deleted entries in this chain
        ezUInt32 uiPrevIndex = (uiIndex != 0) ? uiIndex : m_uiCapacity;
        --uiPrevIndex;

        while (IsDeletedEntry(uiPrevIndex))
        {
          MarkEntryAsFree(uiPrevIndex);

          if (uiPrevIndex == 0)
            uiPrevIndex = m_uiCapacity;
          --uiPrevIndex;
        }
      }
      else
      {
        MarkEntryAsDeleted(uiIndex);
      }

      --m_uiCount;
      return true;
    }

    return false;
  }

  bool TryGetValue(T)(auto ref T key, ref Value out_value) //TODO const (inout) ?
  {
    ezUInt32 uiIndex = FindEntry(key);
    if (uiIndex != ezInvalidIndex)
    {
      out_value = m_pEntries[uiIndex].value;
      return true;
    }

    return false;
  }

  bool TryGetValue(T)(auto ref T key, ref V* out_pValue) // TODO const (inout) ?
  {
    ezUInt32 uiIndex = FindEntry(key);
    if (uiIndex != ezInvalidIndex)
    {
      out_pValue = &m_pEntries[uiIndex].value;
      return true;
    }

    return false;
  }

  ref inout(Value) opIndex(T)(auto ref T key) inout
  {
    ezUInt32 uiIndex = FindEntry(key);
    if (uiIndex != ezInvalidIndex)
    {
      return m_pEntries[uiIndex].value;
    }
    throw new RangeError();
  }

  void opIndexAssign(V value, K key)
  {
    Insert(move(key), move(value), null);
  }

  bool Contains(T)(auto ref T key) const
  {
    return FindEntry(key) != ezInvalidIndex;
  }

private:

  struct Entry
  {
    KeyType key;
    ValueType value;
  };

  Entry* m_pEntries;
  ezUInt32* m_pEntryFlags;

  ezUInt32 m_uiCount;
  ezUInt32 m_uiCapacity;

  ezAllocatorBase m_allocator;

  enum 
  { 
    FREE_ENTRY = 0,
    VALID_ENTRY = 1,
    DELETED_ENTRY = 2,
    FLAGS_MASK = 3,
  }

  enum ezUInt32 CAPACITY_ALIGNMENT = 32;

  version = EZ_HASHTABLE_USE_BITFLAGS;

  ezUInt32 GetFlagsCapacity() const
  {
    version(EZ_HASHTABLE_USE_BITFLAGS)
      return (m_uiCapacity + 15) / 16;
    else
      return m_uiCapacity;
  }

  ezUInt32 GetFlags(ezUInt32* pFlags, ezUInt32 uiEntryIndex) const
  {
    version(EZ_HASHTABLE_USE_BITFLAGS)
    {
      const ezUInt32 uiIndex = uiEntryIndex / 16;
      const ezUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
      return (pFlags[uiIndex] >> uiSubIndex) & FLAGS_MASK;
    }
    else
      return pFlags[uiEntryIndex] & FLAGS_MASK;
  }

  void SetFlags(ezUInt32 uiEntryIndex, ezUInt32 uiFlags)
  {
    version(EZ_HASHTABLE_USE_BITFLAGS)
    {
      const ezUInt32 uiIndex = uiEntryIndex / 16;
      const ezUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
      EZ_ASSERT_DEV(uiIndex < GetFlagsCapacity(), "Out of bounds access");
      m_pEntryFlags[uiIndex] &= ~(FLAGS_MASK << uiSubIndex);
      m_pEntryFlags[uiIndex] |= (uiFlags << uiSubIndex);
    }
    else
    {
      EZ_ASSERT_DEV(uiEntryIndex < GetFlagsCapacity(), "Out of bounds access");
      m_pEntryFlags[uiEntryIndex] = uiFlags;
    }
  }

  bool IsFreeEntry(ezUInt32 uiEntryIndex) const
  {
    return GetFlags(m_pEntryFlags, uiEntryIndex) == FREE_ENTRY;
  }

  bool IsValidEntry(ezUInt32 uiEntryIndex) const
  {
    return GetFlags(m_pEntryFlags, uiEntryIndex) == VALID_ENTRY;
  }

  bool IsDeletedEntry(ezUInt32 uiEntryIndex) const
  {
    return GetFlags(m_pEntryFlags, uiEntryIndex) == DELETED_ENTRY;
  }

  void MarkEntryAsFree(ezUInt32 uiEntryIndex)
  {
    SetFlags(uiEntryIndex, FREE_ENTRY);
  }

  void eMarkEntryAsValid(ezUInt32 uiEntryIndex)
  {
    SetFlags(uiEntryIndex, VALID_ENTRY);
  }

  void MarkEntryAsDeleted(ezUInt32 uiEntryIndex)
  {
    SetFlags(uiEntryIndex, DELETED_ENTRY);
  }
}

struct ezHashTable(KeyType, ValueType, Hasher = ezHashHelper!KeyType, AllocatorWrapper = ezDefaultAllocatorWrapper) 
{
public:
  ezHashTableBase!(KeyType, ValueType, Hasher) m_impl;

  alias m_impl this;

  @disable this();
  this(ezAllocatorBase pAllocator)
  {
    m_impl(pAllocator);
  }
}
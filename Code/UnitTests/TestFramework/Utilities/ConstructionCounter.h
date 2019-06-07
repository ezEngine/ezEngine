#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <TestFramework/TestFrameworkDLL.h>

struct ezConstructionCounter
{
  /// Dummy m_iData, such that one can test the constructor with initialization
  ezInt32 m_iData;
  bool m_valid;

  /// Default Constructor
  ezConstructionCounter()
      : m_iData(0)
      , m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Constructor with initialization
  ezConstructionCounter(ezInt32 d)
      : m_iData(d)
      , m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Copy Constructor
  ezConstructionCounter(const ezConstructionCounter& cc)
      : m_iData(cc.m_iData)
      , m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Move construction counts as a construction as well.
  ezConstructionCounter(ezConstructionCounter&& cc)
      : m_iData(cc.m_iData)
      , m_valid(true)
  {
    cc.m_iData = 0; // data has been moved, so "destroy" it.
    ++s_iConstructions;
  }

  /// Destructor
  ~ezConstructionCounter()
  {
    EZ_ASSERT_ALWAYS(m_valid, "Destroying object twice");
    m_valid = false;
    ++s_iDestructions;
  }

  /// Assignment does not change the construction counter, because it is only executed on already constructed objects.
  void operator=(const ezConstructionCounter& cc) { m_iData = cc.m_iData; }
  /// Move assignment does not change the construction counter, because it is only executed on already constructed objects.
  void operator=(const ezConstructionCounter&& cc) { m_iData = cc.m_iData; }

  bool operator==(const ezConstructionCounter& cc) const { return m_iData == cc.m_iData; }

  bool operator!=(const ezConstructionCounter& cc) const { return m_iData != cc.m_iData; }

  bool operator<(const ezConstructionCounter& rhs) const { return m_iData < rhs.m_iData; }

  /// Checks whether n constructions have been done since the last check.
  static bool HasConstructed(ezInt32 cons)
  {
    const bool b = s_iConstructions == s_iConstructionsLast + cons;
    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    if (!b)
      PrintStats();

    return (b);
  }

  /// Checks whether n destructions have been done since the last check.
  static bool HasDestructed(ezInt32 cons)
  {
    const bool b = s_iDestructions == s_iDestructionsLast + cons;
    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    if (!b)
      PrintStats();

    return (b);
  }

  /// Checks whether n constructions and destructions have been done since the last check.
  static bool HasDone(ezInt32 cons, ezInt32 des)
  {
    const bool bc = (s_iConstructions == (s_iConstructionsLast + cons));
    const bool bd = (s_iDestructions == (s_iDestructionsLast + des));

    if (!(bc && bd))
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    return (bc && bd);
  }

  /// For debugging and getting tests right: Prints out the current number of constructions and destructions
  static void PrintStats()
  {
    printf("Constructions: %d (New: %i), Destructions: %d (New: %i) \n", s_iConstructions, s_iConstructions - s_iConstructionsLast,
           s_iDestructions, s_iDestructions - s_iDestructionsLast);
  }

  /// Checks that all instances have been destructed.
  static bool HasAllDestructed()
  {
    if (s_iConstructions != s_iDestructions)
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    return (s_iConstructions == s_iDestructions);
  }

  static void Reset()
  {
    s_iConstructions = 0;
    s_iConstructionsLast = 0;
    s_iDestructions = 0;
    s_iDestructionsLast = 0;
  }

  static ezInt32 s_iConstructions;
  static ezInt32 s_iConstructionsLast;
  static ezInt32 s_iDestructions;
  static ezInt32 s_iDestructionsLast;
};

struct ezConstructionCounterRelocatable
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Dummy m_iData, such that one can test the constructor with initialization
  ezInt32 m_iData;

  /// Bool to track if the element was default constructed or received valid data.
  bool m_valid = false;

  ezConstructionCounterRelocatable() = default;

  ezConstructionCounterRelocatable(ezInt32 d)
      : m_iData(d)
      , m_valid(true)
  {
    s_iConstructions++;
  }

  ezConstructionCounterRelocatable(const ezConstructionCounterRelocatable& other) = delete;

  ezConstructionCounterRelocatable(ezConstructionCounterRelocatable&& other)
  {
    m_iData = other.m_iData;
    m_valid = other.m_valid;

    other.m_valid = false;
  }

  ~ezConstructionCounterRelocatable()
  {
    if (m_valid)
      s_iDestructions++;
  }

  void operator=(ezConstructionCounterRelocatable&& other)
  {
    m_iData = other.m_iData;
    m_valid = other.m_valid;

    other.m_valid = false;
    ;
  }

  /// For debugging and getting tests right: Prints out the current number of constructions and destructions
  static void PrintStats()
  {
    printf("Constructions: %d (New: %i), Destructions: %d (New: %i) \n", s_iConstructions, s_iConstructions - s_iConstructionsLast,
           s_iDestructions, s_iDestructions - s_iDestructionsLast);
  }

  /// Checks whether n constructions and destructions have been done since the last check.
  static bool HasDone(ezInt32 cons, ezInt32 des)
  {
    const bool bc = (s_iConstructions == (s_iConstructionsLast + cons));
    const bool bd = (s_iDestructions == (s_iDestructionsLast + des));

    if (!(bc && bd))
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    return (bc && bd);
  }

  /// Checks that all instances have been destructed.
  static bool HasAllDestructed()
  {
    if (s_iConstructions != s_iDestructions)
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    return (s_iConstructions == s_iDestructions);
  }

  static void Reset()
  {
    s_iConstructions = 0;
    s_iConstructionsLast = 0;
    s_iDestructions = 0;
    s_iDestructionsLast = 0;
  }

  static ezInt32 s_iConstructions;
  static ezInt32 s_iConstructionsLast;
  static ezInt32 s_iDestructions;
  static ezInt32 s_iDestructionsLast;
};

template <>
struct ezHashHelper<ezConstructionCounter>
{
  static ezUInt32 Hash(const ezConstructionCounter& value) { return ezHashHelper<ezInt32>::Hash(value.m_iData); }

  EZ_ALWAYS_INLINE static bool Equal(const ezConstructionCounter& a, const ezConstructionCounter& b) { return a == b; }
};

#pragma once

#include <TestFramework/Basics.h>

struct ezConstructionCounter
{
  /// Dummy m_iData, such that one can test the constructor with initialization
  ezInt32 m_iData;

  /// Default Constructor
  ezConstructionCounter() : m_iData (0) 
  { 
    ++s_iConstructions; 
  }

  /// Constructor with initialization
  ezConstructionCounter(ezInt32 d) : m_iData (d) 
  { 
    ++s_iConstructions; 
  }
  
  /// Copy Constructor
  ezConstructionCounter(const ezConstructionCounter& cc) : m_iData(cc.m_iData) 
  { 
    ++s_iConstructions; 
  }

  /// Destructor
  ~ezConstructionCounter()
  { 
    ++s_iDestructions; 
  }

  /// Assignment does not change the construction counter, because it is only executed on already constructed objects.
  void operator= (const ezConstructionCounter& cc)
  { 
    m_iData = cc.m_iData; 
  }

  bool operator== (const ezConstructionCounter& cc) const
  { 
    return m_iData == cc.m_iData; 
  }

  bool operator!= (const ezConstructionCounter& cc) const
  { 
    return m_iData != cc.m_iData; 
  }

  bool operator< (const ezConstructionCounter& rhs) const
  {
    return m_iData < rhs.m_iData;
  }

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
    printf("Constructions: %d (New: %i), Destructions: %d (New: %i) \n", s_iConstructions, s_iConstructions - s_iConstructionsLast, s_iDestructions, s_iDestructions - s_iDestructionsLast);
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

  static ezInt32 s_iConstructions;
  static ezInt32 s_iConstructionsLast;
  static ezInt32 s_iDestructions;
  static ezInt32 s_iDestructionsLast;
};


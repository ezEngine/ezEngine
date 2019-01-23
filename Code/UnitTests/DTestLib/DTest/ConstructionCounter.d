module DTest.ConstructionCounter;
import ez.Foundation.Basics;

struct ezConstructionCounter
{
  /// Dummy m_iData, such that one can test the constructor with initialization
  ezInt32 m_iData;

  /// Default Constructor
  @disable this();
  /*ezConstructionCounter()
  { 
    m_iData = 0;
    ++s_iConstructions; 
  }*/

  /// Constructor with initialization
  this(ezInt32 d) 
  { 
    m_iData = d;
    ++s_iConstructions; 
  }

  /// Copy Constructor
  this(this)
  { 
    ++s_iConstructions; 
  }

  /// Destructor
  ~this()
  { 
    ++s_iDestructions; 
  }

  /// Assignment does not change the construction counter, because it is only executed on already constructed objects.
  void opAssign (ref const(ezConstructionCounter) cc)
  { 
    m_iData = cc.m_iData; 
  }

  bool opEquals (ref const(ezConstructionCounter) cc) const
  { 
    return m_iData == cc.m_iData; 
  }

  int opCmp (ref const(ezConstructionCounter) rhs) const
  {
    return m_iData - rhs.m_iData;
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
    import core.stdc.stdio : printf;
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

  __gshared ezInt32 s_iConstructions;
  __gshared ezInt32 s_iConstructionsLast;
  __gshared ezInt32 s_iDestructions;
  __gshared ezInt32 s_iDestructionsLast;
}
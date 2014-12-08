#include <PCH.h>

// This test does not actually run, it tests compile time stuff

namespace
{
  struct AggregatePod
  {
    int m_1;
    float m_2;

    EZ_DETECT_TYPE_CLASS(int, float);
  };

  struct AggregatePod2
  {
    int m_1;
    float m_2;
    AggregatePod m_3;

    EZ_DETECT_TYPE_CLASS(int, float, AggregatePod);
  };

  struct MemRelocateable
  {
    EZ_DECLARE_MEM_RELOCATABLE_TYPE();
  };

  struct AggregateMemRelocateable
  {
    int m_1;
    float m_2;
    AggregatePod m_3;
    MemRelocateable m_4;

    EZ_DETECT_TYPE_CLASS(int, float, AggregatePod, MemRelocateable);
  };

  class ClassType
  {

  };

  struct AggregateClass
  {
    int m_1;
    float m_2;
    AggregatePod m_3;
    MemRelocateable m_4;
    ClassType m_5;

    EZ_DETECT_TYPE_CLASS(int, float, AggregatePod, MemRelocateable, ClassType);
  };

  EZ_CHECK_AT_COMPILETIME(ezGetTypeClass<AggregatePod>::value == ezTypeIsPod::value);
  EZ_CHECK_AT_COMPILETIME(ezGetTypeClass<AggregatePod2>::value == ezTypeIsPod::value);
  EZ_CHECK_AT_COMPILETIME(ezGetTypeClass<MemRelocateable>::value == ezTypeIsMemRelocatable::value);
  EZ_CHECK_AT_COMPILETIME(ezGetTypeClass<AggregateMemRelocateable>::value == ezTypeIsMemRelocatable::value);
  EZ_CHECK_AT_COMPILETIME(ezGetTypeClass<ClassType>::value == ezTypeIsClass::value);
  EZ_CHECK_AT_COMPILETIME(ezGetTypeClass<AggregateClass>::value == ezTypeIsClass::value);
}

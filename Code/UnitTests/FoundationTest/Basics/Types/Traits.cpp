#include <FoundationTest/FoundationTestPCH.h>

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

  static_assert(ezGetTypeClass<AggregatePod>::value == ezTypeIsPod::value);
  static_assert(ezGetTypeClass<AggregatePod2>::value == ezTypeIsPod::value);
  static_assert(ezGetTypeClass<MemRelocateable>::value == ezTypeIsMemRelocatable::value);
  static_assert(ezGetTypeClass<AggregateMemRelocateable>::value == ezTypeIsMemRelocatable::value);
  static_assert(ezGetTypeClass<ClassType>::value == ezTypeIsClass::value);
  static_assert(ezGetTypeClass<AggregateClass>::value == ezTypeIsClass::value);
} // namespace

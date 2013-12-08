#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include "Reflected.h"

// add reflection for some other type
EZ_DECLARE_STATIC_REFLECTION(float);
EZ_DECLARE_STATIC_REFLECTION(bool);
EZ_DECLARE_STATIC_REFLECTION(ezInt32);
EZ_DECLARE_STATIC_REFLECTION(ezVec3);
EZ_DECLARE_STATIC_REFLECTION(ezArrayProperty);

class TestBase;
EZ_DECLARE_STATIC_REFLECTION_WITH_BASE(TestBase, ezReflectedBase);

class TestBase : public ezReflectedBase
{
  EZ_ADD_DYNAMIC_REFLECTION(TestBase);

public:
  TestBase()
  {
    somedata = 42;
    m_Vector.Set(1, 2, 3);
  }
  
  int GetSomeData() const { return somedata; }
  void SetSomeData(int d) { somedata = d; }

  ezReflectedBase m_ReflectedMember;

private:
  int somedata;
  ezVec3 m_Vector;
};

class TestA;
EZ_DECLARE_STATIC_REFLECTION_WITH_BASE(TestA, TestBase);

class TestA : public TestBase
{
  EZ_ADD_DYNAMIC_REFLECTION(TestA);

public:

  TestBase m_TestBaseMember;

};

class TestAB;
EZ_DECLARE_STATIC_REFLECTION_WITH_BASE(TestAB, TestA);

class TestAB : public TestA
{
  EZ_ADD_DYNAMIC_REFLECTION(TestAB);

public:
  TestAB()
  {
    somebool = true;
    privateint = 4;
    privateint2 = 8;
  }

  bool GetSomeBool() const { return somebool; }
  void SetSomeBool(bool d) { somebool = d; }

  

private:
  int privateint;
  int privateint2;
  bool somebool;
  
};


class TestB;
EZ_DECLARE_STATIC_REFLECTION_WITH_BASE(TestB, TestBase);

class TestB : public TestBase
{
  EZ_ADD_DYNAMIC_REFLECTION(TestB);

public:

};


// this construct is quite ugly, but at least it works !
// EZ_DECLARE_STATIC_REFLECTION needs to be done before the definition of 
namespace stuff
{
  struct StructB;

  // somehow this all works, but currently the type will get the name that we provide here
  // so it could be 'stuff::StructB' or 'StructB', both works, as long as the code does not use the actual name to figure out the type
  EZ_DECLARE_STATIC_REFLECTION(StructB);

  struct StructB : public TestBase
  {
    EZ_ADD_DYNAMIC_REFLECTION(StructB);

    StructB()
    {
      StructB_Float = 13.37;
      StructB_Bool = true;
    }

    float StructB_Float;
    bool StructB_Bool;

    ezDeque<float, ezStaticAllocatorWrapper> m_Deque;
  };

  
}



struct StructA;
EZ_DECLARE_STATIC_REFLECTION(StructA);

struct StructA
{
  StructA()
  {
    data = 23.42f;
    m_Vec3.Set(4, 5, 6);
  }

  float GetSomeFloat() const { return data; }
  void SetSomeFloat(float d) { data = d; }

  ezVec3 Getm_Vec3() const { return m_Vec3; }
  void Setm_Vec3(ezVec3 d) { m_Vec3 = d; }

  float data;

  stuff::StructB m_SubProperty;
  ezVec3 m_Vec3;
};




#pragma once

#include <Foundation/Logging/Log.h>
#include "ReflectionSystem.h"

class TestBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(TestBase);

public:
  TestBase()
  {
    ezLog::Debug("TestBase");
    m_Float1 = 23;
    m_Test = 42;
    m_Test2 = 11.22f;
  }

  ~TestBase()
  {
    ezLog::Debug("~TestBase");
  }

  ezInt32 m_Test;

private:
  float GetFloat1() const { return m_Float1; }
  void SetFloat1(float v) { m_Float1 = v; }

  float m_Test2;
  float m_Float1;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, TestBase);

class TestClassA : public TestBase
{
  EZ_ADD_DYNAMIC_REFLECTION(TestClassA);

public:
  TestClassA()
  {
    ezLog::Debug("TestClassA");
  }

  ~TestClassA()
  {
    ezLog::Debug("~TestClassA");
  }

  void DoSomething()
  {
    ezLog::Dev("I'm useful!");
  }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, TestClassA);
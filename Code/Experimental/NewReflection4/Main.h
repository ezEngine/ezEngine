#pragma once

#include "StaticRTTI.h"
#include "StandardTypes.h"
#include "DynamicRTTI.h"
#include <Foundation/Logging/Log.h>

class TestBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(TestBase);

public:
  TestBase()
  {
    ezLog::Debug("TestBase");
  }

  ~TestBase()
  {
    ezLog::Debug("~TestBase");
  }
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
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, TestClassA);
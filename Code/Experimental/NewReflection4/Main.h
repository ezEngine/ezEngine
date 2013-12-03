#pragma once

#include "StaticRTTI.h"
#include "StandardTypes.h"
#include "DynamicRTTI.h"

class TestBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(TestBase);

};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, TestBase);

class TestClassA : public TestBase
{
  EZ_ADD_DYNAMIC_REFLECTION(TestClassA);

};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, TestClassA);
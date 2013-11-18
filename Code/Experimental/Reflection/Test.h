#pragma once

#include <Foundation/Basics.h>
#include "Reflected.h"

class TestBase : public ezReflectedBase
{
  EZ_DECLARE_REFLECTED_CLASS(TestBase, ezReflectedBase);

public:

};


class TestA : public TestBase
{
  EZ_DECLARE_REFLECTED_CLASS(TestA, TestBase);

public:

};

class TestAB : public TestA
{
  EZ_DECLARE_REFLECTED_CLASS(TestAB, TestA);

public:

};


class TestB : public TestBase
{
  EZ_DECLARE_REFLECTED_CLASS(TestB, TestBase);

public:

};

struct StructA
{
  EZ_DECLARE_REFLECTED_STRUCT;

  int data;
};
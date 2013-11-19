#pragma once

#include <Foundation/Basics.h>
#include "Reflected.h"

class TestBase : public ezReflectedBase
{
  EZ_DECLARE_REFLECTED_CLASS(TestBase, ezReflectedBase);

public:
  TestBase()
  {
    somedata = 42;
  }
  
  int GetSomeData() const { return somedata; }
  void SetSomeData(int d) { somedata = d; }

private:
  int somedata;
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


class TestB : public TestBase
{
  EZ_DECLARE_REFLECTED_CLASS(TestB, TestBase);

public:

};

struct StructA
{
  EZ_DECLARE_REFLECTED_STRUCT;

  StructA()
  {
    data = 23.42f;
  }

  float GetSomeFloat() const { return data; }
  void SetSomeFloat(float d) { data = d; }

  float data;
};
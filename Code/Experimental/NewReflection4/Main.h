#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Communication/Message.h>

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

struct MyStruct
{
  float f1;
  int i2;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, MyStruct);

class ezTestMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezTestMessage);

};

class ezTestMessage2 : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezTestMessage2);

};

class TestClassA : public TestBase
{
  EZ_ADD_DYNAMIC_REFLECTION(TestClassA);

public:
  TestClassA()
  {
    ezLog::Debug("TestClassA");

    m_MyStruct.f1 = 11.11f;
    m_MyStruct.i2 = 22;
  }

  ~TestClassA()
  {
    ezLog::Debug("~TestClassA");
  }

  void DoSomething()
  {
    ezLog::Dev("I'm useful!");
  }

private:
  void HandleMessage(ezTestMessage* msg)
  {
    ezLog::Dev("TestClassA::HandleMessage: ezTestMessage");
  }

  void HandleMessage2(const ezTestMessage2* msg)
  {
    // I have no idea how one could get a function pointer to the right version of an overloaded function
    // Therefore each message handler must have a unique name :-(

    ezLog::Dev("TestClassA::HandleMessage: ezTestMessage2");
  }

  MyStruct m_MyStruct;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, TestClassA);
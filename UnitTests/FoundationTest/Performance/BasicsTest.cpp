#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Logging/Log.h>

/* Performance Statistics:

  AMD E-350 Processr 1.6 GHz ('Fusion'), 32 Bit, Debug Mode
    Virtual Function Calls:   ~60 ns
    Simple Function Calls:    ~27 ns
    Fastcall Function Calls:  ~27 ns
    Integer Division:         52 ns
    Integer Multiplication:   23 ns
    Float Division:           25 ns
    Float Multiplication:     25 ns

  AMD E-350 Processr 1.6 GHz ('Fusion'), 64 Bit, Debug Mode 
    Virtual Function Calls:   ~80 ns
    Simple Function Calls:    ~55 ns
    Fastcall Function Calls:  ~55 ns
    Integer Division:         ~97 ns
    Integer Multiplication:   ~52 ns
    Float Division:           ~66 ns
    Float Multiplication:     ~58 ns

  AMD E-350 Processr 1.6 GHz ('Fusion'), 32 Bit, Release Mode
    Virtual Function Calls:   ~9 ns
    Simple Function Calls:    ~5 ns
    Fastcall Function Calls:  ~5 ns
    Integer Division:         35 ns
    Integer Multiplication:   3.78 ns
    Float Division:           10.7 ns
    Float Multiplication:     9.5 ns

  AMD E-350 Processr 1.6 GHz ('Fusion'), 64 Bit, Release Mode 
    Virtual Function Calls:   ~10 ns
    Simple Function Calls:    ~5 ns
    Fastcall Function Calls:  ~5 ns
    Integer Division:         35 ns
    Integer Multiplication:   3.23 ns
    Float Division:           8.13 ns
    Float Multiplication:     4.13 ns

  Intel Core i7 3770 3.4 GHz, 64 Bit, Release Mode
    Virtual Function Calls:   ~3.8 ns
    Simple Function Calls:    ~4.4 ns
    Fastcall Function Calls:  ~4.0 ns
    Integer Division:         8.25 ns
    Integer Multiplication:   1.55 ns
    Float Division:           4.40 ns
    Float Multiplication:     1.87 ns

*/

EZ_CREATE_SIMPLE_TEST_GROUP(Performance);

struct GetValueMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(GetValueMessage);

  ezInt32 m_iValue;
};
EZ_IMPLEMENT_MESSAGE_TYPE(GetValueMessage);


class Base : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(Base);
public:
  virtual ~Base() {}

  virtual ezInt32 Virtual() = 0;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Base, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #define EZ_FASTCALL __fastcall
  #define EZ_NO_INLINE __declspec(noinline)
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #if EZ_ENABLED(EZ_PLATFORM_64BIT)
    #define EZ_FASTCALL 
  #else
    #define EZ_FASTCALL __attribute((fastcall)) // Fastcall only relevant on x86-32 and would otherwise generate warnings
  #endif
  #define EZ_NO_INLINE __attribute__((noinline))
#else
  #warning Unknown Platform.
  #define EZ_FASTCALL
  #define EZ_NO_INLINE __attribute__((noinline)) /* should work on GCC */
#endif

class Derived1 : public Base
{
  EZ_ADD_DYNAMIC_REFLECTION(Derived1);
public:
  EZ_NO_INLINE ezInt32 EZ_FASTCALL FastCall() { return 1; }
  EZ_NO_INLINE ezInt32 NonVirtual() { return 1; }
  EZ_NO_INLINE virtual ezInt32 Virtual() override { return 1; }
  EZ_NO_INLINE void OnGetValueMessage(GetValueMessage& msg) { msg.m_iValue = 1; }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Derived1, Base, 1, ezRTTINoAllocator);
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(GetValueMessage, OnGetValueMessage)
  EZ_END_MESSAGEHANDLERS
EZ_END_DYNAMIC_REFLECTED_TYPE();

class Derived2 : public Base
{
  EZ_ADD_DYNAMIC_REFLECTION(Derived2);
public:
  EZ_NO_INLINE ezInt32 EZ_FASTCALL FastCall() { return 2; }
  EZ_NO_INLINE ezInt32 NonVirtual() { return 2; }
  EZ_NO_INLINE virtual ezInt32 Virtual() override { return 2; }
  EZ_NO_INLINE void OnGetValueMessage(GetValueMessage& msg) { msg.m_iValue = 2; }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(Derived2, Base, 1, ezRTTINoAllocator);
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(GetValueMessage, OnGetValueMessage)
  EZ_END_MESSAGEHANDLERS
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_CREATE_SIMPLE_TEST(Performance, Basics)
{
  const ezInt32 iNumObjects = 1000000;
  const float fNumObjects = (float) iNumObjects;

  ezDynamicArray<Derived1> Der1;
  Der1.SetCount(iNumObjects / 2);

  ezDynamicArray<Derived2> Der2;
  Der2.SetCount(iNumObjects / 2);

  ezDynamicArray<Base*> Objects;
  Objects.SetCount(iNumObjects);

  for (ezInt32 i = 0; i < iNumObjects; i += 2)
  {
    Objects[i] = &Der1[i / 2];
    Objects[i + 1] = &Der2[i / 2];
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Dispatch Message")
  {
    ezInt32 iResult = 0;

    // warm up
    for (ezUInt32 i = 0; i < iNumObjects; ++i)
    {
      GetValueMessage msg;
      Objects[i]->GetDynamicRTTI()->DispatchMessage(Objects[i], msg);
      iResult += msg.m_iValue;
    }

    ezTime t0 = ezTime::Now();

    for (ezUInt32 i = 0; i < iNumObjects; ++i)
    {
      GetValueMessage msg;
      Objects[i]->GetDynamicRTTI()->DispatchMessage(Objects[i], msg);
      iResult += msg.m_iValue;
    }

    ezTime t1 = ezTime::Now();

    EZ_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    ezTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double) iNumObjects;

    ezLog::Info("[test]Dispatch Message: %.2fns", tFC, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Virtual")
  {
    ezInt32 iResult = 0;

    // warm up
    for (ezUInt32 i = 0; i < iNumObjects; ++i)
      iResult += Objects[i]->Virtual();

    ezTime t0 = ezTime::Now();

    for (ezUInt32 i = 0; i < iNumObjects; ++i)
      iResult += Objects[i]->Virtual();

    ezTime t1 = ezTime::Now();

    EZ_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    ezTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double) iNumObjects;

    ezLog::Info("[test]Virtual Function Calls: %.2fns", tFC, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NonVirtual")
  {
    ezInt32 iResult = 0;

    // warm up
    for (ezUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*) Objects[i])->NonVirtual();
      iResult += ((Derived2*) Objects[i])->NonVirtual();
    }

    ezTime t0 = ezTime::Now();

    for (ezUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*) Objects[i])->NonVirtual();
      iResult += ((Derived2*) Objects[i])->NonVirtual();
    }

    ezTime t1 = ezTime::Now();

    EZ_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    ezTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double) iNumObjects;

    ezLog::Info("[test]Non-Virtual Function Calls: %.2fns", tFC, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FastCall")
  {
    ezInt32 iResult = 0;

    // warm up
    for (ezUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*) Objects[i])->FastCall();
      iResult += ((Derived2*) Objects[i])->FastCall();
    }

    ezTime t0 = ezTime::Now();

    for (ezUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*) Objects[i])->FastCall();
      iResult += ((Derived2*) Objects[i])->FastCall();
    }

    ezTime t1 = ezTime::Now();

    EZ_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    ezTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double) iNumObjects;

    ezLog::Info("[test]FastCall Function Calls: %.2fns", tFC, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "32 Bit Integer Division")
  {
    ezDynamicArray<ezInt32> Ints;
    Ints.SetCount(iNumObjects);

    for (ezInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100;

    ezTime t0 = ezTime::Now();

    ezInt32 iResult = 0;

    for (ezInt32 i = 1; i < iNumObjects; i += 1)
      iResult += Ints[i] / i;

    ezTime t1 = ezTime::Now();

    ezTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double) (iNumObjects-1);

    ezLog::Info("[test]32 Bit Integer Division: %.2fns", t, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "32 Bit Integer Multiplication")
  {
    ezDynamicArray<ezInt32> Ints;
    Ints.SetCount(iNumObjects);

    for (ezInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = iNumObjects - i;

    ezTime t0 = ezTime::Now();

    ezInt32 iResult = 0;

    for (ezInt32 i = 0; i < iNumObjects; i += 1)
      iResult += Ints[i] * i;

    ezTime t1 = ezTime::Now();

    ezTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double) (iNumObjects);

    ezLog::Info("[test]32 Bit Integer Multiplication: %.2fns", t, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "64 Bit Integer Division")
  {
    ezDynamicArray<ezInt64> Ints;
    Ints.SetCount(iNumObjects);

    for (ezInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = (ezInt64) i * (ezInt64) 100;

    ezTime t0 = ezTime::Now();

    ezInt64 iResult = 0;

    for (ezInt32 i = 1; i < iNumObjects; i += 1)
      iResult += Ints[i] / (ezInt64) i;

    ezTime t1 = ezTime::Now();

    ezTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double) (iNumObjects-1);

    ezLog::Info("[test]64 Bit Integer Division: %.2fns", t, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "64 Bit Integer Multiplication")
  {
    ezDynamicArray<ezInt64> Ints;
    Ints.SetCount(iNumObjects);

    for (ezInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = iNumObjects - i;

    ezTime t0 = ezTime::Now();

    ezInt64 iResult = 0;

    for (ezInt32 i = 0; i < iNumObjects; i += 1)
      iResult += Ints[i] * (ezInt64) i;

    ezTime t1 = ezTime::Now();

    ezTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double) (iNumObjects);

    ezLog::Info("[test]64 Bit Integer Multiplication: %.2fns", t, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "32 Bit Float Division")
  {
    ezDynamicArray<float> Ints;
    Ints.SetCount(iNumObjects);

    for (ezInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100.0f;

    ezTime t0 = ezTime::Now();

    float fResult = 0;

    float d = 1.0f;
    for (ezInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      fResult += Ints[i] / d;

    ezTime t1 = ezTime::Now();

    ezTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double) (iNumObjects);

    ezLog::Info("[test]32 Bit Float Division: %.2fns", t, fResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "32 Bit Float Multiplication")
  {
    ezDynamicArray<float> Ints;
    Ints.SetCount(iNumObjects);

    for (ezInt32 i = 0; i < iNumObjects; i++)
      Ints[i] = (float) (fNumObjects) - (float) (i);

    ezTime t0 = ezTime::Now();

    float iResult = 0;

    float d = 1.0f;
    for (ezInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      iResult += Ints[i] * d;

    ezTime t1 = ezTime::Now();

    ezTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double) (iNumObjects);

    ezLog::Info("[test]32 Bit Float Multiplication: %.2fns", t, iResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "64 Bit Double Division")
  {
    ezDynamicArray<double> Ints;
    Ints.SetCount(iNumObjects);

    for (ezInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100.0;

    ezTime t0 = ezTime::Now();

    double fResult = 0;

    double d = 1.0;
    for (ezInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      fResult += Ints[i] / d;

    ezTime t1 = ezTime::Now();

    ezTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double) (iNumObjects);

    ezLog::Info("[test]64 Bit Double Division: %.2fns", t, fResult);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "64 Bit Double Multiplication")
  {
    ezDynamicArray<double> Ints;
    Ints.SetCount(iNumObjects);

    for (ezInt32 i = 0; i < iNumObjects; i++)
      Ints[i] = (double) (fNumObjects) - (double) (i);

    ezTime t0 = ezTime::Now();

    double iResult = 0;

    double d = 1.0;
    for (ezInt32 i = 0; i < iNumObjects; i++, d += 1.0)
      iResult += Ints[i] * d;

    ezTime t1 = ezTime::Now();

    ezTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double) (iNumObjects);

    ezLog::Info("[test]64 Bit Double Multiplication: %.2fns", t, iResult);
  }
}



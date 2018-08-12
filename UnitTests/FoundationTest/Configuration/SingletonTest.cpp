#include <PCH.h>

#include <Foundation/Configuration/Singleton.h>

class TestSingleton
{
  EZ_DECLARE_SINGLETON(TestSingleton);

public:
  TestSingleton()
      : m_SingletonRegistrar(this)
  {
  }

  ezInt32 m_iValue = 41;
};

EZ_IMPLEMENT_SINGLETON(TestSingleton);

class SingletonInterface
{
public:
  virtual ezInt32 GetValue() = 0;
};

class TestSingletonOfInterface : public SingletonInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(TestSingletonOfInterface, SingletonInterface);

public:
  TestSingletonOfInterface()
      : m_SingletonRegistrar(this)
  {
  }

  virtual ezInt32 GetValue() { return 23; }
};

EZ_IMPLEMENT_SINGLETON(TestSingletonOfInterface);


EZ_CREATE_SIMPLE_TEST(Configuration, Singleton)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Singleton Registration")
  {
    {
      TestSingleton* pSingleton = ezSingletonRegistry::GetSingletonInstance<TestSingleton>("TestSingleton");
      EZ_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingleton g_Singleton;

      {
        TestSingleton* pSingleton = ezSingletonRegistry::GetSingletonInstance<TestSingleton>("TestSingleton");
        EZ_TEST_BOOL(pSingleton == &g_Singleton);
        EZ_TEST_INT(pSingleton->m_iValue, 41);
      }
    }

    {
      TestSingleton* pSingleton = ezSingletonRegistry::GetSingletonInstance<TestSingleton>("TestSingleton");
      EZ_TEST_BOOL(pSingleton == nullptr);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Singleton of Interface")
  {
    {
      SingletonInterface* pSingleton = ezSingletonRegistry::GetSingletonInstance<SingletonInterface>("SingletonInterface");
      EZ_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingletonOfInterface g_Singleton;

      {
        SingletonInterface* pSingleton = ezSingletonRegistry::GetSingletonInstance<SingletonInterface>("SingletonInterface");
        EZ_TEST_BOOL(pSingleton == &g_Singleton);
        EZ_TEST_INT(pSingleton->GetValue(), 23);
      }
    }

    {
      SingletonInterface* pSingleton = ezSingletonRegistry::GetSingletonInstance<SingletonInterface>("SingletonInterface");
      EZ_TEST_BOOL(pSingleton == nullptr);
    }
  }
}

#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>

#include <vector>

namespace 
{
    enum constants { 
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        NUM_SAMPLES = 128,
        NUM_APPENDS = 1024 * 32,
        NUM_RECUSRIVE_APPENDS = 128
#else
        NUM_SAMPLES = 1024,
        NUM_APPENDS = 1024 * 64,
        NUM_RECUSRIVE_APPENDS = 256
#endif
    };

    struct SomeBigObject
    {
        EZ_DECLARE_MEM_RELOCATABLE_TYPE();

        static ezUInt32 constructionCount;
        static ezUInt32 destructionCount;
        ezUInt64 i1, i2, i3, i4, i5, i6, i7, i8;

        SomeBigObject(ezUInt64 init) : i1(init), i2(init), i3(init), i4(init),
            i5(init), i6(init), i7(init), i8(init) 
        {
            constructionCount++;
        }

        ~SomeBigObject()
        {
            destructionCount++;
        }

        SomeBigObject(const SomeBigObject& rh)
        {
            constructionCount++;
            this->i1 = rh.i1;
            this->i2 = rh.i2;
            this->i3 = rh.i3;
            this->i4 = rh.i4;
            this->i5 = rh.i5;
            this->i6 = rh.i6;
            this->i7 = rh.i7;
            this->i8 = rh.i8;
        }
    };

    ezUInt32 SomeBigObject::constructionCount = 0;
    ezUInt32 SomeBigObject::destructionCount = 0;
}

// Enable when needed
#define EZ_PERFORMANCE_TESTS_STATE ezTestBlock::Disabled

EZ_CREATE_SIMPLE_TEST(Performance, Container)
{
  const char* TestString = "There are 10 types of people in the world. Those who understand binary and those who don't.";
  const ezUInt32 TestStringLength = (ezUInt32)strlen(TestString);

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "POD Dynamic Array Appending")
  {
      ezTime t0 = ezTime::Now();
      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          ezDynamicArray<int> a;
          for(ezUInt32 i=0; i < NUM_APPENDS; i++)
          {
              a.PushBack(i);
          }

          for(ezUInt32 i=0; i < NUM_APPENDS; i++)
          {
              sum += a[i];
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]POD Dynamic Array Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "POD std::vector Appending")
  {
      ezTime t0 = ezTime::Now();

      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          std::vector<int> a;
          for(ezUInt32 i=0; i < NUM_APPENDS; i++)
          {
              a.push_back(i);
          }

          for(ezUInt32 i=0; i < NUM_APPENDS; i++)
          {
              sum += a[i];
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]POD std::vector Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "ezDynamicArray<ezDynamicArray<char>> Appending")
  {
      ezTime t0 = ezTime::Now();

      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          ezDynamicArray<ezDynamicArray<char>> a;
          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              ezUInt32 count = a.GetCount();
              a.SetCount(count + 1);
              ezDynamicArray<char>& cur = a[count];
              for(ezUInt32 j=0; j < TestStringLength; j++)
              {
                  cur.PushBack(TestString[j]);
              }
          }

          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              sum += a[i].GetCount();
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]ezDynamicArray<ezDynamicArray<char>> Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "ezDynamicArray<ezHybridArray<char, 64>> Appending")
  {
      ezTime t0 = ezTime::Now();

      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          ezDynamicArray<ezHybridArray<char, 64>> a;
          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              ezUInt32 count = a.GetCount();
              a.SetCount(count + 1);
              ezHybridArray<char, 64>& cur = a[count];
              for(ezUInt32 j=0; j < TestStringLength; j++)
              {
                  cur.PushBack(TestString[j]);
              }
          }

          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              sum += a[i].GetCount();
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]ezDynamicArray<ezHybridArray<char, 64>> Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "std::vector<std::vector<char>> Appending")
  {
      ezTime t0 = ezTime::Now();

      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          std::vector<std::vector<char>> a;
          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              ezUInt32 count = (ezUInt32)a.size();
              a.resize(count + 1);
              std::vector<char>& cur = a[count];
              for(ezUInt32 j=0; j < TestStringLength; j++)
              {
                  cur.push_back(TestString[j]);
              }
          }

          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              sum += (ezUInt32)a[i].size();
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]std::vector<std::vector<char>> Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "ezDynamicArray<ezString> Appending")
  {
      ezTime t0 = ezTime::Now();

      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          ezDynamicArray<ezString> a;
          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              ezUInt32 count = a.GetCount();
              a.SetCount(count + 1);
              ezString& cur = a[count];
              ezStringBuilder b;
              for(ezUInt32 j=0; j < TestStringLength; j++)
              {
                  b.Append(TestString[i]);
              }
              cur = std::move(b);
          }

          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              sum += a[i].GetElementCount();
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]ezDynamicArray<ezString> Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "std::vector<std::string> Appending")
  {
      ezTime t0 = ezTime::Now();

      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          std::vector<std::string> a;
          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              std::string cur;
              for(ezUInt32 j=0; j < TestStringLength; j++)
              {
                  cur += TestString[i];
              }
              a.push_back(std::move(cur));
          }

          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              sum += (ezUInt32)a[i].length();
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]std::vector<std::string> Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "ezDynamicArray<SomeBigObject> Appending")
  {
      ezTime t0 = ezTime::Now();

      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          ezDynamicArray<SomeBigObject> a;
          for(ezUInt32 i=0; i < NUM_APPENDS; i++)
          {
              a.PushBack(SomeBigObject(i));
          }

          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              sum += (ezUInt32)a[i].i1;
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]ezDynamicArray<SomeBigObject> Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(EZ_PERFORMANCE_TESTS_STATE, "std::vector<SomeBigObject> Appending")
  {
      ezTime t0 = ezTime::Now();

      ezUInt32 sum = 0;
      for(ezUInt32 n=0; n < NUM_SAMPLES; n++)
      {
          std::vector<SomeBigObject> a;
          for(ezUInt32 i=0; i < NUM_APPENDS; i++)
          {
              a.push_back(SomeBigObject(i));
          }

          for(ezUInt32 i=0; i < NUM_RECUSRIVE_APPENDS; i++)
          {
              sum += (ezUInt32)a[i].i1;
          }
      }

      ezTime t1 = ezTime::Now();
      ezLog::Info("[test]std::vector<SomeBigObject> Appending %.4fms", (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
  }

  EZ_TEST_BLOCK(ezTestBlock::Disabled, "ezMap<void*, ezUInt32>")
  {
    ezUInt32 sum = 0;

    

    for (ezUInt32 size = 1024; size < 4096 * 32; size += 1024)
    {
      ezMap<void*, ezUInt32> map;

      for (ezUInt32 i = 0; i < size; i++)
      {
        map.Insert(malloc(64), 64);
      }

      void* ptrs[1024];

      ezTime t0 = ezTime::Now();
      for (ezUInt32 n = 0; n < NUM_SAMPLES; n++)
      {
        for (ezUInt32 i = 0; i < 1024; i++)
        {
          void* mem = malloc(64);
          map.Insert(mem, 64);
          map.Remove(mem);
          ptrs[i] = mem;
        }

        for (ezUInt32 i = 0; i < 1024; i++)
          free(ptrs[i]);

        auto last = map.GetLastIterator();
        for (auto it = map.GetIterator(); it != last; ++it)
        {
          sum += it.Value();
        }
      }
      ezTime t1 = ezTime::Now();

      auto last = map.GetLastIterator();
      for (auto it = map.GetIterator(); it != last; ++it)
      {
        free(it.Key());
      }
      ezLog::Info("[test]ezMap<void*, ezUInt32> size = %d => %.4fms", size, (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Disabled, "ezHashTable<void*, ezUInt32>")
  {
    ezUInt32 sum = 0;



    for (ezUInt32 size = 1024; size < 4096 * 32; size += 1024)
    {
      ezHashTable<void*, ezUInt32> map;

      for (ezUInt32 i = 0; i < size; i++)
      {
        map.Insert(malloc(64), 64);
      }
      
      void* ptrs[1024];

      ezTime t0 = ezTime::Now();
      for (ezUInt32 n = 0; n < NUM_SAMPLES; n++)
      {

        for (ezUInt32 i = 0; i < 1024; i++)
        {
          void* mem = malloc(64);
          map.Insert(mem, 64);
          map.Remove(mem);
          ptrs[i] = mem;
        }

        for (ezUInt32 i = 0; i < 1024; i++)
          free(ptrs[i]);

        for (auto it = map.GetIterator(); it.IsValid(); it.Next())
        {
          sum += it.Value();
        }
      }
      ezTime t1 = ezTime::Now();

      for (auto it = map.GetIterator(); it.IsValid(); it.Next())
      {
        free(it.Key());
      }

      ezLog::Info("[test]ezHashTable<void*, ezUInt32> size = %d => %.4fms", size, (t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), sum);
    }
  }
}
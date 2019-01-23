#include <PCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Random.h>


// only works when also linking against CoreUtils
//#define USE_EZIMAGE

#ifdef USE_EZIMAGE
#include <Foundation/Image/Image.h>
#endif


EZ_CREATE_SIMPLE_TEST(Math, Random)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UIntInRange")
  {
    ezRandom r;
    r.Initialize(0xAABBCCDDEEFF0011ULL);

    for (ezUInt32 i = 2; i < 10000; ++i)
    {
      const ezUInt32 val = r.UIntInRange(i);
      EZ_TEST_BOOL(val < i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IntInRange")
  {
    ezRandom r;
    r.Initialize(0xBBCCDDEEFF0011AAULL);

    EZ_TEST_INT(r.IntInRange(5, 1), 5);
    EZ_TEST_INT(r.IntInRange(-5, 1), -5);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const ezInt32 val = r.IntInRange(i, i);
      EZ_TEST_BOOL(val >= i);
      EZ_TEST_BOOL(val < i + i);
    }

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const ezInt32 val = r.IntInRange(-i, 2 * i);
      EZ_TEST_BOOL(val >= -i);
      EZ_TEST_BOOL(val < -i + 2 * i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IntMinMax")
  {
    ezRandom r;
    r.Initialize(0xCCDDEEFF0011AABBULL);

    EZ_TEST_INT(r.IntMinMax(5, 5), 5);
    EZ_TEST_INT(r.IntMinMax(-5, -5), -5);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const ezInt32 val = r.IntMinMax(i, 2 * i);
      EZ_TEST_BOOL(val >= i);
      EZ_TEST_BOOL(val <= i + i);
    }

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const ezInt32 val = r.IntMinMax(-i, i);
      EZ_TEST_BOOL(val >= -i);
      EZ_TEST_BOOL(val <= i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DoubleZeroToOneExclusive")
  {
    ezRandom r;
    r.Initialize(0xDDEEFF0011AABBCCULL);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneExclusive();
      EZ_TEST_BOOL(val >= 0.0);
      EZ_TEST_BOOL(val < 1.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DoubleZeroToOneInclusive")
  {
    ezRandom r;
    r.Initialize(0xEEFF0011AABBCCDDULL);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneInclusive();
      EZ_TEST_BOOL(val >= 0.0);
      EZ_TEST_BOOL(val <= 1.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DoubleInRange")
  {
    ezRandom r;
    r.Initialize(0xFF0011AABBCCDDEEULL);

    EZ_TEST_DOUBLE(r.DoubleInRange(5, 0), 5, 0.0);
    EZ_TEST_DOUBLE(r.DoubleInRange(-5, 0), -5, 0.0);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleInRange(i, i);
      EZ_TEST_BOOL(val >= i);
      EZ_TEST_BOOL(val < i + i);
    }

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleInRange(-i, 2 * i);
      EZ_TEST_BOOL(val >= -i);
      EZ_TEST_BOOL(val < -i + 2 * i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DoubleMinMax")
  {
    ezRandom r;
    r.Initialize(0x0011AABBCCDDEEFFULL);

    EZ_TEST_DOUBLE(r.DoubleMinMax(5, 5), 5, 0.0);
    EZ_TEST_DOUBLE(r.DoubleMinMax(-5, -5), -5, 0.0);

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(i, 2 * i);
      EZ_TEST_BOOL(val >= i);
      EZ_TEST_BOOL(val <= i + i);
    }

    for (ezInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(-i, i);
      EZ_TEST_BOOL(val >= -i);
      EZ_TEST_BOOL(val <= i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Save / Load")
  {
    ezRandom r, r2;
    r.Initialize(0x0011AABBCCDDE11FULL);

    for (int i = 0; i < 1000; ++i)
      r.UInt();

    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    r.Save(writer);

    ezDynamicArray<ezUInt32> temp;
    temp.SetCountUninitialized(1000);

    for (int i = 0; i < 1000; ++i)
      temp[i] = r.UInt();

    r2.Load(reader);

    for (int i = 0; i < 1000; ++i)
    {
      EZ_TEST_INT(temp[i], r2.UInt());
    }
  }
}

static void SaveToImage(ezDynamicArray<ezUInt32>& Values, ezUInt32 uiMaxValue, const char* szFile)
{
#ifdef USE_EZIMAGE
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "Clear") == EZ_SUCCESS);

  ezImage img;
  img.SetWidth(Values.GetCount());
  img.SetHeight(100);
  img.SetImageFormat(ezImageFormat::B8G8R8A8_UNORM);
  img.AllocateImageData();

  for (ezUInt32 y = 0; y < img.GetHeight(); ++y)
  {
    for (ezUInt32 x = 0; x < img.GetWidth(); ++x)
    {
      ezUInt32* pPixel = img.GetPixelPointer<ezUInt32>(0, 0, 0, x, y);
      *pPixel = 0xFF000000;
    }
  }

  for (ezUInt32 i = 0; i < Values.GetCount(); ++i)
  {
    double val = ((double)Values[i] / (double)uiMaxValue) * 100.0;
    ezUInt32 y = 99 - ezMath::Clamp<ezUInt32>((ezUInt32)val, 0, 99);

    ezUInt32* pPixel = img.GetPixelPointer<ezUInt32>(0, 0, 0, i, y);
    *pPixel = 0xFFFFFFFF;
  }

  img.SaveTo(szFile);

  ezFileSystem::RemoveDataDirectoryGroup("Clear");
  ezFileSystem::ClearAllDataDirectoryFactories();
#endif
}

EZ_CREATE_SIMPLE_TEST(Math, RandomGauss)
{
  const float fVariance = 1.0f;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UnsignedValue")
  {
    ezRandomGauss r;
    r.Initialize(0xABCDEF0012345678ULL, 100, fVariance);

    ezDynamicArray<ezUInt32> Values;
    Values.SetCount(100);

    ezUInt32 uiMaxValue = 0;

    const ezUInt32 factor = 10; // with a factor of 100 the bell curve becomes more pronounced, with less samples it has more exceptions
    for (ezUInt32 i = 0; i < 10000 * factor; ++i)
    {
      auto val = r.UnsignedValue();

      EZ_TEST_BOOL(val < 100);

      if (val < Values.GetCount())
      {
        Values[val]++;

        uiMaxValue = ezMath::Max(uiMaxValue, Values[val]);
      }
    }

    SaveToImage(Values, uiMaxValue, "D:/GaussUnsigned.tga");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SignedValue")
  {
    ezRandomGauss r;
    r.Initialize(0xABCDEF0012345678ULL, 100, fVariance);

    ezDynamicArray<ezUInt32> Values;
    Values.SetCount(2 * 100);

    ezUInt32 uiMaxValue = 0;

    const ezUInt32 factor = 10; // with a factor of 100 the bell curve becomes more pronounced, with less samples it has more exceptions
    for (ezUInt32 i = 0; i < 10000 * factor; ++i)
    {
      auto val = r.SignedValue();

      EZ_TEST_BOOL(val > -100 && val < 100);

      val += 100;

      if (val < (ezInt32)Values.GetCount())
      {
        Values[val]++;

        uiMaxValue = ezMath::Max(uiMaxValue, Values[val]);
      }
    }

    SaveToImage(Values, uiMaxValue, "D:/GaussSigned.tga");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Save / Load")
  {
    ezRandomGauss r, r2;
    r.Initialize(0x0011AABBCCDDE11FULL, 1000, 1.7f);

    for (int i = 0; i < 1000; ++i)
      r.UnsignedValue();

    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    r.Save(writer);

    ezDynamicArray<ezUInt32> temp;
    temp.SetCountUninitialized(1000);

    for (int i = 0; i < 1000; ++i)
      temp[i] = r.UnsignedValue();

    r2.Load(reader);

    for (int i = 0; i < 1000; ++i)
    {
      EZ_TEST_INT(temp[i], r2.UnsignedValue());
    }
  }
}

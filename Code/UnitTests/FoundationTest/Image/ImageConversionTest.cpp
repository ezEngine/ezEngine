#include <PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Foundation/Memory/MemoryTracker.h>

static const ezImageFormat::Enum defaultFormat = ezImageFormat::R32G32B32A32_FLOAT;

class ezImageConversionTest : public ezTestBaseClass
{

public:
  virtual const char* GetTestName() const override { return "Image Conversion"; }

  virtual ezResult GetImage(ezImage& img) override
  {
    img.ResetAndMove(std::move(m_image));
    return EZ_SUCCESS;
  }

private:
  virtual void SetupSubTests() override
  {
    for (ezUInt32 i = 0; i < ezImageFormat::NUM_FORMATS; ++i)
    {
      ezImageFormat::Enum format = static_cast<ezImageFormat::Enum>(i);

      const char* name = ezImageFormat::GetName(format);
      EZ_ASSERT_DEV(name != nullptr, "Missing format information for format %i", i);

      bool isEncodable = ezImageConversion::IsConvertible(defaultFormat, format);

      if (!isEncodable)
      {
        // If a format doesn't have an encoder, ignore
        continue;
      }

      AddSubTest(name, i);
    }
  }

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override
  {
    ezImageFormat::Enum format = static_cast<ezImageFormat::Enum>(iIdentifier);

    bool isDecodable = ezImageConversion::IsConvertible(format, defaultFormat);

    if (!isDecodable)
    {
      EZ_TEST_BOOL_MSG(false, "Format %s can be encoded from %s but not decoded - add a decoder for this format please",
                       ezImageFormat::GetName(format), ezImageFormat::GetName(defaultFormat));

      return ezTestAppRun::Quit;
    }

    {
      ezHybridArray<ezImageConversion::ConversionPathNode, 16> decodingPath;
      ezUInt32 decodingPathScratchBuffers;
      ezImageConversion::BuildPath(format, defaultFormat, false, decodingPath, decodingPathScratchBuffers);

      // the [test] tag tells the test framework to output the log message in the GUI
      ezLog::Info("[test]Default decoding Path:");
      for (ezUInt32 i = 0; i < decodingPath.GetCount(); ++i)
      {
        ezLog::Info("[test]  {} -> {}", ezImageFormat::GetName(decodingPath[i].m_sourceFormat),
                       ezImageFormat::GetName(decodingPath[i].m_targetFormat));
      }
    }

    {
      ezHybridArray<ezImageConversion::ConversionPathNode, 16> encodingPath;
      ezUInt32 encodingPathScratchBuffers;
      ezImageConversion::BuildPath(defaultFormat, format, false, encodingPath, encodingPathScratchBuffers);

      // the [test] tag tells the test framework to output the log message in the GUI
      ezLog::Info("[test]Default encoding Path:");
      for (ezUInt32 i = 0; i < encodingPath.GetCount(); ++i)
      {
        ezLog::Info("[test]  {} -> {}", ezImageFormat::GetName(encodingPath[i].m_sourceFormat),
                       ezImageFormat::GetName(encodingPath[i].m_targetFormat));
      }
    }

    // Test LDR: Load, encode to target format, then do image comparison (which internally decodes to BGR8_UNORM again).
    // This visualizes quantization for low bit formats, block compression artifacts, or whether formats have fewer than 3 channels.
    {
      EZ_TEST_BOOL(m_image.LoadFrom("ImageConversions/reference.png").Succeeded());

      EZ_TEST_BOOL(m_image.Convert(format).Succeeded());

      EZ_TEST_IMAGE(iIdentifier * 2, ezImageFormat::IsCompressed(format) ? 10 : 0);
    }

    // Test HDR: Load, decode to FLOAT32, stretch to [-range, range] and encode;
    // then decode to FLOAT32 again, bring back into LDR range and do image comparison.
    // If the format doesn't support negative values, the left half of the image will be black.
    // If the format doesn't support values with absolute value > 1, the image will appear clipped to fullbright.
    // Also, fill the first few rows in the top left with Infinity, -Infinity, and NaN, which should
    // show up as White, White, and Black, resp., in the comparison.
    {
      const float range = 8;

      EZ_TEST_BOOL(m_image.LoadFrom("ImageConversions/reference.png").Succeeded());

      EZ_TEST_BOOL(m_image.Convert(ezImageFormat::R32G32B32A32_FLOAT).Succeeded());

      float posInf = +ezMath::BasicType<float>::GetInfinity();
      float negInf = -ezMath::BasicType<float>::GetInfinity();
      float NaN = ezMath::BasicType<float>::GetNaN();

      for (ezUInt32 y = 0; y < m_image.GetHeight(); ++y)
      {
        ezColor* pPixelPointer = m_image.GetPixelPointer<ezColor>(0, 0, 0, 0, y);

        for (ezUInt32 x = 0; x < m_image.GetWidth(); ++x)
        {
          // Fill with Inf or Nan resp. scale the image into positive and negative HDR range
          if (x < 30 && y < 10)
          {
            *pPixelPointer = ezColor(posInf, posInf, posInf, posInf);
          }
          else if (x < 30 && y < 20)
          {
            *pPixelPointer = ezColor(negInf, negInf, negInf, negInf);
          }
          else if (x < 30 && y < 30)
          {
            *pPixelPointer = ezColor(NaN, NaN, NaN, NaN);
          }
          else
          {
            float scale = (x / float(m_image.GetWidth()) - 0.5f) * 2.0f * range;

            if (ezMath::Abs(scale) > 0.5)
            {
              *pPixelPointer *= scale;
            }
          }

          pPixelPointer++;
        }
      }

      EZ_TEST_BOOL(m_image.Convert(format).Succeeded());

      EZ_TEST_BOOL(m_image.Convert(ezImageFormat::R32G32B32A32_FLOAT).Succeeded());

      for (ezUInt32 y = 0; y < m_image.GetHeight(); ++y)
      {
        ezColor* pPixelPointer = m_image.GetPixelPointer<ezColor>(0, 0, 0, 0, y);

        for (ezUInt32 x = 0; x < m_image.GetWidth(); ++x)
        {
          // Scale the image back into LDR range if possible
          if (x < 30 && y < 10)
          {
            // Leave pos inf as is - this should be clipped to 1 in the LDR conversion for img cmp
          }
          else if (x < 30 && y < 20)
          {
            // Flip neg inf to pos inf
            *pPixelPointer *= -1.0f;
          }
          else if (x < 30 && y < 30)
          {
            // Leave nan as is - this should be clipped to 0 in the LDR conversion for img cmp
          }
          else
          {
            float scale = (x / float(m_image.GetWidth()) - 0.5f) * 2.0f * range;
            if (ezMath::Abs(scale) > 0.5)
            {
              *pPixelPointer /= scale;
            }
          }

          pPixelPointer++;
        }
      }

      EZ_TEST_IMAGE(iIdentifier * 2 + 1, ezImageFormat::IsCompressed(format) ? 10 : 0);
    }

    return ezTestAppRun::Quit;
  }

  virtual ezResult InitializeTest() override
  {
    ezStartup::StartupCoreSystems();

    const ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    if (ezFileSystem::AddDataDirectory(sReadDir.GetData(), "ImageConversionTest").Failed())
    {
      return EZ_FAILURE;
    }

    ezFileSystem::AddDataDirectory(">eztest/", "ImageComparisonDataDir", "imgout", ezFileSystem::AllowWrites);

    return EZ_SUCCESS;
  }

  virtual ezResult DeInitializeTest() override
  {
    ezFileSystem::RemoveDataDirectoryGroup("ImageConversionTest");
    ezFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");

    ezStartup::ShutdownCoreSystems();
    ezMemoryTracker::DumpMemoryLeaks();

    return EZ_SUCCESS;
  }

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override { return EZ_SUCCESS; }

  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override { return EZ_SUCCESS; }

  ezImage m_image;
};

static ezImageConversionTest s_ImageConversionTest;

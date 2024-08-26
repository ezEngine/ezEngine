#include <CoreTest/CoreTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/ProcessGroup.h>
#include <Texture/Image/Image.h>

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES) && (EZ_ENABLED(EZ_PLATFORM_WINDOWS) || EZ_ENABLED(EZ_PLATFORM_LINUX)) && defined(BUILDSYSTEM_TEXCONV_PRESENT)

class ezTexConvTest : public ezTestBaseClass
{
public:
  virtual const char* GetTestName() const override { return "TexConvTool"; }

  virtual ezResult GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) override
  {
    ref_img.ResetAndMove(std::move(m_pState->m_image));
    return EZ_SUCCESS;
  }

private:
  enum SubTest
  {
    RgbaToRgbPNG,
    Combine4,
    LinearUsage,
    ExtractChannel,
    TGA,
  };

  virtual void SetupSubTests() override;

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  virtual ezResult InitializeTest() override
  {
    ezStartup::StartupCoreSystems();

    m_pState = EZ_DEFAULT_NEW(State);

    const ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());

    if (ezFileSystem::AddDataDirectory(sReadDir.GetData(), "TexConvTest", "testdata").Failed())
    {
      return EZ_FAILURE;
    }

    ezFileSystem::AddDataDirectory(">eztest/", "TexConvDataDir", "imgout", ezDataDirUsage::AllowWrites).IgnoreResult();

    return EZ_SUCCESS;
  }

  virtual ezResult DeInitializeTest() override
  {
    m_pState.Clear();

    ezFileSystem::RemoveDataDirectoryGroup("TexConvTest");
    ezFileSystem::RemoveDataDirectoryGroup("TexConvDataDir");

    ezStartup::ShutdownCoreSystems();

    return EZ_SUCCESS;
  }

  void RunTexConv(ezProcessOptions& options, const char* szOutName)
  {
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    const char* szTexConvExecutableName = "ezTexConv.exe";
#  else
    const char* szTexConvExecutableName = "ezTexConv";
#  endif
    ezStringBuilder sTexConvExe = ezOSFile::GetApplicationDirectory();
    sTexConvExe.AppendPath(szTexConvExecutableName);
    sTexConvExe.MakeCleanPath();

    if (!EZ_TEST_BOOL_MSG(ezOSFile::ExistsFile(sTexConvExe), "%s does not exist", szTexConvExecutableName))
      return;

    options.m_sProcess = sTexConvExe;

    ezStringBuilder sOut = ezTestFramework::GetInstance()->GetAbsOutputPath();
    sOut.AppendPath("Temp", szOutName);

    options.AddArgument("-out");
    options.AddArgument(sOut);

    if (!EZ_TEST_BOOL(m_pState->m_TexConvGroup.Launch(options).Succeeded()))
      return;

    if (!EZ_TEST_BOOL_MSG(m_pState->m_TexConvGroup.WaitToFinish(ezTime::MakeFromMinutes(1.0)).Succeeded(), "TexConv did not finish in time."))
      return;

    EZ_TEST_INT_MSG(m_pState->m_TexConvGroup.GetProcesses().PeekBack().GetExitCode(), 0, "TexConv failed to process the image");

    if (!EZ_TEST_BOOL_MSG(m_pState->m_image.LoadFrom(sOut).Succeeded(), "Failed to load converted image"))
      return;

    ezByteBlobPtr rawImgData = m_pState->m_image.GetByteBlobPtr();
    ezUInt64 rawDataHash = ezHashingUtils::xxHash64(rawImgData.GetPtr(), rawImgData.GetCount(), 1234);
    // The [test] tag tells the UnitTest to actually output this:
    ezLog::Info("[test]Converted file '{0}' has raw data hash: 0x{1}", szOutName, ezArgU(rawDataHash, 16, true, 16, false));
  }

  struct State
  {
    ezProcessGroup m_TexConvGroup;
    ezImage m_image;
  };

  ezUniquePtr<State> m_pState;
};

void ezTexConvTest::SetupSubTests()
{
  AddSubTest("RGBA to RGB - PNG", SubTest::RgbaToRgbPNG);
  AddSubTest("Combine4 - DDS", SubTest::Combine4);
  AddSubTest("Linear Usage", SubTest::LinearUsage);
  AddSubTest("Extract Channel", SubTest::ExtractChannel);
  AddSubTest("TGA loading", SubTest::TGA);
}

ezTestAppRun ezTexConvTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ezStringBuilder sImageData;
  ezFileSystem::ResolvePath(":testdata/TexConv", &sImageData, nullptr).IgnoreResult();

  const ezStringBuilder sPathEZ(sImageData, "/EZ.png");
  const ezStringBuilder sPathE(sImageData, "/E.png");
  const ezStringBuilder sPathZ(sImageData, "/Z.png");
  const ezStringBuilder sPathShape(sImageData, "/Shape.png");
  const ezStringBuilder sPathTGAv(sImageData, "/EZ_flipped_v.tga");
  const ezStringBuilder sPathTGAh(sImageData, "/EZ_flipped_h.tga");
  const ezStringBuilder sPathTGAvhCompressed(sImageData, "/EZ_flipped_vh.tga");

  if (iIdentifier == SubTest::RgbaToRgbPNG)
  {
    ezProcessOptions opt;
    opt.AddArgument("-rgb");
    opt.AddArgument("in0");

    opt.AddArgument("-in0");
    opt.AddArgument(sPathEZ);

    RunTexConv(opt, "RgbaToRgbPNG.png");

    EZ_TEST_IMAGE(0, 10);
  }

  if (iIdentifier == SubTest::Combine4)
  {
    ezProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathEZ);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in1.r");

    opt.AddArgument("-g");
    opt.AddArgument("in0.r");

    opt.AddArgument("-b");
    opt.AddArgument("in2.r");

    opt.AddArgument("-a");
    opt.AddArgument("in3.r");

    opt.AddArgument("-type");
    opt.AddArgument("2D");

    opt.AddArgument("-compression");
    opt.AddArgument("medium");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("linear");

    opt.AddArgument("-usage");
    opt.AddArgument("color");

    RunTexConv(opt, "Combine4.dds");

    // Threshold needs to be higher here since we might fall back to software dxt compression
    // which results in slightly different results than GPU dxt compression.
    EZ_TEST_IMAGE(1, 100);
  }

  if (iIdentifier == SubTest::LinearUsage)
  {
    ezProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathEZ);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in3");

    opt.AddArgument("-g");
    opt.AddArgument("in0");

    opt.AddArgument("-b");
    opt.AddArgument("in2");

    opt.AddArgument("-compression");
    opt.AddArgument("high");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("kaiser");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-downscale");
    opt.AddArgument("1");

    RunTexConv(opt, "Linear.dds");

    EZ_TEST_IMAGE(2, 10);
  }

  if (iIdentifier == SubTest::ExtractChannel)
  {
    ezProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathEZ);

    opt.AddArgument("-r");
    opt.AddArgument("in0.r");

    opt.AddArgument("-compression");
    opt.AddArgument("none");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("none");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-maxRes");
    opt.AddArgument("64");

    RunTexConv(opt, "ExtractChannel.dds");

    EZ_TEST_IMAGE(3, 10);
  }

  if (iIdentifier == SubTest::TGA)
  {
    {
      ezProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAv);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "EZ_flipped_v.dds");

      EZ_TEST_IMAGE(3, 10);
    }

    {
      ezProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAh);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "EZ_flipped_h.dds");

      EZ_TEST_IMAGE(4, 10);
    }

    {
      ezProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAvhCompressed);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "EZ_flipped_vh.dds");

      EZ_TEST_IMAGE(5, 10);
    }
  }

  return ezTestAppRun::Quit;
}



static ezTexConvTest s_ezTexConvTest;

#endif

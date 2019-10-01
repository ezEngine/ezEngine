#include <CoreTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/ProcessGroup.h>
#include <Texture/Image/Image.h>

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES) && EZ_ENABLED(EZ_PLATFORM_WINDOWS)

class ezTexConvTest : public ezTestBaseClass
{
public:
  virtual const char* GetTestName() const override { return "TexConvTool"; }

  virtual ezResult GetImage(ezImage& img) override
  {
    img.ResetAndMove(std::move(m_State->m_image));
    return EZ_SUCCESS;
  }

private:
  enum SubTest
  {
    RgbaToRgbPNG,
    Combine4,
    LinearUsage,
    ExtractChannel,
  };

  virtual void SetupSubTests() override;

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  virtual ezResult InitializeTest() override
  {
    ezStartup::StartupCoreSystems();

    m_State = EZ_DEFAULT_NEW(State);

    const ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    if (ezFileSystem::AddDataDirectory(sReadDir.GetData(), "TexConvTest", "testdata").Failed())
    {
      return EZ_FAILURE;
    }

    ezFileSystem::AddDataDirectory(">eztest/", "TexConvDataDir", "imgout", ezFileSystem::AllowWrites);

    return EZ_SUCCESS;
  }

  virtual ezResult DeInitializeTest() override
  {
    m_State.Clear();

    ezFileSystem::RemoveDataDirectoryGroup("TexConvTest");
    ezFileSystem::RemoveDataDirectoryGroup("TexConvDataDir");

    ezStartup::ShutdownCoreSystems();

    return EZ_SUCCESS;
  }

  void RunTexConv(ezProcessOptions& options, const char* szOutName)
  {
    ezStringBuilder sTexConvExe = ezOSFile::GetApplicationDirectory();
    sTexConvExe.AppendPath("TexConv.exe");
    sTexConvExe.MakeCleanPath();

    if (EZ_TEST_BOOL_MSG(ezOSFile::ExistsFile(sTexConvExe), "TexConv.exe does not exist").Failed())
      return;

    options.m_sProcess = sTexConvExe;

    ezStringBuilder sOut = ezTestFramework::GetInstance()->GetAbsOutputPath();
    sOut.AppendPath("Temp", szOutName);

    options.AddArgument("-out");
    options.AddArgument(sOut);

    if (EZ_TEST_BOOL(m_State->m_TexConvGroup.Launch(options).Succeeded()).Failed())
      return;

    if (EZ_TEST_BOOL_MSG(m_State->m_TexConvGroup.WaitToFinish(ezTime::Minutes(1.0)).Succeeded(), "TexConv did not finish in time.").Failed())
      return;

    EZ_TEST_INT_MSG(m_State->m_TexConvGroup.GetProcesses().PeekBack().GetExitCode(), 0, "TexConv failed to process the image");

    m_State->m_image.LoadFrom(sOut);
  }

  struct State
  {
    ezProcessGroup m_TexConvGroup;
    ezImage m_image;
  };

  ezUniquePtr<State> m_State;
};

void ezTexConvTest::SetupSubTests()
{
  AddSubTest("RGBA to RGB - PNG", SubTest::RgbaToRgbPNG);
  AddSubTest("Combine4 - DDS", SubTest::Combine4);
  AddSubTest("Linear Usage", SubTest::LinearUsage);
  AddSubTest("Extract Channel", SubTest::ExtractChannel);
}

ezTestAppRun ezTexConvTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ezStringBuilder sImageData;
  ezFileSystem::ResolvePath(":testdata/TexConv", &sImageData, nullptr);

  const ezStringBuilder sPathEZ(sImageData, "/EZ.png");
  const ezStringBuilder sPathE(sImageData, "/E.png");
  const ezStringBuilder sPathZ(sImageData, "/Z.png");
  const ezStringBuilder sPathShape(sImageData, "/Shape.png");

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

    EZ_TEST_IMAGE(1, 10);
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

  return ezTestAppRun::Quit;
}



static ezTexConvTest s_ezTexConvTest;

#endif

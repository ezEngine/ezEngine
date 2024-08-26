#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadUtils.h>

namespace
{
  void WriteOutProfilingCapture(const char* szFilePath)
  {
    ezStringBuilder outputPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", ezDataDirUsage::AllowWrites) == EZ_SUCCESS);

    ezFileWriter fileWriter;
    if (fileWriter.Open(szFilePath) == EZ_SUCCESS)
    {
      ezProfilingSystem::ProfilingData profilingData;
      ezProfilingSystem::Capture(profilingData);
      profilingData.Write(fileWriter).IgnoreResult();
      ezLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }
} // namespace

EZ_CREATE_SIMPLE_TEST_GROUP(Profiling);

EZ_CREATE_SIMPLE_TEST(Profiling, Profiling)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Nested scopes")
  {
    ezProfilingSystem::Clear();

    {
      EZ_PROFILE_SCOPE("Prewarm scope");
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(1));
    }

    ezTime endTime = ezTime::Now() + ezTime::MakeFromMilliseconds(1);

    {
      EZ_PROFILE_SCOPE("Outer scope");

      {
        EZ_PROFILE_SCOPE("Inner scope");

        while (ezTime::Now() < endTime)
        {
        }
      }
    }

    WriteOutProfilingCapture(":output/profilingScopes.json");
  }
}

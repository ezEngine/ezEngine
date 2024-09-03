#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>

ezResult ezProfilingUtils::SaveProfilingCapture(ezStringView sCapturePath)
{
  ezFileWriter fileWriter;
  if (fileWriter.Open(sCapturePath) == EZ_SUCCESS)
  {
    ezProfilingSystem::ProfilingData profilingData;
    ezProfilingSystem::Capture(profilingData);
    // Set sort index to ezInvalidIndex so that the runtime process is always at the bottom and editor is always on top when opening the trace.
    profilingData.m_uiProcessSortIndex = ezInvalidIndex;
    if (profilingData.Write(fileWriter).Failed())
    {
      ezLog::Error("Failed to write profiling capture: {0}.", sCapturePath);
      return EZ_FAILURE;
    }

    ezLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  }
  else
  {
    ezLog::Error("Could not write profiling capture to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

ezResult ezProfilingUtils::MergeProfilingCaptures(ezStringView sCapturePath1, ezStringView sCapturePath2, ezStringView sMergedCapturePath)
{
  ezString sFirstProfilingJson;
  {
    ezFileReader reader;
    if (reader.Open(sCapturePath1).Failed())
    {
      ezLog::Error("Failed to read first profiling capture to be merged: {}.", sCapturePath1);
      return EZ_FAILURE;
    }
    sFirstProfilingJson.ReadAll(reader);
  }
  ezString sSecondProfilingJson;
  {
    ezFileReader reader;
    if (reader.Open(sCapturePath2).Failed())
    {
      ezLog::Error("Failed to read second profiling capture to be merged: {}.", sCapturePath2);
      return EZ_FAILURE;
    }
    sSecondProfilingJson.ReadAll(reader);
  }

  ezStringBuilder sMergedProfilingJson;
  {
    // Just glue the array together
    sMergedProfilingJson.Reserve(sFirstProfilingJson.GetElementCount() + 1 + sSecondProfilingJson.GetElementCount());
    const char* szEndArray = sFirstProfilingJson.FindLastSubString("]");
    sMergedProfilingJson.Append(ezStringView(sFirstProfilingJson.GetData(), static_cast<ezUInt32>(szEndArray - sFirstProfilingJson.GetData())));
    sMergedProfilingJson.Append(",");
    const char* szStartArray = sSecondProfilingJson.FindSubString("[") + 1;
    sMergedProfilingJson.Append(ezStringView(szStartArray, static_cast<ezUInt32>(sSecondProfilingJson.GetElementCount() - (szStartArray - sSecondProfilingJson.GetData()))));
  }

  ezFileWriter fileWriter;
  if (fileWriter.Open(sMergedCapturePath).Failed() || fileWriter.WriteBytes(sMergedProfilingJson.GetData(), sMergedProfilingJson.GetElementCount()).Failed())
  {
    ezLog::Error("Failed to write merged profiling capture: {}.", sMergedCapturePath);
    return EZ_FAILURE;
  }
  ezLog::Info("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  return EZ_SUCCESS;
}

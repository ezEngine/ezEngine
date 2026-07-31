#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/TextFileWriter.h>
#include <Foundation/Time/Timestamp.h>

ezLogWriter::TextFile::~TextFile()
{
  EndLog();
}

ezResult ezLogWriter::TextFile::BeginLog(ezStringView sFile)
{
  EZ_LOCK(m_Mutex);

  EndLog();

  ezStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  ezStringBuilder sFolder = sPath;
  sFolder.PathParentDirectory();

  if (!sFolder.IsEmpty())
  {
    EZ_SUCCEED_OR_RETURN(ezOSFile::CreateDirectoryStructure(sFolder));
  }

  return m_File.Open(sPath, ezFileOpenMode::Write, ezFileShareMode::SharedReads);
}

void ezLogWriter::TextFile::EndLog()
{
  EZ_LOCK(m_Mutex);

  m_File.Close();
}

bool ezLogWriter::TextFile::IsOpen() const
{
  EZ_LOCK(m_Mutex);

  return m_File.IsOpen();
}

void ezLogWriter::TextFile::SetTimestampMode(ezLog::TimestampMode mode)
{
  EZ_LOCK(m_Mutex);

  m_TimestampMode = mode;
}

void ezLogWriter::TextFile::LogMessageHandler(const ezLoggingEventData& eventData)
{
  ezStringBuilder sTimestamp;
  ezLog::GenerateFormattedTimestamp(m_TimestampMode, sTimestamp);

  ezTempHybridArray<char, 11> indentation;
  indentation.SetCount(eventData.m_uiIndentation + 1, ' ');
  indentation[eventData.m_uiIndentation] = 0;

  ezStringBuilder sText, sTemp1, sTemp2;

  switch (eventData.m_EventType)
  {
    case ezLogMsgType::Flush:
      // writes are unbuffered, nothing to do
      return;

    case ezLogMsgType::BeginGroup:
      sText.SetFormat("\n{0}+++++ {1} ({2}) +++++\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), eventData.m_sTag.GetData(sTemp2));
      break;

    case ezLogMsgType::EndGroup:
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      sText.SetFormat("{0}----- {1} ({2} sec) -----\n\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), ezArgF(eventData.m_fSeconds, 6));
#else
      sText.SetFormat("{0}----- {1} ({2}) -----\n\n", indentation.GetData(), eventData.m_sText.GetData(sTemp1), "timing info not available");
#endif
      break;

    case ezLogMsgType::ErrorMsg:
      sText.SetFormat("{0}{1}Error: {2}\n", indentation.GetData(), sTimestamp, eventData.m_sText.GetData(sTemp1));
      break;

    case ezLogMsgType::SeriousWarningMsg:
      sText.SetFormat("{0}{1}Seriously: {2}\n", indentation.GetData(), sTimestamp, eventData.m_sText.GetData(sTemp1));
      break;

    case ezLogMsgType::WarningMsg:
      sText.SetFormat("{0}{1}Warning: {2}\n", indentation.GetData(), sTimestamp, eventData.m_sText.GetData(sTemp1));
      break;

    default:
      sText.SetFormat("{0}{1}{2}\n", indentation.GetData(), sTimestamp, eventData.m_sText.GetData(sTemp1));
      break;
  }

  EZ_LOCK(m_Mutex);

  if (!m_File.IsOpen())
    return;

  m_File.Write(sText.GetData(), sText.GetElementCount()).IgnoreResult();
}

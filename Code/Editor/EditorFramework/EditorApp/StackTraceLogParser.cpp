#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/StackTraceLogParser.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>

namespace ezStackTraceLogParser
{
  static void StackTraceLogCallback(const ezStringView& sLogText)
  {
    ezStringView sFileName;
    ezInt32 lineNumber;

    if (!ParseStackTraceFileNameAndLineNumber(sLogText, sFileName, lineNumber))
    {
      if (!ParseAssertFileNameAndLineNumber(sLogText, sFileName, lineNumber))
      {
        return;
      }
    }

    ezCppSettings cpp;
    cpp.Load().IgnoreResult();
    const ezStatus res = ezCppProject::OpenInCodeEditor(sFileName, lineNumber);
  }

  bool ParseAssertFileNameAndLineNumber(const ezStringView& sLine, ezStringView& ref_sFileName, ezInt32& ref_iLineNumber)
  {
    const char* szFileMarker = "File: ";
    const ezUInt32 fileMarkerLength = ezStringUtils::GetStringElementCount(szFileMarker);
    const char* szLineMarker = "Line: ";
    const ezUInt32 lineMarkerLength = ezStringUtils::GetStringElementCount(szLineMarker);

    if (sLine.FindSubString("*** Assertion ***") == nullptr)
    {
      return false;
    }

    const char* szFile = sLine.FindSubString(szFileMarker);
    if (szFile == nullptr)
    {
      return false;
    }

    const char* szLine = sLine.FindSubString(szLineMarker);
    if (szLine == nullptr)
    {
      return false;
    }

    const char* szFileEnd = ezStringUtils::FindSubString(szFile + fileMarkerLength + 1, "\"", sLine.GetEndPointer());
    if (szFileEnd == nullptr)
    {
      return false;
    }

    const char* szLineEnd = ezStringUtils::FindSubString(szLine + lineMarkerLength + 1, "\"", sLine.GetEndPointer());
    if (szLineEnd == nullptr)
    {
      return false;
    }

    ref_sFileName = ezStringView(szFile + fileMarkerLength + 1, szFileEnd);
    ref_sFileName.Trim(" ");

    if (!ref_sFileName.IsAbsolutePath())
    {
      return false;
    }

    const ezResult res = ezConversionUtils::StringToInt(ezStringView(szLine + lineMarkerLength + 1, szLineEnd), ref_iLineNumber);
    if (res != EZ_SUCCESS)
    {
      return false;
    }

    return true;
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  bool ParseStackTraceFileNameAndLineNumber(const ezStringView& sLine, ezStringView& ref_sFileName, ezInt32& ref_iLineNumber)
  {
    const char* szEndFileNameMarker = "(";
    const char* szEndLineNumberMarker = "):";

    const char* szEndLineNumber = sLine.FindSubString(szEndLineNumberMarker);
    if (szEndLineNumber == nullptr)
    {
      return false;
    }

    const char* szEndFileName = sLine.FindLastSubString(szEndFileNameMarker, szEndLineNumber);
    if (szEndFileName == nullptr)
    {
      return false;
    }

    ref_sFileName = ezStringView(sLine.GetStartPointer(), szEndFileName);
    ref_sFileName.Trim(" ");

    if (!ref_sFileName.IsAbsolutePath())
    {
      return false;
    }

    const ezResult res = ezConversionUtils::StringToInt(ezStringView(szEndFileName + 1, szEndLineNumber), ref_iLineNumber);
    if (res != EZ_SUCCESS)
    {
      return false;
    }

    return true;
  }
#else
  bool ParseStackTraceFileNameAndLineNumber(const ezStringView& sLine, ezStringView& ref_sFileName, ezInt32& ref_iLineNumber)
  {
    return false;
  }
#endif

  void Register()
  {
    ezQtLogWidget::AddLogItemContextActionCallback("StackTraceLog", &StackTraceLogCallback);
  }

  void Unregister()
  {
    ezQtLogWidget::RemoveLogItemContextActionCallback("StackTraceLog");
  }
} // namespace ezStackTraceLogParser

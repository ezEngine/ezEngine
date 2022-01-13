#pragma once

#include <DLangPlugin/DLangPluginDLL.h>
#include <Foundation/Logging/Log.h>

struct EZ_DLANGPLUGIN_DLL DLangLog
{
  DLangLog() = delete;
  DLangLog(const DLangLog&) = delete;
  ~DLangLog() = delete;
  void operator=(const DLangLog&) = delete;

  static void Error(const char* text)
  {
    ezLog::Error(text);
  }

  static void SeriousWarning(const char* text)
  {
    ezLog::SeriousWarning(text);
  }

  static void Warning(const char* text)
  {
    ezLog::Warning(text);
  }

  static void Success(const char* text)
  {
    ezLog::Success(text);
  }

  static void Info(const char* text)
  {
    ezLog::Info(text);
  }

  static void Dev(const char* text)
  {
    ezLog::Dev(text);
  }

  static void Debug(const char* text)
  {
    ezLog::Debug(text);
  }
};

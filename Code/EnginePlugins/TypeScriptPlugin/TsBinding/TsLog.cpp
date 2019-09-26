#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Log(duk_context* pContext);

ezStatus ezTypeScriptBinding::Init_Log()
{
  m_Duk.RegisterFunction("__CPP_Log_Error", __CPP_Log, 1, ezLogMsgType::ErrorMsg);
  m_Duk.RegisterFunction("__CPP_Log_SeriousWarning", __CPP_Log, 1, ezLogMsgType::SeriousWarningMsg);
  m_Duk.RegisterFunction("__CPP_Log_Warning", __CPP_Log, 1, ezLogMsgType::WarningMsg);
  m_Duk.RegisterFunction("__CPP_Log_Success", __CPP_Log, 1, ezLogMsgType::SuccessMsg);
  m_Duk.RegisterFunction("__CPP_Log_Info", __CPP_Log, 1, ezLogMsgType::InfoMsg);
  m_Duk.RegisterFunction("__CPP_Log_Dev", __CPP_Log, 1, ezLogMsgType::DevMsg);
  m_Duk.RegisterFunction("__CPP_Log_Debug", __CPP_Log, 1, ezLogMsgType::DebugMsg);

  return ezStatus(EZ_SUCCESS);
}

static int __CPP_Log(duk_context* pContext)
{
  ezDuktapeFunction duk(pContext);
  const ezInt16 iMagic = duk.GetFunctionMagicValue();

  switch (iMagic)
  {
    case ezLogMsgType::ErrorMsg:
      ezLog::Error(duk.GetStringParameter(0));
      break;
    case ezLogMsgType::SeriousWarningMsg:
      ezLog::SeriousWarning(duk.GetStringParameter(0));
      break;
    case ezLogMsgType::WarningMsg:
      ezLog::Warning(duk.GetStringParameter(0));
      break;
    case ezLogMsgType::SuccessMsg:
      ezLog::Success(duk.GetStringParameter(0));
      break;
    case ezLogMsgType::InfoMsg:
      ezLog::Info(duk.GetStringParameter(0));
      break;
    case ezLogMsgType::DevMsg:
      ezLog::Dev(duk.GetStringParameter(0));
      break;
    case ezLogMsgType::DebugMsg:
      ezLog::Debug(duk.GetStringParameter(0));
      break;
  }

  return duk.ReturnVoid();
}

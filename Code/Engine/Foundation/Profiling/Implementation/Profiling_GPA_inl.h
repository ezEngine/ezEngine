
#include <Foundation/Strings/StringBuilder.h>
#include <../ThirdParty/gpa_ITT/include/ittnotify.h>

namespace
{
  __itt_domain* g_pIttDomain = __itt_domain_create("ezEngine");

  __itt_string_handle* g_pIttMetaKeyFileName = __itt_string_handle_create("File");
  __itt_string_handle* g_pIttMetaKeyFunctionName = __itt_string_handle_create("Function");
  __itt_string_handle* g_pIttMetaKeyLineNumber = __itt_string_handle_create("Line Number");

  struct ProfilingInfo
  {
    EZ_DECLARE_POD_TYPE();

    ProfilingInfo(const char* szName)
    {
      name = __itt_string_handle_create(szName);
    }

    __itt_string_handle* name;
  };
}

ezProfilingScope::ezProfilingScope(const ezProfilingId& id, const char* szFileName, 
  const char* szFunctionName, ezUInt32 uiLineNumber) :
  m_Id(id)
{
  const ProfilingInfo& profilingInfo = GetProfilingInfo(id.m_Id);
  
  __itt_task_begin(g_pIttDomain, __itt_null, __itt_null, profilingInfo.name);

  __itt_metadata_str_add(g_pIttDomain, __itt_null, g_pIttMetaKeyFileName, szFileName, 0);
  __itt_metadata_str_add(g_pIttDomain, __itt_null, g_pIttMetaKeyFunctionName, szFunctionName, 0);
  __itt_metadata_add(g_pIttDomain, __itt_null, g_pIttMetaKeyLineNumber, __itt_metadata_u32, 1, &uiLineNumber);
}

ezProfilingScope::~ezProfilingScope()
{
  __itt_task_end(g_pIttDomain);
}

//static
void ezProfilingSystem::SetThreadName(const char* szThreadName)
{
  ezStringBuilder fullName(szThreadName);
  fullName.AppendFormat(" (%u)", GetCurrentThreadId());

  __itt_thread_set_name(fullName.GetData());
}

//static
void ezProfilingSystem::Capture(ezStreamWriterBase& outputStream)
{
}

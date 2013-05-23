
namespace
{
struct ProfilingInfo
{
  EZ_DECLARE_POD_TYPE();
  
  ProfilingInfo(const char* szName)
  {
  }
};
}


ezProfilingScope::ezProfilingScope(const ezProfilingId& id, const char* szFileName,
  const char* szFunctionName, ezUInt32 uiLineNumber) :
  m_Id(id)
{
  //EZ_ASSERT_NOT_IMPLEMENTED;
}

ezProfilingScope::~ezProfilingScope()
{
  //EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezProfilingSystem::SetThreadName(const char* szThreadName)
{
}

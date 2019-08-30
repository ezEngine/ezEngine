#include <TypeScriptPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTypeScriptComponent, 1, ezComponentMode::Static)
{
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezTypeScriptComponent::ezTypeScriptComponent()
{
}

ezTypeScriptComponent::~ezTypeScriptComponent()
{
}

void ezTypeScriptComponent::SerializeComponent(ezWorldWriter& stream) const
{
}

void ezTypeScriptComponent::DeserializeComponent(ezWorldReader& stream)
{
}

void ezTypeScriptComponent::Update(ezDuktapeWrapper& script)
{
  if (!m_bScriptLoaded)
  {
    m_bScriptLoaded = true;

    ezStringBuilder js;
    if (TranspileFile(script, "TypeScript/Component.ts", js).Failed())
      return;

    if (script.ExecuteString(js, "Component.ts").Failed())
      return;


  }

  if (script.BeginFunctionCall("Update").Succeeded())
  {
    script.ExecuteFunctionCall();
    script.EndFunctionCall();
  }
}

ezResult ezTypeScriptComponent::TranspileFile(ezDuktapeWrapper& script, const char* szFile, ezStringBuilder& result) const
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  result.ReadAll(file);

  EZ_SUCCEED_OR_RETURN(script.OpenObject("ts"));
  EZ_SUCCEED_OR_RETURN(script.BeginFunctionCall("transpile"));
  script.PushParameter(result);
  EZ_SUCCEED_OR_RETURN(script.ExecuteFunctionCall());

  result = script.GetStringReturnValue();

  script.EndFunctionCall();
  script.CloseObject();

  return EZ_SUCCESS;
}

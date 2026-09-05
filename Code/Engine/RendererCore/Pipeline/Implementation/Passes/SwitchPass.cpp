#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SwitchPass.h>

// clang-format off
EZ_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(ezSwitchBasePass, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BlackboardProperty", m_sBlackboardProperty),
    EZ_ARRAY_MEMBER_PROPERTY("Values", m_Values)->AddAttributes(new ezMaxArraySizeAttribute(ezSwitchBasePass::s_uiMaxInputs), new ezNoTemporaryTransactionsAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureSwitchPass, 1, ezRTTIDefaultAllocator<ezTextureSwitchPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Output", m_Output),
    EZ_MEMBER_PROPERTY("Input0", m_Input0)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input1", m_Input1)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input2", m_Input2)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input3", m_Input3)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input4", m_Input4)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input5", m_Input5)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input6", m_Input6)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input7", m_Input7)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Texture Switch: {Name}"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBufferSwitchPass, 1, ezRTTIDefaultAllocator<ezBufferSwitchPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Output", m_Output),
    EZ_MEMBER_PROPERTY("Input0", m_Input0)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input1", m_Input1)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input2", m_Input2)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input3", m_Input3)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input4", m_Input4)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input5", m_Input5)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input6", m_Input6)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Input7", m_Input7)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Buffer Switch: {Name}"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Teal)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSwitchBasePass::ezSwitchBasePass(const char* szName)
  : ezRenderPipelinePass(szName)
{
}

bool ezSwitchBasePass::SetSwitchValue(ezInt32 iValue)
{
  ezUInt8 uiNewIndex = 0;
  for (ezUInt32 i = 0; i < ezMath::Min(m_Values.GetCount(), s_uiMaxInputs); ++i)
  {
    if (iValue == m_Values[i])
    {
      uiNewIndex = static_cast<ezUInt8>(i);
      break;
    }
  }

  const bool bChanged = m_uiSelectedValueIndex != uiNewIndex;
  m_uiSelectedValueIndex = uiNewIndex;
  return bChanged;
}

ezStatus ezSwitchBasePass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  if (m_uiSelectedValueIndex >= inputs.GetCount() || outputs.IsEmpty())
    return ezStatus(ezFmt("Switch '{}' has an invalid selected input.", GetName()));

  outputs[0] = inputs[m_uiSelectedValueIndex];
  return EZ_SUCCESS;
}

ezResult ezSwitchBasePass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sBlackboardProperty;
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Values));
  return EZ_SUCCESS;
}

ezResult ezSwitchBasePass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");
  EZ_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sBlackboardProperty;
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Values));
  return EZ_SUCCESS;
}

void ezSwitchBasePass::AddDynamicInputPins(ezArrayPtr<const ezRenderPipelineNodePin* const> pins, ezHashTable<ezHashedString, const ezRenderPipelineNodePin*>& ref_nameToPin)
{
  const ezUInt32 uiCount = ezMath::Min(m_Values.GetCount(), pins.GetCount());

  ezStringBuilder sName;
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    sName.SetFormat("{}", m_Values[i]);

    ezHashedString sHashedName;
    sHashedName.Assign(sName);

    if (ref_nameToPin.Contains(sHashedName))
    {
      ezLog::Error("Switch '{}' uses the value '{}' more than once, only the first input pin with that value is reachable.", GetName(), m_Values[i]);
      continue;
    }

    ref_nameToPin.Insert(sHashedName, pins[i]);
  }
}

ezTextureSwitchPass::ezTextureSwitchPass()
  : ezSwitchBasePass("TextureSwitchPass")
{
}

void ezTextureSwitchPass::AddDynamicPins(ezHashTable<ezHashedString, const ezRenderPipelineNodePin*>& ref_nameToPin)
{
  const ezRenderPipelineNodePin* pins[] = {&m_Input0, &m_Input1, &m_Input2, &m_Input3, &m_Input4, &m_Input5, &m_Input6, &m_Input7};
  static_assert(EZ_ARRAY_SIZE(pins) == s_uiMaxInputs);

  AddDynamicInputPins(pins, ref_nameToPin);
}

ezBufferSwitchPass::ezBufferSwitchPass()
  : ezSwitchBasePass("BufferSwitchPass")
{
}

void ezBufferSwitchPass::AddDynamicPins(ezHashTable<ezHashedString, const ezRenderPipelineNodePin*>& ref_nameToPin)
{
  const ezRenderPipelineNodePin* pins[] = {&m_Input0, &m_Input1, &m_Input2, &m_Input3, &m_Input4, &m_Input5, &m_Input6, &m_Input7};
  static_assert(EZ_ARRAY_SIZE(pins) == s_uiMaxInputs);

  AddDynamicInputPins(pins, ref_nameToPin);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SwitchPass);

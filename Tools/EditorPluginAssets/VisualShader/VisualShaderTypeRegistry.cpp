#include <PCH.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>

EZ_IMPLEMENT_SINGLETON(ezVisualShaderTypeRegistry);

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, VisualShader)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "PluginAssets"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezVisualShaderTypeRegistry);

    ezVisualShaderTypeRegistry::GetSingleton()->LoadNodeData();
    const ezRTTI* pBaseType = ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    ezQtNodeScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezVisualShaderPin>(), [](const ezRTTI* pRtti)->ezQtPin* { return new ezQtVisualShaderPin(); });
    ezQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtVisualShaderNode(); });
  }

  ON_CORE_SHUTDOWN
  {
    ezVisualShaderTypeRegistry* pDummy = ezVisualShaderTypeRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION

ezVisualShaderTypeRegistry::ezVisualShaderTypeRegistry()
  : m_SingletonRegistrar(this)
{

}

ezVisualShaderTypeRegistry::~ezVisualShaderTypeRegistry()
{

}


const ezVisualShaderNodeDescriptor* ezVisualShaderTypeRegistry::GetDescriptorForType(const ezRTTI* pRtti) const
{
  auto it = m_NodeDescriptors.Find(pRtti);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

void ezVisualShaderTypeRegistry::LoadNodeData()
{
  // Base Node Type
  {
    ezReflectedTypeDescriptor desc;
    desc.m_sTypeName = "ezVisualShaderNodeBase";
    desc.m_sPluginName = "VisualShaderTypes";
    desc.m_sParentTypeName = ezGetStaticRTTI<ezReflectedClass>()->GetTypeName();
    desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Abstract | ezTypeFlags::Class;
    desc.m_uiTypeSize = 0;
    desc.m_uiTypeVersion = 1;

    m_pBaseType = ezPhantomRttiManager::RegisterType(desc);
  }

  {
    ezVisualShaderPinDescriptor pin;

    ezVisualShaderNodeDescriptor nd;
    nd.m_sName = "MaterialOutput";
    nd.m_Color = ezColorGammaUB(92, 30, 123);

    {
      pin.m_Color = ezColor::White;
      pin.m_pDataType = ezGetStaticRTTI<ezVec4>();
      pin.m_sName = "Base Color";
      pin.m_PropertyDesc.m_sName = pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<ezColor>()->GetTypeName();

      nd.m_InputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Red;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Base Color (R)";
      pin.m_PropertyDesc.m_sName = ""; //pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<float>()->GetTypeName();

      nd.m_InputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Lime;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Base Color (G)";
      pin.m_PropertyDesc.m_sName = ""; //pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<float>()->GetTypeName();

      nd.m_InputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Blue;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Base Color (B)";
      pin.m_PropertyDesc.m_sName = ""; //pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<float>()->GetTypeName();

      nd.m_InputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColorGammaUB(228, 228, 155);
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Base Color (A)";
      pin.m_PropertyDesc.m_sName = ""; //pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<float>()->GetTypeName();

      nd.m_InputPins.PushBack(pin);
    }

    m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
  }

  {
    ezVisualShaderPinDescriptor pin;

    ezVisualShaderNodeDescriptor nd;
    nd.m_sName = "ConstantColor";
    nd.m_Color = ezColorGammaUB(140, 13, 38);

    {
      pin.m_Color = ezColor::White;
      pin.m_pDataType = ezGetStaticRTTI<ezVec4>();
      pin.m_sName = "RGBA";
      pin.m_PropertyDesc.m_sName = pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<ezColor>()->GetTypeName();

      nd.m_OutputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Red;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Red";
      pin.m_PropertyDesc.m_sName = ""; // pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<float>()->GetTypeName();

      nd.m_OutputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Lime;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Green";
      pin.m_PropertyDesc.m_sName = ""; //pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<float>()->GetTypeName();

      nd.m_OutputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Blue;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Blue";
      pin.m_PropertyDesc.m_sName = ""; //pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<float>()->GetTypeName();

      nd.m_OutputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColorGammaUB(228, 228, 155);
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Alpha";
      pin.m_PropertyDesc.m_sName = ""; //pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<float>()->GetTypeName();

      nd.m_OutputPins.PushBack(pin);
    }

    m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
  }
}

const ezRTTI* ezVisualShaderTypeRegistry::GenerateTypeFromDesc(const ezVisualShaderNodeDescriptor& nd) const
{
  ezStringBuilder temp;

  temp.Set("ShaderNode::", nd.m_sName);

  ezReflectedTypeDescriptor desc;
  desc.m_sTypeName = temp;
  desc.m_sPluginName = "VisualShaderTypes";
  desc.m_sParentTypeName = "ezVisualShaderNodeBase";
  desc.m_Flags = ezTypeFlags::Phantom | ezTypeFlags::Class;
  desc.m_uiTypeSize = 0;
  desc.m_uiTypeVersion = 1;
  desc.m_Properties = nd.m_Properties;

  for (const auto& pin : nd.m_InputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  for (const auto& pin : nd.m_OutputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  return ezPhantomRttiManager::RegisterType(desc);
}


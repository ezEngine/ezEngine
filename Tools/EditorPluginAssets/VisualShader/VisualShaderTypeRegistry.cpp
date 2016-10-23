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
    nd.m_NodeType = ezVisualShaderNodeType::Main;
    nd.m_sName = "MaterialOutput";
    nd.m_Color = ezColorGammaUB(92, 30, 123);
    nd.m_sShaderCodePermutations = "\
BLEND_MODE\n\
RENDER_PASS\n\
SHADING_MODE\n\
TWO_SIDED\n\
INSTANCING\n\
DEFAULT_MAT_USE_BASE_TEXTURE\n\
DEFAULT_MAT_USE_NORMAL_TEXTURE\n\
DEFAULT_MAT_USE_METALLIC_TEXTURE\n\
DEFAULT_MAT_USE_ROUGHNESS_TEXTURE\n\
";
    nd.m_sShaderCodeMaterialParams = "\
Permutation BLEND_MODE;\n\
Permutation SHADING_MODE;\n\
Permutation TWO_SIDED;\n\
";
    nd.m_sShaderCodeRenderState = "\
#include <Shaders/Materials/MaterialState.h>\n\
";
    nd.m_sShaderCodeVertexShader = "\
#define USE_NORMAL\n\
#define USE_TANGENT\n\
#define USE_TEXCOORD0\n\
\n\
#include <Shaders/Materials/DefaultMaterialCB.h>\n\
#include <Shaders/Materials/MaterialVertexShader.h>\n\
\n\
VS_OUT main(VS_IN Input)\n\
{\n\
  return FillVertexData(Input);\n\
}\n\
";
    nd.m_sShaderCodePixelDefines = "\
#define USE_NORMAL\n\
#define USE_TANGENT\n\
#define USE_TEXCOORD0\n\
#define USE_SIMPLE_MATERIAL_MODEL\n\
";

    nd.m_sShaderCodePixelIncludes = "\
#include <Shaders/Materials/DefaultMaterialCB.h>\n\
#include <Shaders/Materials/MaterialPixelShader.h>\n\
";

    nd.m_sShaderCodePixelBody = "\
float3 GetNormal(PS_IN Input) { return Input.Normal; }\n\
float3 GetBaseColor(PS_IN Input) { return $in0.rgb; }\n\
float GetMetallic(PS_IN Input) { return 0.0; }\n\
float GetReflectance(PS_IN Input) { return 0.5f; }\n\
float GetRoughness(PS_IN Input) { return 0.5; }\n\
float GetOpacity(PS_IN Input) { return 1.0; }\n\
";

    {
      pin.m_Color = ezColor::White;
      pin.m_pDataType = ezGetStaticRTTI<ezVec4>();
      pin.m_sName = "Base Color";
      pin.m_DefaultValue = ezColor::White;
      pin.m_PropertyDesc.m_sName = pin.m_sName;
      pin.m_PropertyDesc.m_Category = ezPropertyCategory::Member;
      pin.m_PropertyDesc.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
      pin.m_PropertyDesc.m_sType = ezGetStaticRTTI<ezColor>()->GetTypeName();

      nd.m_InputPins.PushBack(pin);
    }

    m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);
  }

  {
    ezVisualShaderPinDescriptor pin;

    ezVisualShaderNodeDescriptor nd;
    nd.m_sName = "ConstantColor";
    nd.m_Color = ezColorGammaUB(140, 13, 38);
    nd.m_sShaderCodePixelConstants = "static float4 $out0 = $prop0;\nstatic float $out1 = $prop0.x;\nstatic float $out2 = $prop0.y;\nstatic float $out3 = $prop0.z;\nstatic float $out4 = $prop0.w;\n\n";
    auto& prop0 = nd.m_Properties.ExpandAndGetRef();
    prop0.m_sName = "Color";
    prop0.m_Category = ezPropertyCategory::Member;
    prop0.m_Flags.SetValue((ezUInt16)ezPropertyFlags::Phantom | (ezUInt16)ezPropertyFlags::StandardType);
    prop0.m_sType = ezGetStaticRTTI<ezColor>()->GetTypeName();
    // TODO: Property default value

    {
      pin.m_Color = ezColor::White;
      pin.m_pDataType = ezGetStaticRTTI<ezVec4>();
      pin.m_sName = "RGBA";

      nd.m_OutputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Red;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Red";

      nd.m_OutputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Lime;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Green";

      nd.m_OutputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColor::Blue;
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Blue";

      nd.m_OutputPins.PushBack(pin);
    }

    {
      pin.m_Color = ezColorGammaUB(228, 228, 155);
      pin.m_pDataType = ezGetStaticRTTI<float>();
      pin.m_sName = "Alpha";

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


#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

class ezRenderPipelineNode;

struct ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  struct Type
  {
    typedef ezUInt8 StorageType;

    enum Enum
    {
      Unknown,
      Input,
      Output,
      PassThrough,

      Default = Unknown
    };
  };

  ezEnum<Type> m_Type;
  ezUInt8 m_uiInputIndex = 0xFF;
  ezUInt8 m_uiOutputIndex = 0xFF;
  ezRenderPipelineNode* m_pParent = nullptr;
};

struct ezRenderPipelineNodeInputPin : public ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodeInputPin() { m_Type = Type::Input; }
};

struct ezRenderPipelineNodeOutputPin : public ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodeOutputPin() { m_Type = Type::Output; }
};

struct ezRenderPipelineNodePassThrougPin : public ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodePassThrougPin() { m_Type = Type::PassThrough; }
};

class EZ_RENDERERCORE_DLL ezRenderPipelineNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineNode, ezReflectedClass);

public:
  virtual ~ezRenderPipelineNode() {}

  void InitializePins();

  ezHashedString GetPinName(const ezRenderPipelineNodePin* pPin) const;
  const ezRenderPipelineNodePin* GetPinByName(const char* szName) const;
  const ezRenderPipelineNodePin* GetPinByName(ezHashedString sName) const;
  const ezArrayPtr<const ezRenderPipelineNodePin* const> GetInputPins() const { return m_InputPins; }
  const ezArrayPtr<const ezRenderPipelineNodePin* const> GetOutputPins() const { return m_OutputPins; }

private:
  ezDynamicArray<const ezRenderPipelineNodePin*> m_InputPins;
  ezDynamicArray<const ezRenderPipelineNodePin*> m_OutputPins;
  ezHashTable<ezHashedString, const ezRenderPipelineNodePin*> m_NameToPin;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineNodePin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineNodeInputPin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineNodeOutputPin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineNodePassThrougPin);

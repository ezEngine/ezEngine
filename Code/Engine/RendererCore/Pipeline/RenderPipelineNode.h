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
    using StorageType = ezUInt8;

    enum Enum
    {
      Input = EZ_BIT(0),
      Output = EZ_BIT(1),
      PassThrough = EZ_BIT(2),
      TextureProvider = EZ_BIT(3), ///< Pass provides pin texture to the pipeline each frame.

      Default = 0
    };

    struct Bits
    {
      StorageType Input : 1;
      StorageType Output : 1;
      StorageType PassThrough : 1;
      StorageType TextureProvider : 1;
    };
  };

  ezBitflags<Type> m_Type;
  ezUInt8 m_uiInputIndex = 0xFF;
  ezUInt8 m_uiOutputIndex = 0xFF;
  ezRenderPipelineNode* m_pParent = nullptr;
};
EZ_DECLARE_FLAGS_OPERATORS(ezRenderPipelineNodePin::Type);

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

struct ezRenderPipelineNodeInputProviderPin : public ezRenderPipelineNodeInputPin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodeInputProviderPin() { m_Type = Type::Input | Type::TextureProvider; }
};

struct ezRenderPipelineNodeOutputProviderPin : public ezRenderPipelineNodeOutputPin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodeOutputProviderPin() { m_Type = Type::Output | Type::TextureProvider; }
};

struct ezRenderPipelineNodePassThroughPin : public ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodePassThroughPin() { m_Type = Type::PassThrough; }
};

class EZ_RENDERERCORE_DLL ezRenderPipelineNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineNode, ezReflectedClass);

public:
  virtual ~ezRenderPipelineNode() = default;

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
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineNodeInputProviderPin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineNodeOutputProviderPin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRenderPipelineNodePassThroughPin);

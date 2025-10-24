#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

class ezRenderPipelineNode;

/// Pin for connecting render pipeline nodes.
///
/// Pins represent input or output connections on render pipeline nodes. They can be used to pass
/// textures, render targets, or other data between pipeline nodes.
struct ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Input = EZ_BIT(0),           ///< Pin accepts input from other nodes.
      Output = EZ_BIT(1),          ///< Pin provides output to other nodes.
      PassThrough = EZ_BIT(2),     ///< Pin passes data through without modification.
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

/// Input pin for receiving data from other nodes.
struct ezRenderPipelineNodeInputPin : public ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodeInputPin() { m_Type = Type::Input; }
};

/// Output pin for sending data to other nodes.
struct ezRenderPipelineNodeOutputPin : public ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodeOutputPin() { m_Type = Type::Output; }
};

/// Input pin that also provides a texture each frame.
struct ezRenderPipelineNodeInputProviderPin : public ezRenderPipelineNodeInputPin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodeInputProviderPin() { m_Type = Type::Input | Type::TextureProvider; }
};

/// Output pin that also provides a texture each frame.
struct ezRenderPipelineNodeOutputProviderPin : public ezRenderPipelineNodeOutputPin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodeOutputProviderPin() { m_Type = Type::Output | Type::TextureProvider; }
};

/// Pass-through pin that forwards data without modification.
struct ezRenderPipelineNodePassThroughPin : public ezRenderPipelineNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezRenderPipelineNodePassThroughPin() { m_Type = Type::PassThrough; }
};

/// Base class for nodes in a render pipeline.
///
/// Nodes represent stages in the rendering pipeline and are connected via pins.
/// Each node can have multiple input and output pins for passing textures and render targets.
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

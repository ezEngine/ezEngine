#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/CustomData/SampleCustomData.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct ezMsgSetColor;

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

// Bitmask to allow the user to select what debug rendering the component should do
struct DebugRenderComponentMask
{
  using StorageType = ezUInt8;

  // the enum names for the bits
  enum Enum
  {
    Box = EZ_BIT(0),
    Sphere = EZ_BIT(1),
    Cross = EZ_BIT(2),
    Quad = EZ_BIT(3),
    All = 0xFF,

    // required enum member; used by ezBitflags for default initialization
    Default = All
  };

  // this allows the debugger to show us names for a bitmask
  // just try this out by looking at an ezBitflags variable in a debugger
  struct Bits
  {
    ezUInt8 Box : 1;
    ezUInt8 Sphere : 1;
    ezUInt8 Cross : 1;
    ezUInt8 Quad : 1;
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_SAMPLEGAMEPLUGIN_DLL, DebugRenderComponentMask);

// use ezComponentUpdateType::Always for this component to use 'Update' called even inside the editor when it is not simulating
// otherwise we would see the debug render output only when simulating the scene
using DebugRenderComponentManager = ezComponentManagerSimple<class DebugRenderComponent, ezComponentUpdateType::Always>;

class EZ_SAMPLEGAMEPLUGIN_DLL DebugRenderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DebugRenderComponent, ezComponent, DebugRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // DebugRenderComponent

public:
  DebugRenderComponent();
  ~DebugRenderComponent();

  float m_fSize = 1.0f;                               // [ property ]
  ezColor m_Color = ezColor::White;                   // [ property ]

  ezTexture2DResourceHandle m_hTexture;               // [ property ]

  ezBitflags<DebugRenderComponentMask> m_RenderTypes; // [ property ]

  void OnSetColor(ezMsgSetColor& ref_msg);            // [ msg handler ]

  void SetRandomColor();                              // [ scriptable ]

  // BEGIN-DOCS-CODE-SNIPPET: customdata-interface
  SampleCustomDataResourceHandle m_hCustomData;
  // END-DOCS-CODE-SNIPPET


private:
  void Update();
};

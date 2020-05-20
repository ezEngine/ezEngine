#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

struct DebugRenderComponentMask
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Box = EZ_BIT(0),
    Sphere = EZ_BIT(1),
    Cross = EZ_BIT(2),
    Quad = EZ_BIT(3),
    All = 0xFF,

    Default = All
  };

  struct Bits
  {
    ezUInt8 Box : 1;
    ezUInt8 Sphere : 1;
    ezUInt8 Cross : 1;
    ezUInt8 Quad : 1;
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, DebugRenderComponentMask);

// use ezComponentUpdateType::Always for this component to use 'Update' called even inside the editor when it is not simulating
// otherwise we would see the debug render output only when simulating the scene
typedef ezComponentManagerSimple<class DebugRenderComponent, ezComponentUpdateType::Always> DebugRenderComponentManager;

class DebugRenderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DebugRenderComponent, ezComponent, DebugRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // DebugRenderComponent

public:
  DebugRenderComponent();
  ~DebugRenderComponent();

  float m_fSize = 1.0f;             // [ property ]
  ezColor m_Color = ezColor::White; // [ property ]

  void SetTexture(const ezTexture2DResourceHandle& hTexture);
  const ezTexture2DResourceHandle& GetTexture() const;

  void SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile(void) const;  // [ property ]

  ezBitflags<DebugRenderComponentMask> m_RenderTypes; // [ property ]

private:
  void Update();

  ezTexture2DResourceHandle m_hTexture;
};

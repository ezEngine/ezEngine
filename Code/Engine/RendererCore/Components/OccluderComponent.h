#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgTransformChanged;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractOccluderData;

class EZ_RENDERERCORE_DLL ezOccluderComponentManager final : public ezComponentManager<class ezOccluderComponent, ezBlockStorageType::FreeList>
{
public:
  ezOccluderComponentManager(ezWorld* pWorld);
};

class EZ_RENDERERCORE_DLL ezOccluderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezOccluderComponent, ezComponent, ezOccluderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezBoxReflectionProbeComponent

public:
  ezOccluderComponent();
  ~ezOccluderComponent();

  const ezVec3& GetExtents() const
  {
    return m_vExtents;
  }

  void SetExtents(const ezVec3& extents);

private:
  ezVec3 m_vExtents = ezVec3(5.0f);

  mutable ezSharedPtr<ezRasterizerObject> m_pOccluderObject;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractOccluderData(ezMsgExtractOccluderData& msg) const;
};

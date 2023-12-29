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

/// \brief Adds invisible geometry to a scene that is used for occlusion culling.
///
/// The component adds a box occluder to the scene. The renderer uses this geometry
/// to cull other objects which are behind occluder geometry. Use occluder components to optimize levels.
/// Make the shapes conservative, meaning that they shouldn't be bigger than the actual shapes, otherwise
/// they may incorrectly occlude other objects and lead to incorrectly culled objects.
///
/// The ezGreyBoxComponent can also create occluder geometry in different shapes.
///
/// Contrary to ezGreyBoxComponent, occluder components can be moved around dynamically and thus can be attached to
/// doors and other objects that may dynamically change the visible areas of a level.
class EZ_RENDERERCORE_DLL ezOccluderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezOccluderComponent, ezComponent, ezOccluderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezOccluderComponent

public:
  ezOccluderComponent();
  ~ezOccluderComponent();

  /// \brief Sets the size of the box occluder.
  void SetExtents(const ezVec3& vExtents);                // [ property ]
  const ezVec3& GetExtents() const { return m_vExtents; } // [ property ]

private:
  ezVec3 m_vExtents = ezVec3(5.0f);

  mutable ezSharedPtr<const ezRasterizerObject> m_pOccluderObject;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractOccluderData(ezMsgExtractOccluderData& msg) const;
};

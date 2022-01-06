#pragma once

#include <Foundation/Math/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

struct ezMsgQueryAnimationSkeleton;

using ezVisualizeSkeletonComponentManager = ezComponentManagerSimple<class ezSkeletonComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact>;

class EZ_RENDERERCORE_DLL ezSkeletonComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkeletonComponent, ezRenderComponent, ezVisualizeSkeletonComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSkeletonComponent

public:
  ezSkeletonComponent();
  ~ezSkeletonComponent();

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetSkeleton(const ezSkeletonResourceHandle& hResource);
  const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  bool m_bVisualizeSkeleton = true;   // [ property ]
  bool m_bVisualizeColliders = false; // [ property ]
  bool m_bVisualizeJoints = false;    // [ property ]

  void SetBonesToHighlight(const char* szFilter);
  const char* GetBonesToHighlight() const;

protected:
  void Update();
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg); // [ msg handler ]

  void BuildSkeletonVisualization(ezMsgAnimationPoseUpdated& msg, ezBoundingSphere& bsphere);
  void BuildColliderVisualization(ezMsgAnimationPoseUpdated& msg, ezBoundingSphere& bsphere);
  void BuildJointVisualization(ezMsgAnimationPoseUpdated& msg);

  void OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg);
  void UpdateSkeletonVis();
  ezDebugRenderer::Line& AddLine(const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color);

  ezSkeletonResourceHandle m_hSkeleton;
  ezUInt32 m_uiSkeletonChangeCounter = 0;
  ezString m_sBonesToHighlight;

  ezBoundingBoxSphere m_LocalBounds;
  ezDynamicArray<ezDebugRenderer::Line> m_LinesSkeleton;

  struct SphereShape
  {
    ezTransform m_Transform;
    ezBoundingSphere m_Shape;
    ezColor m_Color;
  };

  struct BoxShape
  {
    ezTransform m_Transform;
    ezBoundingBox m_Shape;
    ezColor m_Color;
  };

  struct CapsuleShape
  {
    ezTransform m_Transform;
    float m_fLength;
    float m_fRadius;
    ezColor m_Color;
  };

  ezDynamicArray<SphereShape> m_SpheresSkeleton;
  ezDynamicArray<BoxShape> m_BoxesSkeleton;
  ezDynamicArray<CapsuleShape> m_CapsulesSkeleton;
};

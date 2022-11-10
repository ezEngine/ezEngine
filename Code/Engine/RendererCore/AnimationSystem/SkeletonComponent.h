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
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSkeletonComponent

public:
  ezSkeletonComponent();
  ~ezSkeletonComponent();

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetSkeleton(const ezSkeletonResourceHandle& hResource);
  const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  void SetBonesToHighlight(const char* szFilter); // [ property ]
  const char* GetBonesToHighlight() const;        // [ property ]

  void VisualizeSkeletonDefaultState();

  bool m_bVisualizeBones = true;
  bool m_bVisualizeColliders = false;
  bool m_bVisualizeJoints = false;
  bool m_bVisualizeSwingLimits = false;
  bool m_bVisualizeTwistLimits = false;

protected:
  void Update();
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg); // [ msg handler ]

  void BuildSkeletonVisualization(ezMsgAnimationPoseUpdated& msg);
  void BuildColliderVisualization(ezMsgAnimationPoseUpdated& msg);
  void BuildJointVisualization(ezMsgAnimationPoseUpdated& msg);

  void OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg);
  ezDebugRenderer::Line& AddLine(const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color);

  ezSkeletonResourceHandle m_hSkeleton;
  ezTransform m_RootTransform = ezTransform::IdentityTransform();
  ezUInt32 m_uiSkeletonChangeCounter = 0;
  ezString m_sBonesToHighlight;

  ezBoundingBox m_MaxBounds;
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

  struct AngleShape
  {
    ezTransform m_Transform;
    ezColor m_Color;
    ezAngle m_StartAngle;
    ezAngle m_EndAngle;
  };

  struct ConeLimitShape
  {
    ezTransform m_Transform;
    ezColor m_Color;
    ezAngle m_Angle1;
    ezAngle m_Angle2;
  };

  struct CylinderShape
  {
    ezTransform m_Transform;
    ezColor m_Color;
    float m_fRadius1;
    float m_fRadius2;
    float m_fLength;
  };

  ezDynamicArray<SphereShape> m_SpheresShapes;
  ezDynamicArray<BoxShape> m_BoxShapes;
  ezDynamicArray<CapsuleShape> m_CapsuleShapes;
  ezDynamicArray<AngleShape> m_AngleShapes;
  ezDynamicArray<ConeLimitShape> m_ConeLimitShapes;
  ezDynamicArray<CylinderShape> m_CylinderShapes;
};

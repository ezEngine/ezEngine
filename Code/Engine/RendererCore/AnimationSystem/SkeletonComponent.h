#pragma once

#include <Foundation/Math/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

struct ezMsgQueryAnimationSkeleton;

using ezVisualizeSkeletonComponentManager = ezComponentManagerSimple<class ezSkeletonComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact>;

/// \brief Uses debug rendering to visualize various aspects of an animation skeleton.
///
/// This is meant for visually inspecting skeletons. It is used by the main skeleton editor,
/// but can also be added to a scene or added to an animated mesh on-demand.
///
/// There are different options what to visualize and also to highlight certain bones.
class EZ_RENDERERCORE_DLL ezSkeletonComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkeletonComponent, ezRenderComponent, ezVisualizeSkeletonComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSkeletonComponent

public:
  ezSkeletonComponent();
  ~ezSkeletonComponent();

  void SetSkeleton(const ezSkeletonResourceHandle& hResource);                // [ property ]
  const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; } // [ property ]

  /// \brief Sets a semicolon-separated list of bone names that should be highlighted.
  ///
  /// Set it to "*" to highlight all bones.
  /// Set it to empty to not highlight any bone.
  /// Set it to "BoneA;BoneB" to highlight the bones with name "BoneA" and "BoneB".
  void SetBonesToHighlight(const char* szFilter); // [ property ]
  const char* GetBonesToHighlight() const;        // [ property ]

  bool m_bVisualizeBones = true;                  // [ property ]
  bool m_bVisualizeColliders = false;             // [ property ]
  bool m_bVisualizeJoints = false;                // [ property ]
  bool m_bVisualizeSwingLimits = false;           // [ property ]
  bool m_bVisualizeTwistLimits = false;           // [ property ]

protected:
  void Update();
  void VisualizeSkeletonDefaultState();
  void OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg); // [ msg handler ]

  void BuildSkeletonVisualization(ezMsgAnimationPoseUpdated& msg);
  void BuildColliderVisualization(ezMsgAnimationPoseUpdated& msg);
  void BuildJointVisualization(ezMsgAnimationPoseUpdated& msg);

  void OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg);
  ezDebugRendererLine& AddLine(const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color);

  ezSkeletonResourceHandle m_hSkeleton;
  ezTransform m_RootTransform = ezTransform::MakeIdentity();
  ezUInt32 m_uiSkeletonChangeCounter = 0;
  ezString m_sBonesToHighlight;

  ezBoundingBox m_MaxBounds;
  ezDynamicArray<ezDebugRendererLine> m_LinesSkeleton;

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

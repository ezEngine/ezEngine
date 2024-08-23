#pragma once

#include <Core/World/ComponentManager.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

class ezSkeletonPoseComponentManager : public ezComponentManager<class ezSkeletonPoseComponent, ezBlockStorageType::Compact>
{
public:
  using SUPER = ezComponentManager<ezSkeletonPoseComponent, ezBlockStorageType::Compact>;

  ezSkeletonPoseComponentManager(ezWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const ezWorldModule::UpdateContext& context);
  void EnqueueUpdate(ezComponentHandle hComponent);

private:
  mutable ezMutex m_Mutex;
  ezDeque<ezComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Which pose to apply to an animated mesh.
struct ezSkeletonPoseMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    CustomPose, ///< Set a custom pose on the mesh.
    RestPose,   ///< Set the rest pose (bind pose) on the mesh.
    Disabled,   ///< Don't set any pose.
    Default = CustomPose
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSkeletonPoseMode);

/// \brief Used in conjunction with an ezAnimatedMeshComponent to set a specific pose for the animated mesh.
///
/// This component is used to set one, static pose for an animated mesh. The pose is applied once at startup.
/// This can be used to either just pose a mesh in a certain way, or to set a start pose that is then used
/// by other systems, for example a ragdoll component, to generate further poses.
///
/// The component needs to be attached to the same game object where the animated mesh component is attached.
class EZ_RENDERERCORE_DLL ezSkeletonPoseComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkeletonPoseComponent, ezComponent, ezSkeletonPoseComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSkeletonPoseComponent

public:
  ezSkeletonPoseComponent();
  ~ezSkeletonPoseComponent();

  /// \brief Sets the ezSkeletonResource to use.
  void SetSkeleton(const ezSkeletonResourceHandle& hResource);                // [ property ]
  const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; } // [ property ]

  /// \brief Configures which pose to apply to the animated mesh.
  void SetPoseMode(ezEnum<ezSkeletonPoseMode> mode);
  ezEnum<ezSkeletonPoseMode> GetPoseMode() const { return m_PoseMode; }

  const ezRangeView<const char*, ezUInt32> GetBones() const;   // [ property ] (exposed bones)
  void SetBone(const char* szKey, const ezVariant& value);     // [ property ] (exposed bones)
  void RemoveBone(const char* szKey);                          // [ property ] (exposed bones)
  bool GetBone(const char* szKey, ezVariant& out_value) const; // [ property ] (exposed bones)

  /// \brief Instructs the component to apply the pose to the animated mesh again.
  void ResendPose();

protected:
  void Update();
  void SendRestPose();
  void SendCustomPose();

  float m_fDummy = 0;
  ezUInt8 m_uiResendPose = 0;
  ezSkeletonResourceHandle m_hSkeleton;
  ezArrayMap<ezHashedString, ezExposedBone> m_Bones; // [ property ]
  ezEnum<ezSkeletonPoseMode> m_PoseMode;             // [ property ]
};

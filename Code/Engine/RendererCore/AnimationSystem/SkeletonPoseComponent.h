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

struct ezSkeletonPoseMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    CustomPose,
    RestPose,
    Disabled,
    Default = CustomPose
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSkeletonPoseMode);


class EZ_RENDERERCORE_DLL ezSkeletonPoseComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkeletonPoseComponent, ezComponent, ezSkeletonPoseComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSkeletonPoseComponent

public:
  ezSkeletonPoseComponent();
  ~ezSkeletonPoseComponent();

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetSkeleton(const ezSkeletonResourceHandle& hResource);
  const ezSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  ezEnum<ezSkeletonPoseMode> GetPoseMode() const { return m_PoseMode; }
  void SetPoseMode(ezEnum<ezSkeletonPoseMode> mode);

  void ResendPose();

  const ezRangeView<const char*, ezUInt32> GetBones() const;   // [ property ] (exposed bones)
  void SetBone(const char* szKey, const ezVariant& value);     // [ property ] (exposed bones)
  void RemoveBone(const char* szKey);                          // [ property ] (exposed bones)
  bool GetBone(const char* szKey, ezVariant& out_value) const; // [ property ] (exposed bones)

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

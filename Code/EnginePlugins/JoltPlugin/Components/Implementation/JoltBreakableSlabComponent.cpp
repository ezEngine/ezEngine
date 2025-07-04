#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/Implementation/ResourceHandleReflection.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <JoltPlugin/Components/JoltBreakableSlabComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <Physics/Collision/Shape/ConvexShape.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezJoltBreakableSlabFlags, 1)
  EZ_ENUM_CONSTANT(ezJoltBreakableSlabFlags::FixedEdgeTop),
    EZ_ENUM_CONSTANT(ezJoltBreakableSlabFlags::FixedEdgeRight),
    EZ_ENUM_CONSTANT(ezJoltBreakableSlabFlags::FixedEdgeBottom),
    EZ_ENUM_CONSTANT(ezJoltBreakableSlabFlags::FixedEdgeLeft),
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltBreakableShape, 1)
  EZ_ENUM_CONSTANT(ezJoltBreakableShape::Rectangle),
    EZ_ENUM_CONSTANT(ezJoltBreakableShape::Triangle),
    EZ_ENUM_CONSTANT(ezJoltBreakableShape::Circle),
EZ_END_STATIC_REFLECTED_ENUM

ezAtomicInteger32 ezJoltBreakableSlabComponent::s_iShardMeshCounter;

ezCVarBool cvar_BreakableSlabVis("BreakableSlab.DebugVis", false, ezCVarFlags::Default, "Debug draw the state of breakable slabs.");

ezJoltBreakableSlabComponentManager::ezJoltBreakableSlabComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltBreakableSlabComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezJoltBreakableSlabComponentManager::~ezJoltBreakableSlabComponentManager() = default;

void ezJoltBreakableSlabComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltBreakableSlabComponentManager::Update, this);
    desc.m_Phase = ezWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void ezJoltBreakableSlabComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("UpdateBreakableSlabs");

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  // when updating slabs, we activate bodies, which would immediately modify the active slabs map
  // and then create a crash during iteration
  // therefore we have to make a copy of the active slabs first
  ezHybridArray<ezJoltBreakableSlabComponent*, 64> activeSlabs;
  activeSlabs.Reserve(pModule->GetActiveSlabs().GetCount());

  for (auto it : pModule->GetActiveSlabs())
  {
    ezJoltBreakableSlabComponent* pComponent = it.Key();
    activeSlabs.PushBack(pComponent);
  }

  for (auto pComponent : activeSlabs)
  {
    pComponent->RetrieveShardTransforms();
  }

  for (ezJoltBreakableSlabComponent* pComponent : pModule->GetSlabsPutToSleep())
  {
    pComponent->RetrieveShardTransforms();
  }

  for (auto hComponent : m_RequireBreakage)
  {
    ezJoltBreakableSlabComponent* pComponent;
    if (TryGetComponent(hComponent, pComponent))
    {
      if (pComponent->IsActiveAndSimulating() && pComponent->m_pShatterTask == nullptr)
      {
        pComponent->m_pShatterTask = EZ_DEFAULT_NEW(ezShatterTask);
        pComponent->m_pShatterTask->ConfigureTask("ShatterGlass", ezTaskNesting::Never);
        pComponent->m_pShatterTask->m_pComponent = pComponent;
        pComponent->m_pShatterTask->m_ShatterPoints = pComponent->m_ShatterPoints;

        ezTaskSystem::StartSingleTask(pComponent->m_pShatterTask, ezTaskPriority::In3Frames); // allow some delay, this can take longer than a single frame

        m_RequireBreakUpdate.Insert(hComponent);
      }

      // make sure the shatter points are cleared, even if nothing was done
      // otherwise the component won't be put into m_RequireBreakage again
      pComponent->m_ShatterPoints.Clear();
    }
  }

  m_RequireBreakage.Clear();

  for (auto hComponent : m_RequireBreakUpdate)
  {
    ezJoltBreakableSlabComponent* pComponent;
    if (TryGetComponent(hComponent, pComponent) && pComponent->IsActiveAndSimulating() && pComponent->m_pShatterTask != nullptr)
    {
      if (pComponent->m_pShatterTask->IsTaskFinished())
      {
        pComponent->ApplyBreak(pComponent->m_pShatterTask->m_Shapes, pComponent->m_pShatterTask->m_hShardsMesh, pComponent->m_pShatterTask->m_vFinalImpulse);
        pComponent->m_pShatterTask = nullptr;

        m_RequireBreakUpdate.Remove(hComponent);
        break; // sufficient to update one slab per frame
      }
    }
    else
    {
      m_RequireBreakUpdate.Remove(hComponent);
    }
  }

  if (cvar_BreakableSlabVis)
  {
    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->IsActiveAndSimulating())
      {
        it->DebugDraw();
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltBreakableSlabComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 8.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 8.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new ezDefaultValueAttribute(0.02f), new ezClampValueAttribute(0.005f, 1.0f), new ezSuffixAttribute(" m")),
    EZ_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_ACCESSOR_PROPERTY("UVScale", GetUvScale, SetUvScale)->AddAttributes(new ezDefaultValueAttribute(ezVec2(1.0f))),
    EZ_MEMBER_PROPERTY("CollisionLayerStatic", m_uiCollisionLayerStatic)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("CollisionLayerDynamic", m_uiCollisionLayerDynamic)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ENUM_ACCESSOR_PROPERTY("Shape", ezJoltBreakableShape, GetShape, SetShape),
    EZ_BITFLAGS_ACCESSOR_PROPERTY("Flags", ezJoltBreakableSlabFlags, GetFlags, SetFlags),
    EZ_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("ContactReportForceThreshold", m_fContactReportForceThreshold),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Effects"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, OnMsgPhysicsAddImpulse),
    EZ_MESSAGE_HANDLER(ezMsgPhysicContact, OnMsgPhysicContactMsg),
    EZ_MESSAGE_HANDLER(ezMsgPhysicCharacterContact, OnMsgPhysicCharacterContact),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Restore),
    EZ_SCRIPT_FUNCTION_PROPERTY(ShatterCellular, In, "vGlobalPosition", In, "fCellSize", In, "vImpulse", In, "fMakeDynamicRadius"),
    EZ_SCRIPT_FUNCTION_PROPERTY(ShatterRadial, In, "vGlobalPosition", In, "fImpactRadius", In, "vImpulse", In, "fMakeDynamicRadius"),
    EZ_SCRIPT_FUNCTION_PROPERTY(ShatterAll, In, "fShardSize", In, "vImpulse"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezJoltBreakableSlabComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fWidth;
  s << m_fHeight;
  s << m_fThickness;
  s << m_hMaterial;
  s << m_vUvScale;
  s << m_uiCollisionLayerStatic;
  s << m_uiCollisionLayerDynamic;
  s << m_Flags;
  s << m_Shape;
  s << m_fGravityFactor;
  s << m_fContactReportForceThreshold;
}

void ezJoltBreakableSlabComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_fWidth;
  s >> m_fHeight;
  s >> m_fThickness;
  s >> m_hMaterial;
  s >> m_vUvScale;
  s >> m_uiCollisionLayerStatic;
  s >> m_uiCollisionLayerDynamic;
  s >> m_Flags;
  s >> m_Shape;
  s >> m_fGravityFactor;
  s >> m_fContactReportForceThreshold;
}

void ezJoltBreakableSlabComponent::OnActivated()
{
  SUPER::OnActivated();

  ReinitMeshes();
}

void ezJoltBreakableSlabComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  if (m_uiObjectFilterID == ezInvalidIndex)
  {
    // only create a new filter ID, if none has been passed in manually
    m_uiObjectFilterID = pModule->CreateObjectFilterID();
  }

  if (m_uiUserDataIndexStatic == ezInvalidIndex)
  {
    ezJoltUserData* pUserData = nullptr;
    m_uiUserDataIndexStatic = pModule->AllocateUserData(pUserData);
    pUserData->Init(this, ezOnJoltContact::SendContactMsg);
  }

  if (m_uiUserDataIndexDynamic == ezInvalidIndex)
  {
    ezJoltUserData* pUserData = nullptr;
    m_uiUserDataIndexDynamic = pModule->AllocateUserData(pUserData);
    pUserData->Init(this, ezOnJoltContact::ImpactReactions);
  }

  ezHybridArray<JPH::Ref<JPH::ConvexShape>, 2> shapes;
  PrepareShardColliders(0, shapes);

  m_hSwitchToMesh = m_hMeshToRender;
  m_iSwitchToSkinningState = CreateShardColliders(0, shapes);
  m_uiShardsSleeping = 190;

  InvalidateCachedRenderData();
}

void ezJoltBreakableSlabComponent::OnDeactivated()
{
  Cleanup();

  if (ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>())
  {
    pModule->DeallocateUserData(m_uiUserDataIndexStatic);
    pModule->DeallocateUserData(m_uiUserDataIndexDynamic);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  SUPER::OnDeactivated();
}

ezResult ezJoltBreakableSlabComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = m_Bounds;
  return EZ_SUCCESS;
}

void ezJoltBreakableSlabComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;

  ReinitMeshes();
}

float ezJoltBreakableSlabComponent::GetWidth() const
{
  return m_fWidth;
}

void ezJoltBreakableSlabComponent::SetHeight(float fHeight)
{
  if (fHeight <= 0.0f)
    return;

  m_fHeight = fHeight;

  ReinitMeshes();
}

float ezJoltBreakableSlabComponent::GetHeight() const
{
  return m_fHeight;
}

void ezJoltBreakableSlabComponent::SetThickness(float fThickness)
{
  if (fThickness <= 0.0f)
    return;

  m_fThickness = fThickness;
  ReinitMeshes();
}

float ezJoltBreakableSlabComponent::GetThickness() const
{
  return m_fThickness;
}

void ezJoltBreakableSlabComponent::SetUvScale(ezVec2 vScale)
{
  if (m_vUvScale == vScale)
    return;

  m_vUvScale = vScale;
  ReinitMeshes();
}


ezVec2 ezJoltBreakableSlabComponent::GetUvScale() const
{
  return m_vUvScale;
}

void ezJoltBreakableSlabComponent::SetFlags(ezBitflags<ezJoltBreakableSlabFlags> flags)
{
  m_Flags = flags;
  ReinitMeshes();
}

void ezJoltBreakableSlabComponent::SetShape(ezEnum<ezJoltBreakableShape> shape)
{
  m_Shape = shape;
  ReinitMeshes();
}

void ezJoltBreakableSlabComponent::Restore()
{
  Cleanup();
  ReinitMeshes();

  ezHybridArray<JPH::Ref<JPH::ConvexShape>, 2> shapes;
  PrepareShardColliders(0, shapes);

  m_hSwitchToMesh = m_hMeshToRender;
  m_iSwitchToSkinningState = CreateShardColliders(0, shapes);
  m_uiShardsSleeping = 190;

  InvalidateCachedRenderData();
}

bool ezJoltBreakableSlabComponent::IsPointOnSlab(const ezVec3& vGlobalPosition) const
{
  const ezPlane mainPlane = ezPlane::MakeFromNormalAndPoint(GetOwner()->GetGlobalDirUp(), GetOwner()->GetGlobalPosition());
  const float fDist = ezMath::Abs(mainPlane.GetDistanceTo(vGlobalPosition));
  if (fDist > ezMath::Max(JPH::cDefaultConvexRadius * 2.0f, m_fThickness))
    return false;

  return true;
}

ezUInt32 ezJoltBreakableSlabComponent::FindClosestShard(const ezVec3& vGlobalPosition) const
{
  const ezVec2 vLocalPos = (GetOwner()->GetGlobalTransform().GetInverse() * vGlobalPosition).GetAsVec2();
  float fBestDistance = ezMath::HighValue<float>();

  ezUInt32 uiShardIdx = ezInvalidIndex;

  const ezUInt32 uiShards = m_Breakable.m_Shards.GetCount();
  for (ezUInt32 i = 0; i < uiShards; ++i)
  {
    const auto& shard = m_Breakable.m_Shards[i];
    if (shard.m_bShattered || shard.m_bDynamic)
      continue;

    const float fDistSqr = (shard.m_vCenterPosition - vLocalPos).GetLengthSquared();

    if (fDistSqr > fBestDistance)
      continue;

    // if the shard is smaller than we are close to it, ignore it
    if (fDistSqr > ezMath::Square(shard.m_fBoundingRadius))
      continue;

    fBestDistance = fDistSqr;
    uiShardIdx = i;
  }

  return uiShardIdx;
}

void ezJoltBreakableSlabComponent::ShatterCellular(const ezVec3& vGlobalPosition, float fCellSize, const ezVec3& vImpulse, float fMakeDynamicRadius)
{
  if (!IsPointOnSlab(vGlobalPosition))
    return;

  const ezUInt32 uiShardIdx = FindClosestShard(vGlobalPosition);
  if (uiShardIdx == ezInvalidIndex)
    return;

  if (m_ShatterPoints.IsEmpty())
  {
    ((ezJoltBreakableSlabComponentManager*)GetOwningManager())->m_RequireBreakage.Insert(GetHandle());
  }

  auto& pt = m_ShatterPoints.ExpandAndGetRef();
  pt.m_vGlobalPosition = vGlobalPosition;
  pt.m_vImpulse = vImpulse;
  pt.m_fCellSize = fCellSize;
  pt.m_uiShardIdx = uiShardIdx;
  pt.m_fMakeDynamicRadius = fMakeDynamicRadius;
  pt.m_uiAllowedBreakPatterns = (ezUInt8)ezBreakablePattern::Cellular;
}

void ezJoltBreakableSlabComponent::ShatterRadial(const ezVec3& vGlobalPosition, float fImpactRadius, const ezVec3& vImpulse, float fMakeDynamicRadius)
{
  if (!IsPointOnSlab(vGlobalPosition))
    return;

  const ezUInt32 uiShardIdx = FindClosestShard(vGlobalPosition);
  if (uiShardIdx == ezInvalidIndex)
    return;

  if (m_ShatterPoints.IsEmpty())
  {
    ((ezJoltBreakableSlabComponentManager*)GetOwningManager())->m_RequireBreakage.Insert(GetHandle());
  }

  auto& pt = m_ShatterPoints.ExpandAndGetRef();
  pt.m_vGlobalPosition = vGlobalPosition;
  pt.m_vImpulse = vImpulse;
  pt.m_fImpactRadius = fImpactRadius;
  pt.m_fCellSize = 0.4f;
  pt.m_uiShardIdx = uiShardIdx;
  pt.m_fMakeDynamicRadius = fMakeDynamicRadius;
  pt.m_uiAllowedBreakPatterns = (ezUInt8)ezBreakablePattern::Radial | (ezUInt8)ezBreakablePattern::Cellular;
}

void ezJoltBreakableSlabComponent::ShatterAll(float fShardSize, const ezVec3& vImpulse)
{
  m_fContactReportForceThreshold = 0.0f;

  if (m_ShatterPoints.IsEmpty())
  {
    ((ezJoltBreakableSlabComponentManager*)GetOwningManager())->m_RequireBreakage.Insert(GetHandle());
  }

  auto& pt = m_ShatterPoints.ExpandAndGetRef();
  pt.m_vGlobalPosition.Set(1024, 1024, 1024);
  pt.m_vImpulse = vImpulse;
  pt.m_fCellSize = fShardSize;
  pt.m_uiShardIdx = ezInvalidIndex;
}

void ezJoltBreakableSlabComponent::PrepareBreakAsync(ezDynamicArray<JPH::Ref<JPH::ConvexShape>>& out_Shapes, ezArrayPtr<const ezShatterPoint> points)
{
  EZ_PROFILE_SCOPE("PrepareBreakAsync");


  for (ezUInt32 i = 0; i < points.GetCount(); ++i)
  {
    const auto& point = points[i];

    if (point.m_vGlobalPosition == ezVec3(1024, 1024, 1024))
    {
      // shatter all pieces
      m_Breakable.ShatterAll(point.m_fCellSize, GetWorld()->GetRandomNumberGenerator(), true);
      break;
    }
    else
    {

      if (point.m_uiShardIdx != ezInvalidIndex && !m_Breakable.m_Shards[point.m_uiShardIdx].m_bShattered)
      {
        const ezUInt32 uiShardIdxOffset = m_Breakable.m_Shards.GetCount();
        const ezVec2 vLocalPos = (GetOwner()->GetGlobalTransform().GetInverse() * point.m_vGlobalPosition).GetAsVec2();

        m_Breakable.ShatterShard(point.m_uiShardIdx, vLocalPos, GetWorld()->GetRandomNumberGenerator(), ezMath::Clamp(point.m_fImpactRadius, 0.05f, 0.4f), ezMath::Clamp(point.m_fCellSize, 0.3f, 0.5f), point.m_uiAllowedBreakPatterns);

        const float fDistSqr = ezMath::Square(point.m_fMakeDynamicRadius);

        for (ezUInt32 idx = uiShardIdxOffset; idx < m_Breakable.m_Shards.GetCount(); ++idx)
        {
          // make everything dynamic that can't be broken further
          if (m_Breakable.m_Shards[idx].m_uiBreakablePatterns == (ezUInt8)ezBreakablePattern::None)
          {
            m_Breakable.m_Shards[idx].m_bDynamic = true;
          }
          else if ((m_Breakable.m_Shards[idx].m_vCenterPosition - vLocalPos).GetLengthSquared() <= fDistSqr)
          {
            // and everything that's close enough to the shatter point
            m_Breakable.m_Shards[idx].m_bDynamic = true;
          }
        }
      }
    }
  }


  m_Breakable.RecalculateDymamic();

  PrepareShardColliders(m_ShardBodyIDs.GetCount(), out_Shapes);
}

void ezJoltBreakableSlabComponent::ApplyBreak(ezArrayPtr<JPH::Ref<JPH::ConvexShape>> shapes, const ezMeshResourceHandle& hMesh, const ezVec3& vImpulse)
{
  EZ_PROFILE_SCOPE("ApplyBreak");

  UpdateShardColliders();

  const ezUInt32 uiFirstShard = m_ShardBodyIDs.GetCount();

  m_hSwitchToMesh = hMesh;
  m_iSwitchToSkinningState = CreateShardColliders(uiFirstShard, shapes);

  ApplyImpulse(vImpulse, uiFirstShard);

  // make sure everyone around is awake
  WakeUpBodies();
}

void ezJoltBreakableSlabComponent::Cleanup()
{
  if (m_pShatterTask)
  {
    ezTaskSystem::CancelTask(m_pShatterTask, ezOnTaskRunning::WaitTillFinished).IgnoreResult();
    m_pShatterTask = nullptr;
  }

  DestroyAllShardColliders();

  m_Breakable.Clear();
  m_ShatterPoints.Clear();

  m_SkinningState[0].Clear();
  m_SkinningState[1].Clear();
  m_bSkinningUpdated[0] = false;
  m_bSkinningUpdated[1] = false;

  m_hMeshToRender.Invalidate();
  m_hSwitchToMesh.Invalidate();

  m_iSkinningStateToUse = -1;
  m_iSwitchToSkinningState = -1;
}

void ezJoltBreakableSlabComponent::ReinitMeshes()
{
  Cleanup();

  if (!IsActive())
    return;

  m_Breakable.Initialize();

  auto& shard = m_Breakable.m_Shards[0];

  if (m_Shape == ezJoltBreakableShape::Rectangle)
  {
    shard.m_vCenterPosition.Set(m_fWidth * 0.5f, m_fHeight * 0.5f);
    shard.m_fBoundingRadius = ezMath::Max(m_fWidth, m_fHeight) * 0.75f;
    shard.m_Edges.SetCount(4);

    shard.m_Edges[0].m_vStartPosition.Set(0, 0);
    if (m_Flags.IsSet(ezJoltBreakableSlabFlags::FixedEdgeLeft))
      shard.m_Edges[0].m_uiOutsideShardIdx = ezBreakableShard2D::FixedEdge;

    shard.m_Edges[1].m_vStartPosition.Set(0, m_fHeight);
    if (m_Flags.IsSet(ezJoltBreakableSlabFlags::FixedEdgeTop))
      shard.m_Edges[1].m_uiOutsideShardIdx = ezBreakableShard2D::FixedEdge;

    shard.m_Edges[2].m_vStartPosition.Set(m_fWidth, m_fHeight);
    if (m_Flags.IsSet(ezJoltBreakableSlabFlags::FixedEdgeRight))
      shard.m_Edges[2].m_uiOutsideShardIdx = ezBreakableShard2D::FixedEdge;

    shard.m_Edges[3].m_vStartPosition.Set(m_fWidth, 0);
    if (m_Flags.IsSet(ezJoltBreakableSlabFlags::FixedEdgeBottom))
      shard.m_Edges[3].m_uiOutsideShardIdx = ezBreakableShard2D::FixedEdge;
  }
  else if (m_Shape == ezJoltBreakableShape::Triangle)
  {
    shard.m_vCenterPosition.Set(m_fWidth * 0.5f, m_fHeight * 0.5f);
    shard.m_fBoundingRadius = ezMath::Max(m_fWidth, m_fHeight) * 0.75f;
    shard.m_Edges.SetCount(3);

    shard.m_Edges[0].m_vStartPosition.Set(0, 0);
    if (m_Flags.IsSet(ezJoltBreakableSlabFlags::FixedEdgeLeft))
      shard.m_Edges[0].m_uiOutsideShardIdx = ezBreakableShard2D::FixedEdge;

    shard.m_Edges[1].m_vStartPosition.Set(0, m_fHeight);
    if (m_Flags.IsSet(ezJoltBreakableSlabFlags::FixedEdgeTop) || m_Flags.IsSet(ezJoltBreakableSlabFlags::FixedEdgeRight))
      shard.m_Edges[1].m_uiOutsideShardIdx = ezBreakableShard2D::FixedEdge;

    shard.m_Edges[2].m_vStartPosition.Set(m_fWidth, 0);
    if (m_Flags.IsSet(ezJoltBreakableSlabFlags::FixedEdgeBottom))
      shard.m_Edges[2].m_uiOutsideShardIdx = ezBreakableShard2D::FixedEdge;
  }
  else if (m_Shape == ezJoltBreakableShape::Circle)
  {
    const ezUInt32 num = 16;

    shard.m_vCenterPosition.Set(m_fWidth * 0.5f, m_fHeight * 0.5f);
    shard.m_fBoundingRadius = ezMath::Max(m_fWidth, m_fHeight) * 0.75f;
    shard.m_Edges.SetCount(num);

    const float hw = m_fWidth * 0.5f;
    const float hh = m_fHeight * 0.5f;

    const ezUInt32 uiOutsideIdx = m_Flags.IsAnySet(ezJoltBreakableSlabFlags::Default) ? ezBreakableShard2D::FixedEdge : ezBreakableShard2D::LooseEdge;

    for (ezUInt32 i = 0; i < num; ++i)
    {
      const ezAngle a = ezAngle::MakeFromDegree((360.0f / num) * i);

      shard.m_Edges[i].m_vStartPosition.Set(hw + ezMath::Sin(a) * hw, hh + ezMath::Cos(a) * hh);
      shard.m_Edges[i].m_uiOutsideShardIdx = uiOutsideIdx;
    }
  }

  m_hMeshToRender = CreateShardsMesh();

  InvalidateCachedRenderData();

  m_Bounds = ezBoundingBox::MakeFromMinMax(ezVec3::MakeZero(), ezVec3(m_fWidth, m_fHeight, m_fThickness));
  TriggerLocalBoundsUpdate();
}

void ezJoltBreakableSlabComponent::BuildMeshResourceFromGeometry(ezGeometry& Geometry, ezMeshResourceDescriptor& MeshDesc, bool bWithSkinningData) const
{
  auto& MeshBufferDesc = MeshDesc.MeshBufferDesc();

  MeshBufferDesc.AddCommonStreams();

  if (bWithSkinningData)
  {
    MeshBufferDesc.AddStream(ezGALVertexAttributeSemantic::BoneWeights0, ezGALResourceFormat::RGBAUByteNormalized);
    MeshBufferDesc.AddStream(ezGALVertexAttributeSemantic::BoneIndices0, ezGALResourceFormat::RGBAUShort);
  }
  MeshBufferDesc.AllocateStreamsFromGeometry(Geometry, ezGALPrimitiveTopology::Triangles);

  MeshDesc.AddSubMesh(MeshBufferDesc.GetPrimitiveCount(), 0, 0);

  MeshDesc.ComputeBounds();
}

const ezJoltMaterial* ezJoltBreakableSlabComponent::GetPhysicsMaterial()
{
  if (m_hMaterial.IsValid())
  {
    if (!m_hSurface.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMat(m_hMaterial, ezResourceAcquireMode::BlockTillLoaded);

      if (!pMat->GetSurface().IsEmpty())
      {
        m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(pMat->GetSurface());
      }
    }

    if (m_hSurface.IsValid())
    {
      ezResourceLock<ezSurfaceResource> pSurf(m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

      if (pSurf->m_pPhysicsMaterialJolt != nullptr)
      {
        return static_cast<ezJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);
      }
    }
  }

  return ezJoltCore::GetDefaultMaterial();
}

void AddSkirtPolygons(const ezVec2& vPoint0, const ezVec2& vPoint1, float fThickness, ezGeometry& ref_geometry, const ezGeometry::GeoOptions& opt)
{
  const float fSpanX = ezMath::Abs(vPoint0.x - vPoint1.x);
  const float fSpanY = ezMath::Abs(vPoint0.y - vPoint1.y);
  const float fSpan = ezMath::Max(fSpanX, fSpanY);

  ezVec3 vPoint0Front(vPoint0.x, vPoint0.y, fThickness);
  ezVec2 vPoint0FrontUV(fThickness, 0);
  ezVec3 vPoint0Back(vPoint0.x, vPoint0.y, 0);
  ezVec2 vPoint0BackUV(0, 0);
  ezVec3 vPoint1Front(vPoint1.x, vPoint1.y, fThickness);
  ezVec2 vPoint1FrontUV(fThickness, fSpan);
  ezVec3 vPoint1Back(vPoint1.x, vPoint1.y, 0);
  ezVec2 vPoint1BackUV(0, fSpan);

  ezVec3 FaceNormal;
  if (FaceNormal.CalculateNormal(vPoint0Front, vPoint1Front, vPoint0Back).Failed())
  {
    // ignore degenerate triangles
    return;
  }

  const ezUInt32 uiIdx0 = ref_geometry.AddVertex(opt, vPoint0Front, FaceNormal, vPoint0FrontUV);
  const ezUInt32 uiIdx1 = ref_geometry.AddVertex(opt, vPoint0Back, FaceNormal, vPoint0BackUV);
  const ezUInt32 uiIdx2 = ref_geometry.AddVertex(opt, vPoint1Front, FaceNormal, vPoint1FrontUV);
  const ezUInt32 uiIdx3 = ref_geometry.AddVertex(opt, vPoint1Back, FaceNormal, vPoint1BackUV);

  {
    ezUInt32 idx[3] = {uiIdx0, uiIdx2, uiIdx1};
    ref_geometry.AddPolygon(idx, false);
  }

  {
    ezUInt32 idx[3] = {uiIdx1, uiIdx2, uiIdx3};
    ref_geometry.AddPolygon(idx, false);
  }
}

ezMeshResourceHandle ezJoltBreakableSlabComponent::CreateShardsMesh() const
{
  EZ_PROFILE_SCOPE("CreateShardsMesh");

  ezStringBuilder meshName;
  meshName.SetFormat("ezJoltBreakableSlab_Shards-{}", s_iShardMeshCounter.Increment());

  ezGeometry geo;
  ezHybridArray<ezUInt32, 16> vtxIdx1;
  ezHybridArray<ezUInt32, 16> vtxIdx2;

  const ezVec3 vNormal(0, 0, 1);

  const ezVec3 vOffset = vNormal * m_fThickness;

  for (ezUInt32 uiShardIdx = 0; uiShardIdx < m_Breakable.m_Shards.GetCount(); ++uiShardIdx)
  {
    const auto& shard = m_Breakable.m_Shards[uiShardIdx];
    vtxIdx1.Clear();
    vtxIdx2.Clear();

    ezGeometry::GeoOptions opt;
    opt.m_uiBoneIndex = uiShardIdx;

    for (ezUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {
      ezVec3 v = shard.m_Edges[i].m_vStartPosition.GetAsVec3(0);

      ezVec2 vTexCoord;
      vTexCoord.x = v.x / m_fWidth;
      vTexCoord.y = 1.0f - (v.y / m_fHeight);
      vTexCoord = vTexCoord.CompMul(m_vUvScale);

      v -= shard.m_vCenterPosition.GetAsVec3(0);

      vtxIdx1.PushBack(geo.AddVertex(opt, v, -vNormal, vTexCoord));
      vtxIdx2.PushBack(geo.AddVertex(opt, v + vOffset, vNormal, vTexCoord));
    }

    geo.AddPolygon(vtxIdx1, false);
    geo.AddPolygon(vtxIdx2, true);

    ezUInt32 uiPrev = shard.m_Edges.GetCount() - 1;
    for (ezUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {
      AddSkirtPolygons(shard.m_Edges[uiPrev].m_vStartPosition - shard.m_vCenterPosition, shard.m_Edges[i].m_vStartPosition - shard.m_vCenterPosition, m_fThickness, geo, opt);
      uiPrev = i;
    }
  }

  geo.TriangulatePolygons();
  geo.ComputeTangents();

  ezMeshResourceDescriptor desc;
  BuildMeshResourceFromGeometry(geo, desc, true /* include skinning data */);

  return ezResourceManager::CreateResource<ezMeshResource>(meshName, std::move(desc));
}

void ezJoltBreakableSlabComponent::PrepareShardColliders(ezUInt32 uiFirstShard, ezDynamicArray<JPH::Ref<JPH::ConvexShape>>& out_Shapes) const
{
  EZ_PROFILE_SCOPE("PrepareShardColliders");

  out_Shapes.SetCount(m_Breakable.m_Shards.GetCount() - uiFirstShard);

  const float fShardThickness = ezMath::Max(m_fThickness, JPH::cDefaultConvexRadius * 2.0f);
  const float fThick1 = -(fShardThickness - m_fThickness) * 0.5f;
  const float fThick2 = fShardThickness + fThick1;

  for (ezUInt32 uiShardIdx = 0; uiShardIdx < out_Shapes.GetCount(); ++uiShardIdx)
  {
    const auto& shard = m_Breakable.m_Shards[uiFirstShard + uiShardIdx];

    if (shard.m_bShattered)
      continue;

    JPH::Array<JPH::Vec3> points;
    points.reserve(shard.m_Edges.GetCount() * 2);

    for (ezUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {
      const ezVec2 v = shard.m_Edges[i].m_vStartPosition - shard.m_vCenterPosition;
      points.push_back(JPH::Vec3(v.x, v.y, fThick1));
      points.push_back(JPH::Vec3(v.x, v.y, fThick2));
    }

    JPH::ConvexHullShapeSettings shapeSettings(points);
    shapeSettings.mDensity = 0.1f;
    shapeSettings.mUserData = 0;

    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError())
    {
      // ezLog::Warning("Invalid shard shape: {} - {} points", shapeResult.GetError().c_str(), points.size());
      continue;
    }

    out_Shapes[uiShardIdx] = (JPH::ConvexShape*)(shapeResult.Get().GetPtr());
  }
}

void ezJoltBreakableSlabComponent::UpdateShardColliders()
{
  EZ_PROFILE_SCOPE("UpdateShardColliders");

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto& jphBodies = pModule->GetBodyInterface();

  for (ezUInt32 uiShardIdx = 0; uiShardIdx < m_ShardBodyIDs.GetCount(); ++uiShardIdx)
  {
    const auto& shard = m_Breakable.m_Shards[uiShardIdx];

    if (m_ShardBodyIDs[uiShardIdx] == ezInvalidIndex)
      continue;

    if (shard.m_bShattered)
    {
      // remove shard colliders that are now destroyed
      DestroyShardCollider(uiShardIdx, jphBodies, true);
      continue;
    }

    if (!shard.m_bDynamic)
      continue;

    JPH::BodyID bodyId(m_ShardBodyIDs[uiShardIdx]);
    bool bRemoveBody = false;

    {
      JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

      if (bodyLock.Succeeded())
      {
        JPH::Body& body = bodyLock.GetBody();
        if (body.GetMotionType() == JPH::EMotionType::Static)
        {
          bRemoveBody = true;

          JPH::BodyCreationSettings bodyCfg;
          bodyCfg.SetShape(body.GetShape());
          bodyCfg.mRestitution = body.GetRestitution();
          bodyCfg.mFriction = body.GetFriction();
          bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
          bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
          bodyCfg.mUserData = body.GetUserData();
          bodyCfg.mMassPropertiesOverride.mMass = 1.0f;
          bodyCfg.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
          bodyCfg.mMotionType = JPH::EMotionType::Dynamic;
          bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayerDynamic, ezJoltBroadphaseLayer::Debris);
          bodyCfg.mPosition = body.GetPosition();
          bodyCfg.mRotation = body.GetRotation();
          bodyCfg.mGravityFactor = m_fGravityFactor;

          // Create and add body
          JPH::Body* pBody = jphBodies.CreateBody(bodyCfg);
          pModule->QueueBodyToAdd(pBody, true);

          m_ShardBodyIDs[uiShardIdx] = pBody->GetID().GetIndexAndSequenceNumber();
        }
      }
    }

    if (bRemoveBody)
    {
      // can only do this outside the lock
      if (jphBodies.IsAdded(bodyId))
      {
        jphBodies.RemoveBody(bodyId);
      }

      jphBodies.DestroyBody(bodyId);
    }
  }
}

void ezJoltBreakableSlabComponent::WakeUpBodies()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  auto& jphBodies = pModule->GetBodyInterface();

  ezJoltObjectLayerFilter objectFilter(m_uiCollisionLayerStatic);

  ezUInt32 broadphase = ezPhysicsShapeType::Default;
  broadphase &= ~ezPhysicsShapeType::Static;
  broadphase &= ~ezPhysicsShapeType::Character;
  broadphase &= ~ezPhysicsShapeType::Query;
  broadphase &= ~ezPhysicsShapeType::Trigger;
  ezJoltBroadPhaseLayerFilter broadphaseFilter(static_cast<ezPhysicsShapeType::Enum>(broadphase));

  ezBoundingBox bbox;
  bbox.m_vMin.SetZero();
  bbox.m_vMax = ezVec3(m_fWidth, m_fHeight, m_fThickness);
  bbox.Grow(ezVec3(0.3f));

  bbox.TransformFromOrigin(GetOwner()->GetGlobalTransform().GetAsMat4());

  const JPH::AABox box = JPH::AABox::sFromTwoPoints(ezJoltConversionUtils::ToVec3(bbox.m_vMin), ezJoltConversionUtils::ToVec3(bbox.m_vMax));
  jphBodies.ActivateBodiesInAABox(box, broadphaseFilter, objectFilter);
}

ezUInt8 ezJoltBreakableSlabComponent::CreateShardColliders(ezUInt32 uiFirstShard, ezArrayPtr<JPH::Ref<JPH::ConvexShape>> shapes)
{
  EZ_PROFILE_SCOPE("CreateShardColliders");

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  auto& jphBodies = pModule->GetBodyInterface();

  const ezJoltUserData* pUserDataStatic = &pModule->GetUserData(m_uiUserDataIndexStatic);
  const ezJoltUserData* pUserDataDynamic = &pModule->GetUserData(m_uiUserDataIndexDynamic);
  const ezJoltMaterial* pMaterial = GetPhysicsMaterial();

  const ezInt8 iOldSkinningState = m_iSkinningStateToUse;
  const ezInt8 iNewSkinningState = m_iSkinningStateToUse == 0 ? 1 : 0;
  m_SkinningState[iNewSkinningState].Clear();

  if (iOldSkinningState != -1)
  {
    m_SkinningState[iNewSkinningState].m_Transforms = m_SkinningState[iOldSkinningState].m_Transforms;
  }

  m_SkinningState[iNewSkinningState].m_Transforms.SetCount(m_Breakable.m_Shards.GetCount());
  m_ShardBodyIDs.SetCount(m_Breakable.m_Shards.GetCount(), ezInvalidIndex);

  for (ezUInt32 uiShardIdx = uiFirstShard; uiShardIdx < m_Breakable.m_Shards.GetCount(); ++uiShardIdx)
  {
    const auto& shard = m_Breakable.m_Shards[uiShardIdx];

    if (shard.m_bShattered)
      continue;

    m_SkinningState[iNewSkinningState].m_Transforms[uiShardIdx] = ezTransform::MakeIdentity();

    JPH::ConvexShape* pShape = shapes[uiShardIdx - uiFirstShard];
    if (pShape == nullptr)
      continue;

    pShape->SetMaterial(pMaterial);

    // Body settings
    JPH::BodyCreationSettings bodyCfg;
    bodyCfg.SetShape(pShape);
    // bodyCfg.mMotionQuality = JPH::EMotionQuality::LinearCast; // don't use CCD, shards are allowed to tunnel through geometry, they will be destroyed, if necessary
    bodyCfg.mRestitution = pMaterial->m_fRestitution;
    bodyCfg.mFriction = pMaterial->m_fFriction;
    bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
    bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
    bodyCfg.mMassPropertiesOverride.mMass = 1.0f;
    bodyCfg.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    bodyCfg.mGravityFactor = m_fGravityFactor;

    if (shard.m_bDynamic == false)
    {
      bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserDataStatic);
      bodyCfg.mMotionType = JPH::EMotionType::Static;
      bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayerStatic, ezJoltBroadphaseLayer::Static);
    }
    else
    {
      bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserDataDynamic);
      bodyCfg.mMotionType = JPH::EMotionType::Dynamic;
      bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayerDynamic, ezJoltBroadphaseLayer::Debris);
    }

    // Set transform to owner
    ezTransform trans = GetOwner()->GetGlobalTransform();
    trans.m_vPosition += trans.m_qRotation * shard.m_vCenterPosition.GetAsVec3(0);

    bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_vPosition);
    bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_qRotation).Normalized();

    m_SkinningState[iNewSkinningState].m_Transforms[uiShardIdx] = trans;

    // Create and add body
    JPH::Body* pBody = jphBodies.CreateBody(bodyCfg);
    pModule->QueueBodyToAdd(pBody, shard.m_bDynamic);

    m_ShardBodyIDs[uiShardIdx] = pBody->GetID().GetIndexAndSequenceNumber();
  }

  m_bSkinningUpdated[iNewSkinningState] = true;
  m_SkinningState[iNewSkinningState].TransformsChanged();

  return iNewSkinningState;
}

void ezJoltBreakableSlabComponent::DestroyAllShardColliders()
{
  if (m_ShardBodyIDs.IsEmpty())
    return;

  if (ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>())
  {
    auto& jphBodies = pModule->GetBodyInterface();

    for (ezUInt32 uiShardIdx = 0; uiShardIdx < m_ShardBodyIDs.GetCount(); ++uiShardIdx)
    {
      DestroyShardCollider(uiShardIdx, jphBodies, false);
    }
  }

  m_ShardBodyIDs.Clear();
}

void ezJoltBreakableSlabComponent::DestroyShardCollider(ezUInt32 uiShardIdx, JPH::BodyInterface& jphBodies, bool bUpdateVis)
{
  JPH::BodyID bodyId(m_ShardBodyIDs[uiShardIdx]);
  m_ShardBodyIDs[uiShardIdx] = ezInvalidIndex;

  if (bUpdateVis)
  {
    ezInt32 iSkinningState = m_iSkinningStateToUse;
    if (m_iSwitchToSkinningState != -1)
      iSkinningState = m_iSwitchToSkinningState;

    EZ_ASSERT_DEBUG(iSkinningState != -1, "");
    m_SkinningState[iSkinningState].m_Transforms[uiShardIdx] = ezMat4::MakeScaling(ezVec3(0)); // make it disappear by scaling to zero

    // not needed here
    // m_SkinningState[m_uiSkinningStateToUse].TransformsChanged();
  }

  if (bodyId.IsInvalid())
    return;

  if (jphBodies.IsAdded(bodyId))
  {
    jphBodies.RemoveBody(bodyId);
  }

  jphBodies.DestroyBody(bodyId);
}

void ezJoltBreakableSlabComponent::RetrieveShardTransforms()
{
  if (m_uiShardsSleeping >= 200)
  {
    // invalidate the cache, because at this point it is set to 'static'
    InvalidateCachedRenderData();
  }

  m_uiShardsSleeping = 0;

  ezInt32 iSkinningState = m_iSkinningStateToUse;

  if (m_iSwitchToSkinningState != -1)
    iSkinningState = m_iSwitchToSkinningState;

  if (iSkinningState == -1)
    return;

  ezBoundingBox bbox = ezBoundingBox::MakeInvalid();

  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
  auto& jphBodies = pModule->GetBodyInterface();

  const ezVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  for (ezUInt32 idx = 0; idx < m_ShardBodyIDs.GetCount(); ++idx)
  {
    JPH::BodyID bodyId(m_ShardBodyIDs[idx]);

    if (bodyId.IsInvalid())
      continue;

    JPH::RVec3 pos;
    JPH::Quat rot;
    jphBodies.GetPositionAndRotation(bodyId, pos, rot);

    m_SkinningState[iSkinningState].m_Transforms[idx] = ezJoltConversionUtils::ToTransform(pos, rot);

    const ezVec3 vPos = ezJoltConversionUtils::ToVec3(pos);

    if ((vPos - vOwnPos).GetLengthSquared() > ezMath::Square(30.0f))
    {
      // if the shard moved pretty far away from the center, it probably tunneled through geometry and just keeps falling
      // we don't want to use continuous collision detection (CCD), because this is just low-priority eye-candy
      // so simply remove shards that appear problematic
      DestroyShardCollider(idx, jphBodies, true);
      continue;
    }

    bbox.ExpandToInclude(vPos);
  }

  if (bbox.IsValid())
  {
    const ezTransform tInv = GetOwner()->GetGlobalTransform().GetInverse();

    bbox.TransformFromOrigin(tInv.GetAsMat4());
    bbox.Grow(ezVec3(m_Breakable.m_fMaxRadius));

    m_Bounds = bbox;
    TriggerLocalBoundsUpdate();
  }

  if (!m_bSkinningUpdated[iSkinningState])
  {
    m_bSkinningUpdated[iSkinningState] = true;
    m_SkinningState[iSkinningState].TransformsChanged();
  }
}

void ezJoltBreakableSlabComponent::ApplyImpulse(const ezVec3& vImpulse, ezUInt32 uiFirstShard)
{
  const float fTorque = vImpulse.GetLength();
  if (fTorque <= 0.01f)
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  const ezVec3 vTorques[] = {
    ezVec3::MakeAxisX(),
    -ezVec3::MakeAxisX(),
    ezVec3::MakeAxisY(),
    -ezVec3::MakeAxisY(),
    ezVec3::MakeAxisZ(),
    -ezVec3::MakeAxisZ(),
    ezVec3(0.7f, 0.7f, 0.0f),
    ezVec3(0.7f, -0.7f, 0.0f),
    ezVec3(-0.7f, 0.7f, 0.0f),
    ezVec3(-0.7f, -0.7f, 0.0f),
    ezVec3(0.7f, 0, 0.7f),
    ezVec3(0.7f, 0, -0.7f),
    ezVec3(-0.7f, 0, -0.7f),
    ezVec3(-0.7f, 0, 0.7f),
    ezVec3(0, 0.7f, 0.7f),
    ezVec3(0, -0.7f, 0.7f),
  };

  const ezUInt32 uiNumTorques = EZ_ARRAY_SIZE(vTorques);

  for (ezUInt32 idx = uiFirstShard; idx < m_Breakable.m_Shards.GetCount(); ++idx)
  {
    const auto& shard = m_Breakable.m_Shards[idx];

    if (shard.m_bDynamic)
    {
      const ezUInt32 uiBodyID = m_ShardBodyIDs[idx];

      if (uiBodyID != ezInvalidIndex)
      {
        pModule->AddImpulse(uiBodyID, vImpulse);
        pModule->AddTorque(uiBodyID, vTorques[idx % uiNumTorques] * fTorque);
      }
    }
  }
}

void ezJoltBreakableSlabComponent::OnMsgPhysicsAddImpulse(ezMsgPhysicsAddImpulse& ref_msg)
{
  // not implemented atm
}

void ezJoltBreakableSlabComponent::OnMsgPhysicContactMsg(ezMsgPhysicContact& ref_msg)
{
  if (m_fContactReportForceThreshold <= 0.0f)
    return;

  if (ref_msg.m_fImpactSqr < ezMath::Square(m_fContactReportForceThreshold))
    return;

  // let the script decide what to do
  GetOwner()->PostEventMessage(ref_msg, this, ezTime::MakeZero(), ezObjectMsgQueueType::PostTransform);
}

void ezJoltBreakableSlabComponent::OnMsgPhysicCharacterContact(ezMsgPhysicCharacterContact& ref_msg)
{
  if (m_fContactReportForceThreshold <= 0.0f)
    return;

  if (ref_msg.m_fImpact < m_fContactReportForceThreshold)
    return;

  // let the script decide what to do
  GetOwner()->PostEventMessage(ref_msg, this, ezTime::MakeZero(), ezObjectMsgQueueType::PostTransform);
}

void ezJoltBreakableSlabComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  m_bSkinningUpdated[0] = false;
  m_bSkinningUpdated[1] = false;

  if (m_hSwitchToMesh.IsValid())
  {
    m_hMeshToRender = m_hSwitchToMesh;
    m_iSkinningStateToUse = m_iSwitchToSkinningState;
    m_hSwitchToMesh.Invalidate();
  }

  if (m_hMeshToRender.IsValid())
  {
    ezMeshRenderData* pRenderData = nullptr;

    if (m_iSkinningStateToUse == -1)
    {
      pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner());
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalTransform.m_vPosition += pRenderData->m_GlobalTransform.m_qRotation * ezVec3(m_fWidth * 0.5f, m_fHeight * 0.5f, 0);
    }
    else
    {
      auto pSkinnedRenderData = ezCreateRenderDataForThisFrame<ezSkinnedMeshRenderData>(GetOwner());
      pSkinnedRenderData->m_hSkinningTransforms = m_SkinningState[m_iSkinningStateToUse].m_hGpuBuffer;

      pRenderData = pSkinnedRenderData;
      pRenderData->m_GlobalTransform = ezTransform::MakeIdentity();
    }

    {
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMeshToRender;
      pRenderData->m_hMaterial = m_hMaterial;
      pRenderData->m_Color = ezColor::White;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

      pRenderData->FillSortingKey();
    }

    ezRenderData::Category category = ezDefaultRenderDataCategories::LitOpaque;
    if (m_hMaterial.IsValid())
    {
      ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
      category = pMaterial->GetRenderDataCategory();
    }

    if (m_iSkinningStateToUse != -1)
    {
      if (m_uiShardsSleeping >= 200)
      {
        msg.AddRenderData(pRenderData, category, ezRenderData::Caching::IfStatic);
      }
      else
      {
        ++m_uiShardsSleeping;
        msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);
      }
    }
    else
    {
      msg.AddRenderData(pRenderData, category, ezRenderData::Caching::IfStatic);
    }
  }
}

void ezJoltBreakableSlabComponent::DebugDraw()
{
  const ezTransform trans = GetOwner()->GetGlobalTransform();

  ezDynamicArray<ezDebugRendererLine> lines;

  const ezColor edgeColor = ezColor::White;
  const ezColor centerColor = ezColor::Yellow;

  for (const auto& shard : m_Breakable.m_Shards)
  {
    if (shard.m_bShattered)
      continue;

    auto& center = lines.ExpandAndGetRef();
    center.m_start = shard.m_vCenterPosition.GetAsVec3(0);
    center.m_end = center.m_start + ezVec3(0, 0, 0.1f);
    center.m_startColor = centerColor;
    center.m_endColor = centerColor;

    const ezColor c = shard.m_bDynamic ? ezColor::GreenYellow : ezColor::OrangeRed;


    ezUInt32 uiPrevEdge = shard.m_Edges.GetCount() - 1;
    for (ezUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {

      auto& l = lines.ExpandAndGetRef();
      l.m_start = shard.m_Edges[uiPrevEdge].m_vStartPosition.GetAsVec3(0);
      l.m_end = shard.m_Edges[i].m_vStartPosition.GetAsVec3(0);

      const ezUInt32 uiOut = shard.m_Edges[uiPrevEdge].m_uiOutsideShardIdx;
      if (uiOut > ezInvalidIndex - 32 && uiOut < ezInvalidIndex)
      {
        l.m_startColor = edgeColor;
        l.m_endColor = edgeColor;
      }
      else
      {
        l.m_startColor = c;
        l.m_endColor = c;
      }

      uiPrevEdge = i;
    }
  }

  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White, trans);

  {
    const ezVec3 vCenter = GetOwner()->GetGlobalPosition() + GetOwner()->GetGlobalRotation() * (ezVec3(m_fWidth * 0.5f, m_fHeight * 0.2f, 0));

    ezStringBuilder txt;
    txt.AppendFormat("Shards: {}\n", m_Breakable.m_Shards.GetCount());

    if (m_uiShardsSleeping >= 200)
      txt.Append("Sleeping\n");
    else
      txt.Append("Moving\n");

    ezDebugRenderer::Draw3DText(GetWorld(), txt, vCenter, ezColor::LightGray);
  }
}

void ezShatterTask::Execute()
{
  m_vFinalImpulse.SetZero();

  if (!m_ShatterPoints.IsEmpty())
  {
    for (const auto& pt : m_ShatterPoints)
    {
      m_vFinalImpulse += pt.m_vImpulse;
    }

    m_vFinalImpulse /= (float)m_ShatterPoints.GetCount();
  }

  m_pComponent->PrepareBreakAsync(m_Shapes, m_ShatterPoints);
  m_hShardsMesh = m_pComponent->CreateShardsMesh();
}

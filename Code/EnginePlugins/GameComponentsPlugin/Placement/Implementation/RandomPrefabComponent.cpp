#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GameComponentsPlugin/Placement/RandomPrefabComponent.h>

ezRandomPrefabComponentManager::ezRandomPrefabComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezRandomPrefabComponent, ezBlockStorageType::Compact>(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezRandomPrefabComponentManager::ResourceEventHandler, this));
}

ezRandomPrefabComponentManager::~ezRandomPrefabComponentManager()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezRandomPrefabComponentManager::ResourceEventHandler, this));
}

void ezRandomPrefabComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRandomPrefabComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezRandomPrefabComponentManager::Update(const ezWorldModule::UpdateContext& /*context*/)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    ezRandomPrefabComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    if (!pComponent->IsActive())
      continue;

    pComponent->InstantiatePrefabs();
  }

  m_ComponentsToUpdate.Clear();
}

void ezRandomPrefabComponentManager::AddToUpdateList(ezRandomPrefabComponent* pComponent)
{
  m_ComponentsToUpdate.Insert(pComponent->GetHandle());
}

void ezRandomPrefabComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezPrefabResource>())
  {
    ezPrefabResourceHandle hUpdatedPrefab((ezPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      for (auto& entry : it->m_Prefabs)
      {
        if (entry == hUpdatedPrefab)
        {
          AddToUpdateList(it);
          break;
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRandomPrefabComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Preview", GetPreview, SetPreview)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("Count", GetCount, SetCount)->AddAttributes(new ezDefaultValueAttribute(1)),
    EZ_ACCESSOR_PROPERTY("InstantiateAsChildren", GetInstantiateAsChildren, SetInstantiateAsChildren),
    EZ_ACCESSOR_PROPERTY("Position", GetPositionDeviation, SetPositionDeviation)->AddAttributes(new ezGroupAttribute("Random Transform"), new ezClampValueAttribute(ezVec3::MakeZero(), ezVariant())),
    EZ_ACCESSOR_PROPERTY("Rotation", GetRotationDeviation, SetRotationDeviation)->AddAttributes(new ezGroupAttribute("Random Transform"), new ezSuffixAttribute("°"), new ezClampValueAttribute(ezVec3::MakeZero(), ezVec3(180.0f))),
    EZ_ACCESSOR_PROPERTY("MinScale", GetMinUniformScale, SetMinUniformScale)->AddAttributes(new ezGroupAttribute("Random Transform"), new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_ACCESSOR_PROPERTY("MaxScale", GetMaxUniformScale, SetMaxUniformScale)->AddAttributes(new ezGroupAttribute("Random Transform"), new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_ACCESSOR_PROPERTY("Color1", GetColor1, SetColor1)->AddAttributes(new ezExposeColorAlphaAttribute(), new ezGroupAttribute("Random Color")),
    EZ_ACCESSOR_PROPERTY("Color2", GetColor2, SetColor2)->AddAttributes(new ezExposeColorAlphaAttribute(), new ezGroupAttribute("Random Color")),
    EZ_ARRAY_ACCESSOR_PROPERTY("Prefabs", Prefabs_GetCount, Prefabs_GetValue, Prefabs_SetValue, Prefabs_Insert, Prefabs_Remove)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab")),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Construction"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezRandomPrefabComponent::ezRandomPrefabComponent() = default;
ezRandomPrefabComponent::~ezRandomPrefabComponent() = default;

void ezRandomPrefabComponent::OnActivated()
{
  SUPER::OnActivated();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefabs();
}

void ezRandomPrefabComponent::OnDeactivated()
{
  // if this was created procedurally during editor runtime, we do not need to clear specific nodes
  // after simulation, the scene is deleted anyway

  ClearCreatedInstances();

  SUPER::OnDeactivated();
}

enum class RandomPrefabComponentFlags : ezUInt8
{
  SelfDeletion = 1
};

void ezRandomPrefabComponent::Deinitialize()
{
  if (GetUserFlag((ezUInt8)RandomPrefabComponentFlags::SelfDeletion))
  {
    // do nothing, ie do not call OnDeactivated()
    // we do want to keep the created child objects around when this component gets destroyed during simulation
    // that's because the component actually deletes itself when simulation starts
    return;
  }

  // remove the children (through Deactivate)
  OnDeactivated();
}

void ezRandomPrefabComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // at runtime, not in editor
  if (GetUniqueID() == ezInvalidIndex)
  {
    SetUserFlag((ezUInt8)RandomPrefabComponentFlags::SelfDeletion, true);

    // remove the prefab reference component, to prevent issues after another serialization/deserialization
    // and also to save some memory

    if (GetOwner()->GetChildCount() == 0 && GetOwner()->GetComponents().GetCount() == 1)
    {
      // if the owner object is now empty (except for this component)
      // delete the entire object (plus empty parents)
      // this happens when spawned objects are not attached as children
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
    }
    else
    {
      DeleteComponent();
    }
  }
  else // in editor
  {
    if (!m_bPreview)
    {
      InstantiatePrefabs();
    }

    if (!m_bInstantiateAsChildren)
    {
      // editor needs to always instantiate as child, so if we don't want that, fix it here

      for (auto child = GetOwner()->GetChildren(); child.IsValid(); ++child)
      {
        GetOwner()->DetachChild(child->GetHandle());
      }
    }
  }
}

void ezRandomPrefabComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_uiCount;

  s << m_bInstantiateAsChildren;
  s << m_bPreview;

  s << m_vPositionDeviation;
  s << m_vRotationDeviation;
  s << m_fMinUniformScale;
  s << m_fMaxUniformScale;

  s << m_Color1;
  s << m_Color2;

  s.WriteArray(m_Prefabs).AssertSuccess();
}

void ezRandomPrefabComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_uiCount;

  s >> m_bInstantiateAsChildren;
  s >> m_bPreview;

  s >> m_vPositionDeviation;
  s >> m_vRotationDeviation;
  s >> m_fMinUniformScale;
  s >> m_fMaxUniformScale;

  s >> m_Color1;
  s >> m_Color2;

  s.ReadArray(m_Prefabs).AssertSuccess();
}

void ezRandomPrefabComponent::SetCount(ezUInt16 uiCount)
{
  m_uiCount = uiCount;

  InstantiatePrefabs();
}

ezUInt16 ezRandomPrefabComponent::GetCount() const
{
  return m_uiCount;
}

void ezRandomPrefabComponent::SetPositionDeviation(const ezVec3& vValue)
{
  m_vPositionDeviation = vValue;
  InstantiatePrefabs();
}

void ezRandomPrefabComponent::SetRotationDeviation(const ezVec3& vValue)
{
  m_vRotationDeviation = vValue;
  InstantiatePrefabs();
}

void ezRandomPrefabComponent::SetMinUniformScale(float fValue)
{
  m_fMinUniformScale = fValue;
  InstantiatePrefabs();
}

void ezRandomPrefabComponent::SetMaxUniformScale(float fValue)
{
  m_fMaxUniformScale = fValue;
  InstantiatePrefabs();
}

void ezRandomPrefabComponent::SetColor1(const ezColor& value)
{
  m_Color1 = value;

  InstantiatePrefabs();
}

void ezRandomPrefabComponent::SetColor2(const ezColor& value)
{
  m_Color2 = value;

  InstantiatePrefabs();
}

void ezRandomPrefabComponent::SetPreview(bool bValue)
{
  m_bPreview = bValue;

  InstantiatePrefabs();
}

void ezRandomPrefabComponent::SetInstantiateAsChildren(bool bValue)
{
  m_bInstantiateAsChildren = bValue;

  InstantiatePrefabs();
}

void ezRandomPrefabComponent::ClearCreatedInstances()
{
  // we assume all children are our created instances
  // if the user attaches any other children, there will be collateral damage

  for (auto child = GetOwner()->GetChildren(); child.IsValid(); ++child)
  {
    GetWorld()->DeleteObjectNow(child->GetHandle());
  }
}

void ezRandomPrefabComponent::InstantiatePrefabs()
{
  if (!IsActiveAndInitialized())
    return;

  ClearCreatedInstances();

  if (m_Prefabs.IsEmpty())
    return;

  const ezUInt32 uiUniqueID = GetUniqueID();
  const bool bIsInEditor = (uiUniqueID != ezInvalidIndex);

  if (bIsInEditor && !m_bPreview)
  {
    if (!IsActiveAndSimulating())
    {
      return;
    }
  }

  auto MarkAsCreatedByPrefab = [](ezGameObject* pChild, ezUInt32 uiUniqueID)
  {
    // while exporting a scene all game objects with this flag are ignored and not exported
    // set this flag on all game objects that were created by instantiating this prefab
    // instead it should be instantiated at runtime again
    // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
    pChild->SetCreatedByPrefab();
    pChild->SetHideShapeIcon();

    for (auto pComponent : pChild->GetComponents())
    {
      pComponent->SetUniqueID(uiUniqueID);
      pComponent->SetCreatedByPrefab();
    }
  };

  ezHybridArray<ezGameObject*, 64> allCreatedRootObjects;
  ezHybridArray<ezGameObject*, 8> createdRootObjects;
  ezHybridArray<ezGameObject*, 8> createdChildObjects;

  const bool bAttachAsChildren = m_bInstantiateAsChildren || bIsInEditor;

  ezPrefabInstantiationOptions options;
  if (bAttachAsChildren)
  {
    options.m_hParent = GetOwner()->GetHandle();
    options.m_RandomSeedMode = ezPrefabInstantiationOptions::RandomSeedMode::DeterministicFromParent;
  }
  else
  {
    options.m_RandomSeedMode = ezPrefabInstantiationOptions::RandomSeedMode::CustomRootValue;
    options.m_uiCustomRandomSeedRootValue = GetOwner()->GetStableRandomSeed();
  }
  options.m_pCreatedRootObjectsOut = &createdRootObjects;
  options.m_pCreatedChildObjectsOut = &createdChildObjects;

  const ezSimdVec4f half(0.5f);

  const ezSimdVec4f minOffset = ezSimdConversion::ToVec3(-m_vPositionDeviation);
  const ezSimdVec4f maxOffset = ezSimdConversion::ToVec3(m_vPositionDeviation);
  // position step currently not exposed
  // const ezSimdVec4f offsetStep = ezSimdConversion::ToVec3(m_vPositionStep);

  const ezSimdVec4f minRotAndIndex = ezSimdConversion::ToVec4(-m_vRotationDeviation.GetAsVec4(0.0f));
  const ezSimdVec4f maxRotAndIndex = ezSimdConversion::ToVec4(m_vRotationDeviation.GetAsVec4(static_cast<float>(m_Prefabs.GetCount())));
  // rotation step currently not exposed
  // const ezSimdVec4f rotStep = ezSimdConversion::ToVec3(m_vRotationStep);

  const ezSimdVec4f minScaleAndColorIndex = ezSimdConversion::ToVec4(ezVec4(m_fMinUniformScale, 1, 1, 0));
  const ezSimdVec4f maxScaleAndColorIndex = ezSimdConversion::ToVec4(ezVec4(m_fMaxUniformScale, 1, 1, 1));
  // scale step currently not exposed
  // const ezSimdVec4f minScaleAndColorIndex = ezSimdConversion::ToVec4(m_vMinScale.GetAsVec4(0.0f));
  // const ezSimdVec4f maxScaleAndColorIndex = ezSimdConversion::ToVec4(m_vMaxScale.GetAsVec4(1.0f));
  // const ezSimdVec4f scaleStep = ezSimdConversion::ToVec3(m_vScaleStep);

  const bool bRandomColor = (m_Color1 != ezColor::White || m_Color2 != ezColor::White);

  for (ezUInt32 i = 0; i < m_uiCount; ++i)
  {
    ezSimdVec4i randPos = ezSimdVec4i(0, 1, 2, 3);
    ezSimdVec4u seed = ezSimdVec4u(GetOwner()->GetStableRandomSeed() + i * 137);

    ezSimdVec4f rotAndIndex = ezSimdRandom::FloatMinMax(randPos, minRotAndIndex.CompMin(maxRotAndIndex), minRotAndIndex.CompMax(maxRotAndIndex), seed);

    const auto& entry = m_Prefabs[static_cast<int>(rotAndIndex.w())];
    if (!entry.IsValid())
      continue;

    ezTransform transform;

    // position
    {
      randPos += ezSimdVec4i(13);

      ezSimdVec4f offset = ezSimdRandom::FloatMinMax(randPos, minOffset.CompMin(maxOffset), minOffset.CompMax(maxOffset), seed);

      // position step currently not exposed
      // ezSimdVec4f roundedOffset = (offset.CompDiv(offsetStep) + half).Floor().CompMul(offsetStep);
      // offset = ezSimdVec4f::Select(offsetStep == ezSimdVec4f::MakeZero(), offset, roundedOffset);

      transform.m_vPosition = ezSimdConversion::ToVec3(offset);
    }

    // rotation
    {
      ezSimdVec4f rot = rotAndIndex;

      // rotation step currently not exposed
      // ezSimdVec4f roundedRot = (rot.CompDiv(rotStep) + half).Floor().CompMul(rotStep);
      // rot = ezSimdVec4f::Select(rotStep == ezSimdVec4f::MakeZero(), rot, roundedRot);

      transform.m_qRotation = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(rot.x()), ezAngle::MakeFromDegree(rot.y()), ezAngle::MakeFromDegree(rot.z()));
    }

    float colorIndex = 0.0f;

    // scale + color
    {
      randPos += ezSimdVec4i(11);

      ezSimdVec4f scaleAndColorIndex = ezSimdRandom::FloatMinMax(randPos, minScaleAndColorIndex.CompMin(maxScaleAndColorIndex), minScaleAndColorIndex.CompMax(maxScaleAndColorIndex), seed);
      ezSimdVec4f scale = scaleAndColorIndex;
      colorIndex = scaleAndColorIndex.w();

      // scale step currently not exposed
      // ezSimdVec4f roundedScale = (scale.CompDiv(scaleStep) + half).Floor().CompMul(scaleStep);
      // scale = ezSimdVec4f::Select(scaleStep == ezSimdVec4f::MakeZero(), scale, roundedScale);

      scale = ezSimdVec4f::Select(scale == ezSimdVec4f::MakeZero(), ezSimdVec4f(1.0f), scale);

      // only use the scale value for uniform scaling
      transform.m_vScale.Set(scale.x());

      // non-uniform scaling not exposed
      // transform.m_vScale = ezSimdConversion::ToVec3(scale);
    }

    if (!bAttachAsChildren)
    {
      transform = ezTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), transform);
    }

    {
      createdRootObjects.Clear();
      createdChildObjects.Clear();

      ezResourceLock<ezPrefabResource> pResource(entry, ezResourceAcquireMode::AllowLoadingFallback);

      pResource->InstantiatePrefab(*GetWorld(), transform, options);

      allCreatedRootObjects.PushBackRange(createdRootObjects);
    }

    if (bRandomColor)
    {
      ezMsgSetColor msg;
      msg.m_Color = ezMath::Lerp(m_Color1, m_Color2, colorIndex);

      for (auto pObject : createdRootObjects)
      {
        pObject->PostMessageRecursive(msg, ezTime::MakeZero(), ezObjectMsgQueueType::AfterInitialized);
      }
    }

    if (bIsInEditor)
    {
      for (ezGameObject* pChild : createdRootObjects)
      {
        MarkAsCreatedByPrefab(pChild, uiUniqueID);
      }

      for (ezGameObject* pChild : createdChildObjects)
      {
        MarkAsCreatedByPrefab(pChild, uiUniqueID);
      }
    }
  }
}

ezUInt32 ezRandomPrefabComponent::Prefabs_GetCount() const
{
  return m_Prefabs.GetCount();
}

ezString ezRandomPrefabComponent::Prefabs_GetValue(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Prefabs.GetCount())
  {
    return "";
  }

  return m_Prefabs[uiIndex].GetResourceID();
}

void ezRandomPrefabComponent::Prefabs_SetValue(ezUInt32 uiIndex, ezString sValue)
{
  m_Prefabs.EnsureCount(uiIndex + 1);

  if (!sValue.IsEmpty())
  {
    m_Prefabs[uiIndex] = ezResourceManager::LoadResource<ezPrefabResource>(sValue);
    ezResourceManager::PreloadResource(m_Prefabs[uiIndex]);
  }
  else
  {
    m_Prefabs[uiIndex].Invalidate();
  }

  InstantiatePrefabs();
}

void ezRandomPrefabComponent::Prefabs_Insert(ezUInt32 uiIndex, ezString sValue)
{
  ezPrefabResourceHandle hResource;

  if (!sValue.IsEmpty())
  {
    hResource = ezResourceManager::LoadResource<ezPrefabResource>(sValue);
    ezResourceManager::PreloadResource(hResource);
  }

  m_Prefabs.InsertAt(uiIndex, hResource);

  InstantiatePrefabs();
}

void ezRandomPrefabComponent::Prefabs_Remove(ezUInt32 uiIndex)
{
  m_Prefabs.RemoveAtAndCopy(uiIndex);

  InstantiatePrefabs();
}

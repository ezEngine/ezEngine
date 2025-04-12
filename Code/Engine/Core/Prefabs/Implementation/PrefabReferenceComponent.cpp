#include <Core/CorePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPrefabReferenceComponent, 4, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Prefab", GetPrefab, SetPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab")),
    EZ_ACCESSOR_PROPERTY("ShowShapeIcons", GetShowShapeIcons, SetShowShapeIcons),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Prefab")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Prefabs"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum class PrefabComponentFlags : ezUInt8
{
  SelfDeletion = 1,   ///< the prefab component is currently deleting itself but does not want to remove the instantiated objects
  ShowShapeIcons = 2, ///< the prefab component should show shape icons for the instantiated objects
  InUpdateList = 3,   ///< the prefab component is currently in the update list of the prefab manager
};

ezPrefabReferenceComponent::ezPrefabReferenceComponent() = default;
ezPrefabReferenceComponent::~ezPrefabReferenceComponent() = default;

void ezPrefabReferenceComponent::SerializePrefabParameters(const ezWorld& world, ezWorldWriter& inout_stream, ezArrayMap<ezHashedString, ezVariant> parameters)
{
  // we need a copy of the parameters here, therefore we don't take it by reference

  auto& s = inout_stream.GetStream();
  const ezUInt32 numParams = parameters.GetCount();

  ezHybridArray<ezGameObjectHandle, 8> GoReferences;

  // Version 4
  {
    // to support game object references as exposed parameters (which are currently exposed as strings)
    // we need to remap the string from an 'editor uuid' to something that can be interpreted as a proper ezGameObjectHandle at runtime

    // so first we get the resolver and try to map any string parameter to a valid ezGameObjectHandle
    auto resolver = world.GetGameObjectReferenceResolver();

    if (resolver.IsValid())
    {
      ezStringBuilder tmp;

      for (ezUInt32 i = 0; i < numParams; ++i)
      {
        // if this is a string parameter
        ezVariant& var = parameters.GetValue(i);
        if (var.IsA<ezString>())
        {
          // and the resolver CAN map this string to a game object handle
          ezGameObjectHandle hObject = resolver(var.Get<ezString>().GetData(), ezComponentHandle(), nullptr);
          if (!hObject.IsInvalidated())
          {
            // write the handle properly to file (this enables correct remapping during deserialization)
            // and discard the string's value, and instead write a string that specifies the index of the serialized handle to use

            // local game object reference - index into GoReferences
            tmp.SetFormat("#!LGOR-{}", GoReferences.GetCount());
            var = tmp.GetData();

            GoReferences.PushBack(hObject);
          }
        }
      }
    }

    // now write all the ezGameObjectHandle's such that during deserialization the ezWorldReader will remap it as needed
    const ezUInt8 numRefs = static_cast<ezUInt8>(GoReferences.GetCount());
    s << numRefs;

    for (ezUInt8 i = 0; i < numRefs; ++i)
    {
      inout_stream.WriteGameObjectHandle(GoReferences[i]);
    }
  }

  // Version 2
  s << numParams;
  for (ezUInt32 i = 0; i < numParams; ++i)
  {
    s << parameters.GetKey(i);
    s << parameters.GetValue(i); // this may contain modified strings now, to map the game object handle references
  }
}

void ezPrefabReferenceComponent::DeserializePrefabParameters(ezArrayMap<ezHashedString, ezVariant>& out_parameters, ezWorldReader& inout_stream)
{
  out_parameters.Clear();

  // versioning of this stuff is tied to the version number of ezPrefabReferenceComponent
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(ezGetStaticRTTI<ezPrefabReferenceComponent>());
  auto& s = inout_stream.GetStream();

  // temp array to hold (and remap) the serialized game object handles
  ezHybridArray<ezGameObjectHandle, 8> GoReferences;

  if (uiVersion >= 4)
  {
    ezUInt8 numRefs = 0;
    s >> numRefs;
    GoReferences.SetCountUninitialized(numRefs);

    // just read them all, this will remap as necessary to the ezWorldReader
    for (ezUInt8 i = 0; i < numRefs; ++i)
    {
      GoReferences[i] = inout_stream.ReadGameObjectHandle();
    }
  }

  if (uiVersion >= 2)
  {
    ezUInt32 numParams = 0;
    s >> numParams;

    out_parameters.Reserve(numParams);

    ezHashedString key;
    ezVariant value;
    ezStringBuilder tmp;

    for (ezUInt32 i = 0; i < numParams; ++i)
    {
      s >> key;
      s >> value;

      if (value.IsA<ezString>())
      {
        // if we find a string parameter, check if it is a 'local game object reference'
        const ezString& str = value.Get<ezString>();
        if (str.StartsWith("#!LGOR-"))
        {
          // if so, extract the index into the GoReferences array
          ezInt32 idx;
          if (ezConversionUtils::StringToInt(str.GetData() + 7, idx).Succeeded())
          {
            // now we can lookup the remapped ezGameObjectHandle from our array
            const ezGameObjectHandle hObject = GoReferences[idx];

            // and stringify the handle into a 'global game object reference', ie. one that contains the internal integer data of the handle
            // a regular runtime world has a reference resolver that is capable to reverse this stringified format to a handle again
            // which will happen once 'InstantiatePrefab' passes the m_Parameters list to the newly created objects
            tmp.SetFormat("#!GGOR-{}", hObject.GetInternalID().m_Data);

            // map local game object reference to global game object reference
            value = tmp.GetData();
          }
        }
      }

      out_parameters.Insert(key, value);
    }
  }
}

void ezPrefabReferenceComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hPrefab;

  ezPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void ezPrefabReferenceComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hPrefab;

  if (uiVersion < 3)
  {
    bool bDummy;
    s >> bDummy;
  }

  ezPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
}

void ezPrefabReferenceComponent::SetPrefab(const ezPrefabResourceHandle& hPrefab)
{
  if (m_hPrefab == hPrefab)
    return;

  m_hPrefab = hPrefab;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway

    GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void ezPrefabReferenceComponent::SetShowShapeIcons(bool bShow)
{
  SetUserFlag((ezUInt8)PrefabComponentFlags::ShowShapeIcons, bShow);

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway

    GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

bool ezPrefabReferenceComponent::GetShowShapeIcons() const
{
  return GetUserFlag((ezUInt8)PrefabComponentFlags::ShowShapeIcons);
}

void ezPrefabReferenceComponent::InstantiatePrefab()
{
  // now instantiate the prefab
  if (m_hPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pResource(m_hPrefab, ezResourceAcquireMode::AllowLoadingFallback);

    ezTransform id;
    id.SetIdentity();

    ezPrefabInstantiationOptions options;
    options.m_hParent = GetOwner()->GetHandle();
    options.m_ReplaceNamedRootWithParent = "<Prefab-Root>";
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    // if this ID is valid, this prefab is instantiated at editor runtime
    // replicate the same ID across all instantiated sub components to get correct picking behavior
    if (GetUniqueID() != ezInvalidIndex)
    {
      ezHybridArray<ezGameObject*, 8> createdRootObjects;
      ezHybridArray<ezGameObject*, 16> createdChildObjects;

      options.m_pCreatedRootObjectsOut = &createdRootObjects;
      options.m_pCreatedChildObjectsOut = &createdChildObjects;

      ezUInt32 uiPrevCompCount = GetOwner()->GetComponents().GetCount();

      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);

      auto FixComponent = [](ezGameObject* pChild, ezUInt32 uiUniqueID, bool bShowShapeIcons)
      {
        // while exporting a scene all game objects with this flag are ignored and not exported
        // set this flag on all game objects that were created by instantiating this prefab
        // instead it should be instantiated at runtime again
        // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
        pChild->SetCreatedByPrefab();

        if (!bShowShapeIcons)
        {
          pChild->SetHideShapeIcon();
        }

        for (auto pComponent : pChild->GetComponents())
        {
          pComponent->SetUniqueID(uiUniqueID);
          pComponent->SetCreatedByPrefab();
        }
      };

      const ezUInt32 uiUniqueID = GetUniqueID();
      const bool bShowShapeIcons = GetShowShapeIcons();

      for (ezGameObject* pChild : createdRootObjects)
      {
        if (pChild == GetOwner())
          continue;

        FixComponent(pChild, uiUniqueID, bShowShapeIcons);
      }

      for (ezGameObject* pChild : createdChildObjects)
      {
        FixComponent(pChild, uiUniqueID, bShowShapeIcons);
      }

      for (; uiPrevCompCount < GetOwner()->GetComponents().GetCount(); ++uiPrevCompCount)
      {
        GetOwner()->GetComponents()[uiPrevCompCount]->SetUniqueID(GetUniqueID());
        GetOwner()->GetComponents()[uiPrevCompCount]->SetCreatedByPrefab();
      }
    }
    else
    {
      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);
    }
  }
}

void ezPrefabReferenceComponent::OnActivated()
{
  SUPER::OnActivated();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefab();
}

void ezPrefabReferenceComponent::OnDeactivated()
{
  // if this was created procedurally during editor runtime, we do not need to clear specific nodes
  // after simulation, the scene is deleted anyway

  ClearPreviousInstances();

  SUPER::OnDeactivated();
}

void ezPrefabReferenceComponent::ClearPreviousInstances()
{
  if (GetUniqueID() != ezInvalidIndex)
  {
    // if this is in the editor, and the 'activate' flag is toggled,
    // get rid of all our created child objects

    ezArrayPtr<ezComponent* const> comps = GetOwner()->GetComponents();

    for (ezUInt32 ip1 = comps.GetCount(); ip1 > 0; ip1--)
    {
      const ezUInt32 i = ip1 - 1;

      if (comps[i] != this && // don't try to delete yourself
          comps[i]->WasCreatedByPrefab())
      {
        comps[i]->DeleteComponent();
      }
    }

    for (auto it = GetOwner()->GetChildren(); it.IsValid(); ++it)
    {
      if (it->WasCreatedByPrefab())
      {
        GetWorld()->DeleteObjectNow(it->GetHandle());
      }
    }
  }
}

void ezPrefabReferenceComponent::Deinitialize()
{
  if (GetUserFlag((ezUInt8)PrefabComponentFlags::SelfDeletion))
  {
    // do nothing, ie do not call OnDeactivated()
    // we do want to keep the created child objects around when this component gets destroyed during simulation
    // that's because the component actually deletes itself when simulation starts
    return;
  }

  // remove the children (through Deactivate)
  OnDeactivated();
}

void ezPrefabReferenceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetUniqueID() == ezInvalidIndex)
  {
    SetUserFlag((ezUInt8)PrefabComponentFlags::SelfDeletion, true);

    // remove the prefab reference component, to prevent issues after another serialization/deserialization
    // and also to save some memory
    DeleteComponent();
  }
}

const ezRangeView<const char*, ezUInt32> ezPrefabReferenceComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_Parameters.GetCount(); },
    [](ezUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void ezPrefabReferenceComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway
    GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void ezPrefabReferenceComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(ezTempHashedString(szKey)))
  {
    if (IsActiveAndInitialized())
    {
      // only add to update list, if not yet activated,
      // since OnActivate will do the instantiation anyway
      GetWorld()->GetComponentManager<ezPrefabReferenceComponentManager>()->AddToUpdateList(this);
    }
  }
}

bool ezPrefabReferenceComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////

ezPrefabReferenceComponentManager::ezPrefabReferenceComponentManager(ezWorld* pWorld)
  : ezComponentManager<ComponentType, ezBlockStorageType::Compact>(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezPrefabReferenceComponentManager::ResourceEventHandler, this));
}


ezPrefabReferenceComponentManager::~ezPrefabReferenceComponentManager()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezPrefabReferenceComponentManager::ResourceEventHandler, this));
}

void ezPrefabReferenceComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPrefabReferenceComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezPrefabReferenceComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezPrefabResource>())
  {
    ezPrefabResourceHandle hPrefab((ezPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hPrefab == hPrefab)
      {
        AddToUpdateList(it);
      }
    }
  }
}

void ezPrefabReferenceComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  EZ_IGNORE_UNUSED(context);

  for (auto hComp : m_ComponentsToUpdate)
  {
    ezPrefabReferenceComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    pComponent->SetUserFlag((ezUInt8)PrefabComponentFlags::InUpdateList, false);
    if (!pComponent->IsActive())
      continue;

    pComponent->ClearPreviousInstances();
    pComponent->InstantiatePrefab();
  }

  m_ComponentsToUpdate.Clear();
}

void ezPrefabReferenceComponentManager::AddToUpdateList(ezPrefabReferenceComponent* pComponent)
{
  if (!pComponent->GetUserFlag((ezUInt8)PrefabComponentFlags::InUpdateList))
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
    pComponent->SetUserFlag((ezUInt8)PrefabComponentFlags::InUpdateList, true);
  }
}



EZ_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabReferenceComponent);

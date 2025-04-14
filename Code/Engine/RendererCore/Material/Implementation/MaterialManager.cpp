#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Material/MaterialManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

EZ_IMPLEMENT_SINGLETON(ezMaterialManager);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, MaterialManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezMaterialManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezMaterialManager* pDummy = ezMaterialManager::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    // Required here so that we clean up extracted data before the frame allocator gets destroyed.
    ezMaterialManager::GetSingleton()->Cleanup();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezEvent<const ezMaterialShaderChanged&, ezMutex> ezMaterialManager::s_MaterialShaderChangedEvent;

void ezMaterialManager::MaterialAdded(ezMaterialResource* pMaterial)
{
  ezMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return;
  pManager->RegisterMaterial(pMaterial);
}

void ezMaterialManager::MaterialModified(ezMaterialResourceHandle hMaterial)
{
  ezMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return;
  EZ_LOCK(pManager->m_ExtractionMutex);
  pManager->m_ModifiedMaterials.Insert(hMaterial);
}

void ezMaterialManager::MaterialRemoved(ezMaterialResource* pMaterial)
{
  ezMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return;
  if (pMaterial->m_MaterialId.IsInvalidated())
    return;
  pManager->UnregisterMaterial(pMaterial);
}

const ezMaterialManager::MaterialData* ezMaterialManager::GetMaterialData(const ezMaterialResource* pMaterial)
{
  ezMaterialManager* pManager = GetSingleton();
  if (!pManager)
    return nullptr;

  auto it = pManager->m_Materials.Find(pMaterial);
  // EZ_ASSERT_DEV(it.IsValid(), "Loaded materials must always have a valid entry in m_Materials");
  return it.IsValid() ? &it.Value() : nullptr;
}

ezMaterialManager::ezMaterialManager()
  : m_SingletonRegistrar(this)
{
  ezRenderWorld::GetExtractionEvent().AddEventHandler(ezMakeDelegate(&ezMaterialManager::OnExtractionEvent, this));
  ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezMaterialManager::OnRenderEvent, this));
}

ezMaterialManager::~ezMaterialManager()
{
  ezRenderWorld::GetExtractionEvent().RemoveEventHandler(ezMakeDelegate(&ezMaterialManager::OnExtractionEvent, this));
  ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMaterialManager::OnRenderEvent, this));
  Cleanup();
}

void ezMaterialManager::OnExtractionEvent(const ezRenderWorldExtractionEvent& e)
{
  // ezUInt32 uiDataIndex = ezRenderWorld::GetDataIndexForExtraction();
  if (e.m_Type != ezRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  ExtractMaterialUpdates();
}

void ezMaterialManager::OnRenderEvent(const ezGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case ezGALDeviceEvent::BeforeShutdown:
    {
      Cleanup();
    }
    break;
    case ezGALDeviceEvent::AfterBeginFrame:
    {
      ApplyMaterialChanges();
    }
    break;
    default:
      break;
  }
}

void ezMaterialManager::ExtractMaterialUpdates()
{
  EZ_PROFILE_SCOPE("ExtractMaterialUpdates");
  EZ_ASSERT_DEBUG(m_pPendingChanges == nullptr, "OnRenderEvent should have been called to consume pending changes or ExtractMaterialUpdates was called twice.");

  ezHashSet<ezMaterialResourceHandle> modifiedMaterials;
  ezDynamicArray<const void*> removedMaterials;
  {
    EZ_LOCK(m_ExtractionMutex);
    modifiedMaterials.Swap(m_ModifiedMaterials);
    removedMaterials.Swap(m_RemovedMaterials);
  }

  m_pPendingChanges = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), PendingChanges);
  m_pPendingChanges->m_AddedOrModifiedMaterials.Reserve(modifiedMaterials.GetCount());

  for (const ezMaterialResourceHandle& hMaterial : modifiedMaterials)
  {
    ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::BlockTillLoaded);
    if (pMaterial->m_hShader.IsValid())
    {
      ExtractedMaterial& extractedMaterial = m_pPendingChanges->m_AddedOrModifiedMaterials.ExpandAndGetRef();
      ExtractMaterial(pMaterial.GetPointerNonConst(), extractedMaterial);
    }
  }
  m_pPendingChanges->m_RemovedMaterials = removedMaterials;
}

void ezMaterialManager::RegisterMaterial(ezMaterialResource* pMaterial)
{
  EZ_PROFILE_SCOPE("RegisterMaterial");
  EZ_LOCK(m_MaterialShaderMutex);
  EZ_LOCK(m_ExtractionMutex);

  const ezShaderResourceHandle hOldShader = pMaterial->m_hShader;
  const ezMaterialResource::ezMaterialId oldId = pMaterial->m_MaterialId;
  const bool bShaderChanged = pMaterial->m_mDesc.m_hShader != hOldShader;

  if (!hOldShader.IsValid())
  {
    if (pMaterial->m_mDesc.m_hShader.IsValid())
    {
      // Newly created material
      pMaterial->m_hShader = pMaterial->m_mDesc.m_hShader;
      MaterialShaderConstants& msc = GetShaderConstants(pMaterial->m_hShader);
      pMaterial->m_MaterialId = msc.AddMaterial(pMaterial->GetResourceHandle());
    }
  }
  else if (bShaderChanged)
  {
    // Material reloaded and changed shader
    UnregisterMaterial(pMaterial);

    // Create new material registration
    if (pMaterial->m_mDesc.m_hShader.IsValid())
    {
      pMaterial->m_hShader = pMaterial->m_mDesc.m_hShader;
      MaterialShaderConstants& msc = GetShaderConstants(pMaterial->m_hShader);
      pMaterial->m_MaterialId = msc.AddMaterial(pMaterial->GetResourceHandle());
    }
    else
    {
      pMaterial->m_hShader = pMaterial->m_mDesc.m_hShader;
      pMaterial->m_MaterialId.Invalidate();
    }

    // Fire event that the shader / id has changed and needs to be invalidated.
    ezMaterialShaderChanged change;
    change.m_hMaterial = pMaterial->GetResourceHandle();
    change.m_hOldShader = hOldShader;
    change.m_OldId = oldId;
    change.m_hNewShader = pMaterial->m_hShader;
    change.m_NewId = pMaterial->m_MaterialId;
    s_MaterialShaderChangedEvent.Broadcast(change);
  }
  else
  {
    // Material reloaded with same shader. Nothing to do except marking material dirty below.
  }
  m_ModifiedMaterials.Insert(pMaterial->GetResourceHandle());
}

void ezMaterialManager::UnregisterMaterial(ezMaterialResource* pMaterial)
{
  EZ_LOCK(m_MaterialShaderMutex);
  EZ_LOCK(m_ExtractionMutex);

  if (pMaterial->m_hShader.IsValid())
  {
    MaterialShaderConstants& msc = GetShaderConstants(pMaterial->m_hShader);
    msc.RemoveMaterial(pMaterial->m_MaterialId);
    m_RemovedMaterials.PushBack(pMaterial);
  }
}

void ezMaterialManager::ExtractMaterial(ezMaterialResource* pMaterial, ezMaterialManager::ExtractedMaterial& extractedMaterial)
{
  EZ_PROFILE_SCOPE("ExtractMaterial");
  extractedMaterial.m_hMaterial = pMaterial->GetResourceHandle();
  extractedMaterial.m_hShader = pMaterial->m_mDesc.m_hShader;
  extractedMaterial.m_MaterialId = pMaterial->m_MaterialId;
  extractedMaterial.m_DirtyFlags = pMaterial->m_DirtyFlags;
  for (ezMaterialResource::DirtyFlags::Enum flag : pMaterial->m_DirtyFlags)
  {
    switch (flag)
    {
      case ezMaterialResource::DirtyFlags::Parameter:
        extractedMaterial.m_Parameters = pMaterial->m_mDesc.m_Parameters;
        break;
      case ezMaterialResource::DirtyFlags::Texture2D:
        extractedMaterial.m_Texture2DBindings = pMaterial->m_mDesc.m_Texture2DBindings;
        break;
      case ezMaterialResource::DirtyFlags::TextureCube:
        extractedMaterial.m_TextureCubeBindings = pMaterial->m_mDesc.m_TextureCubeBindings;
        break;
      case ezMaterialResource::DirtyFlags::PermutationVar:
        extractedMaterial.m_PermutationVars = pMaterial->m_mDesc.m_PermutationVars;
        break;
      default:
        break;
    }
  }
  pMaterial->m_DirtyFlags.Clear();
}

void ezMaterialManager::ApplyMaterialChanges()
{
  if (m_pPendingChanges == nullptr)
  {
    // If no pending changes are present, we will start extracting here.
    // This is usually the case for applications that don't use extraction like tests and basic sample apps.
    ExtractMaterialUpdates();
  }
  EZ_PROFILE_SCOPE("ApplyMaterialChanges");
  // Remove dead pointers first
  for (const void* pRemovedMaterial : m_pPendingChanges->m_RemovedMaterials)
  {
    m_Materials.Remove(pRemovedMaterial);
  }

  // Execute updates and additions
  for (const auto& extractedMaterial : m_pPendingChanges->m_AddedOrModifiedMaterials)
  {
    ezResourceLock<ezMaterialResource> pMaterial(extractedMaterial.m_hMaterial, ezResourceAcquireMode::PointerOnly);
    auto matIt = m_Materials.FindOrAdd(pMaterial.GetPointer());
    MaterialData& md = matIt.Value();
    for (ezMaterialResource::DirtyFlags::Enum flag : extractedMaterial.m_DirtyFlags)
    {
      switch (flag)
      {
        case ezMaterialResource::DirtyFlags::Parameter:
          md.m_Parameters = std::move(extractedMaterial.m_Parameters);
          break;
        case ezMaterialResource::DirtyFlags::Texture2D:
          md.m_Texture2DBindings = std::move(extractedMaterial.m_Texture2DBindings);
          break;
        case ezMaterialResource::DirtyFlags::TextureCube:
          md.m_TextureCubeBindings = std::move(extractedMaterial.m_TextureCubeBindings);
          break;
        case ezMaterialResource::DirtyFlags::PermutationVar:
          md.m_PermutationVars = std::move(extractedMaterial.m_PermutationVars);
          break;
        case ezMaterialResource::DirtyFlags::ShaderAndId:
          md.m_MaterialId = extractedMaterial.m_MaterialId;
          md.m_hShader = extractedMaterial.m_hShader;
          break;
        default:
          break;
      }
    }

    EZ_LOCK(m_MaterialShaderMutex);
    const ezMaterialResource::ezMaterialId mid = md.m_MaterialId;
    auto matShaderIt = m_MaterialShaders.Find(md.m_hShader);
    EZ_ASSERT_DEBUG(matIt.IsValid(), "Material shader must exist if dirty");
    MaterialShaderConstants& ms = *matShaderIt.Value();
    ms.MarkDirty(mid);
  }

  // We need to make sure that we are not holding the m_MaterialShaderMutex when calling MaterialShaderConstants::UpdateConstantBuffers or we may deadlock. See comment in that function.
  ezHybridArray<MaterialShaderConstants*, 8> requireUpdates;
  {
    EZ_LOCK(m_MaterialShaderMutex);
    for (auto it = m_MaterialShaders.GetIterator(); it.IsValid();)
    {
      if (it.Value()->IsEmpty())
      {
        // Delete empty
        it = m_MaterialShaders.Remove(it);
      }
      else
      {
        if (it.Value()->RequiresUpdates())
        {
          // Enqueue for updates
          requireUpdates.PushBack(it.Value().Borrow());
        }
        ++it;
      }
    }
  }

  for (MaterialShaderConstants* materialShader : requireUpdates)
  {
    // Load resources outside of lock
    materialShader->UpdateConstantBuffers();
  }

  m_pPendingChanges.Clear();
}

void ezMaterialManager::Cleanup()
{
  m_ModifiedMaterials.Clear();
  m_RemovedMaterials.Clear();
  m_pPendingChanges.Clear();
  m_MaterialShaders.Clear();
  m_Materials.Clear();
}

ezMaterialManager::MaterialShaderConstants& ezMaterialManager::GetShaderConstants(ezShaderResourceHandle hShader)
{
  EZ_ASSERT_DEBUG(hShader.IsValid(), "Invalid materials should not be handled by the material manager");
  EZ_ASSERT_DEBUG(m_MaterialShaderMutex.IsLocked(), "m_MaterialShaderMutex must be locked before accessing m_MaterialShaders");
  auto it = m_MaterialShaders.FindOrAdd(hShader);
  if (it.Value() == nullptr)
  {
    it.Value() = EZ_DEFAULT_NEW(MaterialShaderConstants, hShader, this);
  }
  return *it.Value();
}


/////////////////////////////////////////////////
// MaterialShaderConstants
ezMaterialManager::MaterialShaderConstants::MaterialShaderConstants(ezShaderResourceHandle hShader, ezMaterialManager* pParent)
  : m_hShader(hShader)
  , m_pParent(pParent)
{
  // BEGIN-DOCS-CODE-SNIPPET: resource-management-listen
  // Subscribe to resource changes of the shader
  ezResourceLock<ezShaderResource> pShader(m_hShader, ezResourceAcquireMode::PointerOnly);
  pShader->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezMaterialManager::MaterialShaderConstants::OnResourceEvent, this), m_ShaderResourceEventSubscriber);
  // END-DOCS-CODE-SNIPPET

  ezEnum<ezGALBufferLayout> layout = ezGALDevice::GetDefaultDevice()->GetCapabilities().m_materialBufferLayout;
  switch (layout)
  {
    case ezGALBufferLayout::Vulkan_Std140_relaxed:
    case ezGALBufferLayout::DirectX_ConstantButter:
      m_Mode = MaterialStorageMode::MultipleConstantBuffer;
      break;
    case ezGALBufferLayout::Vulkan_Std430_relaxed:
    case ezGALBufferLayout::DirectX_StructuredButter:
      m_Mode = MaterialStorageMode::MultipleStructuredBuffer;
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  OnShaderChanged(pShader.GetPointerNonConst());
}

ezMaterialManager::MaterialShaderConstants::~MaterialShaderConstants()
{
  DestroyGpuResources();
}

ezMaterialResource::ezMaterialId ezMaterialManager::MaterialShaderConstants::AddMaterial(ezMaterialResourceHandle hMaterial)
{
  EZ_LOCK(m_MaterialsMutex);
  return m_Materials.Insert(hMaterial);
}

void ezMaterialManager::MaterialShaderConstants::RemoveMaterial(ezMaterialResource::ezMaterialId id)
{
  EZ_LOCK(m_MaterialsMutex);
  m_Materials.Remove(id);
  // We ignore the state of the buffers as the next added material will reuse it.
}

void ezMaterialManager::MaterialShaderConstants::MarkDirty(ezMaterialResource::ezMaterialId id)
{
  m_DirtyMaterials.Insert(id);
}

bool ezMaterialManager::MaterialShaderConstants::RequiresUpdates() const
{
  if (m_bShaderDirty || !m_DirtyMaterials.IsEmpty())
    return true;

  if (m_bShaderHasNoConstants)
    return false;

  EZ_LOCK(m_MaterialsMutex);
  const ezUInt32 uiCapacity = m_Materials.GetCapacity();
  const ezUInt64 uiNewSize = m_pLayout != nullptr ? m_pLayout->m_uiTotalSize * uiCapacity : 0;
  return uiNewSize != m_MaterialsData.GetCount();
}

void ezMaterialManager::MaterialShaderConstants::UpdateConstantBuffers()
{
  EZ_PROFILE_SCOPE("UpdateConstantBuffers");
  bool bLayoutChanged = false;
  if (m_bShaderDirty)
  {
    m_bShaderDirty = false;
    bLayoutChanged = UpdateMaterialLayout();
  }

  if (m_bShaderHasNoConstants)
  {
    m_DirtyMaterials.Clear();
    return;
  }

  // Resize data array
  EZ_LOCK(m_MaterialsMutex);
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezUInt32 uiCapacity = m_Materials.GetCapacity();
  const ezUInt32 uiNewSize = m_pLayout->m_uiTotalSize * uiCapacity;
  const bool bBufferResized = uiNewSize != m_MaterialsData.GetCount();
  if (bBufferResized || bLayoutChanged)
  {
    m_MaterialsData.SetCount(uiNewSize);
    switch (m_Mode)
    {
      case ezMaterialManager::MaterialStorageMode::MultipleConstantBuffer:
        m_MaterialBuffers.SetCount(uiCapacity);
        break;
      case ezMaterialManager::MaterialStorageMode::MultipleStructuredBuffer:
        m_MaterialBuffers.SetCount(uiCapacity);
        m_MaterialBufferViews.SetCount(uiCapacity);
        break;
      case ezMaterialManager::MaterialStorageMode::SingleStructuredBuffer:
      {
        if (!m_hStructuredBufferView.IsInvalidated())
          pDevice->DestroyResourceView(m_hStructuredBufferView);

        if (!m_hStructuredBuffer.IsInvalidated())
          pDevice->DestroyBuffer(m_hStructuredBuffer);

        ezGALBufferCreationDescription bufferDesc;
        bufferDesc.m_uiStructSize = m_pLayout->m_uiTotalSize;
        bufferDesc.m_uiTotalSize = uiNewSize;
        bufferDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
        bufferDesc.m_ResourceAccess.m_bImmutable = false;
        m_hStructuredBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(bufferDesc);

        ezGALBufferResourceViewCreationDescription viewDesc;
        viewDesc.m_hBuffer = m_hStructuredBuffer;
        viewDesc.m_uiFirstElement = 0;
        viewDesc.m_uiNumElements = uiCapacity;
        m_hStructuredBufferView = ezGALDevice::GetDefaultDevice()->CreateResourceView(viewDesc);
      }
      break;
      default:
        break;
    }
  }

  if (!bLayoutChanged && m_DirtyMaterials.IsEmpty() && !bBufferResized)
    return;

  ezGALCommandEncoder* pEncoder = ezGALDevice::GetDefaultDevice()->BeginCommands("UpdateMaterials");
  // #TODO_MATERIAL On resize of the array, this is recomputing the CPU work which is not necessary but was easy to implement.
  const bool bRecreateAll = bBufferResized && m_Mode == MaterialStorageMode::SingleStructuredBuffer;
  if (bLayoutChanged || bRecreateAll)
  {
    m_DirtyMaterials.Clear();
    for (auto it = m_Materials.GetIterator(); it.IsValid(); ++it)
    {
      ezMaterialResource::ezMaterialId id = it.Id();
      ezMaterialResourceHandle hMaterial = it.Value();
      UpdateMaterial(id, hMaterial, pEncoder);
    }
  }
  else
  {
    for (ezMaterialResource::ezMaterialId id : m_DirtyMaterials)
    {
      ezMaterialResourceHandle hMaterial;
      if (m_Materials.TryGetValue(id, hMaterial))
      {
        UpdateMaterial(id, hMaterial, pEncoder);
      }
    }
    m_DirtyMaterials.Clear();
  }

  ezGALDevice::GetDefaultDevice()->EndCommands(pEncoder);
}

void ezMaterialManager::MaterialShaderConstants::DestroyGpuResources()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  if (!m_hStructuredBufferView.IsInvalidated())
    pDevice->DestroyResourceView(m_hStructuredBufferView);

  if (!m_hStructuredBuffer.IsInvalidated())
    pDevice->DestroyBuffer(m_hStructuredBuffer);

  for (ezGALBufferResourceViewHandle hBufferView : m_MaterialBufferViews)
  {
    if (!hBufferView.IsInvalidated())
      pDevice->DestroyResourceView(hBufferView);
  }
  m_MaterialBufferViews.Clear();

  for (ezGALBufferHandle hBuffer : m_MaterialBuffers)
  {
    if (!hBuffer.IsInvalidated())
      pDevice->DestroyBuffer(hBuffer);
  }
  m_MaterialBuffers.Clear();
}

void ezMaterialManager::MaterialShaderConstants::OnResourceEvent(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUpdated)
  {
    OnShaderChanged(static_cast<ezShaderResource*>(e.m_pResource));
  }
}

void ezMaterialManager::MaterialShaderConstants::OnShaderChanged(ezShaderResource* pShader)
{
  ezSharedPtr<ezShaderConstantBufferLayout> pNewLayout = pShader->GetMaterialLayout();
  const bool bChanged = m_pLayout == nullptr || *m_pLayout != *pNewLayout;
  if (bChanged)
  {
    m_bShaderDirty = true;
  }
}

bool ezMaterialManager::MaterialShaderConstants::UpdateMaterialLayout()
{
  // Rebuild mapping
  // IMPORTANT: We must not hold the m_MaterialsMutex or any of the ezMaterialManager locks in this scope or the BlockTillLoaded can deadlock if all resource loading threads are occupied with materials which all wait for one of the material manager or the m_MaterialsMutex lock.
  ezResourceLock<ezShaderResource> pShader(m_hShader, ezResourceAcquireMode::BlockTillLoaded);
  ezSharedPtr<ezShaderConstantBufferLayout> pNewLayout = pShader->GetMaterialLayout();
  bool bLayoutChanged = m_pLayout == nullptr || pNewLayout == nullptr || *m_pLayout != *pNewLayout;
  if (bLayoutChanged)
  {
    m_pLayout = pNewLayout;
    m_ParameterNameToLayoutIndex.Clear();
    // If the layout has changed, we need to re-create all GPU resources.
    DestroyGpuResources();

    // It is valid for materials not not have constants. We still track them in case the shader gets reloaded and suddenly has constants.
    m_bShaderHasNoConstants = m_pLayout == nullptr;
    if (m_pLayout != nullptr)
    {
      m_bShaderHasNoConstants = false;
      // Build map
      for (ezUInt32 i = 0; i < m_pLayout->m_Constants.GetCount(); ++i)
      {
        m_ParameterNameToLayoutIndex.Insert(m_pLayout->m_Constants[i].m_sName, i);
      }
      // We still have stale GPU handles in the MaterialData, but these will be overwritten as we return bLayoutChanged == true here which will trigger a complete rebuild.
    }
    else
    {
      EZ_LOCK(m_MaterialsMutex);
      // If we reloaded the shader and it has now become one without constants, we need to make sure to clear any old buffer handles on the MaterialData.
      for (auto it = m_Materials.GetIterator(); it.IsValid(); ++it)
      {
        ezMaterialResource::ezMaterialId id = it.Id();
        ezMaterialResourceHandle hMaterial = it.Value();
        ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::PointerOnly);
        auto itMaterial = m_pParent->m_Materials.Find(pMaterial.GetPointer());
        if (itMaterial.IsValid())
        {
          itMaterial.Value().m_hConstantBuffer.Invalidate();
          itMaterial.Value().m_hStructuredBufferView.Invalidate();
        }
      }
    }
  }
  return bLayoutChanged;
}

void ezMaterialManager::MaterialShaderConstants::UpdateMaterial(ezMaterialResource::ezMaterialId id, ezMaterialResourceHandle hMaterial, ezGALCommandEncoder* pEncoder)
{
  ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::PointerOnly);
  auto itMaterial = m_pParent->m_Materials.Find(pMaterial.GetPointer());
  if (!itMaterial.IsValid() || !itMaterial.Value().m_hShader.IsValid())
  {
    // As materials can be added at any time, we might have entries that were registered between extraction and rendering and will this be first extracted in the next frame. Ignore these.
    return;
  }

  MaterialData& md = itMaterial.Value();
  ezArrayPtr<ezUInt8> data = m_MaterialsData.GetArrayPtr().GetSubArray(id.m_InstanceIndex * m_pLayout->m_uiTotalSize, m_pLayout->m_uiTotalSize);
  for (const auto& param : md.m_Parameters)
  {
    auto it = m_ParameterNameToLayoutIndex.Find(param.m_Name);
    if (it.IsValid())
    {
      const ezShaderConstant& constant = m_pLayout->m_Constants[it.Value()];
      if (constant.m_uiOffset + ezShaderConstant::s_TypeSize[constant.m_Type.GetValue()] <= data.GetCount())
      {
        ezUInt8* pDest = &data[constant.m_uiOffset];
        constant.CopyDataFromVariant(pDest, &param.m_Value);
      }
    }
  }

  switch (m_Mode)
  {
    case ezMaterialManager::MaterialStorageMode::MultipleConstantBuffer:
    {
      // Create and update constant buffer
      if (m_MaterialBuffers[id.m_InstanceIndex].IsInvalidated())
      {
        ezGALBufferCreationDescription bufferDesc;
        bufferDesc.m_uiTotalSize = m_pLayout->m_uiTotalSize;
        bufferDesc.m_BufferFlags = ezGALBufferUsageFlags::ConstantBuffer;
        bufferDesc.m_ResourceAccess.m_bImmutable = false;
        m_MaterialBuffers[id.m_InstanceIndex] = ezGALDevice::GetDefaultDevice()->CreateBuffer(bufferDesc);
      }

      // We lazily create the buffer so we need to always update the MaterialData as this could be a constant buffer from a previous material that resided in this slot.
      md.m_hConstantBuffer = m_MaterialBuffers[id.m_InstanceIndex];
      pEncoder->UpdateBuffer(m_MaterialBuffers[id.m_InstanceIndex], 0, data, ezGALUpdateMode::AheadOfTime);
    }
    break;
    case ezMaterialManager::MaterialStorageMode::MultipleStructuredBuffer:
    {
      // Create and update structured buffer
      if (m_MaterialBuffers[id.m_InstanceIndex].IsInvalidated())
      {
        ezGALBufferCreationDescription bufferDesc;
        bufferDesc.m_uiStructSize = m_pLayout->m_uiTotalSize;
        bufferDesc.m_uiTotalSize = m_pLayout->m_uiTotalSize;
        bufferDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
        bufferDesc.m_ResourceAccess.m_bImmutable = false;
        m_MaterialBuffers[id.m_InstanceIndex] = ezGALDevice::GetDefaultDevice()->CreateBuffer(bufferDesc);

        ezGALBufferResourceViewCreationDescription viewDesc;
        viewDesc.m_hBuffer = m_MaterialBuffers[id.m_InstanceIndex];
        viewDesc.m_uiFirstElement = 0;
        viewDesc.m_uiNumElements = 1;
        m_MaterialBufferViews[id.m_InstanceIndex] = ezGALDevice::GetDefaultDevice()->CreateResourceView(viewDesc);
      }

      // We lazily create the buffer so we need to always update the MaterialData as this could be a constant buffer from a previous material that resided in this slot.
      md.m_hStructuredBufferView = m_MaterialBufferViews[id.m_InstanceIndex];
      pEncoder->UpdateBuffer(m_MaterialBuffers[id.m_InstanceIndex], 0, data, ezGALUpdateMode::AheadOfTime);
    }
    break;
    case ezMaterialManager::MaterialStorageMode::SingleStructuredBuffer:
    {
      md.m_hStructuredBufferView = m_hStructuredBufferView;
      pEncoder->UpdateBuffer(m_hStructuredBuffer, m_pLayout->m_uiTotalSize * id.m_InstanceIndex, data, ezGALUpdateMode::AheadOfTime);
    }
    break;
    default:
      break;
  }
}

bool ezMaterialManager::MaterialShaderConstants::IsEmpty() const
{
  return m_Materials.IsEmpty();
}

ezMaterialManager::PendingChanges::PendingChanges()
  : m_RemovedMaterials(ezFrameAllocator::GetCurrentAllocator())
  , m_AddedOrModifiedMaterials(ezFrameAllocator::GetCurrentAllocator())
{
}

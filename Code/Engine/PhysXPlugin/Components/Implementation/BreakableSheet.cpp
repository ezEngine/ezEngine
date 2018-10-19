#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <PhysXPlugin/Components/BreakableSheet.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererFoundation/Device/Device.h>

#define JC_VORONOI_IMPLEMENTATION
#include <ThirdParty/jc_voronoi/jc_voronoi.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>

EZ_DEFINE_AS_POD_TYPE(jcv_point);

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgBreakableSheetBroke)
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgBreakableSheetBroke, 1, ezRTTIDefaultAllocator<ezMsgBreakableSheetBroke>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// TODOs:
// - Switch to box extents so box manipulator could be used OR
// - Switch to breakable asset and breakable component, in theory most of the code in this class (except fracture generation) would be the
// same for different fracture types
// - Once we have skinned meshes & instancing working together the batch id generation needs to be fixed
// - Better breaking behavior
// - Only spawn X actors per frame to reduce the spike of actor spawning

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezBreakableSheetComponent, 1, ezComponentMode::Dynamic)
{
  // TODO: Switch to extents so box manipulators work
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_ACCESSOR_PROPERTY("BrokenMaterial", GetBrokenMaterialFile, SetBrokenMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.001f, ezVariant()), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.001f, ezVariant()), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.001f, ezVariant()), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Density", GetDensity, SetDensity)->AddAttributes(new ezDefaultValueAttribute(1500.0f), new ezClampValueAttribute(1.0f, ezVariant()), new ezSuffixAttribute(" kg/m^3")),
    EZ_ACCESSOR_PROPERTY("NumPieces", GetNumPieces, SetNumPieces)->AddAttributes(new ezDefaultValueAttribute(32), new ezClampValueAttribute(5, 1024)),
    EZ_ACCESSOR_PROPERTY("DisappearTimeout", GetDisappearTimeout, SetDisappearTimeout)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezMinValueTextAttribute("Never"), new ezSuffixAttribute(" s")),
    EZ_ACCESSOR_PROPERTY("BreakImpulseStrength", GetBreakImpulseStrength, SetBreakImpulseStrength)->AddAttributes(new ezDefaultValueAttribute(25.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("FixedBorder", GetFixedBorder, SetFixedBorder),
    EZ_ACCESSOR_PROPERTY("FixedRandomSeed", GetFixedRandomSeed, SetFixedRandomSeed),
    EZ_MEMBER_PROPERTY("CollisionLayerUnbroken", m_uiCollisionLayerUnbroken)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("CollisionLayerBrokenPieces", m_uiCollisionLayerBrokenPieces)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("IncludeInNavmesh", m_bIncludeInNavmesh)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics"),
    new ezNonUniformBoxManipulatorAttribute("Width", "Thickness", "Height"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_BreakEventSender)
  }
  EZ_END_MESSAGESENDERS
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnExtractGeometry),
    EZ_MESSAGE_HANDLER(ezMsgCollision, OnCollision),
    EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBreakableSheetComponent::ezBreakableSheetComponent()
    : m_UnbrokenUserData(this)
{
  m_vExtents = ezVec3(m_fWidth, m_fThickness, m_fHeight);
}

void ezBreakableSheetComponent::Update()
{
  if (m_bBroken)
  {
    if (m_uiNumActiveBrokenPieceActors > 0)
    {
      m_bPiecesMovedThisFrame = true;
      m_uiNumActiveBrokenPieceActors = 0;

      UpdateBrokenPiecesBoundingSphere();
    }
    else
    {
      m_bPiecesMovedThisFrame = false;
    }

    // If this breakable sheet has a disappear timeout set we decrement the time since it was broken
    if (m_fDisappearTimeout.AsFloat() > 0.0f && m_fTimeUntilDisappear > 0.0f)
    {
      m_fTimeUntilDisappear -= GetWorld()->GetClock().GetTimeDiff().AsFloat();

      if (m_fTimeUntilDisappear <= 0.0f)
      {
        DestroyPiecesPhysicsObjects();

        // If this instance has a fixed border we need to set the scale of all pieces
        // to 0 so the border is still rendered, otherwise we can deactivate the component
        if (m_bFixedBorder)
        {
          ezMat4 scaleMatrix;
          scaleMatrix.SetScalingMatrix(ezVec3(0, 0, 0));
          for (ezUInt32 i = 1; i < m_PieceTransforms.GetCount(); ++i)
          {
            m_PieceTransforms[i] = m_PieceTransforms[i] * scaleMatrix;
          }

          m_bPiecesMovedThisFrame = true;
        }
        else
        {
          Deactivate();
        }
      }
    }
  }
}

void ezBreakableSheetComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  s << m_fWidth;
  s << m_fHeight;
  s << m_fThickness;
  s << m_fDensity;
  s << m_fBreakImpulseStrength;
  s << m_fDisappearTimeout;
  s << m_bFixedBorder;
  s << m_uiFixedRandomSeed;
  s << m_uiNumPieces;
  s << m_hMaterial;
  s << m_hBrokenMaterial;
  s << m_uiCollisionLayerUnbroken;
  s << m_uiCollisionLayerBrokenPieces;
  s << m_bIncludeInNavmesh;
}

void ezBreakableSheetComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();
  s >> m_fWidth;
  s >> m_fHeight;
  s >> m_fThickness;
  s >> m_fDensity;
  s >> m_fBreakImpulseStrength;
  s >> m_fDisappearTimeout;
  s >> m_bFixedBorder;
  s >> m_uiFixedRandomSeed;
  s >> m_uiNumPieces;
  s >> m_hMaterial;
  s >> m_hBrokenMaterial;
  s >> m_uiCollisionLayerUnbroken;
  s >> m_uiCollisionLayerBrokenPieces;
  s >> m_bIncludeInNavmesh;
}

ezResult ezBreakableSheetComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_bBroken)
  {
    bounds = m_BrokenPiecesBoundingSphere;
    return EZ_SUCCESS;
  }
  else
  {
    if (m_hUnbrokenMesh.IsValid())
    {
      ezResourceLock<ezMeshResource> pMesh(m_hUnbrokenMesh);
      bounds = pMesh->GetBounds();
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

void ezBreakableSheetComponent::OnExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (!m_bIncludeInNavmesh)
    return;

  const ezVec3 vScale = ezSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().m_Scale.Abs());

  auto& box = msg.m_pWorldGeometry->m_BoxShapes.ExpandAndGetRef();
  box.m_vPosition = GetOwner()->GetGlobalPosition();
  box.m_qRotation = GetOwner()->GetGlobalRotation();

  ezVec3 vExtents(m_fWidth, m_fThickness, m_fHeight);

  box.m_vHalfExtents = vExtents.CompMul(vScale) * 0.5f;
}

void ezBreakableSheetComponent::Initialize()
{
  SUPER::Initialize();

  // If no specific random seed is specified in the component
  // we generate one here so the rest of the function is deterministic
  // (and can be restored for save games etc.)
  if (m_uiFixedRandomSeed == 0)
  {
    ezRandom r;
    r.InitializeFromCurrentTime();

    m_uiRandomSeedUsed = r.UInt();
  }
  else
  {
    m_uiRandomSeedUsed = m_uiFixedRandomSeed;
  }

  CreateMeshes();
}

void ezBreakableSheetComponent::OnSimulationStarted()
{
  CreateUnbrokenPhysicsObject();
}

void ezBreakableSheetComponent::Deinitialize()
{
  SUPER::Deinitialize();

  Cleanup();
}

void ezBreakableSheetComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hUnbrokenMesh.IsValid())
    return;

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const ezUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  ezMaterialResourceHandle hMaterial;
  ezMeshResourceHandle hMesh;
  ezMeshRenderData* pRenderData = nullptr;
  ezUInt32 uiSortingKey = 0;

  if (m_bBroken)
  {
    hMaterial = m_hBrokenMaterial;
    hMesh = m_hPiecesMesh;
  }
  else
  {
    hMaterial = m_hMaterial;
    hMesh = m_hUnbrokenMesh;
  }

  const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;
  const ezUInt32 uiMeshIDHash = hMesh.GetResourceIDHash();

  // Generate batch id from mesh, material and part index.
  const ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, GetUniqueIdForRendering(), uiFlipWinding};
  const ezUInt32 uiBatchId = ezHashing::xxHash32(data, sizeof(data));

  pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = hMesh;
    pRenderData->m_hMaterial = hMaterial;
    pRenderData->m_Color = ezColor::White;

    if (m_bBroken)
    {
      pRenderData->m_hSkinningMatrices = m_hPieceTransformsBuffer;

      // We only supply this pointer if any transform changed
      if (m_bPiecesMovedThisFrame)
      {
        auto pMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezMat4, m_PieceTransforms.GetCount());
        ezMemoryUtils::Copy(pMatrices.GetPtr(), m_PieceTransforms.GetData(), m_PieceTransforms.GetCount());

        pRenderData->m_pNewSkinningMatricesData =
            ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(pMatrices.GetPtr()), m_PieceTransforms.GetCount() * sizeof(ezMat4));
      }
    }

    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiFlipWinding = uiFlipWinding;
    pRenderData->m_uiUniformScale = uiUniformScale;

    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
  }

  uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | uiFlipWinding;

  ezRenderData::Category category = ezDefaultRenderDataCategories::LitOpaque;
  if (hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::AllowFallback);
    ezTempHashedString blendModeValue = pMaterial->GetPermutationValue("BLEND_MODE");
    if (blendModeValue == "BLEND_MODE_OPAQUE" || blendModeValue == "")
    {
      category = ezDefaultRenderDataCategories::LitOpaque;
    }
    else if (blendModeValue == "BLEND_MODE_MASKED")
    {
      category = ezDefaultRenderDataCategories::LitMasked;
    }
    else
    {
      category = ezDefaultRenderDataCategories::LitTransparent;
    }
  }

  msg.AddRenderData(pRenderData, category, uiSortingKey, ezRenderData::Caching::Never);
}

void ezBreakableSheetComponent::OnCollision(ezMsgCollision& msg)
{
  if (m_bBroken)
    return;

  if (msg.m_vImpulse.IsZero())
    return;

  const float fImpulse = msg.m_vImpulse.GetLength();

  if (fImpulse > m_fBreakImpulseStrength)
  {
    Break();
  }
}

void ezBreakableSheetComponent::AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg)
{
  if (msg.m_uiShapeId == ezInvalidIndex)
    return;

  if (!m_bBroken)
  {
    if (msg.m_vImpulse.GetLength() > m_fBreakImpulseStrength)
    {
      ezMsgCollision fakeMsg;
      fakeMsg.m_vPosition = msg.m_vGlobalPosition;
      fakeMsg.m_vImpulse = msg.m_vImpulse;

      Break(&fakeMsg);
    }
  }
  else
  {
    physx::PxRigidDynamic* pActor = m_ShapeIDsToActors.GetValueOrDefault(msg.m_uiShapeId, nullptr);

    if (pActor != nullptr)
    {
      EZ_PX_WRITE_LOCK(*pActor->getScene());

      PxRigidBodyExt::addForceAtPos(*pActor, ezPxConversionUtils::ToVec3(msg.m_vImpulse),
                                    ezPxConversionUtils::ToVec3(msg.m_vGlobalPosition), PxForceMode::eIMPULSE);
    }
  }
}

void ezBreakableSheetComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;
  m_vExtents = ezVec3(m_fWidth, m_fThickness, m_fHeight);

  ReinitMeshes();
}

float ezBreakableSheetComponent::GetWidth() const
{
  return m_fWidth;
}

void ezBreakableSheetComponent::SetHeight(float fHeight)
{
  if (fHeight <= 0.0f)
    return;

  m_fHeight = fHeight;
  m_vExtents = ezVec3(m_fWidth, m_fThickness, m_fHeight);

  ReinitMeshes();
}

float ezBreakableSheetComponent::GetHeight() const
{
  return m_fHeight;
}

void ezBreakableSheetComponent::SetThickness(float fThickness)
{
  if (fThickness <= 0.0f)
    return;

  m_fThickness = fThickness;
  m_vExtents = ezVec3(m_fWidth, m_fThickness, m_fHeight);

  ReinitMeshes();
}

float ezBreakableSheetComponent::GetThickness() const
{
  return m_fThickness;
}

void ezBreakableSheetComponent::SetDensity(float fDensity)
{
  if (fDensity <= 0.0f)
    return;

  m_fDensity = fDensity;
}

float ezBreakableSheetComponent::GetDensity() const
{
  return m_fDensity;
}

void ezBreakableSheetComponent::SetBreakImpulseStrength(float fBreakImpulseStrength)
{
  if (fBreakImpulseStrength < 0.0f)
    return;

  m_fBreakImpulseStrength = fBreakImpulseStrength;
}

float ezBreakableSheetComponent::GetBreakImpulseStrength() const
{
  return m_fBreakImpulseStrength;
}

void ezBreakableSheetComponent::SetDisappearTimeout(ezTime fDisappearTimeout)
{
  if (fDisappearTimeout.AsFloat() < 0.0f)
    return;

  m_fDisappearTimeout = fDisappearTimeout;
}

ezTime ezBreakableSheetComponent::GetDisappearTimeout() const
{
  return m_fDisappearTimeout;
}

void ezBreakableSheetComponent::SetFixedBorder(bool bFixedBorder)
{
  if (bFixedBorder == m_bFixedBorder)
    return;

  m_bFixedBorder = bFixedBorder;

  ReinitMeshes();
}

bool ezBreakableSheetComponent::GetFixedBorder() const
{
  return m_bFixedBorder;
}

void ezBreakableSheetComponent::SetFixedRandomSeed(ezUInt32 uiFixedRandomSeed)
{
  if (m_uiFixedRandomSeed == uiFixedRandomSeed)
    return;

  m_uiFixedRandomSeed = uiFixedRandomSeed;

  ReinitMeshes();
}

ezUInt32 ezBreakableSheetComponent::GetFixedRandomSeed() const
{
  return m_uiFixedRandomSeed;
}

void ezBreakableSheetComponent::SetNumPieces(ezUInt32 uiNumPieces)
{
  if (m_uiNumPieces < 3 || uiNumPieces == m_uiNumPieces)
    return;

  m_uiNumPieces = uiNumPieces;

  ReinitMeshes();
}

ezUInt32 ezBreakableSheetComponent::GetNumPieces() const
{
  return m_uiNumPieces;
}

void ezBreakableSheetComponent::SetMaterialFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* ezBreakableSheetComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

ezMaterialResourceHandle ezBreakableSheetComponent::GetMaterial() const
{
  return m_hMaterial;
}

void ezBreakableSheetComponent::SetBrokenMaterialFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hBrokenMaterial = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }
  else
  {
    m_hBrokenMaterial.Invalidate();
  }
}

const char* ezBreakableSheetComponent::GetBrokenMaterialFile() const
{
  if (!m_hBrokenMaterial.IsValid())
    return "";

  return m_hBrokenMaterial.GetResourceID();
}

ezMaterialResourceHandle ezBreakableSheetComponent::GetBrokenMaterial() const
{
  return m_hBrokenMaterial;
}

void ezBreakableSheetComponent::Break(const ezMsgCollision* pMessage /*= nullptr*/)
{
  if (m_bBroken)
    return;

  m_bBroken = true;

  m_fTimeUntilDisappear = m_fDisappearTimeout.AsFloat();

  DestroyUnbrokenPhysicsObject();
  CreatePiecesPhysicsObjects(pMessage ? pMessage->m_vImpulse : ezVec3::ZeroVector(),
                             pMessage ? pMessage->m_vPosition : ezVec3::ZeroVector());

  ezMsgBreakableSheetBroke msg;

  // Set the handle of the instigator object (e.g. which object broke this component)
  if (pMessage)
  {
    if (pMessage->m_hObjectA == GetOwner()->GetHandle())
    {
      msg.m_hInstigatorObject = pMessage->m_hObjectB;
    }
    else
    {
      msg.m_hInstigatorObject = pMessage->m_hObjectA;
    }
  }

  m_BreakEventSender.PostMessage(msg, this, GetOwner(), ezObjectMsgQueueType::PostAsync);
}

void ezBreakableSheetComponent::CreateMeshes()
{
  // Deal with the unbroken mesh first
  {
    ezStringBuilder unbrokenMeshName;
    unbrokenMeshName.Format("ezBreakableSheetComponent_unbroken_{0}_{1}_{2}_{3}.createdAtRuntime.ezMesh", m_fWidth, m_fThickness, m_fHeight,
                            m_bFixedBorder);

    m_hUnbrokenMesh = ezResourceManager::GetExistingResource<ezMeshResource>(unbrokenMeshName);

    // Build unbroken mesh if no existing one is found
    if (!m_hUnbrokenMesh.IsValid())
    {
      ezGeometry g;

      g.AddTexturedBox(m_vExtents, ezColor::White);
      g.ComputeFaceNormals();
      g.ComputeTangents();

      ezMeshResourceDescriptor desc;
      BuildMeshResourceFromGeometry(g, desc, false /* no skinning data */);

      m_hUnbrokenMesh = ezResourceManager::CreateResource<ezMeshResource>(unbrokenMeshName, desc);
    }
  }

  // Build broken mesh
  {
    ezStringBuilder piecesMeshName;
    piecesMeshName.Format("ezBreakableSheetComponent_pieces_{0}_{1}_{2}_{3}_{4}_{5}.createdAtRuntime.ezMesh", m_fWidth, m_fThickness,
                          m_fHeight, m_uiNumPieces, m_uiRandomSeedUsed, m_bFixedBorder);

    m_hPiecesMesh = ezResourceManager::GetExistingResource<ezMeshResource>(piecesMeshName);

    const bool bNeedsMeshBuilding = !m_hPiecesMesh.IsValid();

    // Generate pieces mesh if no existing one is found
    // TODO: If breakable information is stored all of this could be skipped,
    // for now we only skip the mesh building if a cached mesh exists
    {
      ezHybridArray<jcv_point, 64> diagramPoints;

      ezVec3 halfSize = m_vExtents * 0.5f;

      // Limit point generation to the inner 97% of the sheet
      ezVec2 pointBounds = ezVec2(halfSize.x, halfSize.z) * 0.97f;

      ezRandom r;
      r.Initialize(m_uiRandomSeedUsed);

      for (ezUInt32 i = 0; i < m_uiNumPieces; ++i)
      {
        const float x = static_cast<float>(r.DoubleMinMax(-pointBounds.x, pointBounds.x));
        const float y = static_cast<float>(r.DoubleMinMax(-pointBounds.y, pointBounds.y));
        diagramPoints.PushBack({x, y});
      }

      jcv_rect boundingBox;
      boundingBox.min = {-halfSize.x, -halfSize.z};
      boundingBox.max = {halfSize.x, halfSize.z};

      jcv_diagram diagram;
      ezMemoryUtils::ZeroFill(&diagram);
      jcv_diagram_generate(diagramPoints.GetCount(), diagramPoints.GetData(), &boundingBox, &diagram);

      if (diagram.numsites == 0)
      {
        ezLog::Warning("Voronoi diagram generation failed for ezBreakableSheetComponent.");
      }
      else
      {
        ezGeometry g;
        const float fHalfThickness = m_fThickness * 0.5f;
        const float fInvWidth = 1.0f / m_fWidth;
        const float fInvHeight = 1.0f / m_fHeight;

        const jcv_site* sites = jcv_diagram_get_sites(&diagram);

        // Determine for each site if it is a border piece
        ezHybridArray<bool, 256> IsBorderPiece;
        ezUInt32 uiNumBorderPieces = 0;

        if (m_bFixedBorder)
        {
          IsBorderPiece.Reserve(static_cast<ezUInt32>(diagram.numsites));

          for (int i = 0; i < diagram.numsites; ++i)
          {
            const jcv_site* site = &sites[i];
            const jcv_graphedge* e = site->edges;

            bool bIsBorderPiece = false;

            while (e)
            {
              // If the edge is on the same line either horizontally or vertically
              // and it shares the coordinate with the bounding box it's a border piece
              if (ezMath::IsEqual(e->pos[0].x, e->pos[1].x, 0.0001f))
              {
                if (ezMath::IsEqual(e->pos[0].x, boundingBox.min.x, 0.0001f) || ezMath::IsEqual(e->pos[0].x, boundingBox.max.x, 0.0001f))
                {
                  bIsBorderPiece = true;
                  uiNumBorderPieces++;
                  break;
                }
              }
              else if (ezMath::IsEqual(e->pos[0].y, e->pos[1].y, 0.0001f))
              {
                if (ezMath::IsEqual(e->pos[0].y, boundingBox.min.y, 0.0001f) || ezMath::IsEqual(e->pos[0].y, boundingBox.max.y, 0.0001f))
                {
                  bIsBorderPiece = true;
                  uiNumBorderPieces++;
                  break;
                }
              }

              e = e->next;
            }

            IsBorderPiece.PushBack(bIsBorderPiece);
          }
        }

        // Reserve the memory for the piece transforms, if the sheet has a fixed border we reserve the first matrix
        // which will always stay at identity
        m_PieceTransforms.Clear();
        m_PieceTransforms.Reserve(static_cast<ezUInt32>(diagram.numsites) - uiNumBorderPieces + 1);
        if (m_bFixedBorder)
        {
          m_PieceTransforms.PushBack(ezMat4::IdentityMatrix());
        }

        // Build geometry from cells
        ezInt32 iNonBorderPieces = m_bFixedBorder ? 0 : -1;
        for (int i = 0; i < diagram.numsites; ++i)
        {
          ezInt32 iPieceMatrixIndex = -1;

          float fOffset = m_bFixedBorder ? (IsBorderPiece[i] ? 0 : (float)r.DoubleMinMax(0, 0.1)) : 0;

          if (m_bFixedBorder && IsBorderPiece[i])
          {
            iPieceMatrixIndex = 0;
          }
          else
          {
            iNonBorderPieces++;
            iPieceMatrixIndex = iNonBorderPieces;
            m_PieceTransforms.PushBack(ezMat4::IdentityMatrix());
          }

          const jcv_site* site = &sites[i];
          const jcv_graphedge* e = site->edges;
          while (e)
          {

            // Front side
            ezUInt32 frontIdx[3];
            frontIdx[0] = g.AddVertex(ezVec3(site->p.x, fHalfThickness, site->p.y), ezVec3(0, 1, 0),
                                      ezVec2((site->p.x + halfSize.x) * fInvWidth, (site->p.y + halfSize.z) * fInvHeight), ezColor::White,
                                      iPieceMatrixIndex);
            frontIdx[1] = g.AddVertex(ezVec3(e->pos[0].x, fHalfThickness, e->pos[0].y), ezVec3(0, 1, 0),
                                      ezVec2((e->pos[0].x + halfSize.x) * fInvWidth, (e->pos[0].y + halfSize.z) * fInvHeight),
                                      ezColor::White, iPieceMatrixIndex);
            frontIdx[2] = g.AddVertex(ezVec3(e->pos[1].x, fHalfThickness, e->pos[1].y), ezVec3(0, 1, 0),
                                      ezVec2((e->pos[1].x + halfSize.x) * fInvWidth, (e->pos[1].y + halfSize.z) * fInvHeight),
                                      ezColor::White, iPieceMatrixIndex);
            g.AddPolygon(frontIdx, true);

            // Back side
            ezUInt32 backIdx[3];
            backIdx[0] = g.AddVertex(ezVec3(site->p.x, -fHalfThickness, site->p.y), ezVec3(0, -1, 0),
                                     ezVec2(1.0f - ((site->p.x + halfSize.x) * fInvWidth), 1.0f - ((site->p.y + halfSize.z) * fInvHeight)),
                                     ezColor::White, iPieceMatrixIndex);
            backIdx[1] =
                g.AddVertex(ezVec3(e->pos[0].x, -fHalfThickness, e->pos[0].y), ezVec3(0, -1, 0),
                            ezVec2(1.0f - ((e->pos[0].x + halfSize.x) * fInvWidth), 1.0f - ((e->pos[0].y + halfSize.z) * fInvHeight)),
                            ezColor::White, iPieceMatrixIndex);
            backIdx[2] =
                g.AddVertex(ezVec3(e->pos[1].x, -fHalfThickness, e->pos[1].y), ezVec3(0, -1, 0),
                            ezVec2(1.0f - ((e->pos[1].x + halfSize.x) * fInvWidth), 1.0f - ((e->pos[1].y + halfSize.z) * fInvHeight)),
                            ezColor::White, iPieceMatrixIndex);
            g.AddPolygon(backIdx, false);

            // Add skirt connecting front and back side
            AddSkirtPolygons(ezVec2(e->pos[0].x, e->pos[0].y), ezVec2(e->pos[1].x, e->pos[1].y), fHalfThickness, iPieceMatrixIndex, g);

            e = e->next;
          }
        }

        if (bNeedsMeshBuilding)
        {
          g.ComputeTangents();

          ezMeshResourceDescriptor desc;
          BuildMeshResourceFromGeometry(g, desc, true /* include skinning data */);

          m_hPiecesMesh = ezResourceManager::CreateResource<ezMeshResource>(piecesMeshName, desc);
        }

        // Build piece bounding boxes
        m_PieceBoundingBoxes.SetCount(m_PieceTransforms.GetCount());

        for (auto& Box : m_PieceBoundingBoxes)
        {
          Box.SetInvalid();
        }

        for (const auto& vertex : g.GetVertices())
        {
          if (m_PieceBoundingBoxes[vertex.m_iCustomIndex].IsValid())
          {
            m_PieceBoundingBoxes[vertex.m_iCustomIndex].ExpandToInclude(vertex.m_vPosition);
          }
          else
          {
            m_PieceBoundingBoxes[vertex.m_iCustomIndex].SetFromPoints(&vertex.m_vPosition, 1);
          }
        }

        for (auto& bbox : m_PieceBoundingBoxes)
        {
          bbox.ScaleFromCenter(ezVec3(0.5f, 1.0f, 0.5f));
        }

        UpdateBrokenPiecesBoundingSphere();
      }
      jcv_diagram_free(&diagram);
    }
  }

  // Create the buffer for the skinning matrices
  ezGALBufferCreationDescription BufferDesc;
  BufferDesc.m_uiStructSize = sizeof(ezMat4);
  BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_PieceTransforms.GetCount();
  BufferDesc.m_bUseAsStructuredBuffer = true;
  BufferDesc.m_bAllowShaderResourceView = true;
  BufferDesc.m_ResourceAccess.m_bImmutable = false;

  m_hPieceTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(
      BufferDesc, ezArrayPtr<ezUInt8>(reinterpret_cast<ezUInt8*>(m_PieceTransforms.GetData()), BufferDesc.m_uiTotalSize));
  if (m_hPieceTransformsBuffer.IsInvalidated())
  {
    ezLog::Warning("Couldn't allocate buffer for piece transforms of breakable sheet.");
  }
}

void ezBreakableSheetComponent::AddSkirtPolygons(ezVec2 Point0, ezVec2 Point1, float fHalfThickness, ezInt32 iPieceMatrixIndex,
                                                 ezGeometry& Geometry) const
{
  const float fSpanX = ezMath::Abs(Point0.x - Point1.x);
  const float fSpanY = ezMath::Abs(Point0.y - Point1.y);
  const float fSpan = ezMath::Max(fSpanX, fSpanY);

  ezVec3 Point0Front(Point0.x, fHalfThickness, Point0.y);
  ezVec2 Point0FrontUV(m_fThickness, 0);
  ezVec3 Point0Back(Point0.x, -fHalfThickness, Point0.y);
  ezVec2 Point0BackUV(0, 0);
  ezVec3 Point1Front(Point1.x, fHalfThickness, Point1.y);
  ezVec2 Point1FrontUV(m_fThickness, fSpan);
  ezVec3 Point1Back(Point1.x, -fHalfThickness, Point1.y);
  ezVec2 Point1BackUV(0, fSpan);

  ezVec3 FaceNormal;
  FaceNormal.CalculateNormal(Point0Front, Point1Front, Point0Back);

  const ezUInt32 uiIdx0 = Geometry.AddVertex(Point0Front, FaceNormal, Point0FrontUV, ezColor::White, iPieceMatrixIndex);
  const ezUInt32 uiIdx1 = Geometry.AddVertex(Point0Back, FaceNormal, Point0BackUV, ezColor::White, iPieceMatrixIndex);
  const ezUInt32 uiIdx2 = Geometry.AddVertex(Point1Front, FaceNormal, Point1FrontUV, ezColor::White, iPieceMatrixIndex);
  const ezUInt32 uiIdx3 = Geometry.AddVertex(Point1Back, FaceNormal, Point1BackUV, ezColor::White, iPieceMatrixIndex);

  {
    ezUInt32 idx[3] = {uiIdx0, uiIdx2, uiIdx1};
    Geometry.AddPolygon(idx, false);
  }

  {
    ezUInt32 idx[3] = {uiIdx1, uiIdx2, uiIdx3};
    Geometry.AddPolygon(idx, false);
  }
}

void ezBreakableSheetComponent::BuildMeshResourceFromGeometry(ezGeometry& Geometry, ezMeshResourceDescriptor& MeshDesc,
                                                              bool bWithSkinningData) const
{
  Geometry.ComputeFaceNormals();
  Geometry.ComputeTangents();

  auto& MeshBufferDesc = MeshDesc.MeshBufferDesc();

  MeshBufferDesc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  MeshBufferDesc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
  MeshBufferDesc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
  MeshBufferDesc.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);

  if (bWithSkinningData)
  {
    MeshBufferDesc.AddStream(ezGALVertexAttributeSemantic::BoneWeights0, ezGALResourceFormat::XYZWFloat);
    MeshBufferDesc.AddStream(ezGALVertexAttributeSemantic::BoneIndices0, ezGALResourceFormat::RGBAUShort);
  }
  MeshBufferDesc.AllocateStreamsFromGeometry(Geometry, ezGALPrimitiveTopology::Triangles);

  // Add bone indices and weights information
  if (bWithSkinningData)
  {
    const auto& VertexDecl = MeshBufferDesc.GetVertexDeclaration();
    const ezUInt32 VertexCount = Geometry.GetVertices().GetCount();
    for (ezUInt32 s = 0; s < VertexDecl.m_VertexStreams.GetCount(); ++s)
    {
      const ezVertexStreamInfo& si = VertexDecl.m_VertexStreams[s];
      switch (si.m_Semantic)
      {
        case ezGALVertexAttributeSemantic::BoneIndices0:
        {
          for (ezUInt32 v = 0; v < VertexCount; ++v)
          {
            ezUInt16 boneIndices[4] = {static_cast<ezUInt16>(Geometry.GetVertices()[v].m_iCustomIndex), 0, 0, 0};
            ezUInt64 uiPackedBoneIndices = *reinterpret_cast<ezUInt64*>(&boneIndices[0]);

            MeshBufferDesc.SetVertexData<ezUInt64>(s, v, uiPackedBoneIndices);
          }
        }
        break;
        case ezGALVertexAttributeSemantic::BoneWeights0:
        {
          ezVec4 Weight(1, 0, 0, 0);
          for (ezUInt32 v = 0; v < VertexCount; ++v)
          {
            MeshBufferDesc.SetVertexData<ezVec4>(s, v, Weight);
          }
        }
        break;
      }
    }
  }

  MeshDesc.AddSubMesh(MeshBufferDesc.GetPrimitiveCount(), 0, 0);

  MeshDesc.ComputeBounds();
}


void ezBreakableSheetComponent::UpdateBrokenPiecesBoundingSphere()
{
  m_BrokenPiecesBoundingSphere.SetZero();

  for (ezUInt32 i = 0; i < m_PieceBoundingBoxes.GetCount(); ++i)
  {
    ezBoundingSphere TransformedSphere = m_PieceBoundingBoxes[i].GetBoundingSphere();
    TransformedSphere.Translate(m_PieceTransforms[i].GetTranslationVector());
    m_BrokenPiecesBoundingSphere.ExpandToInclude(TransformedSphere);
  }

  GetOwner()->UpdateLocalBounds();
}

static PxMaterial* GetPxMaterial(const ezMaterialResourceHandle& hMaterial)
{
  PxMaterial* pPxMaterial = ezPhysX::GetSingleton()->GetDefaultMaterial();

  if (hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::NoFallback);

    ezHashedString sSurface = pMaterial->GetSurface();
    if (!sSurface.IsEmpty())
    {
      ezSurfaceResourceHandle hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sSurface.GetString());

      if (hSurface.IsValid())
      {
        ezResourceLock<ezSurfaceResource> pSurface(hSurface, ezResourceAcquireMode::NoFallback);
        if (pSurface->m_pPhysicsMaterial != nullptr)
        {
          pPxMaterial = static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterial);
        }
      }
    }
  }

  return pPxMaterial;
}

void ezBreakableSheetComponent::CreateUnbrokenPhysicsObject()
{
  EZ_ASSERT_DEV(m_pUnbrokenActor == nullptr, "Trying to create unbroken physics object while already there. Probable logic bug.");

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const ezSimdTransform& globalTransform = GetOwner()->GetGlobalTransformSimd();

  PxTransform t = ezPxConversionUtils::ToTransform(globalTransform);
  m_pUnbrokenActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidStatic(t);
  EZ_ASSERT_DEBUG(m_pUnbrokenActor != nullptr, "PhysX actor creation failed");

  m_pUnbrokenActor->userData = &m_UnbrokenUserData;

  // PhysX does not get any scale value, so to correctly position child objects
  // we have to pretend that this parent object applies no scale on its children
  ezSimdTransform globalTransformNoScale = globalTransform;
  globalTransformNoScale.m_Scale.Set(1.0f);

  PxShape* pShape = nullptr;
  ezVec3 vScale = ezSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().m_Scale.Abs());

  PxBoxGeometry box;
  box.halfExtents = ezPxConversionUtils::ToVec3(m_vExtents.CompMul(vScale) * 0.5f);

  pShape = m_pUnbrokenActor->createShape(box, *GetPxMaterial(m_hMaterial));

  if (pShape != nullptr)
  {
    m_uiUnbrokenShapeId = pModule->CreateShapeId();
    PxFilterData filterData =
        ezPhysX::CreateFilterData(m_uiCollisionLayerUnbroken, m_uiUnbrokenShapeId, true /* Contact reporting enabled */);

    pShape->setSimulationFilterData(filterData);
    pShape->setQueryFilterData(filterData);

    pShape->userData = &m_UnbrokenUserData;

    {
      EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
      pModule->GetPxScene()->addActor(*m_pUnbrokenActor);
    }
  }
  else
  {
    ezLog::Error("Creation of box shape for unbroken sheet failed.");
  }
}

void ezBreakableSheetComponent::DestroyUnbrokenPhysicsObject()
{
  if (m_pUnbrokenActor != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pUnbrokenActor->getScene()));

    m_pUnbrokenActor->release();
    m_pUnbrokenActor = nullptr;
  }

  if (m_uiUnbrokenShapeId != ezInvalidIndex)
  {
    if (ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>())
    {
      pModule->DeleteShapeId(m_uiUnbrokenShapeId);
      m_uiUnbrokenShapeId = ezInvalidIndex;
    }
  }
}

void ezBreakableSheetComponent::CreatePiecesPhysicsObjects(ezVec3 vImpulse, ezVec3 vPointOfBreakage)
{
  EZ_ASSERT_DEV(m_PieceActors.IsEmpty(), "Trying to create piece physics objects while already there. Probable logic bug.");

  m_PieceActors.SetCount(m_PieceTransforms.GetCount());
  m_PieceShapeIds.SetCount(m_PieceTransforms.GetCount());
  m_PieceUserDatas.SetCount(m_PieceTransforms.GetCount());


  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  const ezSimdTransform& globalTransform = GetOwner()->GetGlobalTransformSimd();

  physx::PxMaterial* pPhysXMat = GetPxMaterial(m_hBrokenMaterial);

  ezVec3 vScale = ezSimdConversion::ToVec3(globalTransform.m_Scale.Abs());

  ezRandom r;
  r.Initialize(0);

  for (ezUInt32 i = 0; i < m_PieceActors.GetCount(); ++i)
  {
    // We don't create a dynamic actor for the border
    // TODO: Think about adding a nice mesh based static actor
    if (i == 0 && m_bFixedBorder)
    {
      m_PieceActors[i] = nullptr;
      continue;
    }

    ezSimdTransform bboxCenterTransform;
    bboxCenterTransform.m_Rotation.SetIdentity();
    bboxCenterTransform.m_Scale.Set(1, 1, 1, 1);
    bboxCenterTransform.m_Position = ezSimdConversion::ToVec4(m_PieceBoundingBoxes[i].GetCenter().GetAsPositionVec4());

    ezSimdTransform finalTransform = globalTransform;
    finalTransform *= bboxCenterTransform;

    PxTransform t = ezPxConversionUtils::ToTransform(finalTransform);
    physx::PxRigidDynamic* pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidDynamic(t);

    EZ_ASSERT_DEBUG(pActor != nullptr, "PhysX actor creation failed");
    if (!pActor)
      continue;

    // Create user data pointing to this component and specifying the piece index
    m_PieceUserDatas[i] = ezPxUserData(this);
    m_PieceUserDatas[i].SetAdditionalUserData(reinterpret_cast<void*>(static_cast<size_t>(i)));
    pActor->userData = &m_PieceUserDatas[i];

    PxBoxGeometry box;
    box.halfExtents = ezPxConversionUtils::ToVec3(m_PieceBoundingBoxes[i].GetHalfExtents().CompMul(vScale));
    physx::PxShape* pShape = pActor->createShape(box, *pPhysXMat);

    m_PieceShapeIds[i] = pModule->CreateShapeId();
    m_ShapeIDsToActors.Insert(m_PieceShapeIds[i], pActor);
    PxFilterData filterData = ezPhysX::CreateFilterData(m_uiCollisionLayerBrokenPieces, m_PieceShapeIds[i]);

    pShape->setSimulationFilterData(filterData);
    pShape->setQueryFilterData(filterData);
    pShape->userData = &m_PieceUserDatas[i];

    // TODO: Expose
    pActor->setLinearDamping(0.1f);
    pActor->setAngularDamping(0.05f);
    pActor->setMaxDepenetrationVelocity(pModule->GetMaxDepenetrationVelocity());

    PxRigidBodyExt::updateMassAndInertia(*pActor, m_fDensity, nullptr /* Center of mass = center of box */);

    pActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, false);
    pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);

    m_PieceActors[i] = pActor;
    const float fExtentsLength = m_vExtents.GetLength();
    const float fImpulseStrength = vImpulse.GetLength();
    {
      EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
      pModule->GetPxScene()->addActor(*pActor);

      // Add initial impulse
      {
        const ezVec3 BoundingBoxCenter = m_PieceBoundingBoxes[i].GetCenter();
        ezVec3 vec(BoundingBoxCenter - vPointOfBreakage);
        float fStrength = static_cast<float>(r.DoubleMinMax(0.5, 1.0));

        ezVec3 modifiedImpulse = vImpulse;
        modifiedImpulse.SetLength(fImpulseStrength * fStrength);

        PxRigidBodyExt::addForceAtPos(*pActor, ezPxConversionUtils::ToVec3(modifiedImpulse), ezPxConversionUtils::ToVec3(BoundingBoxCenter),
                                      PxForceMode::eIMPULSE);
      }
    }
  }
}

void ezBreakableSheetComponent::DestroyPiecesPhysicsObjects()
{
  for (auto pActor : m_PieceActors)
  {
    if (pActor)
    {
      EZ_PX_WRITE_LOCK(*(pActor->getScene()));
      pActor->release();
    }
  }

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  for (ezUInt32 uiShapeId : m_PieceShapeIds)
  {
    pModule->DeleteShapeId(uiShapeId);
  }

  m_PieceActors.Clear();
  m_PieceShapeIds.Clear();
  m_ShapeIDsToActors.Clear();
  m_PieceUserDatas.Clear();
  m_uiNumActiveBrokenPieceActors = 0;
}

void ezBreakableSheetComponent::ReinitMeshes()
{
  // Is this safe?
  if (!m_PieceTransforms.IsEmpty())
  {
    Cleanup();
    CreateMeshes();
  }
}

void ezBreakableSheetComponent::Cleanup()
{
  DestroyUnbrokenPhysicsObject();
  DestroyPiecesPhysicsObjects();

  if (!m_hPieceTransformsBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hPieceTransformsBuffer);
    m_hPieceTransformsBuffer.Invalidate();
  }
}

void ezBreakableSheetComponent::SetPieceTransform(const physx::PxTransform& transform, void* pAdditionalUserData)
{
  m_uiNumActiveBrokenPieceActors++;

  ezSimdTransform t = ezPxConversionUtils::ToSimdTransform(transform);
  t.m_Scale = ezSimdConversion::ToVec3(GetOwner()->GetGlobalScaling());

  const ezUInt32 uiPieceIndex = static_cast<ezUInt32>(reinterpret_cast<size_t>(pAdditionalUserData));

  // The bounding box transforms include the offset from the center of the sheet to the piece center,
  // however for the skinning process this offset needs to be removed:
  ezSimdTransform pieceToBBoxCenter;
  pieceToBBoxCenter.SetIdentity();
  pieceToBBoxCenter.m_Position = ezSimdConversion::ToVec4(m_PieceBoundingBoxes[uiPieceIndex].GetCenter().GetAsPositionVec4());
  pieceToBBoxCenter.Invert();
  t *= pieceToBBoxCenter;

  ezSimdTransform globalTransform = GetOwner()->GetGlobalTransformSimd();

  ezSimdTransform localTransform;
  localTransform.SetLocalTransform(globalTransform, t);

  m_PieceTransforms[uiPieceIndex] = ezSimdConversion::ToMat4(localTransform.GetAsMat4());
}

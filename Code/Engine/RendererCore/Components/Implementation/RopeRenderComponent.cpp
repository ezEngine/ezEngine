#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Components/RopeRenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Types.h>

ezCVarBool cvar_FeatureRopesVisBones("Feature.Ropes.VisBones", false, ezCVarFlags::Default, "Enables debug visualization of rope bones");

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRopeRenderComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White), new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new ezDefaultValueAttribute(6), new ezClampValueAttribute(3, 16)),
    EZ_ACCESSOR_PROPERTY("Subdivide", GetSubdivide, SetSubdivide),
    EZ_ACCESSOR_PROPERTY("UScale", GetUScale, SetUScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgRopePoseUpdated, OnRopePoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
    EZ_MESSAGE_HANDLER(ezMsgSetMeshMaterial, OnMsgSetMeshMaterial),      
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects/Ropes"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRopeRenderComponent::ezRopeRenderComponent() = default;
ezRopeRenderComponent::~ezRopeRenderComponent() = default;

void ezRopeRenderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;
  s << m_fThickness;
  s << m_uiDetail;
  s << m_bSubdivide;
  s << m_fUScale;
}

void ezRopeRenderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;
  s >> m_fThickness;
  s >> m_uiDetail;
  s >> m_bSubdivide;
  s >> m_fUScale;
}

void ezRopeRenderComponent::OnActivated()
{
  SUPER::OnActivated();

  m_LocalBounds = ezBoundingBoxSphere::MakeInvalid();
}

void ezRopeRenderComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

ezResult ezRopeRenderComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg)
{
  bounds = m_LocalBounds;
  return EZ_SUCCESS;
}

void ezRopeRenderComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().HasMirrorScaling() ? 1 : 0;
  const ezUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[0];

  ezSkinnedMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezSkinnedMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = hMaterial;
    pRenderData->m_Color = m_Color;

    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiFlipWinding = uiFlipWinding;
    pRenderData->m_uiUniformScale = uiUniformScale;

    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->m_hSkinningTransforms = m_SkinningState.m_hGpuBuffer;

    pRenderData->FillSortingKey();
  }

  // Determine render data category.
  ezRenderData::Category category = ezDefaultRenderDataCategories::LitOpaque;

  if (hMaterial.IsValid())
  {
    ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::AllowLoadingFallback);
    category = pMaterial->GetRenderDataCategory();
  }

  msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);

  if (cvar_FeatureRopesVisBones)
  {
    ezHybridArray<ezDebugRendererLine, 128> lines(ezFrameAllocator::GetCurrentAllocator());
    lines.Reserve(m_SkinningState.m_Transforms.GetCount() * 3);

    ezMat4 offsetMat;
    offsetMat.SetIdentity();

    for (ezUInt32 i = 0; i < m_SkinningState.m_Transforms.GetCount(); ++i)
    {
      offsetMat.SetTranslationVector(ezVec3(static_cast<float>(i), 0, 0));
      ezMat4 skinningMat = m_SkinningState.m_Transforms[i].GetAsMat4() * offsetMat;

      ezVec3 pos = skinningMat.GetTranslationVector();

      auto& x = lines.ExpandAndGetRef();
      x.m_start = pos;
      x.m_end = x.m_start + skinningMat.TransformDirection(ezVec3::MakeAxisX());
      x.m_startColor = ezColor::Red;
      x.m_endColor = ezColor::Red;

      auto& y = lines.ExpandAndGetRef();
      y.m_start = pos;
      y.m_end = y.m_start + skinningMat.TransformDirection(ezVec3::MakeAxisY() * 2.0f);
      y.m_startColor = ezColor::Green;
      y.m_endColor = ezColor::Green;

      auto& z = lines.ExpandAndGetRef();
      z.m_start = pos;
      z.m_end = z.m_start + skinningMat.TransformDirection(ezVec3::MakeAxisZ() * 2.0f);
      z.m_startColor = ezColor::Blue;
      z.m_endColor = ezColor::Blue;
    }

    ezDebugRenderer::DrawLines(msg.m_pView->GetHandle(), lines, ezColor::White, GetOwner()->GetGlobalTransform());
  }
}

void ezRopeRenderComponent::SetThickness(float fThickness)
{
  if (m_fThickness != fThickness)
  {
    m_fThickness = fThickness;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      ezHybridArray<ezTransform, 128> transforms;
      transforms.SetCountUninitialized(m_SkinningState.m_Transforms.GetCount());

      ezMat4 offsetMat;
      offsetMat.SetIdentity();

      for (ezUInt32 i = 0; i < m_SkinningState.m_Transforms.GetCount(); ++i)
      {
        offsetMat.SetTranslationVector(ezVec3(static_cast<float>(i), 0, 0));
        ezMat4 skinningMat = m_SkinningState.m_Transforms[i].GetAsMat4() * offsetMat;

        transforms[i] = ezTransform::MakeFromMat4(skinningMat);
      }

      UpdateSkinningTransformBuffer(transforms);
    }
  }
}

void ezRopeRenderComponent::SetDetail(ezUInt32 uiDetail)
{
  if (m_uiDetail != uiDetail)
  {
    m_uiDetail = uiDetail;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void ezRopeRenderComponent::SetSubdivide(bool bSubdivide)
{
  if (m_bSubdivide != bSubdivide)
  {
    m_bSubdivide = bSubdivide;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void ezRopeRenderComponent::SetUScale(float fUScale)
{
  if (m_fUScale != fUScale)
  {
    m_fUScale = fUScale;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void ezRopeRenderComponent::OnMsgSetColor(ezMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

void ezRopeRenderComponent::OnMsgSetMeshMaterial(ezMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void ezRopeRenderComponent::OnRopePoseUpdated(ezMsgRopePoseUpdated& msg)
{
  if (msg.m_LinkTransforms.IsEmpty())
    return;

  if (m_SkinningState.m_Transforms.GetCount() != msg.m_LinkTransforms.GetCount())
  {
    m_SkinningState.Clear();

    GenerateRenderMesh(msg.m_LinkTransforms.GetCount());
  }

  UpdateSkinningTransformBuffer(msg.m_LinkTransforms);

  ezBoundingBox newBounds = ezBoundingBox::MakeFromPoints(&msg.m_LinkTransforms[0].m_vPosition, msg.m_LinkTransforms.GetCount(), sizeof(ezTransform));

  // if the existing bounds are big enough, don't update them
  if (!m_LocalBounds.IsValid() || !m_LocalBounds.GetBox().Contains(newBounds))
  {
    m_LocalBounds.ExpandToInclude(ezBoundingBoxSphere::MakeFromBox(newBounds));

    TriggerLocalBoundsUpdate();
  }
}

void ezRopeRenderComponent::GenerateRenderMesh(ezUInt32 uiNumRopePieces)
{
  ezStringBuilder sResourceName;
  sResourceName.SetFormat("Rope-Mesh:{}{}-d{}-u{}", uiNumRopePieces, m_bSubdivide ? "Sub" : "", m_uiDetail, m_fUScale);

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sResourceName);
  if (m_hMesh.IsValid())
    return;

  ezGeometry geom;

  const ezAngle fDegStep = ezAngle::MakeFromDegree(360.0f / m_uiDetail);
  const float fVStep = 1.0f / m_uiDetail;

  auto addCap = [&](float x, const ezVec3& vNormal, ezUInt16 uiBoneIndex, bool bFlipWinding)
  {
    ezVec4U16 boneIndices(uiBoneIndex, 0, 0, 0);

    ezUInt32 centerIndex = geom.AddVertex(ezVec3(x, 0, 0), vNormal, ezVec2(0.5f, 0.5f), ezColor::White, boneIndices);

    ezAngle deg = ezAngle::MakeFromRadian(0);
    for (ezUInt32 s = 0; s < m_uiDetail; ++s)
    {
      const float fY = ezMath::Cos(deg);
      const float fZ = ezMath::Sin(deg);

      geom.AddVertex(ezVec3(x, fY, fZ), vNormal, ezVec2(fY, fZ), ezColor::White, boneIndices);

      deg += fDegStep;
    }

    ezUInt32 triangle[3];
    triangle[0] = centerIndex;
    for (ezUInt32 s = 0; s < m_uiDetail; ++s)
    {
      triangle[1] = s + triangle[0] + 1;
      triangle[2] = ((s + 1) % m_uiDetail) + triangle[0] + 1;

      geom.AddPolygon(triangle, bFlipWinding);
    }
  };

  auto addPiece = [&](float x, const ezVec4U16& vBoneIndices, const ezColorLinearUB& boneWeights, bool bCreatePolygons)
  {
    ezAngle deg = ezAngle::MakeFromRadian(0);
    float fU = x * m_fUScale;
    float fV = 0;

    for (ezUInt32 s = 0; s <= m_uiDetail; ++s)
    {
      const float fY = ezMath::Cos(deg);
      const float fZ = ezMath::Sin(deg);

      const ezVec3 pos(x, fY, fZ);
      const ezVec3 normal(0, fY, fZ);

      geom.AddVertex(pos, normal, ezVec2(fU, fV), ezColor::White, vBoneIndices, boneWeights);

      deg += fDegStep;
      fV += fVStep;
    }

    if (bCreatePolygons)
    {
      ezUInt32 endIndex = geom.GetVertices().GetCount() - (m_uiDetail + 1);
      ezUInt32 startIndex = endIndex - (m_uiDetail + 1);

      ezUInt32 triangle[3];
      for (ezUInt32 s = 0; s < m_uiDetail; ++s)
      {
        triangle[0] = startIndex + s;
        triangle[1] = startIndex + s + 1;
        triangle[2] = endIndex + s + 1;
        geom.AddPolygon(triangle, false);

        triangle[0] = startIndex + s;
        triangle[1] = endIndex + s + 1;
        triangle[2] = endIndex + s;
        geom.AddPolygon(triangle, false);
      }
    }
  };

  // cap
  {
    const ezVec3 normal = ezVec3(-1, 0, 0);
    addCap(0.0f, normal, 0, true);
  }

  // pieces
  {
    // first ring full weight to first bone
    addPiece(0.0f, ezVec4U16(0, 0, 0, 0), ezColorLinearUB(255, 0, 0, 0), false);

    ezUInt16 p = 1;

    if (m_bSubdivide)
    {
      addPiece(0.75f, ezVec4U16(0, 0, 0, 0), ezColorLinearUB(255, 0, 0, 0), true);

      for (; p < uiNumRopePieces - 2; ++p)
      {
        addPiece(static_cast<float>(p) + 0.25f, ezVec4U16(p, 0, 0, 0), ezColorLinearUB(255, 0, 0, 0), true);
        addPiece(static_cast<float>(p) + 0.75f, ezVec4U16(p, 0, 0, 0), ezColorLinearUB(255, 0, 0, 0), true);
      }

      addPiece(static_cast<float>(p) + 0.25f, ezVec4U16(p, 0, 0, 0), ezColorLinearUB(255, 0, 0, 0), true);
      ++p;
    }
    else
    {
      for (; p < uiNumRopePieces - 1; ++p)
      {
        // Middle rings half weight between bones. To ensure that weights sum up to 1 we weight one bone with 128 and the other with 127,
        // since "ubyte normalized" can't represent 0.5 perfectly.
        addPiece(static_cast<float>(p), ezVec4U16(p - 1, p, 0, 0), ezColorLinearUB(128, 127, 0, 0), true);
      }
    }

    // last ring full weight to last bone
    addPiece(static_cast<float>(p), ezVec4U16(p, 0, 0, 0), ezColorLinearUB(255, 0, 0, 0), true);
  }

  // cap
  {
    const ezVec3 normal = ezVec3(1, 0, 0);
    addCap(static_cast<float>(uiNumRopePieces - 1), normal, static_cast<ezUInt16>(uiNumRopePieces - 1), false);
  }

  geom.ComputeTangents();

  ezMeshResourceDescriptor desc;

  // Data/Base/Materials/Prototyping/PrototypeBlack.ezMaterialAsset
  desc.SetMaterial(0, "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }");

  auto& meshBufferDesc = desc.MeshBufferDesc();
  meshBufferDesc.AddCommonStreams();
  meshBufferDesc.AddStream(ezMeshVertexStreamType::SkinningData);
  meshBufferDesc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(meshBufferDesc.GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sResourceName, std::move(desc), sResourceName);
}

void ezRopeRenderComponent::UpdateSkinningTransformBuffer(ezArrayPtr<const ezTransform> skinningTransforms)
{
  ezMat4 bindPoseMat;
  bindPoseMat.SetIdentity();
  m_SkinningState.m_Transforms.SetCountUninitialized(skinningTransforms.GetCount());

  const ezVec3 newScale = ezVec3(1.0f, m_fThickness * 0.5f, m_fThickness * 0.5f);
  for (ezUInt32 i = 0; i < skinningTransforms.GetCount(); ++i)
  {
    ezTransform t = skinningTransforms[i];
    t.m_vScale = newScale;

    // scale x axis to match the distance between this bone and the next bone
    if (i < skinningTransforms.GetCount() - 1)
    {
      t.m_vScale.x = (skinningTransforms[i + 1].m_vPosition - skinningTransforms[i].m_vPosition).GetLength();
    }

    bindPoseMat.SetTranslationVector(ezVec3(-static_cast<float>(i), 0, 0));

    m_SkinningState.m_Transforms[i] = t.GetAsMat4() * bindPoseMat;
  }

  m_SkinningState.TransformsChanged();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RopeRenderComponent);

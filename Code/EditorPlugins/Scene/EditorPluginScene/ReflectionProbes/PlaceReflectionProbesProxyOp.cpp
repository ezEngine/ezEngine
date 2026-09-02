#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/ReflectionProbes/PlaceReflectionProbesProxyOp.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/IO/MemoryStream.h>
#include <ToolsFoundation/Command/TreeCommands.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxy_PlaceReflectionProbes, 1, ezRTTIDefaultAllocator<ezLongOpProxy_PlaceReflectionProbes>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Name of the object that all generated probes are parented to.
  constexpr const char* szGeneratedRootName = "Auto Generated Reflection Probes";
} // namespace

void ezLongOpProxy_PlaceReflectionProbes::InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid)
{
  m_DocumentGuid = documentGuid;
  m_ComponentGuid = componentGuid;
}

void ezLongOpProxy_PlaceReflectionProbes::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& inout_config)
{
  out_sReplicationOpType = "ezLongOpWorker_PlaceReflectionProbes";

  ezVec3 vExtents(50.0f);
  float fCellSize = 0.5f;
  float fProbeHeight = 1.7f;
  float fMinAreaSize = 4.0f;
  float fLargeAreaRadius = 1.5f;
  float fMergeDistance = 4.0f;
  ezUInt32 uiMaxProbes = 64;
  float fMaxProbeSize = 12.0f;
  float fMaxCeilingHeight = 8.0f;
  float fOutdoorSpacing = 30.0f;
  ezUInt8 uiMode = 2; // ezReflectionProbePlacementMode::Both
  bool bGenerateBoxProbes = true;
  ezTransform globalTransform = ezTransform::MakeIdentity();

  if (ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(m_DocumentGuid))
  {
    if (const ezDocumentObject* pComponent = pDoc->GetObjectManager()->GetObject(m_ComponentGuid))
    {
      const auto& accessor = pComponent->GetTypeAccessor();

      vExtents = accessor.GetValue("Extents").ConvertTo<ezVec3>();
      fCellSize = accessor.GetValue("CellSize").ConvertTo<float>();
      fProbeHeight = accessor.GetValue("ProbeHeight").ConvertTo<float>();
      fMinAreaSize = accessor.GetValue("MinAreaSize").ConvertTo<float>();
      fLargeAreaRadius = accessor.GetValue("LargeAreaRadius").ConvertTo<float>();
      fMergeDistance = accessor.GetValue("MergeDistance").ConvertTo<float>();
      uiMaxProbes = accessor.GetValue("MaxProbes").ConvertTo<ezUInt32>();
      fMaxProbeSize = accessor.GetValue("MaxProbeSize").ConvertTo<float>();
      fMaxCeilingHeight = accessor.GetValue("MaxCeilingHeight").ConvertTo<float>();
      fOutdoorSpacing = accessor.GetValue("OutdoorSpacing").ConvertTo<float>();
      uiMode = accessor.GetValue("Mode").ConvertTo<ezUInt8>();
      bGenerateBoxProbes = accessor.GetValue("GenerateBoxProbes").ConvertTo<bool>();

      if (ezSceneDocument* pScene = ezDynamicCast<ezSceneDocument*>(pDoc))
      {
        globalTransform = pScene->GetGlobalTransform(pComponent->GetParent());
      }
    }
  }

  // The engine process works in world space, so turn the local volume into world space bounds here.
  const ezVec3 vHalfExtents = vExtents.CompMul(globalTransform.m_vScale) * 0.5f;
  const ezVec3 vCenter = globalTransform.m_vPosition;

  inout_config << (vCenter - vHalfExtents);
  inout_config << (vCenter + vHalfExtents);
  inout_config << fCellSize;
  inout_config << fProbeHeight;
  inout_config << fMinAreaSize;
  inout_config << fLargeAreaRadius;
  inout_config << fMergeDistance;
  inout_config << uiMaxProbes;
  inout_config << fMaxProbeSize;
  inout_config << fMaxCeilingHeight;
  inout_config << fOutdoorSpacing;
  inout_config << uiMode;
  inout_config << bGenerateBoxProbes;
}

void ezLongOpProxy_PlaceReflectionProbes::Finalize(ezResult result, const ezDataBuffer& resultData)
{
  if (result.Failed())
  {
    ezLog::Error("Reflection probe placement failed.");
    return;
  }

  ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(m_DocumentGuid);
  if (pDoc == nullptr)
    return;

  const ezDocumentObject* pComponent = pDoc->GetObjectManager()->GetObject(m_ComponentGuid);
  if (pComponent == nullptr)
    return;

  // The placement component sits on a game object, and the probes become children of that object.
  const ezDocumentObject* pVolumeObject = pComponent->GetParent();
  if (pVolumeObject == nullptr)
    return;

  ezSceneDocument* pScene = ezDynamicCast<ezSceneDocument*>(pDoc);
  if (pScene == nullptr)
    return;

  ezRawMemoryStreamReader reader(resultData);

  ezUInt32 uiNumProbes = 0;
  reader >> uiNumProbes;

  struct Probe
  {
    ezVec3 m_vPosition;
    ezVec3 m_vExtents;
    bool m_bIsBox;
  };

  ezDynamicArray<Probe> probes;
  probes.Reserve(uiNumProbes);

  for (ezUInt32 i = 0; i < uiNumProbes; ++i)
  {
    auto& probe = probes.ExpandAndGetRef();
    reader >> probe.m_vPosition;
    reader >> probe.m_vExtents;
    reader >> probe.m_bIsBox;
  }

  if (probes.IsEmpty())
  {
    ezLog::Warning("Reflection probe placement did not find any suitable locations.");
    return;
  }

  const ezRTTI* pSphereProbeType = ezRTTI::FindTypeByName("ezSphereReflectionProbeComponent");
  const ezRTTI* pBoxProbeType = ezRTTI::FindTypeByName("ezBoxReflectionProbeComponent");

  if (pSphereProbeType == nullptr || pBoxProbeType == nullptr)
  {
    ezLog::Error("Reflection probe component types are not available.");
    return;
  }

  auto pHistory = pDoc->GetCommandHistory();

  pHistory->StartTransaction("Place Reflection Probes");

  // Remove the previously generated probes, so that repeated runs replace instead of accumulate.
  for (const ezDocumentObject* pChild : pVolumeObject->GetChildren())
  {
    if (pChild->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>() == szGeneratedRootName)
    {
      ezRemoveObjectCommand cmdRemove;
      cmdRemove.m_Object = pChild->GetGuid();

      if (pHistory->AddCommand(cmdRemove).Failed())
      {
        pHistory->CancelTransaction();
        ezLog::Error("Failed to remove the previously generated reflection probes.");
        return;
      }

      break;
    }
  }

  // A single parent for everything that was generated keeps the scene tree tidy and makes the
  // generated objects easy to find, select and delete by hand.
  const ezUuid rootGuid = ezUuid::MakeUuid();
  {
    ezAddObjectCommand cmdAdd;
    cmdAdd.m_NewObjectGuid = rootGuid;
    cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
    cmdAdd.m_sParentProperty = "Children";
    cmdAdd.m_Parent = pVolumeObject->GetGuid();
    cmdAdd.m_Index = -1;

    if (pHistory->AddCommand(cmdAdd).Failed())
    {
      pHistory->CancelTransaction();
      return;
    }

    ezSetObjectPropertyCommand cmdName;
    cmdName.m_Object = rootGuid;
    cmdName.m_sProperty = "Name";
    cmdName.m_NewValue = szGeneratedRootName;
    pHistory->AddCommand(cmdName).IgnoreResult();
  }

  const ezDocumentObject* pRootObject = pDoc->GetObjectManager()->GetObject(rootGuid);
  const ezTransform rootTransform = pRootObject != nullptr ? pScene->GetGlobalTransform(pRootObject) : ezTransform::MakeIdentity();
  const ezTransform invRootTransform = rootTransform.GetInverse();

  ezUInt32 uiNumBox = 0;
  ezUInt32 uiNumSphere = 0;

  for (ezUInt32 i = 0; i < probes.GetCount(); ++i)
  {
    const Probe& probe = probes[i];

    const ezUuid objGuid = ezUuid::MakeUuid();
    const ezUuid compGuid = ezUuid::MakeUuid();

    {
      ezAddObjectCommand cmdAdd;
      cmdAdd.m_NewObjectGuid = objGuid;
      cmdAdd.m_pType = ezGetStaticRTTI<ezGameObject>();
      cmdAdd.m_sParentProperty = "Children";
      cmdAdd.m_Parent = rootGuid;
      cmdAdd.m_Index = -1;

      if (pHistory->AddCommand(cmdAdd).Failed())
        continue;
    }

    {
      ezStringBuilder sName;
      sName.SetFormat("{} Probe {}", probe.m_bIsBox ? "Box" : "Sphere", i);

      ezSetObjectPropertyCommand cmdName;
      cmdName.m_Object = objGuid;
      cmdName.m_sProperty = "Name";
      cmdName.m_NewValue = sName.GetData();
      pHistory->AddCommand(cmdName).IgnoreResult();
    }

    {
      // The engine computed world space positions, so undo the parent's transform here.
      ezSetObjectPropertyCommand cmdPos;
      cmdPos.m_Object = objGuid;
      cmdPos.m_sProperty = "LocalPosition";
      cmdPos.m_NewValue = invRootTransform.TransformPosition(probe.m_vPosition);
      pHistory->AddCommand(cmdPos).IgnoreResult();
    }

    {
      ezAddObjectCommand cmdComp;
      cmdComp.m_NewObjectGuid = compGuid;
      cmdComp.m_pType = probe.m_bIsBox ? pBoxProbeType : pSphereProbeType;
      cmdComp.m_sParentProperty = "Components";
      cmdComp.m_Parent = objGuid;
      cmdComp.m_Index = -1;

      if (pHistory->AddCommand(cmdComp).Failed())
        continue;
    }

    {
      ezSetObjectPropertyCommand cmdSize;
      cmdSize.m_Object = compGuid;

      if (probe.m_bIsBox)
      {
        cmdSize.m_sProperty = "Extents";
        cmdSize.m_NewValue = probe.m_vExtents;
        ++uiNumBox;
      }
      else
      {
        cmdSize.m_sProperty = "Radius";
        cmdSize.m_NewValue = probe.m_vExtents.x;
        ++uiNumSphere;
      }

      pHistory->AddCommand(cmdSize).IgnoreResult();
    }
  }

  pHistory->FinishTransaction();

  ezLog::Success("Placed {} reflection probes ({} box, {} sphere).", probes.GetCount(), uiNumBox, uiNumSphere);
}

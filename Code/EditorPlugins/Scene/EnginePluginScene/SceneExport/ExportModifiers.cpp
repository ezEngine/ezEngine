#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>
#include <EnginePluginScene/SceneExport/ExportModifiers.h>
#include <GameEngine/Messages/ExportMessage.h>
#include <RendererCore/Components/SplineComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_RemoveShapeIconComponents, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_RemoveShapeIconComponents>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  EZ_LOCK(ref_world.GetWriteMarker());

  if (ezShapeIconComponentManager* pSiMan = ref_world.GetComponentManager<ezShapeIconComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      pSiMan->DeleteComponent(it);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_RemovePathNodeComponents, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_RemovePathNodeComponents>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSceneExportModifier_RemovePathNodeComponents::ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  if (!bForExport)
    return;

  EZ_LOCK(ref_world.GetWriteMarker());

  if (ezSplineNodeComponentManager* pManager = ref_world.GetComponentManager<ezSplineNodeComponentManager>())
  {
    for (auto it = pManager->GetComponents(); it.IsValid(); it.Next())
    {
      if (it->GetOwner()->GetComponents().GetCount() == 1 && it->GetOwner()->GetChildCount() == 0)
      {
        // if this is the only component on the object, clear it's name, so that the entire object may get cleaned up
        it->GetOwner()->SetName(ezStringView());
      }

      pManager->DeleteComponent(it);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_GenericExport, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_GenericExport>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSceneExportModifier_GenericExport::ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  if (!bForExport)
    return;

  ezStringBuilder sb;
  ezConversionUtils::ToString(documentGuid, sb);

  EZ_LOCK(ref_world.GetWriteMarker());

  ezMsgExport msg;
  msg.m_sDocumentType = sDocumentType;
  msg.m_sDocumentGuid = sb;

  for (auto it = ref_world.GetObjects(); it.IsValid(); ++it)
  {
    if (!it->IsStatic())
      continue;

    it->SendMessage(msg);
  }
}

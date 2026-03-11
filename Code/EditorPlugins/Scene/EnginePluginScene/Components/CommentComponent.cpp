#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EnginePluginScene/Components/CommentComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCommentComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Comment", GetComment, SetComment),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Editing"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCommentComponent::ezCommentComponent() = default;
ezCommentComponent::~ezCommentComponent() = default;

void ezCommentComponent::SetComment(const char* szText)
{
  m_sComment.Assign(szText);
}

const char* ezCommentComponent::GetComment() const
{
  return m_sComment.GetString();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_RemoveCommentComponents, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_RemoveCommentComponents>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSceneExportModifier_RemoveCommentComponents::ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  EZ_LOCK(ref_world.GetWriteMarker());

  if (ezCommentComponentManager* pMan = ref_world.GetComponentManager<ezCommentComponentManager>())
  {
    for (auto it = pMan->GetComponents(); it.IsValid(); it.Next())
    {
      pMan->DeleteComponent(it);
    }
  }
}

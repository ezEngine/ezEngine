#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>

//static
ezTransform ezSceneDocument::QueryLocalTransform(const ezDocumentObject* pObject)
{
  const ezVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const ezQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();
  const float fScaling = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return ezTransform(vTranslation, qRotation, vScaling * fScaling);
}

//static
ezSimdTransform ezSceneDocument::QueryLocalTransformSimd(const ezDocumentObject* pObject)
{
  const ezVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const ezQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();
  const float fScaling = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return ezSimdTransform(ezSimdConversion::ToVec3(vTranslation), ezSimdConversion::ToQuat(qRotation),
    ezSimdConversion::ToVec3(vScaling * fScaling));
}


ezTransform ezSceneDocument::ComputeGlobalTransform(const ezDocumentObject* pObject) const
{
  if (pObject == nullptr || pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezGameObject>())
  {
    m_GlobalTransforms[pObject] = ezSimdTransform::Identity();
    return ezTransform::Identity();
  }

  const ezSimdTransform tParent = ezSimdConversion::ToTransform(ComputeGlobalTransform(pObject->GetParent()));
  const ezSimdTransform tLocal = QueryLocalTransformSimd(pObject);

  ezSimdTransform tGlobal;
  tGlobal.SetGlobalTransform(tParent, tLocal);

  m_GlobalTransforms[pObject] = tGlobal;

  return ezSimdConversion::ToTransform(tGlobal);
}



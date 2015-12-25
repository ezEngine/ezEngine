#include <PCH.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <GameUtils/Components/RotorComponent.h>
#include <GameUtils/Components/SliderComponent.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <Core/Scene/Scene.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneContext, 1, ezRTTIDefaultAllocator<ezSceneContext>);
EZ_BEGIN_PROPERTIES
EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "ezScene;ezPrefab"),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezUInt32 ezSceneContext::s_uiShapeIconBufferCounter = 0;

void ezSceneContext::ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds)
{
  /// \todo Work around for objects without bounds (?)
  auto b = pObj->GetGlobalBounds();

  if (b.IsValid())
    bounds.ExpandToInclude(b);

  auto it = pObj->GetChildren();

  while (it.IsValid())
  {
    ComputeHierarchyBounds(it, bounds);
    it.Next();
  }
}

ezSceneContext::ezSceneContext()
{
  m_bRenderSelectionOverlay = true;
  m_bRenderShapeIcons = true;
  m_bShapeIconBufferValid = false;
}

void ezSceneContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezSceneSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages

     auto msg = static_cast<const ezSceneSettingsMsgToEngine*>(pMsg);

    const bool bSimulate = msg->m_bSimulateWorld;
    m_bRenderSelectionOverlay = msg->m_bRenderOverlay;
    m_bRenderShapeIcons = msg->m_bRenderShapeIcons;

    if (bSimulate != GetScene()->GetWorld()->GetWorldSimulationEnabled())
    {
      ezLog::Info("World Simulation %s", bSimulate ? "enabled" : "disabled");
      GetScene()->GetWorld()->SetWorldSimulationEnabled(bSimulate);

      if (bSimulate)
      {
        GetScene()->ReinitSceneModules();
      }
    }

    GetScene()->GetWorld()->GetClock().SetSpeed(msg->m_fSimulationSpeed);
    GetScene()->Update();

    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectSelectionMsgToEngine>())
  {
    HandleSelectionMsg(static_cast<const ezObjectSelectionMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxMsgToEngine>())
  {
    if (m_Selection.IsEmpty())
      return;

    ezBoundingBoxSphere bounds;
    bounds.SetInvalid();

    {
      auto pWorld = GetScene()->GetWorld();

      EZ_LOCK(pWorld->GetReadMarker());

      for (const auto& obj : m_Selection)
      {
        ezGameObject* pObj;
        if (!pWorld->TryGetObject(obj, pObj))
          continue;

        ComputeHierarchyBounds(pObj, bounds);
      }

      // if there are no valid bounds, at all, use dummy bounds for each object
      if (!bounds.IsValid())
      {
        for (const auto& obj : m_Selection)
        {
          ezGameObject* pObj;
          if (!pWorld->TryGetObject(obj, pObj))
            continue;

          bounds.ExpandToInclude(ezBoundingBoxSphere(pObj->GetGlobalPosition(), ezVec3(0.5f), 0.5f));
        }
      }
    }

    EZ_ASSERT_DEV(bounds.IsValid() && !bounds.IsNaN(), "Invalid bounds");

    const ezQuerySelectionBBoxMsgToEngine* msg = static_cast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg);

    ezQuerySelectionBBoxResultMsgToEditor res;
    res.m_uiViewID = msg->m_uiViewID;
    res.m_iPurpose = msg->m_iPurpose;
    res.m_vCenter = bounds.m_vCenter;
    res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
    res.m_DocumentGuid = pMsg->m_DocumentGuid;

    SendProcessMessage(&res);

    return;
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectTagMsgToEngine>())
  {
    const ezObjectTagMsgToEngine* pMsg2 = static_cast<const ezObjectTagMsgToEngine*>(pMsg);

    if (pMsg2->m_sTag == "EditorHidden")
      m_bShapeIconBufferValid = false;

    // fall through
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEntityMsgToEngine>())
  {
    m_bShapeIconBufferValid = false;

    // fall through
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezSceneContext::GenerateShapeIconMesh()
{
  if (m_bShapeIconBufferValid)
    return;

  // clear previous data
  {
    for (auto it = m_ShapeIcons.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_hMeshBuffer.Invalidate();
      it.Value().m_IconPositions.Clear();
    }
  }

  {
    const ezWorld* world = m_pScene->GetWorld();
    EZ_LOCK(world->GetReadMarker());

    auto& tagReg = ezTagRegistry::GetGlobalRegistry();
    ezTag tagHidden;
    tagReg.RegisterTag("EditorHidden", &tagHidden);
    ezTag tagEditor;
    ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor", &tagEditor);

    ezUInt32 obj = 0;
    for (auto it = world->GetObjects(); it.IsValid(); ++it)
    {
      if (it->GetComponents().IsEmpty())
        continue;

      if (it->GetTags().IsSet(tagEditor) || it->GetTags().IsSet(tagHidden))
        continue;

      const ezRTTI* pRtti = it->GetComponents()[0]->GetDynamicRTTI();

      ShapeIconData* pData = nullptr;
      if (!m_ShapeIcons.TryGetValue(pRtti, pData))
        continue;

      ShapeIconData::PosID& pid = pData->m_IconPositions.ExpandAndGetRef();
      pid.id = it->GetComponents()[0]->m_uiEditorPickingID;
      pid.pos = it->GetGlobalPosition();
    }
  }

  for (auto it = m_ShapeIcons.GetIterator(); it.IsValid(); ++it)
  {
    auto& dat = it.Value().m_IconPositions;
    if (dat.IsEmpty())
      continue;

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::RUInt);
    desc.AllocateStreams(dat.GetCount() * 4, ezGALPrimitiveTopology::Triangles, dat.GetCount() * 2);


    ezUInt32 obj = 0;
    for (const auto& pid : dat)
    {
      desc.SetVertexData<ezVec3>(0, obj * 4 + 0, pid.pos);
      desc.SetVertexData<ezVec3>(0, obj * 4 + 1, pid.pos);
      desc.SetVertexData<ezVec3>(0, obj * 4 + 2, pid.pos);
      desc.SetVertexData<ezVec3>(0, obj * 4 + 3, pid.pos);

      desc.SetVertexData<ezVec2>(1, obj * 4 + 0, ezVec2(0, 0));
      desc.SetVertexData<ezVec2>(1, obj * 4 + 1, ezVec2(1, 0));
      desc.SetVertexData<ezVec2>(1, obj * 4 + 2, ezVec2(1, 1));
      desc.SetVertexData<ezVec2>(1, obj * 4 + 3, ezVec2(0, 1));

      desc.SetVertexData<ezUInt32>(2, obj * 4 + 0, pid.id);
      desc.SetVertexData<ezUInt32>(2, obj * 4 + 1, pid.id);
      desc.SetVertexData<ezUInt32>(2, obj * 4 + 2, pid.id);
      desc.SetVertexData<ezUInt32>(2, obj * 4 + 3, pid.id);

      desc.SetTriangleIndices(obj * 2 + 0, obj * 4 + 0, obj * 4 + 1, obj * 4 + 2);
      desc.SetTriangleIndices(obj * 2 + 1, obj * 4 + 0, obj * 4 + 2, obj * 4 + 3);

      ++obj;
    }

    ezStringBuilder s;
    s.Format("ShapeIconMeshBuffer%u", s_uiShapeIconBufferCounter);
    ++s_uiShapeIconBufferCounter;

    it.Value().m_hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(s, desc);
  }

  m_bShapeIconBufferValid = true;
}

void ezSceneContext::RenderShapeIcons(ezRenderContext* pContext)
{
  if (!m_bRenderShapeIcons)
    return;

  for (auto it = m_ShapeIcons.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_IconPositions.IsEmpty())
      continue;

    pContext->BindMeshBuffer(it.Value().m_hMeshBuffer);
    pContext->BindTexture("ShapeIcon", it.Value().m_hTexture);
    pContext->BindShader(m_hShapeIconShader);

    pContext->DrawMeshBuffer();
  }

  /// \todo Render all selected ones again (blend overlay color)
}

void ezSceneContext::OnInitialize()
{
  auto pWorld = GetScene()->GetWorld();
  EZ_LOCK(pWorld->GetWriteMarker());

  /// \todo Plugin concept to allow custom initialization
  pWorld->CreateComponentManager<ezMeshComponentManager>();
  pWorld->CreateComponentManager<ezRotorComponentManager>();
  pWorld->CreateComponentManager<ezSliderComponentManager>();

  LoadShapeIconTextures();

  m_hShapeIconShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Editor/ShapeIcon.ezShader");
}

void ezSceneContext::OnDeinitialize()
{
  m_ShapeIcons.Clear();
  m_hShapeIconShader.Invalidate();
}

ezEngineProcessViewContext* ezSceneContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezSceneViewContext, this);
}

void ezSceneContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezSceneContext::HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg)
{
  m_Selection.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_SelectionWithChildren.Clear();

  ezStringBuilder sSel = pMsg->m_sSelection;
  ezStringBuilder sGuid;

  auto pWorld = GetScene()->GetWorld();
  EZ_LOCK(pWorld->GetReadMarker());

  while (!sSel.IsEmpty())
  {
    sGuid.SetSubString_ElementCount(sSel.GetData() + 1, 40);
    sSel.Shrink(41, 0);

    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sGuid);

    auto hObject = m_Context.m_GameObjectMap.GetHandle(guid);

    if (!hObject.IsInvalidated())
    {
      m_Selection.PushBack(hObject);

      ezGameObject* pObject;
      if (pWorld->TryGetObject(hObject, pObject))
        InsertSelectedChildren(pObject);
    }
  }

  for (auto it = m_SelectionWithChildrenSet.GetIterator(); it.IsValid(); ++it)
  {
    m_SelectionWithChildren.PushBack(it.Key());
  }
}

void ezSceneContext::InsertSelectedChildren(const ezGameObject* pObject)
{
  m_SelectionWithChildrenSet.Insert(pObject->GetHandle());

  auto it = pObject->GetChildren();

  while (it.IsValid())
  {
    InsertSelectedChildren(it);

    it.Next();
  }
}

void ezSceneContext::LoadShapeIconTextures()
{
  EZ_LOG_BLOCK("LoadShapeIconTextures");

  ezStringBuilder sPath;

  for (ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezComponent>())
      continue;

    sPath.Set("ShapeIcons/", pRtti->GetTypeName(), ".dds");

    if (ezFileSystem::ExistsFile(sPath))
    {
      m_ShapeIcons[pRtti].m_hTexture = ezResourceManager::LoadResource<ezTextureResource>(sPath);
    }
  }

}

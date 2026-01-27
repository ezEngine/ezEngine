#pragma once

#include <RendererCore/Pipeline/Renderer.h>

/// \brief Registry to get a renderer for a specific render data type. Instances of all renderers are automatically created and registered.
class EZ_RENDERERCORE_DLL ezRendererRegistry
{
public:
  EZ_FORCE_INLINE static const ezRenderer* GetRenderer(const ezRTTI* pRenderDataType)
  {
    if (s_bRendererInstancesDirty)
    {
      CreateRendererInstances();
    }

    ezUInt32 uiIndex = 0;
    if (s_RenderDataTypeToRendererIndex.TryGetValue(pRenderDataType, uiIndex))
    {
      return s_RendererInstances[uiIndex].Borrow();
    }

    return nullptr;
  }

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RendererRegistry);

  static void PluginEventHandler(const ezPluginEvent& e);
  static void UpdateRendererTypes();

  static void CreateRendererInstances();
  static void ClearRendererInstances();

  static ezHybridArray<const ezRTTI*, 16> s_RendererTypes;
  static ezDynamicArray<ezUniquePtr<ezRenderer>> s_RendererInstances;
  static ezHashTable<const ezRTTI*, ezUInt32> s_RenderDataTypeToRendererIndex;
  static bool s_bRendererInstancesDirty;
};

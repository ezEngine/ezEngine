#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezGALDeviceEvent;
class ezRenderGraph;

using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;
using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

/// Renders the custom mouse cursor that was set through ezInputManager::SetMouseCursor().
///
/// The cursor identifier (ezMouseCursorDesc::m_sCursor) may be the GUID or path of either
///  * a 2D texture asset - it is then rendered with the built-in Shaders/MouseCursor/MouseCursor.ezShader, or
///  * a material asset - which allows for arbitrary custom cursor effects.
///
/// The cursor is drawn as a single quad that is generated from SV_VertexID, so it needs neither a
/// vertex nor an index buffer. A custom cursor material has to follow these rules:
///  * Its vertex shader has to `#include <Shaders/MouseCursor/MouseCursorCommon.h>` and return
///    `ezMouseCursorVertex(VertexID)`, or do the equivalent math itself, using ezMouseCursorConstants.
///  * It must set up its own render state (no depth test, no culling, alpha blending).
///  * It must not use constant buffer slot 3 in the EZ_GAL_BIND_GROUP_FRAME bind group,
///    that one holds ezMouseCursorConstants.
///  * It must not declare permutation variables and it must not read ezGlobalConstants,
///    except for the time constants. Outside of a render pipeline neither is in a defined state.
class EZ_GAMEENGINE_DLL ezMouseCursorRenderer
{
  EZ_DECLARE_SINGLETON(ezMouseCursorRenderer);

public:
  ezMouseCursorRenderer();
  ~ezMouseCursorRenderer();

  /// Sets which swap-chain to render the cursor into. Without this, nothing is rendered.
  ///
  /// ezGameState::SetupMainView() sets this up for the main window automatically.
  void SetSwapChain(ezGALSwapChainHandle hSwapChain) { m_hSwapChain = hSwapChain; }

private:
  struct ResolvedCursor
  {
    ezMaterialResourceHandle m_hMaterial;
    ezTexture2DResourceHandle m_hTexture;
  };

  void OnGALDeviceEvent(const ezGALDeviceEvent& e);
  void ResolveCursor(ezStringView sIdentifier);
  ezResult EnsureGpuResourcesExist();
  void ReleaseGpuResources();

  ezGALSwapChainHandle m_hSwapChain;

  ezUInt32 m_uiLastIdentifierChangeCounter = 0;
  bool m_bIdentifierResolved = false;
  ezHashTable<ezString, ResolvedCursor> m_ResolvedCursors;
  ResolvedCursor m_Current;

  ezShaderResourceHandle m_hDefaultShader;
  ezConstantBufferStorageHandle m_hConstantBuffer;
  ezSharedPtr<ezRenderGraph> m_pRenderGraph;
};

#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezCVarFloat cvar_ProcGenCullingDistanceScale("ProcGen.Culling.DistanceScale", 1.0f, ezCVarFlags::Default, "Global scale to control cull distance for all placement outputs");
ezCVarInt cvar_ProcGenCullingMaxRadius("ProcGen.Culling.MaxRadius", 10, ezCVarFlags::Default, "Maximum cull radius in number of tiles");

using namespace ezProcGenInternal;

FindPlacementTilesTask::FindPlacementTilesTask(ezProcPlacementComponent* pComponent, ezUInt32 uiOutputIndex)
  : m_pComponent(pComponent)
  , m_uiOutputIndex(uiOutputIndex)
{
  ezStringBuilder sName;
  sName.SetFormat("UpdateTiles {}", m_pComponent->m_OutputContexts[m_uiOutputIndex].m_pOutput->m_sName);

  ConfigureTask(sName, ezTaskNesting::Never);
}

FindPlacementTilesTask::~FindPlacementTilesTask() = default;

void FindPlacementTilesTask::Execute()
{
  m_NewTiles.Clear();
  m_OldTileKeys.Clear();

  ezHybridArray<ezSimdMat4f, 8, ezAlignedAllocatorWrapper> globalToLocalBoxTransforms;

  auto& outputContext = m_pComponent->m_OutputContexts[m_uiOutputIndex];

  const float fTileSize = outputContext.m_pOutput->GetTileSize();
  const float fCullDistance = outputContext.m_pOutput->m_fCullDistance * cvar_ProcGenCullingDistanceScale;

  float fRadius = ezMath::Min(ezMath::Ceil(fCullDistance / fTileSize + 1.0f), static_cast<float>(cvar_ProcGenCullingMaxRadius));
  ezInt32 iRadius = static_cast<ezInt32>(fRadius);
  ezInt32 iRadiusSqr = iRadius * iRadius;

  ezSimdVec4f fHalfTileSize = ezSimdVec4f(fTileSize * 0.5f);

  for (ezVec3 vCameraPosition : m_CameraPositions)
  {
    ezVec3 cameraPos = vCameraPosition / fTileSize;
    float fPosX = ezMath::Round(cameraPos.x);
    float fPosY = ezMath::Round(cameraPos.y);
    ezInt32 iPosX = static_cast<ezInt32>(fPosX);
    ezInt32 iPosY = static_cast<ezInt32>(fPosY);

    float fY = (fPosY - fRadius) * fTileSize;
    ezInt32 iY = -iRadius;

    while (iY <= iRadius)
    {
      float fX = (fPosX - fRadius) * fTileSize;
      ezInt32 iX = -iRadius;

      while (iX <= iRadius)
      {
        if (iX * iX + iY * iY <= iRadiusSqr)
        {
          ezUInt64 uiTileKey = GetTileKey(iPosX + iX, iPosY + iY);
          if (auto pTile = outputContext.m_TileIndices.GetValue(uiTileKey))
          {
            pTile->m_uiLastSeenFrame = ezRenderWorld::GetFrameCounter();
          }
          else
          {
            ezSimdVec4f testPos = ezSimdVec4f(fX, fY, 0.0f);
            ezSimdFloat minZ = 10000.0f;
            ezSimdFloat maxZ = -10000.0f;

            globalToLocalBoxTransforms.Clear();

            for (auto& bounds : m_pComponent->m_Bounds)
            {
              ezSimdBBox extendedBox = bounds.m_GlobalBoundingBox;
              extendedBox.Grow(fHalfTileSize);

              if (((testPos >= extendedBox.m_Min) && (testPos <= extendedBox.m_Max)).AllSet<2>())
              {
                minZ = minZ.Min(bounds.m_GlobalBoundingBox.m_Min.z());
                maxZ = maxZ.Max(bounds.m_GlobalBoundingBox.m_Max.z());

                globalToLocalBoxTransforms.PushBack(bounds.m_GlobalToLocalBoxTransform);
              }
            }

            if (!globalToLocalBoxTransforms.IsEmpty())
            {
              ezProcPlacementComponent::OutputContext::TileIndexAndAge emptyTile;
              emptyTile.m_uiIndex = NewTileIndex;
              emptyTile.m_uiLastSeenFrame = ezRenderWorld::GetFrameCounter();

              outputContext.m_TileIndices.Insert(uiTileKey, emptyTile);

              auto& newTile = m_NewTiles.ExpandAndGetRef();
              newTile.m_hComponent = m_pComponent->GetHandle();
              newTile.m_uiOutputIndex = m_uiOutputIndex;
              newTile.m_iPosX = iPosX + iX;
              newTile.m_iPosY = iPosY + iY;
              newTile.m_fMinZ = minZ;
              newTile.m_fMaxZ = maxZ;
              newTile.m_fTileSize = fTileSize;
              newTile.m_fDistanceToCamera = ezMath::MaxValue<float>();
              newTile.m_GlobalToLocalBoxTransforms = globalToLocalBoxTransforms;
            }
          }
        }

        ++iX;
        fX += fTileSize;
      }

      ++iY;
      fY += fTileSize;
    }
  }

  m_CameraPositions.Clear();

  // Find old tiles
  ezUInt32 uiMaxOldTiles = (ezUInt32)iRadius * 2;
  uiMaxOldTiles *= uiMaxOldTiles;

  if (outputContext.m_TileIndices.GetCount() > uiMaxOldTiles)
  {
    m_TilesByAge.Clear();

    ezUInt64 uiCurrentFrame = ezRenderWorld::GetFrameCounter();
    for (auto it = outputContext.m_TileIndices.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value().m_uiIndex == EmptyTileIndex)
        continue;

      if (it.Value().m_uiLastSeenFrame == uiCurrentFrame)
      {
        --uiMaxOldTiles;
        continue;
      }

      m_TilesByAge.PushBack({it.Key(), it.Value().m_uiLastSeenFrame});
    }

    if (m_TilesByAge.GetCount() > uiMaxOldTiles)
    {
      m_TilesByAge.Sort([](auto& ref_tileA, auto& ref_tileB)
        { return ref_tileA.m_uiLastSeenFrame < ref_tileB.m_uiLastSeenFrame; });

      ezUInt32 uiOldTileCount = m_TilesByAge.GetCount() - uiMaxOldTiles;
      for (ezUInt32 i = 0; i < uiOldTileCount; ++i)
      {
        m_OldTileKeys.PushBack(m_TilesByAge[i].m_uiTileKey);
      }
    }
  }
}

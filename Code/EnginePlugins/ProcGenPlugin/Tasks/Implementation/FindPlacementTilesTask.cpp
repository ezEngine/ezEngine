#include <ProcGenPluginPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezCVarFloat CVarCullDistanceScale(
  "pp_CullDistanceScale", 1.0f, ezCVarFlags::Default, "Global scale to control cull distance for all placement outputs");
ezCVarInt CVarMaxCullRadius("pp_MaxCullRadius", 10, ezCVarFlags::Default, "Maximum cull radius in number of tiles");

using namespace ezProcGenInternal;

FindPlacementTilesTask::FindPlacementTilesTask(ezProcPlacementComponent* pComponent, ezUInt32 uiOutputIndex)
  : m_pComponent(pComponent)
  , m_uiOutputIndex(uiOutputIndex)
{
  ezStringBuilder sName;
  sName.Format("UpdateTiles {}", m_pComponent->m_OutputContexts[m_uiOutputIndex].m_pOutput->m_sName);

  SetTaskName(sName);
}

FindPlacementTilesTask::~FindPlacementTilesTask() = default;

void FindPlacementTilesTask::Execute()
{
  m_NewTiles.Clear();
  m_OldTileKeys.Clear();

  ezHybridArray<ezSimdTransform, 8, ezAlignedAllocatorWrapper> globalToLocalBoxTransforms;

  auto& outputContext = m_pComponent->m_OutputContexts[m_uiOutputIndex];

  const float fTileSize = outputContext.m_pOutput->GetTileSize();
  const float fPatternSize = outputContext.m_pOutput->m_pPattern->m_fSize;
  const float fCullDistance = outputContext.m_pOutput->m_fCullDistance * CVarCullDistanceScale;

  float fRadius = ezMath::Min<float>(ezMath::Ceil(fCullDistance / fTileSize + 1.0f), CVarMaxCullRadius);
  ezInt32 iRadius = static_cast<ezInt32>(fRadius);
  ezInt32 iRadiusSqr = iRadius * iRadius;

  ezSimdVec4f fHalfTileSize = ezSimdVec4f(fTileSize * 0.5f);

  for (ezVec3 vCameraPosition : m_vCameraPositions)
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
          if (outputContext.m_TileIndices.Contains(uiTileKey))
          {
            outputContext.m_TileIndices[uiTileKey].m_uiLastSeenFrame = ezRenderWorld::GetFrameCounter();
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
              emptyTile.m_uiIndex = EmptyTileIndex;
              emptyTile.m_uiLastSeenFrame = 0;

              outputContext.m_TileIndices.Insert(uiTileKey, emptyTile);

              auto& newTile = m_NewTiles.ExpandAndGetRef();
              newTile.m_hComponent = m_pComponent->GetHandle();
              newTile.m_uiOutputIndex = m_uiOutputIndex;
              newTile.m_iPosX = iPosX + iX;
              newTile.m_iPosY = iPosY + iY;
              newTile.m_fMinZ = minZ;
              newTile.m_fMaxZ = maxZ;
              newTile.m_fPatternSize = fPatternSize;
              newTile.m_fDistanceToCamera = ezMath::BasicType<float>::MaxValue();
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

  // Update distance to camera
  for (auto& newTile : m_NewTiles)
  {
    ezVec2 tilePos = ezVec2((float)newTile.m_iPosX, (float)newTile.m_iPosY);

    for (ezVec3 vCameraPosition : m_vCameraPositions)
    {
      ezVec2 cameraPos = vCameraPosition.GetAsVec2() / fTileSize;

      float fDistance = (tilePos - cameraPos).GetLengthSquared();
      newTile.m_fDistanceToCamera = ezMath::Min(newTile.m_fDistanceToCamera, fDistance);
    }
  }

  // Pre-sort by distance, larger distances come first since new tiles are processed in reverse order.
  m_NewTiles.Sort([](auto& tileA, auto& tileB) { return tileA.m_fDistanceToCamera > tileB.m_fDistanceToCamera; });

  m_vCameraPositions.Clear();

  // Find old tiles
  ezUInt32 uiMaxActiveTiles = (ezUInt32)iRadius * 2;
  uiMaxActiveTiles *= uiMaxActiveTiles;

  if (outputContext.m_TileIndices.GetCount() > uiMaxActiveTiles)
  {
    m_TilesByAge.Clear();

    for (auto it = outputContext.m_TileIndices.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value().m_uiIndex != EmptyTileIndex)
      {
        m_TilesByAge.PushBack({it.Key(), it.Value().m_uiLastSeenFrame});
      }
    }

    if (m_TilesByAge.GetCount() > uiMaxActiveTiles)
    {
      m_TilesByAge.Sort([](auto& tileA, auto& tileB) { return tileA.m_uiLastSeenFrame < tileB.m_uiLastSeenFrame; });

      ezUInt32 uiOldTileCount = m_TilesByAge.GetCount() - uiMaxActiveTiles;
      for (ezUInt32 i = 0; i < uiOldTileCount; ++i)
      {
        m_OldTileKeys.PushBack(m_TilesByAge[i].m_uiTileKey);
      }
    }
  }
}

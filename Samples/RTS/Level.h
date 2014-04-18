#pragma once

#include <GameUtils/DataStructures/GameGrid.h>
#include <GameUtils/PathFinding/GridNavmesh.h>
#include <Core/World/World.h>
#include <RTS/Components/UnitComponent.h>
#include <RTS/Components/RevealerComponent.h>
#include <RTS/Components/SteeringBehaviorComponent.h>
#include <RTS/Components/FollowPathSteeringComponent.h>
#include <RTS/Components/AvoidObstacleSteeringComponent.h>
#include <RTS/Components/ObstacleComponent.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/Formats/TgaFileFormat.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <gl/GL.h>

#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367

#define GL_BGR 0x80E0 
#define GL_BGRA 0x80E1

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

#define GL_TEXTURE_MIN_LOD                0x813A
#define GL_TEXTURE_MAX_LOD                0x813B
#define GL_TEXTURE_BASE_LEVEL             0x813C
#define GL_TEXTURE_MAX_LEVEL              0x813D


class ColorResource;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ColorResource);

class ColorResourceDescriptor
{
public:
  ezColor m_Color;
};

class ColorResource : public ezResource<ColorResource, ColorResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ColorResource);

public:
  static const int MaxLod = 7;

  ColorResource()
  {
    m_Color.SetRGB(ezVec3(0, 0, 0));
    m_uiMaxQualityLevel = MaxLod;
    m_Flags |= ezResourceFlags::UpdateOnMainThread;
    m_uiTextureID = 0;
  }

  

private:
  friend class ezResourceManager;

  virtual void UpdateContent(ezStreamReaderBase& Stream) EZ_OVERRIDE 
  {
    ezColor c;
    ezInt32 state;
    Stream >> state;
    Stream >> c;

    m_Color = c;

    m_LoadingState = ezResourceLoadState::Loaded;
    m_uiLoadedQualityLevel = state;

    ezImage img0;

    ezTgaFileFormat Format;
    EZ_VERIFY(Format.ReadImage(Stream, img0, ezGlobalLog::GetInstance()).Succeeded(), "Could not read TGA from stream.");

    ezImage img;
    EZ_VERIFY(ezImageConversionBase::Convert(img0, img, ezImageFormat::B8G8R8A8_UNORM).Succeeded(), "Could not convert image to BGRA8 format.");

    //return;

    if (m_uiTextureID == 0)
    {
      glGenTextures(1, &m_uiTextureID);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, m_uiTextureID);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S , GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MaxLod);
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MaxLod);

      ezDynamicArray<ezUInt8> Temp;
      Temp.SetCount(1024 * 1024 * 4);

      for (int i = 0; i <= MaxLod; ++i)
        glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA8, ezMath::Pow2(MaxLod - i), ezMath::Pow2(MaxLod - i), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &Temp[0]);
    }
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_uiTextureID);
    ezUInt32 uiWidth = img.GetWidth(0);
    ezUInt32 uiHeight = img.GetHeight(0);
    void* pPixelData = img.GetPixelPointer<void*>(0, 0, 0, 0, 0, 0);
    glTexSubImage2D(GL_TEXTURE_2D, MaxLod - (m_uiLoadedQualityLevel - 1), 0, 0, uiWidth, uiHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pPixelData);
    //glTexImage2D(GL_TEXTURE_2D, MaxLod - (m_uiLoadedQualityLevel - 1), GL_RGBA8, uiWidth, uiHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pPixelData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MaxLod - (m_uiLoadedQualityLevel - 1));

    glDisable(GL_TEXTURE_2D);
  }

  virtual void CreateResource(const ColorResourceDescriptor& descriptor) EZ_OVERRIDE
  {
    m_Color = descriptor.m_Color;
    m_uiMaxQualityLevel = 1;
    m_uiLoadedQualityLevel = 1;
    m_LoadingState = ezResourceLoadState::Loaded;
  }

  virtual void UpdateMemoryUsage() EZ_OVERRIDE
  {
    ezUInt32 uiMemory = 0;
    ezUInt32 uiFactor = 4; // Lowest Mipmap is 1 pixel with 4 bytes

    for (ezUInt32 i = 0; i < m_uiLoadedQualityLevel; ++i)
    {
      uiMemory += uiFactor;
      uiFactor *= 4;
    }

    SetMemoryUsageCPU(0);
    SetMemoryUsageGPU(uiMemory);
  }

  virtual void UnloadData(bool bFullUnload) EZ_OVERRIDE
  {
    if (bFullUnload || m_uiLoadedQualityLevel == 1)
    {
      m_Color.SetRGB(ezVec3(1, 0, 0));
      m_LoadingState = ezResourceLoadState::Uninitialized;
      m_uiLoadedQualityLevel = 0;
    }
    else
    {
      --m_uiLoadedQualityLevel;
      m_Color.SetRGB(ezVec3(1, 1, (float) GetLoadedQualityLevel() / (float) GetMaxQualityLevel()));
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_uiTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MaxLod - (m_uiLoadedQualityLevel - 1));
    glDisable(GL_TEXTURE_2D);
  }

public:

  ezUInt32 GetTextureID() const { return m_uiTextureID; }

  ezColor m_Color;
  ezUInt32 m_uiTextureID;
};

typedef ezResourceHandle<ColorResource> ColorResourceHandle;


struct GameCellData
{
  GameCellData()
  {
    m_iCellType = 0;
    m_uiVisibility = 0;
    m_iThreat = 0;
  }

  ezUInt8 m_uiVisibility;
  ezInt8 m_iCellType;
  ezComponentHandle m_hUnit;
  ezInt32 m_iThreat;
  ColorResourceHandle m_hColorResource;
};

typedef ezGameGrid<GameCellData> GameGrid;

class Level
{
public:
  Level();
  ~Level();

  void SetupLevel();
  void Update();

  ezWorld* GetWorld() { return m_pWorld; }
  const ezWorld* GetWorld() const { return m_pWorld; }

  GameGrid& GetGrid() { return m_GameGrid; }
  const GameGrid& GetGrid() const { return m_GameGrid; }

  ezGridNavmesh& GetNavmesh() { return m_Navmesh; }
  const ezGridNavmesh& GetNavmesh() const { return m_Navmesh; }

  /// Our factory that builds all the different unit types (GameObject + components)
  ezGameObjectHandle CreateUnit(UnitType::Enum Type, const ezVec3& vPosition, const ezQuat& qRotation = ezQuat::IdentityQuaternion(), float fScaling = 1.0f);

private:
  void CreateComponentManagers();
  void CreateRandomLevel();

  static bool IsSameCellType(ezUInt32 uiCell1, ezUInt32 uiCell2, void* pPassThrough);
  static bool IsCellBlocked(ezUInt32 uiCell, void* pPassThrough);
  static ezCallbackResult::Enum SetPointBlocking(ezInt32 x, ezInt32 y, void* pPassThrough);

  ezGameObject* CreateGameObject(const ezVec3& vPosition, const ezQuat& qRotation, float fScaling);
  ezGameObjectHandle CreateUnit_Default(const ezVec3& vPosition, const ezQuat& qRotation, float fScaling);

  GameGrid m_GameGrid;
  ezGridNavmesh m_Navmesh;
  ezWorld* m_pWorld;
};


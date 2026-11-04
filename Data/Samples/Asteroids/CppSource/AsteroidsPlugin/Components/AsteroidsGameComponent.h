#pragma once

#include <AsteroidsPlugin/AsteroidsPluginDLL.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>

class AsteroidComponent;
class ShipComponent;
class AsteroidsGameComponent;
using AsteroidsGameComponentManager = ezComponentManagerSimple<AsteroidsGameComponent, ezComponentUpdateType::WhenSimulating>;

/// Implements the rules of the game: waves of asteroids, collision detection, score and lives.
///
/// The main scene has exactly one of these. It only references what it spawns itself and the sounds that belong to the game as a whole.
/// Everything that belongs to a specific object, such as its explosion and sounds, is selected on that object's prefab.
///
/// The HUD does not know about this code. On the same object there is an ezBlackboardComponent and an ezRmlUiCanvas2DComponent.
/// This component writes the game state into the blackboard, and UI/Hud.rml decides what to display for it through data binding.
///
/// The camera has to look straight down at the XY plane and needs the global key 'MainCamera'.
///
/// Since the game plays in a plane and only with a few dozen objects, it doesn't use a physics engine.
/// Every object is a circle in the XY plane and all possible overlaps are tested once per frame.
class AsteroidsGameComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(AsteroidsGameComponent, ezComponent, AsteroidsGameComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // AsteroidsGameComponent

public:
  AsteroidsGameComponent();
  ~AsteroidsGameComponent();

  void Update();

  /// Moves an object by its velocity for this frame.
  ///
  /// When the object has completely left the visible area on one side, it is moved to the opposite side.
  /// Without a game component in the world, for example in a prefab preview, it just keeps flying.
  static void MoveAndWrap(ezGameObject* pObject, const ezVec3& vVelocity, float fRadius);

  /// Instantiates a prefab and returns its root object. Returns null if the handle is invalid.
  static ezGameObject* SpawnPrefab(ezWorld* pWorld, const ezPrefabResourceHandle& hPrefab, const ezTransform& transform);

  ezPrefabResourceHandle m_hShipPrefab;         // [ property ]
  ezHybridArray<ezString, 4> m_AsteroidPrefabs; // [ property ] one of these is picked at random for every asteroid
  ezHybridArray<ezString, 4> m_PickupPrefabs;   // [ property ] one of these is picked at random for every pickup
  ezString m_sWaveClearedSound;                 // [ property ]
  ezString m_sGameOverSound;                    // [ property ]

  float m_fPickupChance = 0.12f;   // [ property ] chance per destroyed asteroid
  ezUInt32 m_uiStartLives = 3;     // [ property ]
  ezUInt32 m_uiStartAsteroids = 3; // [ property ] number of large asteroids in the first wave

private:
  enum class Phase : ezUInt8
  {
    Title,       ///< Asteroids float by, waiting for the player to press fire.
    Playing,     ///< The ship is in the game.
    Respawning,  ///< The ship was destroyed, waiting for the player to press fire when it looks safe enough.
    WaveCleared, ///< Short pause until the next wave comes in.
    GameOver,    ///< Out of lives, waiting for the player to press fire.
  };

  struct AsteroidHit
  {
    AsteroidComponent* m_pAsteroid;
    ezVec3 m_vDirection;
  };

  void SetPhase(Phase phase);
  void StartNewGame();
  void StartWave();
  void SpawnAsteroidsAtEdge(ezUInt32 uiNumAsteroids, float fSpeedFactor);

  void UpdatePlayfield();
  void UpdateCameraShake(float fTimeDiff);
  void UpdatePhase();
  void UpdateCollisions();
  void UpdateProjectileHits(ezDynamicArray<AsteroidHit>& inout_hits);
  void UpdateShipCollisions(ezDynamicArray<AsteroidHit>& inout_hits);
  void CheckWaveCleared();
  void UpdateHud();

  void AddScore(ezUInt32 uiPoints);
  void DestroyAsteroid(AsteroidComponent* pAsteroid, const ezVec3& vHitDirection);
  void DestroyShip(ShipComponent* pShip);
  void TrySpawnPickup(const ezVec3& vPosition);

  void SpawnShip();
  void SpawnAsteroid(ezUInt8 uiSize, const ezVec3& vPosition, const ezVec3& vVelocity);
  ezGameObject* SpawnRandomPrefab(ezArrayPtr<const ezPrefabResourceHandle> prefabs, const ezVec3& vPosition);
  void ClearObjects();
  void PlaySound(const ezString& sSound);

  ezDynamicArray<ezPrefabResourceHandle> m_hAsteroidPrefabs;
  ezDynamicArray<ezPrefabResourceHandle> m_hPickupPrefabs;

  Phase m_Phase = Phase::Title;
  ezTime m_PhaseTimer;

  ezUInt32 m_uiScore = 0;
  ezUInt32 m_uiHighScore = 0;
  ezUInt32 m_uiLives = 0;
  ezUInt32 m_uiWave = 0;
  ezUInt32 m_uiNextExtraLife = 0;

  ezVec2 m_vHalfExtents = ezVec2(53.0f, 30.0f);

  float m_fShake = 0.0f;
  ezVec3 m_vCameraBasePosition = ezVec3::MakeZero();
  ezGameObjectHandle m_hCamera;
  ezGameObjectHandle m_hShip;

  ezSharedPtr<ezBlackboard> m_pHud;
};

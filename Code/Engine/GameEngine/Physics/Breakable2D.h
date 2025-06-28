#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec2.h>
#include <GameEngine/GameEngineDLL.h>

class ezRandom;

/// \brief The pattern with which to break the shard
enum class ezBreakablePattern
{
  None = 0,
  Radial = EZ_BIT(0),   ///< A radial pattern, like breaking glass.
  Cellular = EZ_BIT(1), ///< Voronoi cells, for wood / stone or smaller glass shards.
  All = 0xFF,
};

/// \brief State of a single broken shard.
struct EZ_GAMEENGINE_DLL ezBreakableShard2D
{
  /// \brief Index for shard edges that are unsupported (need to fall off).
  static constexpr ezUInt32 LooseEdge = ezInvalidIndex;
  /// \brief Index for shard edges that are supported (don't fall off), e.g. because they touch a fixed border.
  static constexpr ezUInt32 FixedEdge = ezInvalidIndex - 1;

  struct Edge
  {
    ezVec2 m_vStartPosition;                  ///< Local position where the edge starts. End point is defined by the next edge.
    ezUInt32 m_uiOutsideShardIdx = LooseEdge; ///< Which other shard connects to this edge. Used to determine whether a shard is still supported by other shards.
  };

  /// Whether this shard is already destroyed, and should not be used/displayed any further.
  bool m_bShattered = false;

  /// Whether this shard should be physically simulated.
  bool m_bDynamic = false;

  /// If not yet broken, this mask of ezBreakablePattern bits defines which patterns can be used to break it further.
  /// Small pieces may not be broken up further, medium sized ones can only use the Cellular pattern.
  ezUInt8 m_uiBreakablePatterns = 0;

  /// Center position and radius, used for culling / proximity detection.
  ezVec2 m_vCenterPosition;
  float m_fBoundingRadius = 0.0f;

  /// The edges that make up the convex (!) shape of the shard.
  ezHybridArray<Edge, 6> m_Edges;
};

/// \brief A 2-dimensional shape that can be broken up into many pieces (shards) using different break patterns.
///
/// This class handles breaking 2D shapes into convex shards using various patterns.
/// It only manages the geometry of the break patterns and does not perform physics simulation.
/// The resulting shards can be used by physics systems to simulate the broken pieces.
///
/// Key features:
/// - Supports radial and cellular (Voronoi) break patterns
/// - Tracks connections between shards to determine which pieces should fall
/// - Allows progressive breaking with different patterns based on shard size
/// - Provides culling information via bounding circles
///
/// Usage:
/// 1. Create instance and call Initialize()
/// 2. Configure initial unbroken shape via first shard,
/// 3. Call ShatterShard() to break pieces at impact points
/// 4. Use RecalculateDynamic() to determine which pieces should fall
/// 5. Feed shard geometry to physics system for simulation
class EZ_GAMEENGINE_DLL ezBreakable2D
{
public:
  ezBreakable2D();
  ~ezBreakable2D();

  /// \brief Resets all state back to the default.
  void Clear();

  /// \brief Clears the state and sets up a single shard to begin with.
  /// Afterwards the calling code has to configure that shard's shape.
  void Initialize();

  /// \brief Sets the given shard to 'shattered' (destroyed).
  void RemoveShard(ezUInt32 uiShardIdx);

  /// \brief Breaks the given shard further apart, using one of the allowed break patterns.
  ///
  /// \param vShatterPosition The world position where the impact occurred that causes the shard to break
  /// \param fImpactRadius The radius of the impact area - affects how far the break pattern spreads
  /// \param fCellSize For cellular break patterns, defines the approximate size of the resulting shards
  /// \param uiAllowedBreakPatterns A mask of ezBreakablePattern bits that defines which patterns may be used
  ///
  /// When a shard is shattered:
  /// * For Radial patterns: Creates a radial break pattern originating from vShatterPosition
  /// * For Cellular patterns: Breaks the shard into roughly equally sized pieces using Voronoi cells
  /// * The shard must have the respective pattern enabled in m_uiBreakablePatterns
  /// * The resulting pieces inherit allowed break patterns based on their size
  /// * Small pieces will have no break patterns enabled (can't break further)
  /// * Medium pieces will only allow cellular patterns
  /// * Large pieces allow all patterns
  void ShatterShard(ezUInt32 uiShardIdx, const ezVec2& vShatterPosition, ezRandom& ref_rng, float fImpactRadius, float fCellSize, ezUInt8 uiAllowedBreakPatterns = (ezUInt8)ezBreakablePattern::All);

  /// \brief Shatters all shards using the cellular pattern and optionally sets them all to dynamic.
  void ShatterAll(float fShardSize, ezRandom& ref_rng, bool bMakeAllDynamic);

  /// \brief Calculates which shards are unsupported (not directly or indirectly connected to a fixed edge) and sets them to dynamic.
  void RecalculateDymamic();

  /// The maximum radius of all active shards.
  float m_fMaxRadius = 0.0f;

  /// List of all shards that make up this breakable 2D object
  ///
  /// Each shard represents a convex piece of the overall shape.
  /// - When unbroken, contains just a single shard representing the whole shape
  /// - After breaking, contains multiple shards representing the broken pieces
  /// - Shattered/destroyed shards remain in the list but are marked with m_bShattered=true
  /// - Dynamic shards (m_bDynamic=true) are intended to be physically simulated, but this must be handled by other code
  /// - Each shard's edges define its shape and connections to neighboring shards
  ezDeque<ezBreakableShard2D> m_Shards;

private:
  struct ClipPlane
  {
    ezPlane m_Plane;
    ezUInt32 m_uiOutsideShardIdx = ezInvalidIndex;
  };

  void ShatterWithRadialPattern(ezArrayPtr<const ClipPlane> clipPlanes, const ezVec2& vShatterPosition, ezRandom& ref_rng, float fImpactRadius);
  bool ShatterWithCellularPattern(ezUInt32 uiShardIdx, ezArrayPtr<const ClipPlane> clipPlanes, ezRandom& ref_rng, float fShardSize);
  ezUInt32 AddShard(ezArrayPtr<const ClipPlane> clipPlanes, ezArrayPtr<ezBreakableShard2D::Edge> shape);
  void GenerateRingVertices(ezDynamicArray<ezVec2>& vertices, const ezVec2& vCenter, ezArrayPtr<float> radii, const ezArrayPtr<ezQuat> qRotations);
  void GenerateRingShards(ezArrayPtr<const ClipPlane> clipPlanes, ezArrayPtr<ezVec2> innerVertices, ezArrayPtr<ezVec2> outerVertices, ezArrayPtr<ezUInt32> prevShardIDs, ezDynamicArray<ezUInt32>& out_ShardIDs);
};

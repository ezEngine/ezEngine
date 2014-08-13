#pragma once

#include <CoreUtils/DataStructures/DynamicTree/Implementation/DynamicTree.h>

/// \brief A loose Octree implementation that is very lightweight on RAM.
///
/// This Octree does not store any bookkeeping information per node.\n
/// Memory usage is linear in the number of objects inserted.\n
/// An empty tree only needs few bytes. This is accomplished by making the tree
/// static in it's dimensions and maximum subdivisions, such that each node can be assigned
/// a unique index. A map is then used to store objects through the node-index in 
/// which they are located.\n
/// At traversals each node's bounding-box needs to be computed on-the-fly thus adding some
/// CPU overhead (though, fewer memory use usually also means fewer cache-misses).
/// \n
/// Inserting an object is O(log d), with d being the tree-depth.\n
/// Removing an object is either O(1) or O(n), with n being the number of objects inserted,
/// depending on whether an iterator to the object is available.\n
/// \n
/// The nodes get indices in such a way that when it is detected, that a whole subtree is
/// visible, all objects can be returned quickly, without further traversal.\n
/// If a subtree does not contain any data, traversal is not continued further, either.\n
/// \n
/// In general this octree implementation is made to be very flexible and easily usable for
/// many kinds of problems. All it stores are two integers for an object (GroupID, InstanceID).
/// The object data itself must be stored somewhere else. You can easily store very different 
/// types of objects in the same tree.\n
/// Once objects are inserted, you can do range queries to find all objects in some location.
/// Since removal is usually O(1) and insertion is O(d) the tree can be used for very dynamic
/// data that changes frequently at run-time.
class EZ_COREUTILS_DLL ezDynamicOctree
{
  /// \brief The amount that cells overlap (this is a loose octree). Typically set to 10%.
  static const float s_LooseOctreeFactor;

public:
  ezDynamicOctree();

  /// \brief Initializes the tree with a fixed size and minimum node dimensions.
  ///
  /// \param vCenter
  ///   The center position of the tree.
  /// \param vHalfExtents
  ///   The dimensions along each axis. The tree is always axis aligned. The tree will enclose all space from
  ///   (vCenter - vHalfExtents) to (vCenter + vHalfExtents).
  ///   Note that the tree will always use the maximum extent for all three extents, making the tree square,
  ///   which affects the total number of nodes.\n
  ///   However, the original (non-square) bounding box is used to determine whether some objects is inside the tree, at all,
  ///   which might affect whether they are rejected during tree insertion.
  /// \param fMinNodeSize
  ///   The length of the cell's edges at the finest level. For a typical game world, where your level might
  ///   have extents of 100 to 1000 meters, the min node size should not be smaller than 1 meter.
  ///   The smaller the node size, the more cells the tree has. The limit of nodes in the tree is 2^32.
  ///   A tree with 100 meters extents in X, Y and Z direction and a min node size of 1 meter, will have 1000000 nodes
  ///   on the finest level (and roughly 1500000 nodes in total).
  void CreateTree(const ezVec3& vCenter, const ezVec3& vHalfExtents, float fMinNodeSize); // [tested]

  /// \brief Returns true when there are no objects stored inside the tree.
  bool IsEmpty() const { return m_NodeMap.IsEmpty(); } // [tested]

  /// \brief Returns the number of objects that have been inserted into the tree.
  ezUInt32 GetCount() const { return m_NodeMap.GetCount(); } // [tested]

  /// \brief Adds an object at position vCenter with bounding-box dimensions vHalfExtents to the tree. If the object is outside the tree and bOnlyIfInside is true, nothing will be inserted.
  ///
  /// Returns EZ_SUCCESS when an object is inserted, EZ_FAILURE when the object was rejected. The latter can only happen when bOnlyIfInside is set to true.
  /// Through out_Object the exact identifier for the object in the tree is returned, which allows for removing the object with O(1) complexity later.
  /// iObjectType and iObjectInstance are the two user values that will be stored for the object. With RemoveObjectsOfType() one can also remove all objects
  /// with the same iObjectType value, if needed.
  ezResult InsertObject(const ezVec3& vCenter, const ezVec3& vHalfExtents, ezInt32 iObjectType, ezInt32 iObjectInstance, ezDynamicTreeObject* out_Object = nullptr, bool bOnlyIfInside = false); // [tested]

  /// \brief Calls the Callback for every object that is inside the View-frustum. pPassThrough is passed to the Callback for custom purposes.
  void FindVisibleObjects (const ezFrustum& Viewfrustum, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough) const;

  /// \brief Returns all objects that are located in a node that overlaps with the given point.
  ///
  /// \note This function will most likely also return objects that do not overlap with the point itself, because they are located
  /// in a node that overlaps with the point. You might need to do more thorough overlap checks to filter those out.
  void FindObjectsInRange(const ezVec3& vPoint, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough = nullptr) const; // [tested]

  /// \brief Returns all objects that are located in a node that overlaps with the rectangle with center vPoint and half edge length fRadius.
  ///
  /// \note This function will most likely also return objects that do not overlap with the rectangle itself, because they are located
  /// in a node that overlaps with the rectangle. You might need to do more thorough overlap checks to filter those out.
  void FindObjectsInRange(const ezVec3& vPoint, float fRadius, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough = nullptr) const; // [tested]

  /// \brief Removes the given Object. Attention: This is an O(n) operation.
  void RemoveObject(ezInt32 iObjectType, ezInt32 iObjectInstance); // [tested]

  /// \brief Removes the given Object. This is an O(1) operation.
  void RemoveObject(ezDynamicTreeObject obj); // [tested]

  /// \brief Removes all Objects of the given Type. This is an O(n) operation.
  void RemoveObjectsOfType(ezInt32 iObjectType); // [tested]

  /// \brief Removes all Objects, but the tree stays intact.
  void RemoveAllObjects() { m_NodeMap.Clear(); m_uiMultiMapCounter = 1; } // [tested]

  /// \brief Returns the tree's adjusted (square) AABB.
  const ezBoundingBox& GetBoundingBox() const { return m_BBox; } // [tested]

private:

  /// \brief Recursively checks in which node an object is located and stores it at the node where it fits best.
  bool InsertObject(const ezVec3& vCenter, const ezVec3& vHalfExtents, const ezDynamicTree::ezObjectData& Obj, float minx, float maxx, float miny, float maxy, float minz, float maxz, ezUInt32 uiNodeID, ezUInt32 uiAddID, ezUInt32 uiSubAddID, ezDynamicTreeObject* out_Object);

  /// \brief Recursively checks which nodes are visible and calls the callback for each object at those nodes.
  void FindVisibleObjects (const ezFrustum& Viewfrustum, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx, float miny, float maxy, float minz, float maxz, ezUInt32 uiNodeID, ezUInt32 uiAddID, ezUInt32 uiSubAddID, ezUInt32 uiNextNodeID) const;

  /// \brief Recursively checks in which node a point is located and calls the callback for all objects at those nodes.
  bool FindObjectsInRange(const ezVec3& vPoint, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx, float miny, float maxy, float minz, float maxz, ezUInt32 uiNodeID, ezUInt32 uiAddID, ezUInt32 uiSubAddID, ezUInt32 uiNextNodeID) const;

  /// \brief Recursively checks which node(s) a circle touches and calls the callback for all objects at those nodes.
  bool FindObjectsInRange(const ezVec3& vPoint, float fRadius, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx, float miny, float maxy, float minz, float maxz, ezUInt32 uiNodeID, ezUInt32 uiAddID, ezUInt32 uiSubAddID, ezUInt32 uiNextNodeID) const;

  /// \brief The tree depth, used for finding a nodes unique ID
  ezUInt32 m_uiMaxTreeDepth;

  // \brief Also used for finding a nodes unique ID
  ezUInt32 m_uiAddIDTopLevel;

  /// \brief The square bounding Box (to prevent long thin nodes) 
  ezBoundingBox m_BBox;

  /// \brief The actual bounding box (to discard objects that are outside the world)
  float m_fRealMinX, m_fRealMaxX, m_fRealMinY, m_fRealMaxY, m_fRealMinZ, m_fRealMaxZ;

  /// \brief Used to turn the map into a multi-map.
  ezUInt32 m_uiMultiMapCounter;

  /// \brief Every node has a unique index, the map allows to store many objects at each node, using that index
  ezMap<ezDynamicTree::ezMultiMapKey, ezDynamicTree::ezObjectData> m_NodeMap;
};



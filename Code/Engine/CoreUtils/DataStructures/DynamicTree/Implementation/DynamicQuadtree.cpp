#include <CoreUtils/PCH.h>
#include <CoreUtils/DataStructures/DynamicTree/DynamicQuadtree.h>

const float ezDynamicQuadtree::s_LooseOctreeFactor = 1.1f;

ezDynamicQuadtree::ezDynamicQuadtree() : m_uiMaxTreeDepth (0), m_uiAddIDTopLevel (0)
{
}

void ezDynamicQuadtree::CreateTree(const ezVec3& vCenter, const ezVec3& vHalfExtents, float fMinNodeSize)
{
  m_uiMultiMapCounter = 1;

  m_NodeMap.Clear();

  // the real bounding box might be long and thing -> bad node-size
  // but still it can be used to reject inserting objects that are entirely outside the world
  m_fRealMinX = vCenter.x - vHalfExtents.x;
  m_fRealMaxX = vCenter.x + vHalfExtents.x;
  m_fRealMinZ = vCenter.z - vHalfExtents.z;
  m_fRealMaxZ = vCenter.z + vHalfExtents.z;

  // the bounding box should be square, so use the maximum of the x and z extents
  float fMax = ezMath::Max(vHalfExtents.x, vHalfExtents.z);

  m_BBox.SetInvalid();

  m_BBox.m_vMin.x = vCenter.x - fMax;
  m_BBox.m_vMax.x = vCenter.x + fMax;
  m_BBox.m_vMin.z = vCenter.z - fMax;
  m_BBox.m_vMax.z = vCenter.z + fMax;


  // compute the maximum tree-depth
  float fLength = fMax * 2.0f;

  m_uiMaxTreeDepth = 0;
  while (fLength > fMinNodeSize)
  {
    ++m_uiMaxTreeDepth;
    fLength = (fLength / 2.0f) * s_LooseOctreeFactor;
  }

  // at every tree depth there are pow(4, depth) additional nodes, each node needs a unique ID
  // therefore compute the maximum ID that is used, for later reference
  m_uiAddIDTopLevel = 0;
  for (ezUInt32 i = 0; i < m_uiMaxTreeDepth; ++i)
    m_uiAddIDTopLevel += ezMath::Pow(4, i);

}

/// The object lies at vCenter and has vHalfExtents as its bounding box.
/// If bOnlyIfInside is false, the object is ALWAYS inserted, even if it is outside the tree.
/// \note In such a case it is inserted at the root-node and thus ALWAYS returned in range/view-frustum queries.
/// 
/// If bOnlyIfInside is true, the object is discarded, if it is not inside the actual bounding box of the tree.
/// 
/// The min and max Y value of the tree's bounding box is updated, if the object lies above/below previously inserted objects.
ezResult ezDynamicQuadtree::InsertObject(const ezVec3& vCenter, const ezVec3& vHalfExtents, ezInt32 iObjectType, ezInt32 iObjectInstance, ezDynamicTreeObject* out_Object, bool bOnlyIfInside)
{
  EZ_ASSERT_DEV(m_uiMaxTreeDepth > 0, "ezDynamicQuadtree::InsertObject: You have to first create the tree.");

  if (out_Object)
    *out_Object = ezDynamicTreeObject();

  if (bOnlyIfInside)
  {
    if (vCenter.x + vHalfExtents.x < m_fRealMinX)
      return EZ_FAILURE;

    if (vCenter.x - vHalfExtents.x > m_fRealMaxX)
      return EZ_FAILURE;

    if (vCenter.z + vHalfExtents.z < m_fRealMinZ)
      return EZ_FAILURE;

    if (vCenter.z - vHalfExtents.z > m_fRealMaxZ)
      return EZ_FAILURE;
  }

  // update the bounding boxes min/max Y values
  m_BBox.m_vMin.y = ezMath::Min(m_BBox.m_vMin.y, vCenter.y - vHalfExtents.y);
  m_BBox.m_vMax.y = ezMath::Max(m_BBox.m_vMax.y, vCenter.y + vHalfExtents.y);


  ezDynamicTree::ezObjectData oData;
  oData.m_iObjectType = iObjectType;
  oData.m_iObjectInstance = iObjectInstance;

  // insert the object into the best child
  if (!InsertObject(vCenter, vHalfExtents, oData, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.z, m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, ezMath::Pow(4, m_uiMaxTreeDepth-1), out_Object))
  {
    if (!bOnlyIfInside)
    {
      ezDynamicTree::ezMultiMapKey mmk;
      mmk.m_uiKey = 0;
      mmk.m_uiCounter = m_uiMultiMapCounter++;

      auto key = m_NodeMap.Insert(mmk, oData);

      if (out_Object)
        *out_Object = key;

      return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezDynamicQuadtree::RemoveObject(ezDynamicTreeObject obj)
{
  m_NodeMap.Remove(obj);
}

void ezDynamicQuadtree::RemoveObject(ezInt32 iObjectType, ezInt32 iObjectInstance)
{
  for (ezDynamicTreeObject it = m_NodeMap.GetIterator(); it.IsValid(); ++it)
  {
    if ((it.Value().m_iObjectInstance == iObjectInstance) && (it.Value().m_iObjectType == iObjectType))
    {
      m_NodeMap.Remove(it);
      return;
    }
  }
}

void ezDynamicQuadtree::RemoveObjectsOfType(ezInt32 iObjectType)
{
  for (ezDynamicTreeObject it = m_NodeMap.GetIterator(); it.IsValid(); )
  {
    if (it.Value().m_iObjectType == iObjectType)
    {
      ezDynamicTreeObject itold = it;
      ++it;

      m_NodeMap.Remove(itold);
    }
    else
      ++it;
  }
}

bool ezDynamicQuadtree::InsertObject(const ezVec3& vCenter, const ezVec3& vHalfExtents, const ezDynamicTree::ezObjectData& Obj, float minx, float maxx, float minz, float maxz, ezUInt32 uiNodeID, ezUInt32 uiAddID, ezUInt32 uiSubAddID, ezDynamicTreeObject* out_Object)
{
  // check if object is COMPLETELY inside the node
  // if it is not entirely enclosed by the bounding box, insert it at the parent instead
  if (vCenter.x - vHalfExtents.x < minx)
    return false;
  if (vCenter.x + vHalfExtents.x > maxx)
    return false;
  if (vCenter.z - vHalfExtents.z < minz)
    return false;
  if (vCenter.z + vHalfExtents.z > maxz)
    return false;

  // if there are any child-nodes, try inserting there
  if (uiAddID > 0)
  {
    const float lx = ((maxx - minx) * 0.5f) * s_LooseOctreeFactor;
    const float lz = ((maxz - minz) * 0.5f) * s_LooseOctreeFactor;

    // to compute the ID of child node 'n', the number of children in child node 'n-1' has to be considered
    // uiAddID is the number of IDs that need to be skipped to get from the ID of child node 'n' to the ID of child 'n+1'

    // every time one goes down one step in the tree the number of child-nodes in the sub-tree is divided by 4
    // so the number of IDs to skip to get from child 'n' to 'n+1' is reduced to 1/4

    const ezUInt32 uiNodeIDBase = uiNodeID + 1;
    const ezUInt32 uiAddIDChild = uiAddID - uiSubAddID;
    const ezUInt32 uiSubAddIDChild = uiSubAddID / 4;

    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild, uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild, uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild, uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild, uiSubAddIDChild, out_Object))
      return true;
  }

  // object is inside this node, but not (completely / exclusively) inside any child nodes -> insert here
  ezDynamicTree::ezMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;
  mmk.m_uiCounter = m_uiMultiMapCounter++;

  auto key = m_NodeMap.Insert(mmk, Obj);

  if (out_Object)
    *out_Object = key;

  return true;
}

void ezDynamicQuadtree::FindVisibleObjects(const ezFrustum& Viewfrustum, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough) const
{
  EZ_ASSERT_DEV(m_uiMaxTreeDepth > 0, "ezDynamicQuadtree::FindVisibleObjects: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  FindVisibleObjects(Viewfrustum, Callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.z, m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, ezMath::Pow (4, m_uiMaxTreeDepth-1), 0xFFFFFFFF);
}

void ezDynamicQuadtree::FindVisibleObjects(const ezFrustum& Viewfrustum, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx, float minz, float maxz, ezUInt32 uiNodeID, ezUInt32 uiAddID, ezUInt32 uiSubAddID, ezUInt32 uiNextNodeID) const
{
  // build the bounding box of this node
  ezVec3 v[8];
  v[0].Set(minx, m_BBox.m_vMin.y, minz);
  v[1].Set(minx, m_BBox.m_vMin.y, maxz);
  v[2].Set(minx, m_BBox.m_vMax.y, minz);
  v[3].Set(minx, m_BBox.m_vMax.y, maxz);
  v[4].Set(maxx, m_BBox.m_vMin.y, minz);
  v[5].Set(maxx, m_BBox.m_vMin.y, maxz);
  v[6].Set(maxx, m_BBox.m_vMax.y, minz);
  v[7].Set(maxx, m_BBox.m_vMax.y, maxz);

  const ezVolumePosition::Enum pos = Viewfrustum.GetObjectPosition(&v[0], 8);

  // stop traversal, if node is outside view-frustum
  if (pos == ezVolumePosition::Outside)
    return;

  ezDynamicTree::ezMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  // get the iterator where the objects stored in this sub-tree start
  ezDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  // if there are no objects AT ALL in the map after the iterator, OR in this subtree there are no objects stored, stop
  if ((!it1.IsValid()) || (it1.Key().m_uiKey >= uiNextNodeID))
    return;

  // if the node is COMPLETELY inside the frustum -> no need to recurse further, the whole subtree will be visible
  if (pos == ezVolumePosition::Inside)
  {
    mmk.m_uiKey = uiNextNodeID;
    const ezDynamicTreeObjectConst itlast = m_NodeMap.LowerBound(mmk);

    while (it1.IsValid())
    {
      // first increase the iterator, the user could erase it in the callback
      ezDynamicTreeObjectConst temp = it1;
      ++it1;

      Callback(pPassThrough, temp);
    }

    return;
  }
  else if (pos == ezVolumePosition::Intersecting)
  {
    // the node is visible, but some parts might be outside, so refine the search

    mmk.m_uiKey = uiNodeID + 1;
    const ezDynamicTreeObjectConst itlast = m_NodeMap.LowerBound(mmk);

    // first return all objects store at this particular node
    while (it1.IsValid())
    {
      // first increase the iterator, the user could erase it in the callback
      ezDynamicTreeObjectConst temp = it1;
      ++it1;

      Callback(pPassThrough, temp);
    }

    // if there are additional child nodes
    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_LooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_LooseOctreeFactor;

      const ezUInt32 uiNodeIDBase = uiNodeID + 1;
      const ezUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const ezUInt32 uiSubAddIDChild = uiSubAddID >> 2;

      // continue the search at each child node
      FindVisibleObjects (Viewfrustum, Callback, pPassThrough, minx, minx + lx, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1);
      FindVisibleObjects (Viewfrustum, Callback, pPassThrough, minx, minx + lx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2);
      FindVisibleObjects (Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3);
      FindVisibleObjects (Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild, uiSubAddIDChild, uiNextNodeID);
    }
  }
}


void ezDynamicQuadtree::FindObjectsInRange(const ezVec3& vPoint, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough) const
{
  EZ_ASSERT_DEV(m_uiMaxTreeDepth > 0, "ezDynamicQuadtree::FindObjectsInRange: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  if (!m_BBox.Contains(vPoint))
    return;

  FindObjectsInRange(vPoint, Callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.z, m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, ezMath::Pow(4, m_uiMaxTreeDepth-1), 0xFFFFFFFF);
}

bool ezDynamicQuadtree::FindObjectsInRange(const ezVec3& vPoint, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx, float minz, float maxz, ezUInt32 uiNodeID, ezUInt32 uiAddID, ezUInt32 uiSubAddID, ezUInt32 uiNextNodeID) const
{
  if (vPoint.x < minx)
    return true;
  if (vPoint.x > maxx)
    return true;
  if (vPoint.z < minz)
    return true;
  if (vPoint.z > maxz)
    return true;

  ezDynamicTree::ezMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  ezDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  // if the sub-tree is empty, or even the whole tree is empty for the remaining nodes, stop traversal
  if (!it1.IsValid() || (it1.Key().m_uiKey >= uiNextNodeID))
    return true;

  {
    // return all objects stored at this node
    {
      ezDynamicTree::ezMultiMapKey mmk2;
      mmk2.m_uiKey = uiNodeID + 1;

      const ezDynamicTreeObjectConst itlast = m_NodeMap.LowerBound(mmk2);

      while  (it1 != itlast)
      {
        // first increase the iterator, the user could erase it in the callback
        ezDynamicTreeObjectConst temp = it1;
        ++it1;

        if (!Callback(pPassThrough, temp))
          return false;
      }
    }

    // if this node has children
    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_LooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_LooseOctreeFactor;

      const ezUInt32 uiNodeIDBase = uiNodeID + 1;
      const ezUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const ezUInt32 uiSubAddIDChild = uiSubAddID >> 2;

      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild, uiSubAddIDChild, uiNextNodeID))
        return false;
    }
  }

  return true;
}




void ezDynamicQuadtree::FindObjectsInRange(const ezVec3& vPoint, float fRadius, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough) const
{
  EZ_ASSERT_DEV(m_uiMaxTreeDepth > 0, "ezDynamicQuadtree::FindObjectsInRange: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  if (!m_BBox.Overlaps(ezBoundingBox(vPoint - ezVec3(fRadius), vPoint + ezVec3(fRadius))))
    return;

  FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.z, m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, ezMath::Pow (4, m_uiMaxTreeDepth-1), 0xFFFFFFFF);
}

bool ezDynamicQuadtree::FindObjectsInRange(const ezVec3& vPoint, float fRadius, EZ_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx, float minz, float maxz, ezUInt32 uiNodeID, ezUInt32 uiAddID, ezUInt32 uiSubAddID, ezUInt32 uiNextNodeID) const
{
  if (vPoint.x + fRadius < minx)
    return true;
  if (vPoint.x - fRadius > maxx)
    return true;
  if (vPoint.z + fRadius < minz)
    return true;
  if (vPoint.z - fRadius > maxz)
    return true;

  ezDynamicTree::ezMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  ezDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  // if the whole sub-tree doesn't contain any data, no need to check further
  if (!it1.IsValid() || (it1.Key().m_uiKey >= uiNextNodeID))
    return true;

  {

    // return all objects stored at this node
    {
      while  (it1.IsValid() && (it1.Key().m_uiKey == uiNodeID))
      {
        // first increase the iterator, the user could erase it in the callback
        ezDynamicTreeObjectConst temp = it1;
        ++it1;

        if (!Callback(pPassThrough, temp))
          return false;
      }
    }

    // if the node has children
    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_LooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_LooseOctreeFactor;

      const ezUInt32 uiNodeIDBase = uiNodeID + 1;
      const ezUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const ezUInt32 uiSubAddIDChild = uiSubAddID >> 2;

      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild, uiSubAddIDChild, uiNextNodeID))
        return false;
    }
  }

  return true;
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_DataStructures_DynamicTree_Implementation_DynamicQuadtree);


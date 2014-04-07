#include <PCH.h>
#include <Foundation/Containers/Deque.h>
#include <CoreUtils/DataStructures/DynamicTree/DynamicOctree.h>

EZ_CREATE_SIMPLE_TEST_GROUP(DataStructures);

static ezInt32 g_iSearchInstance = 0;
static bool g_bFoundSearched = false;
static ezUInt32 g_iReturned = 0;

static bool ObjectFound(void* pPassThrough, ezDynamicTreeObjectConst Object)
{
  EZ_TEST_BOOL(pPassThrough == nullptr);

  ++g_iReturned;

  if (Object.Value().m_iObjectInstance == g_iSearchInstance)
    g_bFoundSearched = true;

  // let it give us all the objects in range and count how many that are
  return true;
}

EZ_CREATE_SIMPLE_TEST(DataStructures, DynamicOctree)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CreateTree / GetBoundingBox")
  {
    ezDynamicOctree o;
    o.CreateTree(ezVec3(100, 200, 300), ezVec3(300, 400, 500), 1.0f);

    const ezBoundingBox& bb = o.GetBoundingBox();

    EZ_TEST_VEC3(bb.GetCenter(), ezVec3(100, 200, 300), 0.01f);
    EZ_TEST_VEC3(bb.GetHalfExtents(), ezVec3(500), 0.01f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert Inside / Outside")
  {
    const ezVec3 c(100, 200, 300); 
    const float e = 50;

    ezDynamicOctree o;
    o.CreateTree(c, ezVec3(e), 1.0f);
    ezInt32 iInstance = 0;

    for (float z = -e - 99; z < e + 100; z += 10.0f)
    {
      for (float y = -e - 99; y < e + 100; y += 10.0f)
      {
        for (float x = -e - 99; x < e + 100; x += 10.0f)
        {
          const bool bInside = (z > -e) && (z < e) && (y > -e) && (y < e) && (x > -e) && (x < e);

          EZ_TEST_BOOL(o.InsertObject(c + ezVec3(x, y, z), ezVec3(1.0f), 0, iInstance, nullptr, true) == (bInside ? EZ_SUCCESS : EZ_FAILURE));
          EZ_TEST_BOOL(o.InsertObject(c + ezVec3(x, y, z), ezVec3(1.0f), 0, iInstance, nullptr, false) == EZ_SUCCESS);

          ++iInstance;
        }
      }
    }
  }

  struct TestObject
  {
    ezVec3 m_vPos;
    ezVec3 m_vExtents;
    ezDynamicTreeObject m_hObject;
  };

  ezDeque<TestObject> Objects;

  {
    TestObject to;

  
    to.m_vPos.Set(-90, 50, 0);
    to.m_vExtents.Set(2.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(-90, 50, 0);
    to.m_vExtents.Set(2.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(-90, 50, 80);
    to.m_vExtents.Set(2.0f, 4.0f, 10.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(0, 0, -50);
    to.m_vExtents.Set(20.0f, 4.0f, 10.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(10, 10, 10);
    to.m_vExtents.Set(50.0f, 2.0f, 1.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(50, -20, 10);
    to.m_vExtents.Set(1.0f, 2.0f, 1.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(50, -20, 10);
    to.m_vExtents.Set(1.0f, 2.0f, 1.0f);

    Objects.PushBack(to);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindObjectsInRange(Point)")
  {
    ezDynamicOctree o;
    o.CreateTree(ezVec3::ZeroVector(), ezVec3(100), 1.0f);

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      EZ_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == EZ_SUCCESS);

      EZ_TEST_BOOL(o.IsEmpty() == false);
      EZ_TEST_INT(o.GetCount(), i + 1);
    }

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      g_iSearchInstance = i;

      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == true);
      EZ_TEST_BOOL(g_iReturned < Objects.GetCount());

      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == true);
      EZ_TEST_BOOL(g_iReturned < Objects.GetCount());

      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == true);
      EZ_TEST_BOOL(g_iReturned < Objects.GetCount());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindObjectsInRange(Radius)")
  {
    ezDynamicOctree o;
    o.CreateTree(ezVec3::ZeroVector(), ezVec3(100), 1.0f);

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      EZ_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == EZ_SUCCESS);

      EZ_TEST_BOOL(o.IsEmpty() == false);
      EZ_TEST_INT(o.GetCount(), i + 1);
    }

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      g_iSearchInstance = i;

      // point inside object

      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == true);
      EZ_TEST_BOOL(g_iReturned < Objects.GetCount());

      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, 1.0f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == true);
      EZ_TEST_BOOL(g_iReturned < Objects.GetCount());

      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, 1.0f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == true);
      EZ_TEST_BOOL(g_iReturned < Objects.GetCount());

      // point outside object

      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents + ezVec3(2, 0, 0), 2.5f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == true);
      EZ_TEST_BOOL(g_iReturned < Objects.GetCount());

      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents - ezVec3(0, 2, 0), 2.5f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == true);
      EZ_TEST_BOOL(g_iReturned < Objects.GetCount());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveObject(handle)")
  {
    ezDynamicOctree o;
    o.CreateTree(ezVec3::ZeroVector(), ezVec3(100), 1.0f);

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      EZ_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == EZ_SUCCESS);

      EZ_TEST_BOOL(o.IsEmpty() == false);
      EZ_TEST_INT(o.GetCount(), i + 1);
    }

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      g_iSearchInstance = i;

      o.RemoveObject(Objects[i].m_hObject);

      // one less in the tree
      EZ_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == false);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveObject(index)")
  {
    ezDynamicOctree o;
    o.CreateTree(ezVec3::ZeroVector(), ezVec3(100), 1.0f);

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      EZ_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == EZ_SUCCESS);

      EZ_TEST_BOOL(o.IsEmpty() == false);
      EZ_TEST_INT(o.GetCount(), i + 1);
    }

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      g_iSearchInstance = i;

      o.RemoveObject(i, i + 1);

      // one less in the tree
      EZ_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == false);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveObjectsOfType")
  {
    ezDynamicOctree o;
    o.CreateTree(ezVec3::ZeroVector(), ezVec3(100), 1.0f);

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      EZ_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == EZ_SUCCESS);

      EZ_TEST_BOOL(o.IsEmpty() == false);
      EZ_TEST_INT(o.GetCount(), i + 1);
    }

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      g_iSearchInstance = i + 1;

      o.RemoveObjectsOfType(i);

      // one less in the tree
      EZ_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      g_iReturned = 0;
      g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, ObjectFound, nullptr);
      EZ_TEST_BOOL(g_bFoundSearched == false);
    }

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      EZ_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == EZ_SUCCESS);

      EZ_TEST_BOOL(o.IsEmpty() == false);
      EZ_TEST_INT(o.GetCount(), i + 1);
    }

    o.RemoveObjectsOfType(0);

    EZ_TEST_BOOL(o.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveAllObjects")
  {
    ezDynamicOctree o;
    o.CreateTree(ezVec3::ZeroVector(), ezVec3(100), 1.0f);

    for (ezUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      EZ_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == EZ_SUCCESS);

      EZ_TEST_BOOL(o.IsEmpty() == false);
      EZ_TEST_INT(o.GetCount(), i + 1);
    }

    o.RemoveAllObjects();
    EZ_TEST_BOOL(o.IsEmpty());
    EZ_TEST_INT(o.GetCount(), 0);
  }
}






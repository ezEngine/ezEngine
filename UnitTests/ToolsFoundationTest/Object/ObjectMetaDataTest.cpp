#include <PCH.h>

#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

static int a = 0, b = 1, c = 2, d = 3;

EZ_CREATE_SIMPLE_TEST(DocumentObject, ObjectMetaData)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Pointers / int")
  {
    ezObjectMetaData<void*, ezInt32> meta;

    EZ_TEST_BOOL(!meta.HasMetaData(&a));
    EZ_TEST_BOOL(!meta.HasMetaData(&b));
    EZ_TEST_BOOL(!meta.HasMetaData(&c));
    EZ_TEST_BOOL(!meta.HasMetaData(&d));

    {
      auto pData = meta.BeginModifyMetaData(&a);
      *pData = a;
      meta.EndModifyMetaData();

      pData = meta.BeginModifyMetaData(&b);
      *pData = b;
      meta.EndModifyMetaData();

      pData = meta.BeginModifyMetaData(&c);
      *pData = c;
      meta.EndModifyMetaData();
    }

    EZ_TEST_BOOL(meta.HasMetaData(&a));
    EZ_TEST_BOOL(meta.HasMetaData(&b));
    EZ_TEST_BOOL(meta.HasMetaData(&c));
    EZ_TEST_BOOL(!meta.HasMetaData(&d));

    {
      auto pDataR = meta.BeginReadMetaData(&a);
      EZ_TEST_INT(*pDataR, a);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&b);
      EZ_TEST_INT(*pDataR, b);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&c);
      EZ_TEST_INT(*pDataR, c);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&d);
      EZ_TEST_INT(*pDataR, 0);
      meta.EndReadMetaData();
    }
  }

  struct md
  {
    md() { b = false; }

    ezString s;
    bool b;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UUID / struct")
  {
    ezObjectMetaData<ezUuid, md> meta;

    const int num = 100;

    ezDynamicArray<ezUuid> obj;
    obj.SetCount(num);

    for (ezUInt32 i = 0; i < num; ++i)
    {
      ezUuid& uid = obj[i];
      uid.CreateNewUuid();

      if (ezMath::IsEven(i))
      {
        auto d = meta.BeginModifyMetaData(uid);
        d->b = true;
        d->s = "test";

        meta.EndModifyMetaData();
      }

      EZ_TEST_BOOL(meta.HasMetaData(uid) == ezMath::IsEven(i));
    }

    for (ezUInt32 i = 0; i < num; ++i)
    {
      const ezUuid& uid = obj[i];

      auto p = meta.BeginReadMetaData(uid);

      EZ_TEST_BOOL(p->b == ezMath::IsEven(i));

      if (ezMath::IsEven(i))
      {
        EZ_TEST_STRING(p->s, "test");
      }
      else
      {
        EZ_TEST_BOOL(p->s.IsEmpty());
      }

      meta.EndReadMetaData();
      EZ_TEST_BOOL(meta.HasMetaData(uid) == ezMath::IsEven(i));
    }
  }
}

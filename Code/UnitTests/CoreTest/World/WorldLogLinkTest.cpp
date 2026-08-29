#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldLogLink.h>

EZ_CREATE_SIMPLE_TEST(World, WorldLogLink)
{
  ezWorldDesc worldDesc("WorldLogLinkTest");
  ezWorld world(worldDesc);
  EZ_LOCK(world.GetWriteMarker());

  ezGameObjectDesc objDesc;
  objDesc.m_sName.Assign("LinkedObject");

  ezGameObject* pObject = nullptr;
  const ezGameObjectHandle hObject = world.CreateObject(objDesc, pObject);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Game object link")
  {
    ezStringBuilder sMsg;
    sMsg.SetFormat("Object {} is broken.", ezArgGameObject(hObject));

    // The display text falls back to the object's name.
    EZ_TEST_BOOL(sMsg.FindSubString("[[LinkedObject|ezObject:") != nullptr);
    EZ_TEST_BOOL(sMsg.EndsWith("]] is broken."));

    ezStringBuilder sExplicit;
    sExplicit.SetFormat("{}", ezArgGameObject(pObject, "Custom Text"));
    EZ_TEST_BOOL(sExplicit.StartsWith("[[Custom Text|ezObject:"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Game object round trip")
  {
    ezStringBuilder sMsg;
    sMsg.SetFormat("{}", ezArgGameObject(hObject));

    const char* szSeparator = sMsg.FindSubString("|");
    EZ_TEST_BOOL(szSeparator != nullptr);

    const ezStringView sTarget(szSeparator + 1, sMsg.GetView().GetEndPointer() - 2);

    ezGameObjectHandle hParsed;
    EZ_TEST_BOOL(ezWorldLogLinkUtils::ParseGameObjectLink(sTarget, hParsed));
    EZ_TEST_BOOL(hParsed == hObject);

    // A component link must not be accepted for a game object target and vice versa.
    ezComponentHandle hWrong;
    EZ_TEST_BOOL(!ezWorldLogLinkUtils::ParseComponentLink(sTarget, hWrong));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Multiple links in one message")
  {
    ezStringBuilder sMsg;
    sMsg.SetFormat("{} and {}", ezArgGameObject(hObject, "First"), ezArgGameObject(hObject, "Second"));

    // Both arguments are built before the message is assembled, so they must not overwrite each other.
    EZ_TEST_BOOL(sMsg.StartsWith("[[First|ezObject:"));
    EZ_TEST_BOOL(sMsg.FindSubString("[[Second|ezObject:") != nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invalid handles")
  {
    ezStringBuilder sMsg;
    sMsg.SetFormat("{}", ezArgGameObject(ezGameObjectHandle()));

    // Nothing to derive a name from, so the handle value is used as the display text.
    EZ_TEST_BOOL(sMsg.StartsWith("[["));
    EZ_TEST_BOOL(sMsg.EndsWith("]]"));

    ezGameObjectHandle hParsed;
    EZ_TEST_BOOL(!ezWorldLogLinkUtils::ParseGameObjectLink("ezObject:notahexnumber", hParsed));
    EZ_TEST_BOOL(!ezWorldLogLinkUtils::ParseGameObjectLink("asset:{ 00000000-0000-0000-0000-000000000000 }", hParsed));

    // An invalid component handle must not be looked up either, its world index doesn't refer to a live world.
    ezStringBuilder sComponentMsg;
    sComponentMsg.SetFormat("{}", ezArgComponent(ezComponentHandle()));
    EZ_TEST_BOOL(sComponentMsg.StartsWith("[["));
    EZ_TEST_BOOL(sComponentMsg.EndsWith("]]"));
  }
}

#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>

struct AddMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(AddMessage);

  ezInt32 m_iValue;
};
EZ_IMPLEMENT_MESSAGE_TYPE(AddMessage);

struct SubMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(SubMessage);

  ezInt32 m_iValue;
};
EZ_IMPLEMENT_MESSAGE_TYPE(SubMessage);

struct MulMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(MulMessage);

  ezInt32 m_iValue;
};
EZ_IMPLEMENT_MESSAGE_TYPE(MulMessage);

class BaseHandler : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(BaseHandler);

public:
  BaseHandler() : m_iValue(0) 
  { 
  }

  void OnAddMessage(AddMessage& msg)
  {
    m_iValue += msg.m_iValue;
  }

  void OnMulMessage(MulMessage& msg)
  {
    m_iValue *= msg.m_iValue;
  }

  ezInt32 m_iValue;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(BaseHandler, ezReflectedClass, ezRTTINoAllocator);
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(AddMessage, OnAddMessage),
    EZ_MESSAGE_HANDLER(MulMessage, OnMulMessage)
  EZ_END_MESSAGEHANDLERS
EZ_END_DYNAMIC_REFLECTED_TYPE();

class DerivedHandler : public BaseHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(DerivedHandler);

public:
  void OnAddMessage(AddMessage& msg)
  {
    m_iValue += msg.m_iValue * 2;
  }

  void OnSubMessage(SubMessage& msg)
  {
    m_iValue -= msg.m_iValue;
  }
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(DerivedHandler, BaseHandler, ezRTTINoAllocator);
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(AddMessage, OnAddMessage),
    EZ_MESSAGE_HANDLER(SubMessage, OnSubMessage)
  EZ_END_MESSAGEHANDLERS
EZ_END_DYNAMIC_REFLECTED_TYPE();


EZ_CREATE_SIMPLE_TEST(Reflection, MessageHandler)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple Dispatch")
  {
    BaseHandler test;
    const ezRTTI* pRTTI = test.GetStaticRTTI();

    EZ_TEST_BOOL(pRTTI->CanHandleMessage<AddMessage>());
    EZ_TEST_BOOL(!pRTTI->CanHandleMessage<SubMessage>());
    EZ_TEST_BOOL(pRTTI->CanHandleMessage<MulMessage>());

    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    EZ_TEST_BOOL(handled);

    EZ_TEST_INT(test.m_iValue, 4);

    SubMessage subMsg;
    subMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, subMsg); // should do nothing
    EZ_TEST_BOOL(!handled);

    EZ_TEST_INT(test.m_iValue, 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Dispatch with inheritance")
  {
    DerivedHandler test;
    const ezRTTI* pRTTI = test.GetStaticRTTI();

    EZ_TEST_BOOL(pRTTI->CanHandleMessage<AddMessage>());
    EZ_TEST_BOOL(pRTTI->CanHandleMessage<SubMessage>());
    EZ_TEST_BOOL(pRTTI->CanHandleMessage<MulMessage>());

    // message handler overridden by derived class
    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    EZ_TEST_BOOL(handled);

    EZ_TEST_INT(test.m_iValue, 8);

    SubMessage subMsg;
    subMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, subMsg);
    EZ_TEST_BOOL(handled);

    EZ_TEST_INT(test.m_iValue, 4);

    // message handled by base class
    MulMessage mulMsg;
    mulMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, mulMsg);
    EZ_TEST_BOOL(handled);

    EZ_TEST_INT(test.m_iValue, 16);
  }
}


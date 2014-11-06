#include <PCH.h>
#include <Foundation/Reflection/Reflection.h>

#ifdef GetMessage
  #undef GetMessage
#endif

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

struct GetMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(GetMessage);

  ezInt32 m_iValue;
};
EZ_IMPLEMENT_MESSAGE_TYPE(GetMessage);



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

  void OnGetMessage(GetMessage& msg) const
  {
    msg.m_iValue = m_iValue;
  }

  ezInt32 m_iValue;
};

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(BaseHandler, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(AddMessage, OnAddMessage),
    EZ_MESSAGE_HANDLER(MulMessage, OnMulMessage),
    EZ_MESSAGE_HANDLER(GetMessage, OnGetMessage)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(DerivedHandler, BaseHandler, 1, ezRTTINoAllocator);
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
    EZ_TEST_BOOL(pRTTI->CanHandleMessage<GetMessage>());

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple Dispatch const")
  {
    const BaseHandler test;
    const ezRTTI* pRTTI = test.GetStaticRTTI();

    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    EZ_TEST_BOOL(!handled); // should do nothing since object is const and the add message handler is non-const

    EZ_TEST_INT(test.m_iValue, 0);

    GetMessage getMsg;
    getMsg.m_iValue = 12;
    handled = pRTTI->DispatchMessage(&test, getMsg);
    EZ_TEST_BOOL(handled);
    EZ_TEST_INT(getMsg.m_iValue, 0);

    EZ_TEST_INT(test.m_iValue, 0); // object must not be modified
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


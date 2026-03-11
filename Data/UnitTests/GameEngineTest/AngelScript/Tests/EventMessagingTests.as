#include "TestFramework.as"

enum Phase
{
    SendAS,
    PostAS,
    WaitPostAS,
    Done,
}

class AsTestEventMsg : ezAngelScriptMessage
{
  ScriptObject@ FromComponent = null;
  int iHandledBy0 = 0;
  int iHandledBy1 = 0;
  int iHandledBy2 = 0;
}

class AsTestEventMsg2 : ezAngelScriptMessage
{
}

class ScriptObject : ezAngelScriptTestClass
{
    int Participant = -1;
    // This number should be equal to the number of objects the message is expected to traverse upwards through the hierarchy
    // e.g. all the ancestors and the sender itself have ScriptComponent which handle AsTestEventMsg
    // then this number should be equal to the "depth" (starting with 1) of the sender component
    int ExpectedResponseCount = -1;
    
    private int m_iResponses = 0;
    private Phase m_Phase = Phase::SendAS;
    private ezTime m_waitStartTime;

    ScriptObject()
    {
        super("EventMessagingTest");
    }

    bool ExecuteTests()
    {
        EZ_TEST_BOOL(Participant != -1);
        
        if (m_Phase == Phase::SendAS)
        {
            AsTestEventMsg msg;
            @msg.FromComponent = @this;

            m_iResponses = 0;
            ezGameObject@ child = GetOwner().FindChildByName("Child");
            EZ_TEST_BOOL(child.SendEventMessage(msg, GetOwnerComponent()));
            EZ_TEST_INT(m_iResponses, ExpectedResponseCount);
            // Ugly part to verify that the message traversed the correct objects
            // The object tree is expected to be 0 -> 1 - > 2
            switch(Participant)
            {
            case 0:
            {
                EZ_TEST_INT(msg.iHandledBy0, 1);
                EZ_TEST_INT(msg.iHandledBy1, 0);
                EZ_TEST_INT(msg.iHandledBy2, 0);
                break;
            }
            case 1:
            {
                EZ_TEST_INT(msg.iHandledBy0, 1);
                EZ_TEST_INT(msg.iHandledBy1, 1);
                EZ_TEST_INT(msg.iHandledBy2, 0);
                break;
            }
            case 2:
            {
                EZ_TEST_INT(msg.iHandledBy0, 1);
                EZ_TEST_INT(msg.iHandledBy1, 1);
                EZ_TEST_INT(msg.iHandledBy1, 1);
                break;
            }
            }
            m_Phase = Phase::PostAS;
        }
        else if (m_Phase == Phase::PostAS)
        {
            AsTestEventMsg msg;
            @msg.FromComponent = @this;
            m_iResponses = 0;
            ezGameObject@ child = GetOwner().FindChildByName("Child");
            child.PostEventMessage(msg, GetOwnerComponent(), ezTime::Milliseconds(200));
            EZ_TEST_INT(m_iResponses, 0);
            EZ_TEST_INT(msg.iHandledBy0, 0);
            EZ_TEST_INT(msg.iHandledBy1, 0);

            m_waitStartTime = GetWorld().GetClock().GetAccumulatedTime();
            m_Phase = Phase::WaitPostAS;
        }
        else if (m_Phase == Phase::WaitPostAS)
        {
            const ezTime tNow = GetWorld().GetClock().GetAccumulatedTime();
            if (tNow - m_waitStartTime < ezTime::Milliseconds(200))
            {
                EZ_TEST_INT(m_iResponses, 0);
            }
            else if (tNow - m_waitStartTime > ezTime::Milliseconds(220))
            {
                EZ_TEST_INT(m_iResponses, ExpectedResponseCount);
                m_Phase = Phase::Done;
            }
        }

        return m_Phase != Phase::Done;
    }

    void OnMsgAsTestMsg(AsTestEventMsg@ msg)
    {
        switch(Participant)
        {
            case 0:
            ++msg.iHandledBy0;
            break;
            case 1:
            ++msg.iHandledBy1;
            break;
            case 2:
            ++msg.iHandledBy2;
            break;
        }

        AsTestEventMsg2 response;
        msg.FromComponent.GetOwner().SendMessage(response);
    }

    void OnMsgAsTestMsg2(AsTestEventMsg2@ msg)
    {
        ++m_iResponses;
    }    
}
#include "TestFramework.as"

enum Phase
{
    Init,

    Done
}

class ScriptObject : ezAngelScriptTestClass
{
    private Phase m_Phase = Phase::Init;

    ScriptObject()
    {
        super("MessagingTest");
    }

    bool ExecuteTests()
    {
        return false;
    }
}
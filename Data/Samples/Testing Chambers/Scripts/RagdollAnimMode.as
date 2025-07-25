class ScriptObject : ezAngelScriptClass
{
    void OnMsgGenericEvent(ezMsgGenericEvent@ msg)
    {
        if (msg.Message == "AnimMode-Powered")
        {
            ezJoltRagdollComponent@ ragdoll;
            if (GetOwner().TryGetComponentOfBaseType(@ragdoll))
            {
                ragdoll.AnimMode = ezJoltRagdollAnimMode::Powered;
                ragdoll.FadeJointMotorStrength(0.0f, ezTime::MakeFromMilliseconds(1000));
            }
        }
        else if (msg.Message == "AnimMode-Limp")
        {
            ezJoltRagdollComponent@ ragdoll;
            if (GetOwner().TryGetComponentOfBaseType(@ragdoll))
            {
                // ragdoll.AnimMode = ezJoltRagdollAnimMode::Limp;
                // ragdoll.FadeJointMotorStrength(0.0f, ezTime::MakeFromMilliseconds(500));
            }
        }
    }

    void OnSimulationStarted()
    {
            // ezJoltRagdollComponent@ ragdoll;
            // if (GetOwner().TryGetComponentOfBaseType(@ragdoll))
            // {
            //     ragdoll.SetJointMotorStrength(100);
            //     ragdoll.AnimMode = ezJoltRagdollAnimMode::Powered;
            //     ragdoll.FadeJointMotorStrength(2.0f, ezTime::MakeFromMilliseconds(5000));
            // }
    }
}

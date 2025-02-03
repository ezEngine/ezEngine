class ScriptObject :  ezAngelScriptClass
{
    void OnMsgTriggerTriggered(ezMsgTriggerTriggered@ msg)
    {
        if (msg.Message == "ActivatePaddleWheel")
        {
            if (msg.TriggerState == ezTriggerState::Activated) {

                ezGameObject@ spawn;
                if (GetWorld().TryGetObjectWithGlobalKey("PaddleWheelSpawn1", @spawn))
                {
                    spawn.SetActiveFlag(true);
                }

            }
            else if (msg.TriggerState == ezTriggerState::Deactivated) {

                ezGameObject@ spawn;
                if (GetWorld().TryGetObjectWithGlobalKey("PaddleWheelSpawn1", @spawn))
                {
                    spawn.SetActiveFlag(false);
                }

            }
        }

        if (msg.Message == "ActivateSwing") {

            if (msg.TriggerState == ezTriggerState::Activated) 
            {
                ezGameObject@ spawn;
                if (GetWorld().TryGetObjectWithGlobalKey("SwingSpawn1", @spawn))
                {
                    spawn.SetActiveFlag(true);
                }

            }
            else if (msg.TriggerState == ezTriggerState::Deactivated) 
            {
                ezGameObject@ spawn;
                if (GetWorld().TryGetObjectWithGlobalKey("SwingSpawn1", @spawn))
                {
                    spawn.SetActiveFlag(false);
                }
            }
        }
    }
}


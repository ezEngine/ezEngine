#include "../../Features/RenderToTexture/Monitor_data/Monitor.as"

class Corridor : ezAngelScriptClass
{
    private int monitor1State = 0;

    void OnMsgGenericEvent(ezMsgGenericEvent@ msg)
    {
        if (msg.Message == "SecretDoorButton")
        {
            ezGameObject@ door;
            if (GetWorld().TryGetObjectWithGlobalKey("SecretDoor", door))
            {
                ezSliderComponent@ slider;
                if (door.TryGetComponentOfBaseType(@slider) && !slider.Running)
                {
                    // slider direction toggles automatically, just need to set the running state again
                    slider.Running = true;
                }
            }
        }

        if (msg.Message == "MoveA" || msg.Message == "MoveB")
        {
            ezGameObject@ obj;
            if (GetWorld().TryGetObjectWithGlobalKey("Obj", @obj))
            {
                ezMoveToComponent@ move;
                if (obj.TryGetComponentOfBaseType(@move))
                {
                    move.Running = true;

                    if (msg.Message == "MoveA")
                    {
                        move.SetTargetPosition(ezVec3(10, -1, 1.5));
                    }
                    else
                    {
                        move.SetTargetPosition(ezVec3(10, 3, 1.5));
                    }
                }
            }
        }

        if (msg.Message == "SwitchMonitor1")
        {
            ++monitor1State;

            if (monitor1State > 2)
                monitor1State = 0;

            MsgSwitchMonitor monMsg;

            switch (monitor1State)
            {
                case 0:
                    monMsg.screenMaterial = "{ 6c56721b-d71a-4795-88ac-39cae26c39f1 }";
                    monMsg.renderTarget = "{ 2fe9db45-6e52-4e17-8e27-5744f9e8ada6 }";
                    break;
                case 1:
                    monMsg.screenMaterial = "{ eb4cb027-44b2-4f69-8f88-3d5594f0fa9d }";
                    monMsg.renderTarget = "{ 852fa58a-7bea-4486-832b-3a2b2792fea3 }";
                    break;
                case 2:
                    monMsg.screenMaterial = "{ eb842e16-7314-4f8a-8479-0f92e43ca708 }";
                    monMsg.renderTarget = "{ 673e8ea0-b70e-4e47-a72b-037d67024a71 }";
                    break;
            }

            ezGameObject@ mon;
            if (GetWorld().TryGetObjectWithGlobalKey("Monitor1", mon))
            {
                mon.SendMessageRecursive(monMsg);
            }
        }
    }
}


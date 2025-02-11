shared class MsgSwitchMonitor : ezAngelScriptMessage
{
    ezString renderTarget;
    ezString screenMaterial;
}

class Monitor : ezAngelScriptClass
{
    void OnMsgSwitchMonitor(MsgSwitchMonitor@ msg)
    {
        auto display = GetOwner().FindChildByName("Display");

        ezMsgSetMeshMaterial mat;
        mat.MaterialSlot = 0;
        mat.Material = msg.screenMaterial;

        display.SendMessage(mat);

        ezRenderTargetActivatorComponent@ activator;
        if (display.TryGetComponentOfBaseType(@activator))
        {
            activator.RenderTarget = msg.renderTarget;
        }
    }
}


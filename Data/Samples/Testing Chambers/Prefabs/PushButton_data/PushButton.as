class ScriptObject :  ezAngelScriptClass
{
    ezHashedString ButtonName;

    void OnMsgGenericEvent(ezMsgGenericEvent@ msg)
    {
        if (msg.Message != "Use")
            return;
         
        ezGameObject@ button = GetOwner().FindChildByName("Button");

        ezTransformComponent@ slider;
        if (!button.TryGetComponentOfBaseType(slider))
            return;

        if (slider.Running)
            return;

        slider.SetDirectionForwards(true);
        slider.Running = true;

        ezMsgGenericEvent newMsg;
        newMsg.Message = ButtonName;

        GetOwnerComponent().BroadcastEventMsg(newMsg);
    }
}


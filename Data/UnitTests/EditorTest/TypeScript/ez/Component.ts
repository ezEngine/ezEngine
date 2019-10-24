import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

import __Message = require("./Message")
export import Message = __Message.Message;

import __Time = require("./Time")
export import Time = __Time.Time;

declare function __CPP_Binding_RegisterMessageHandler(msgTypeNameHash: number, handlerFuncName: string): void;
declare function __CPP_Component_IsValid(component: Component): boolean;
declare function __CPP_Component_GetOwner(component: Component): GameObject;
declare function __CPP_Component_SetActive(component: Component, active: boolean): GameObject;
declare function __CPP_Component_IsActive(component: Component): boolean;
declare function __CPP_Component_IsActiveAndInitialized(component: Component): boolean;
declare function __CPP_Component_IsActiveAndSimulating(component: Component): boolean;
declare function __CPP_Component_SendMessage(_this: Component, typeNameHash: number, msg: Message): void;
declare function __CPP_Component_PostMessage(_this: Component, typeNameHash: number, msg: Message, delay: number): void;

export abstract class Component {

    IsValid(): boolean {
        return __CPP_Component_IsValid(this);
    }

    GetOwner(): GameObject {
        return __CPP_Component_GetOwner(this);
    }

    SetActive(active: boolean): void {
        __CPP_Component_SetActive(this, active);
    }

    IsActive(): boolean {
        return __CPP_Component_IsActive(this);
    }

    IsActiveAndInitialized(): boolean {
        return __CPP_Component_IsActiveAndInitialized(this);
    }

    IsActiveAndSimulating(): boolean {
        return __CPP_Component_IsActiveAndSimulating(this);
    }

    SendMessage(msg: Message): void {
        __CPP_Component_SendMessage(this, msg.TypeNameHash, msg);
    }

    PostMessage(msg: Message, delay: number = Time.Zero()): void {
        __CPP_Component_PostMessage(this, msg.TypeNameHash, msg, delay);
    }

    // GetUniqueID
    // GetMode
    // virtual void Initialize();
    // virtual void Deinitialize();
    // virtual void OnActivated();
    // virtual void OnDeactivated();
    // virtual void OnSimulationStarted();
}

export abstract class TypescriptComponent extends Component {
    abstract Update(): void;

    static RegisterMessageHandler<TYPE extends Message>(msgType: new () => TYPE, handlerFuncName: string) {
        __CPP_Binding_RegisterMessageHandler(msgType.GetTypeNameHash(), handlerFuncName);
    }
}

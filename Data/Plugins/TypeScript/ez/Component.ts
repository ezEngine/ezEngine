import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

import __Message = require("./Message")
export import Message = __Message.Message;
export import EventMessage = __Message.EventMessage;

import __Time = require("./Time")
export import Time = __Time.Time;

declare function __CPP_Binding_RegisterMessageHandler(msgTypeNameHash: number, handlerFuncName: string): void;
declare function __CPP_Component_IsValid(component: Component): boolean;
declare function __CPP_Component_GetUniqueID(component: Component): number;
declare function __CPP_Component_GetOwner(component: Component): GameObject;
declare function __CPP_Component_SetActive(component: Component, active: boolean): GameObject;
declare function __CPP_Component_IsActive(component: Component): boolean;
declare function __CPP_Component_IsActiveAndInitialized(component: Component): boolean;
declare function __CPP_Component_IsActiveAndSimulating(component: Component): boolean;
declare function __CPP_Component_SendMessage(_this: Component, typeNameHash: number, msg: Message, expectMsgResult: boolean): void;
declare function __CPP_Component_PostMessage(_this: Component, typeNameHash: number, msg: Message, delay: number): void;
declare function __CPP_TsComponent_BroadcastEvent(_this: TypescriptComponent, typeNameHash: number, msg: EventMessage): void;

/**
 * Abstract base class for all component classes.
 * 
 * TypeScript instances to components act as weak pointers. Components are always created, managed and destroyed on the C++ side.
 * The TypeScript side can only ask the C++ side to create, destroy or modify components. If a component is destroyed on the
 * C++ side, the TS side is not informed. However, calling 'IsValid()' allows one to query whether the component is still alive
 * on the C++ side. The IsValid() result stays true at least until the end of the frame.
 * 
 * Executing operations other than 'IsValid()' on an invalid component is an error.
 */
export abstract class Component {

    /**
     * Checks whether a component is still alive on the C++ side.
     * 
     * @returns True for live components. False for components that have been destroyed already on the C++ side.
     */
    IsValid(): boolean {
        return __CPP_Component_IsValid(this);
    }

    /**
     * Returns the owning game object of this component.
     */
    GetOwner(): GameObject {
        return __CPP_Component_GetOwner(this);
    }

    /**
     * Activates or deactivates a component.
     * Deactivated components are present, but do not have any effect.
     */
    SetActive(active: boolean): void {
        __CPP_Component_SetActive(this, active);
    }

    /**
     * Checks whether this component is active. See 'SetActive()'.
     */
    IsActive(): boolean {
        return __CPP_Component_IsActive(this);
    }

    /**
     * Checks whether this component is both active and has already been initialized.
     */
    IsActiveAndInitialized(): boolean {
        return __CPP_Component_IsActiveAndInitialized(this);
    }

    /**
     * Checks whether this component is active and has already been configured for game simulation.
     */
    IsActiveAndSimulating(): boolean {
        return __CPP_Component_IsActiveAndSimulating(this);
    }

    /**
     * Sends an ez.Message directly to this component.
     * 
     * @param expectResultData If set to true, the calling code assumes that the message receiver(s) may write result data
     *   back into the message and thus the caller is interested in reading that data afterwards. If set to false
     *   (the default) the state of the message is not synchronized back into the TypeScript message after the message
     *   has been delivered and thus any data written into the message by the receiver, is lost.
     */
    SendMessage(msg: Message, expectResultData: boolean = false): void {
        __CPP_Component_SendMessage(this, msg.TypeNameHash, msg, expectResultData);
    }

    /**
     * Sends and ez.Message to this component.
     * The message is queued and delivered during the next convenient game update phase.
     * It may optionally be sent with a time delay.
     */
    PostMessage(msg: Message, delay: number = Time.Zero()): void {
        __CPP_Component_PostMessage(this, msg.TypeNameHash, msg, delay);
    }

    /**
     * Returns an ID unique to this component.
     */
    GetUniqueID(): number {
        return __CPP_Component_GetUniqueID(this);
    }

    /* 
        These functions are pretty much virtual for derived classes to override.
        However, to prevent unnecessary performance overhead, they are not implemented as virtual here.
        Instead just implement them in derived classes as needed.

        Initialize(): void {
        }
    
        Deinitialize(): void {
        }
    
        OnActivated(): void {
        }
    
        OnDeactivated(): void {
        }
    
        OnSimulationStarted(): void {
        }
    */
}

/**
 * Base class for all component types implemented exclusively with TypeScript.
 */
export abstract class TypescriptComponent extends Component {

    // static RegisterMessageHandlers() {
    // }

    /**
     * Registers a member function to handle certain message types.
     * This must be called exclusively from a static function called 'RegisterMessageHandlers()' 
     * to hook up a member function as a message handler to tell the C++ side which messages to
     * deliver to th TS side for message handling.
     * 
     * @param msgType The TS type of the message to handle. E.g. 'ez.MsgSetColor'
     * @param handlerFuncName The function to call when messages of the given type arrive.
     */
    static RegisterMessageHandler<TYPE extends Message>(msgType: new () => TYPE, handlerFuncName: string) {
        __CPP_Binding_RegisterMessageHandler(msgType.GetTypeNameHash(), handlerFuncName);
    }

    /**
     * Broadcasts and event message up the graph (ie. to parent nodes) and to the next mvent message handler.
     */
    protected BroadcastEvent<TYPE extends EventMessage>(msg: TYPE): void {

        __CPP_TsComponent_BroadcastEvent(this, msg.TypeNameHash, msg);
    }
}

/**
 * Base class for all component types implemented exclusively with TypeScript, that need a regular update call.
 */
export abstract class TickedTypescriptComponent extends TypescriptComponent {

    /**
     * Abstract 'Tick()' function that gets called once per frame.
     * Custom component logic that requires regularly checking of conditions should be implemented here.
     * 
     * @returns A time delta in which to call 'Tick()' again. Returning 0.0 means 'Tick()' will be called
     * again in the very next frame. Any higher value may result in 'Tick()' being skipped for multiple frames
     * until the desired time difference has passed. 
     */
    abstract Tick(): number;
}

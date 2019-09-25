export namespace Log
{
    export declare function Error(text : string) : void;
    export declare function SeriousWarning(text : string) : void;
    export declare function Warning(text : string) : void;
    export declare function Success(text : string) : void;
    export declare function Info(text : string) : void;
    export declare function Dev(text : string) : void;
    export declare function Debug(text : string) : void;
};

export abstract class GameObjectHandle
{
}

declare function __Component_GetOwner(component : Component) : GameObject;
declare function __GameObject_SetLocalPosition(go : GameObject, x : number, y : number, z : number) : void;

export class GameObject
{
    // GetLocalPosition
    // rotation, scaling

    //GetHandle() : GameObjectHandle { return new GameObjectHandle(); }

    SetLocalPosition(x : number, y : number, z : number) : void
    {
        __GameObject_SetLocalPosition(this, x, y, z);
    }    
}

export abstract class Component
{
    abstract Update() : void;

    GetOwner() : GameObject 
    {
        return __Component_GetOwner(this);
    }
}

export function __Ts_CreateGameObject() : GameObject
{
    return new GameObject;
}
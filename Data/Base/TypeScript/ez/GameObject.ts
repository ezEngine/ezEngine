declare function __CPP_GameObject_SetLocalPosition(go : GameObject, x : number, y : number, z : number) : void;

export abstract class GameObjectHandle
{
}

export class GameObject
{
    // GetLocalPosition
    // rotation, scaling

    //GetHandle() : GameObjectHandle { return new GameObjectHandle(); }

    SetLocalPosition(x : number, y : number, z : number) : void
    {
        __CPP_GameObject_SetLocalPosition(this, x, y, z);
    }    
}

export function __TS_CreateGameObject() : GameObject
{
    return new GameObject;
}
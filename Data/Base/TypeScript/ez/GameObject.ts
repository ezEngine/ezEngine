declare function __CPP_GameObject_SetLocalPosition(go : GameObject, x : number, y : number, z : number) : void;
declare function __CPP_GameObject_IsValid(go : GameObject) : boolean;

export class GameObject
{
    // GetLocalPosition
    // rotation, scaling

     IsValid() : boolean
     {
        return __CPP_GameObject_IsValid(this);
     }

    SetLocalPosition(x : number, y : number, z : number) : void
    {
        __CPP_GameObject_SetLocalPosition(this, x, y, z);
    }    
}

export function __TS_CreateGameObject() : GameObject
{
    return new GameObject;
}
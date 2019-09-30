import __Vec3 = require("./Vec3")
import __Quat = require("./Quat")

export import Vec3 = __Vec3.Vec3;
export import Quat = __Quat.Quat;

declare function __CPP_GameObject_IsValid(go : GameObject) : boolean;
declare function __CPP_GameObject_SetLocalPosition(go : GameObject, pos: Vec3) : void;
declare function __CPP_GameObject_SetLocalRotation(go : GameObject, rot: Quat) : void;

export class GameObject
{
    // GetLocalPosition
    // rotation, scaling

     IsValid() : boolean
     {
        return __CPP_GameObject_IsValid(this);
     }

    SetLocalPosition(pos: Vec3) : void
    {
        __CPP_GameObject_SetLocalPosition(this, pos);
    }    
    
    SetLocalRotation(rot: Quat) : void
    {
        __CPP_GameObject_SetLocalRotation(this, rot);
    }   
}

export function __TS_CreateGameObject() : GameObject
{
    return new GameObject;
}
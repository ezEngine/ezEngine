
class ezGameObject
{
    Update() : void
    {   
        
        ezLog_Info("ezGameObject::Update")
        GameObjectMove(this)
    }
}

let go : ezGameObject = null;

function _CreateGameObject()
{
    return new ezGameObject;
}

function Update() : void
{
    if (go === null)
    {
        go = GetGameObject()
        //go = new ezGameObject()
    }

    go.Update();
}
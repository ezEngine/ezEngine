import ez = require("./ez")

class MyComponent extends ez.Component 
{
    constructor(name: string) 
    {
        super()
        ez.Log.Info("Construct MyComponent: " + name)
        this._name = name;
    }

    Update(): void
    {
        //ez.Log.Dev("MyComponent::Update: " + this._name)

        var go = this.GetOwner();

        if (go != null)
        {
            //ez.Log.Dev("Setting Local Position")

            let newPos = ez.Vec3.CreateRandomPointInSphere();
            go.SetLocalPosition(newPos);

            let newDir = ez.Vec3.CreateRandomDirection();
            let newRot = new ez.Quat();
            newRot.SetShortestRotation(new ez.Vec3(1, 0, 0), newDir);

            go.SetLocalRotation(newRot);

        }
    }

    private _name: string;
}

// called by the runtime
function __TS_Create_MyComponent(name: string): MyComponent
{
    ez.Log.Info("_Create_MyComponent: " + name)
    return new MyComponent(name);
}
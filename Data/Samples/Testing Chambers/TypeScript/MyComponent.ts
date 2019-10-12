import ez = require("./ez")

class MyComponent extends ez.TypescriptComponent 
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

        let owner = this.GetOwner();

        if (owner != null)
        {
            //ez.Log.Dev("Setting Local Position")

            let newPos = ez.Vec3.CreateRandomPointInSphere();
            newPos.MulNumber(0.1)
            newPos.AddVec3(owner.GetLocalPosition())
            owner.SetLocalPosition(newPos);

            let newDir = ez.Vec3.CreateRandomDirection();
            let newRot = new ez.Quat();
            newRot.SetShortestRotation(new ez.Vec3(1, 0, 0), newDir);

            //owner.SetLocalRotation(newRot);
        }

        let child = owner.FindChildByName("Light");
        if (child != null)
        {
            child.SetActive(false);//Math.random() > 0.7);
        }

        let comp = owner.FindComponentByTypeNameHash<ez.TransformComponent>(ez.TransformComponent.GetTypeNameHash());
        if (comp != null)
        {
            //comp.SetActive(Math.random() > 0.5);
            if (Math.random() > 0.9)
            {
                if (comp.IsDirectionForwards())
                    comp.SetDirectionForwards(false);
                else
                    comp.SetDirectionForwards(true);
                //comp.ToggleDirection();
            }
            //comp.SetDirectionForwards(Math.random() > 0.9);
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
module ez.interop.components;

public import ez.core.world.gameobject;
public import ez.core.world.component;

extern (C++)
{
    alias ComponentCreationFunc = ezDLangBaseComponent function(ezGameObject owner, ezComponent ownerComponent);
    alias ComponentDestructionFunc = void function(ezDLangBaseComponent);

    interface ezDLangBaseComponent
    {
        void OnSimulationStarted();
        void Update();
    }

    abstract class ezDLangComponent : ezDLangBaseComponent
    {
        private ezGameObject m_Owner;
        private ezComponent m_OwnerComponent;

        this(ezGameObject owner, ezComponent ownerComponent)
        {
            m_Owner = owner;
            m_OwnerComponent = ownerComponent;
        }

        ezGameObject GetOwner()
        {
            return m_Owner;
        }

        ezComponent GetOwnerComponent()
        {
            return m_OwnerComponent;
        }

        override void OnSimulationStarted()
        {
        }

        override void Update()
        {
        }
    }
}

extern (C++,ezDLangInterop)
{
    void RegisterComponentType(const(char)* name, ComponentCreationFunc creator, ComponentDestructionFunc destructor);
}

mixin template ExposeToCpp()
{
    this(ezGameObject owner, ezComponent ownerComponent)
    {
        super(owner, ownerComponent);
    }

    extern (D) shared static this()
    {
        import std.string : toStringz;

        string cn = typeof(this).classinfo.name;
        ez.RegisterComponentType(toStringz(cn), &CreateComponent, &DestroyComponent);
    }

    static ezDLangBaseComponent CreateComponent(ezGameObject owner, ezComponent ownerComponent)
    {
        import core.memory;

        auto comp = new typeof(this)(owner, ownerComponent);
        core.memory.GC.addRoot(cast(void*) comp);
        return comp;
    }

    static void DestroyComponent(ezDLangBaseComponent comp)
    {
        import core.memory;

        // TODO: free object ?
        core.memory.GC.removeRoot(cast(void*) comp);
    }
}

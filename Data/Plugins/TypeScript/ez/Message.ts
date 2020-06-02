
/**
 * Base class for all message types.
 */
export abstract class Message {
    TypeNameHash: number = 0;

    public static GetTypeNameHash(): number { return 0; }
}

/**
 * Base class for all message types that are broadcast as 'events',
 * ie. bubbling up the scene graph, instead of being delivered downwards the graph structure.
 */
export abstract class EventMessage extends Message {
}


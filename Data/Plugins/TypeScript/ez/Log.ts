declare function __CPP_Log_Error(text: string): void;
declare function __CPP_Log_SeriousWarning(text: string): void;
declare function __CPP_Log_Warning(text: string): void;
declare function __CPP_Log_Success(text: string): void;
declare function __CPP_Log_Info(text: string): void;
declare function __CPP_Log_Dev(text: string): void;
declare function __CPP_Log_Debug(text: string): void;

/**
 * Text logging functionality.
 */
export namespace Log {

    /**
     * Logs the given text as an error.
     */
    export function Error(text: string): void { __CPP_Log_Error(text); }
    
    /**
     * Logs the given text as an important warning.
     */
    export function SeriousWarning(text: string): void { __CPP_Log_SeriousWarning(text); }
    
    /**
     * Logs the given text as a warning.
     */
    export function Warning(text: string): void { __CPP_Log_Warning(text); }
    
    /**
     * Logs the given text as a success message.
     */    
    export function Success(text: string): void { __CPP_Log_Success(text); }
    
    /**
     * Logs the given text as an info message.
     */    
    export function Info(text: string): void { __CPP_Log_Info(text); }
    
    /**
     * Logs the given text as a message for developers. Will not be visible in Release builds.
     */    
    export function Dev(text: string): void { __CPP_Log_Dev(text); }
    
    /**
     * Logs the given text as a (verbose) debug message for developers. Will not be visible in Release builds.
     */    
    export function Debug(text: string): void { __CPP_Log_Debug(text); }
};

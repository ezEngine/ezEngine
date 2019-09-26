declare function __CPP_Log_Error(text : string) : void;
declare function __CPP_Log_SeriousWarning(text : string) : void;
declare function __CPP_Log_Warning(text : string) : void;
declare function __CPP_Log_Success(text : string) : void;
declare function __CPP_Log_Info(text : string) : void;
declare function __CPP_Log_Dev(text : string) : void;
declare function __CPP_Log_Debug(text : string) : void;

export namespace Log
{
    export function Error(text : string) : void             { __CPP_Log_Error(text); }
    export function SeriousWarning(text : string) : void    { __CPP_Log_SeriousWarning(text); }
    export function Warning(text : string) : void           { __CPP_Log_Warning(text); }
    export function Success(text : string) : void           { __CPP_Log_Success(text); }
    export function Info(text : string) : void              { __CPP_Log_Info(text); }
    export function Dev(text : string) : void               { __CPP_Log_Dev(text); }
    export function Debug(text : string) : void             { __CPP_Log_Debug(text); }
};

*      PCSTART.S
*
*      Pure C Startup Code
*
*      Copyright (c) Borland International 1988/89/90
*      All Rights Reserved.

* Stripped down version:
* No processing of commandline because we don't need it

*>>>>>> Export references <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

        .EXPORT exit, __exit

        .EXPORT _BasPag
        .EXPORT _app
        .EXPORT errno
        .EXPORT _FilSysVec
        .EXPORT _PgmSize

        .EXPORT __text, __data, __bss

*>>>>>> Import references <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

        .IMPORT main
        .IMPORT _StkSize
        .IMPORT _FreeAll




*>>>>>> Data structures <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


* Base page structure

        .OFFSET 0

TpaStart:
        .DS.L   1
TpaEnd:
        .DS.L   1
TextSegStart:
        .DS.L   1
TextSegSize:
        .DS.L   1
DataSegStart:
        .DS.L   1
DataSegSize:
        .DS.L   1
BssSegStart:
        .DS.L   1
BssSegSize:
        .DS.L   1
DtaPtr:
        .DS.L   1
PntPrcPtr:
        .DS.L   1
Reserved0:
        .DS.L   1
EnvStrPtr:
        .DS.L   1
Reserved1:
        .DS.B   7
CurDrv:
        .DS.B   1
Reserved2:
        .DS.L   18
CmdLine:
        .DS.B   128
BasePageSize:
        .DS     0



*>>>>>>> Data segment <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

        .BSS
__bss:

* Pointer to base page

_BasPag:
        .DS.L   1


* Applikation flag

_app:
        .DS.W   1


* Program size

_PgmSize:
        .DS.L   1

* Redirection address table

*>>>>>>> Initialized data segment <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

        .DATA
__data:

* Global error variable

errno:
        .DC.W   0


* Vector for file system deinitialization

_FilSysVec:
        .DC.L   0


*>>>>>>> Code segment <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

        .CODE
__text:


******** PcStart ********************************************************

Start:
        BRA.B   Start0



******* Configuration data


* Redirection array pointer

        .DC.L   0


* Stack size entry

        .DC.L   _StkSize

******** Pc startup code

* Setup pointer to base page

Start0:
        MOVE.L  A0, A3
        MOVE.L  A3, D0

        MOVE.L  4(A7), A3   ; BasePagePointer from Stack
        MOVEQ.L #1, D0      ; Program is Application

APP:

        MOVE.L  A3, _BasPag

* Setup applikation flag

        MOVE.W  D0,_app


* Compute size of required memory
* := text segment size + data segment size + bss segment size
*  + stack size + base page size
* (base page size includes stack size)

        MOVE.L  TextSegSize(A3),A0
        ADD.L   DataSegSize(A3),A0
        ADD.L   BssSegSize(A3),A0
        ADD.W   #BasePageSize,A0
        MOVE.L  A0, _PgmSize

* Setup longword aligned application stack

        MOVE.L  A3,D0
        ADD.L   A0,D0
        AND.B   #$FC,D0
        MOVE.L  D0,A7

* Free not required memory

        MOVE.L  A0,-(A7)
        MOVE.L  A3,-(A7)
        MOVE.W  #0,-(A7)
        MOVE.W  #74,-(A7)
        TRAP    #1
        LEA.L   12(A7),A7

Start8:

******* Execute main program *******************************************
*
* Parameter passing:
*   <D0.W> = Command line argument count (argc)
*   <A0.L> = Pointer to command line argument pointer array (argv)
*   <A1.L> = Pointer to tos environment string (env)

        MOVE    D3, D0
        MOVE.L  A2, A0
        MOVE.L  A4, A1
        JSR     main



******** exit ***********************************************************
*
* Terminate program
*
* Entry parameters:
*   <D0.W> = Termination status : Integer
* Return parameters:
*   Never returns

exit:
        MOVE.W  D0,-(A7)

* Deinitialize file system

__exit:
        MOVE.L  _FilSysVec,D0
        BEQ     Exit1

        MOVE.L  D0,A0
        JSR     (A0)


* Deallocate all heap blocks

Exit1:
        JSR     _FreeAll


* Program termination with return code

        MOVE.W  #76,-(A7)
        TRAP    #1



******* Module end *****************************************************

/* $Id: pd_patch.c,v 1.16 1994/04/22 23:04:08 peabody Exp $ */
/****************************************************************
**
**  patchdemo.arm patch structure management and file parser.
**
**  By:  Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
*****************************************************************
**  940223 WJB  Created from patchtest.c.
**  940224 WJB  Made FindPatchKnob() public.  Added GetNGrabbedPatchKnobs().
**  940225 WJB  Tweaked DeletePatch() a bit.
**  940228 WJB  Added myalloc(), myfree().
**  940301 WJB  Added LoadSample and AttachSample commands and support code for PatchSample and PatchSampleAttachment.
**  940311 WJB  Converted non-triple-bang C++ comments to C comments.
**  940407 WJB  Made a bunch of functions private to pd_patch.c
**  940407 WJB  Added PatchSymbol.
**  940407 WJB  Removed include patch for strtol().
**  940408 WJB  Added a triple bang.
**  940408 WJB  Added optional start frame arg to AttachSample.
**  940414 WJB  Added DelayLine command.
**  940414 WJB  Made AddPatchSymbol() trap redefined symbols and restructured clients.
**  940422 WJB  Converted C++ comments to C comments.
**  940422 WJB  Added support for PIATTRF_Triggerable and PISTATEF_Started.
****************************************************************/

    /* local */
#include "patchdemo.h"

    /* portfolio */
#include <ctype.h>                  /* isspace() */
#include <filestreamfunctions.h>
#include <operror.h>                /* PrintError() */
#include <stdarg.h>                 /* va_list */
#include <stdio.h>                  /* printf(), vprintf() */
#include <stdlib.h>                 /* strtol() */
#include <string.h>                 /* str...() */


/* -------------------- Debugging */

#define DEBUG_LoadEntry         0   /* entry/exit of LoadPatch()/UnloadPatch() */
#define DEBUG_LoadInterpreter   0   /* LoadPatch() interpreter and object new/delete functions */
#define DEBUG_LoadParser        0   /* LoadPatch() line parser */
#define DEBUG_Symbol            0   /* PatchSymbol system */
#define DEBUG_Trigger           0   /* entry/exit of all start/stop/release functions */


/* -------------------- Misc defines */

#define PINST_Priority  100         /* (default) instrument priority (!!! base on some system constant?) */


/* -------------------- ParserFile (file handle for parser) */

typedef struct ParserFile {
        /* constants */
    const char *pfile_FileName;     /* file name */
    Stream *pfile_Stream;           /* stream associated with file */

        /* variables */
    int32 pfile_LineNum;            /* line number of last line read by ReadParserFileLine().  Set to 0 by OpenParserFile(). */
} ParserFile;

static ParserFile *OpenParserFile (const char *filename, Err *errbuf);
static Err CloseParserFile (ParserFile *);
static int32 GetParserFileLine (ParserFile *, char *buf, int32 bufsize);
static void PrintParserFileError (const ParserFile *, const char *errorfmt, ...);


/* -------------------- Command table */
    /* misc defines */
#define CMD_MaxChars 130            /* Max # of characters on a command line, not including null termination or new line character */
#define CMD_MaxArgs  31             /* Max # of command line arguments including command name, not including null termination */

    /* processing functions */
typedef Err cmdprocproto_t (const ParserFile *, Patch *, int argc, const char * const argv[]);
typedef cmdprocproto_t *cmdprocfn_t;

    /* Info */
typedef struct CommandInfo {
    const char *Name;               /* command name (note: case insensitive) */
    int MinArgC;                    /* min argc value (includes command name) */
    cmdprocfn_t ProcessFN;          /* processing function */
} CommandInfo;

    /* processing function prototypes */
static cmdprocproto_t cmdproc_AttachSample;
static cmdprocproto_t cmdproc_Connect;
static cmdprocproto_t cmdproc_DelayLine;
static cmdprocproto_t cmdproc_LoadInstrument;
static cmdprocproto_t cmdproc_LoadSample;
static cmdprocproto_t cmdproc_Tweak;
#if 0       /* useful for prototyping new commands */
  static cmdprocproto_t cmdproc_dummy;
#endif

    /* command table */
static const CommandInfo CommandTable[] = {
    { "AttachSample",   4, cmdproc_AttachSample },      /* AttachSample <attachment symbol> <instrument> <sample> [<fifo name> [<start frame>]] */
    { "Connect",        5, cmdproc_Connect },           /* Connect <source instrument> <source output> <dest instrument> <dest input> */
    { "DelayLine",      3, cmdproc_DelayLine },         /* DelayLine <sample symbol> <nbytes> [<nchannels> [<loop (1/0)>]] */
    { "LoadInstrument", 3, cmdproc_LoadInstrument },    /* LoadInstrument <instrument symbol> <instrument file name> [<triggerable (1/0)>] */
    { "LoadSample",     3, cmdproc_LoadSample },        /* LoadSample <sample symbol> <sample file name> */
    { "Tweak",          4, cmdproc_Tweak },             /* Tweak <instrument> <knob> <value> */
};

    /* client functions */
static const CommandInfo *LookupCommand (const char *matchcmd);


/* -------------------- Misc local functions */

    /* Local Patch stuff */
#define FindPatchInstrument(patch,symname)       (PatchInstrument *)       FindPatchSymbol (&(patch)->patch_InstrumentList,       (symname))
#define FindPatchSample(patch,symname)           (PatchSample *)           FindPatchSymbol (&(patch)->patch_SampleList,           (symname))
#define FindPatchSampleAttachment(patch,symname) (PatchSampleAttachment *) FindPatchSymbol (&(patch)->patch_SampleAttachmentList, (symname))

    /* Local PatchInstrument functions */
static PatchInstrument *CreatePatchInstrument (const char *symname, const char *filename, bool triggerable, Err *errbuf);
static Err StartPatchInstrument (PatchInstrument *, const TagArg *);
static Err ReleasePatchInstrument (PatchInstrument *, const TagArg *);
static Err StopPatchInstrument (PatchInstrument *, const TagArg *);

    /* Local PatchInstrument KnobTable support */
static PatchKnob *FindPatchKnob (const PatchInstrument *, const char *knobname);
static Err AttachKnobTable (PatchInstrument *);
static Err DetachKnobTable (PatchInstrument *);

    /* Local PatchKnob functions */
static Err GrabPatchKnob (Item instrument, PatchKnob *);
static Err ReleasePatchKnob (PatchKnob *);

    /* Local PatchSample functions */
static PatchSample *CreatePatchSample (const char *symname, const char *filename, Err *errbuf);
static PatchSample *CreatePatchDelayLine (const char *symname, uint32 nbytes, uint32 nchannels, bool loop, Err *errbuf);

    /* Local PatchSampleAttachment functions */
static PatchSampleAttachment *CreatePatchSampleAttachment (const char *symname, const PatchInstrument *, const PatchSample *, const char *fifoname, uint32 startframe, Err *errbuf);

    /* Local PatchSymbol list functions */
static Err AddPatchSymbol (List *symlist, PatchSymbol *newsym);
#define FindPatchSymbol(list,symname)   (PatchSymbol *) FindNamedNodeNoCase ((list), (symname))
static Err DeletePatchSymbolList (List *symlist);

    /* Local PatchSymbol functions */
static PatchSymbol *CreatePatchSymbol (const char *symname, size_t size, const PatchSymbolFunctions *, Err *errbuf);
static Err DeletePatchSymbol (PatchSymbol *);

    /* command line arg processing */
static int ParseLine (char *s, char *argv[], int argvnelts);

    /* string.h emulation and helpers */
static char *skipspace (char *s);
static char *skipnspace (char *s);
static char *strpbrk (const char *cs, const char *ct);

    /* stream */
static int32 GetStreamLine (Stream *, char *buf, int32 bufsize);

    /* utility */
static Node *FindNamedNodeNoCase (const List *, const char *name);


/* -------------------- LoadPatch() */

static Err readpatchfile (const char *filename, Patch *);
static Err readpatchlines (ParserFile *, Patch *);

/*
    LoadPatch - Load a patch file.

    SYNOPSIS

        Patch *LoadPatch (const char *filename, Err *errbuf)

    DESCRIPTION

        !!!

    ARGUMENTS

        filename    File name to read patch from.

        errbuf        Pointer to client error code buffer to store error code
                    on failure.  Can be NULL, in which case client no error
                    code is stored.

    RETURN VALUE

        Pointer to a Patch structure or NULL on failure.

    IMPLEMENTATION

    ASSOCIATED FILES

    CAVEATS

        filename is not checked for a NULL pointer.

        Do not depend on error code buffer being set on function success.

        The new Patch may have an empty patch_InstrumentList.

    SEE ALSO

        UnloadPatch()
*/

Patch *LoadPatch (const char *filename, Err *errbuf)
{
    Patch *patch = NULL;
    Err errcode = 0;

  #if DEBUG_LoadEntry
    printf ("LoadPatch() \"%s\" ", filename); printavail(); printf ("\n");
  #endif

        /* allocate patch */
    if ((patch = (Patch *)myalloc (sizeof *patch)) == NULL) {
        errcode = -1;           /* !!! real error code */
        goto clean;
    }
    InitList (&patch->patch_InstrumentList,       "Instrument");
    InitList (&patch->patch_SampleList,           "Sample");
    InitList (&patch->patch_SampleAttachmentList, "Sample Attachment");

        /* read patch file */
    if ((errcode = readpatchfile (filename, patch)) < 0) goto clean;

clean:
    if (errcode < 0) {
        CloseRes (UnloadPatch, patch);
        if (errbuf) *errbuf = errcode;
    }

  #if DEBUG_LoadEntry
    printf ("  patch=$%p err=%ld ", patch, errcode); printavail(); printf ("\n");
  #endif

    return patch;
}

/*
    Opens, reads, closes patch file.
    Prints diagnostic messages.
    Returns 0 on success, error code on failure.
*/
static Err readpatchfile (const char *filename, Patch *patch)
{
    ParserFile *file = NULL;
    Err errcode = 0;

        /* open file */
    if ((file = OpenParserFile (filename, &errcode)) == NULL) {
            /* !!! really do error message here? */
        PrintError (NULL, "open patch file", filename, errcode);
        goto clean;
    }

        /* process lines */
    if ((errcode = readpatchlines (file, patch)) < 0) goto clean;

clean:
    CloseParserFile (file);
    /* let LoadPatch() do the Patch cleanup on failure */

    return errcode;
}

/*
    Reads all lines from file and applies them to patch.
    Prints syntax errors.
    Returns 0 on success, error code on failure.
*/
static Err readpatchlines (ParserFile *file, Patch *patch)
{
    Err errcode = 0;
    int32 result;
    char linebuf [CMD_MaxChars + 2];    /* <cmdline> '\n' '\0'.  max cmdline chars = sizeof linebuf - 2. */
    int argc;
    char *argv [CMD_MaxArgs + 1];
    const CommandInfo *cmdinfo;

        /* loop on lines */
    while ((result = GetParserFileLine (file, linebuf, sizeof linebuf)) > 0) {

            /* trap line too long: look for new line char in last character of string when string buffer is full */
        if (strlen (linebuf) == CMD_MaxChars+1 && linebuf[CMD_MaxChars] != '\n') {
            PrintParserFileError (file, "Line too long.");
            errcode = -1;           /* !!! real error code */
            goto clean;
        }

            /* get argc, argv for line.  ignore if no command */
        if ((argc = ParseLine (linebuf, argv, NArrayElts (argv))) > 0) {

                /* lookup command in table */
            if ((cmdinfo = LookupCommand (argv[0])) == NULL) {
                PrintParserFileError (file, "Unknown command \"%s\".", argv[0]);
                errcode = -1;           /* !!! real error code */
                goto clean;
            }

                /* check # arg count */
            if (argc < cmdinfo->MinArgC) {
                PrintParserFileError (file, "Missing required argument(s) for %s.", cmdinfo->Name);
                errcode = -1;           /* !!! real error code */
                goto clean;
            }

                /* dispatch command processor */
                /* these can print syntax error messages */
                /* !!! shouldn't need this cast - armcc 1.6 bug */
            if ((errcode = cmdinfo->ProcessFN (file, patch, argc, (const char * const *)argv)) < 0) goto clean;
        }
    }
    if (result < 0) {
            /* !!! error message here?  get operror message? */
        PrintParserFileError (file, "Error reading file.");
        errcode = result;
        goto clean;
    }

clean:
    /* let LoadPatch() do the Patch cleanup on failure */

    return errcode;
}

/*
    find command.  returns NULL on failure.
*/
static const CommandInfo *LookupCommand (const char *matchcmd)
{
    const CommandInfo *cmdinfo = CommandTable;
    int32 count = NArrayElts (CommandTable);

    for (; count--; cmdinfo++) {
        if (!strcasecmp ((char *)cmdinfo->Name, matchcmd)) return cmdinfo;       /* !!! this function should take const char * */
    }

    return NULL;
}


/*
    LoadInstrument <instrument symbol> <instrument file name> [<triggerable (1/0)>]

    Load new instrument and add it to Patch's InstrumentList.

    <triggerable> defaults to 1.
*/
static Err cmdproc_LoadInstrument (const ParserFile *file, Patch *patch, int argc, const char * const argv[])
{
    const char *symname    = argv[1];
    const char *filename   = argv[2];
    const bool triggerable = argc > 3 ? (strtoul (argv[3], NULL, 0) != 0) : TRUE;
    PatchInstrument *inst = NULL;
    Err errcode = 0;

        /* get new PatchInstrument */
    if ((inst = CreatePatchInstrument (symname, filename, triggerable, &errcode)) == NULL) {
            /* !!! generalize this error message */
        PrintParserFileError (file, "LoadInstrument failed.");
        goto clean;
    }

        /* add to InstrumentList */
    if ((errcode = AddPatchSymbol (&patch->patch_InstrumentList, (PatchSymbol *)inst)) < 0) {
            /* !!! generalize this error message */
        PrintParserFileError (file, "Instrument \"%s\" redefined.", symname);
        goto clean;
    }

clean:
    if (errcode < 0) DeletePatchSymbol ((PatchSymbol *)inst);
    return errcode;
}


/*
    LoadSample <sample symbol> <sample file name>

    Load new sample and add it to Patch's SampleList.
*/
static Err cmdproc_LoadSample (const ParserFile *file, Patch *patch, int argc, const char * const argv[])
{
    const char *symname = argv[1];
    const char *filename = argv[2];
    PatchSample *samp = NULL;
    Err errcode = 0;

        /* get new PatchSample */
    if ((samp = CreatePatchSample (symname, filename, &errcode)) == NULL) {
            /* !!! generalize this error message */
        PrintParserFileError (file, "LoadSample failed.");
        goto clean;
    }

        /* add to SampleList */
    if ((errcode = AddPatchSymbol (&patch->patch_SampleList, (PatchSymbol *)samp)) < 0) {
            /* !!! generalize this error message */
        PrintParserFileError (file, "Sample \"%s\" redefined.", symname);
        goto clean;
    }

clean:
    if (errcode < 0) DeletePatchSymbol ((PatchSymbol *)samp);
    return errcode;
}


/*
    DelayLine <sample symbol> <nbytes> [<nchannels> [<loop (1/0)>]]

    Create a delay line and add it to Patch's SampleList.

    <nchannels> defaults to 1.
    <loop> defaults to TRUE (1).

    !!! BUGS:
        . doesn't check pointer returned by strtol() for junk after number.
        . doesn't do bounds checking on knob_value.
*/
static Err cmdproc_DelayLine (const ParserFile *file, Patch *patch, int argc, const char * const argv[])
{
    const char *symname    = argv[1];
    const uint32 nbytes    = strtoul (argv[2], NULL, 0);
    const uint32 nchannels = argc > 3 ? strtoul (argv[3], NULL, 0) : 1;
    const bool   loop      = argc > 4 ? (strtoul (argv[4], NULL, 0) != 0) : TRUE;
    PatchSample *samp = NULL;
    Err errcode = 0;

        /* get new DelayLine PatchSample */
    if ((samp = CreatePatchDelayLine (symname, nbytes, nchannels, loop, &errcode)) == NULL) {
            /* !!! generalize this error message */
        PrintParserFileError (file, "DelayLine failed.");
        goto clean;
    }

        /* add to SampleList */
    if ((errcode = AddPatchSymbol (&patch->patch_SampleList, (PatchSymbol *)samp)) < 0) {
            /* !!! generalize this error message */
        PrintParserFileError (file, "Sample \"%s\" redefined.", symname);
        goto clean;
    }

clean:
    if (errcode < 0) DeletePatchSymbol ((PatchSymbol *)samp);
    return errcode;
}


/*
    Connect <source instrument> <source output> <dest instrument> <dest input>

    Make a connection from one instrument's output to another instrument's input or knob.
*/
static Err cmdproc_Connect (const ParserFile *file, Patch *patch, int argc, const char * const argv[])
{
    const char *from_inst_name = argv[1];
    const char *from_out_name  = argv[2];
    const char *to_inst_name   = argv[3];
    const char *to_in_name     = argv[4];
    PatchInstrument *from_inst;
    PatchInstrument *to_inst;
    Err errcode = 0;

  #if DEBUG_LoadInterpreter
    printf ("  Connect %s.%s -> %s.%s\n", from_inst_name, from_out_name, to_inst_name, to_in_name);
  #endif

        /* locate instruments from patch's instrument list */
    {
        const char *last_name;

        if ( (from_inst = FindPatchInstrument (patch, last_name = from_inst_name)) == NULL ||
             (to_inst   = FindPatchInstrument (patch, last_name = to_inst_name)) == NULL ) {
            PrintParserFileError (file, "Instrument \"%s\" undefined.", last_name);
            errcode = -1;       /* !!! real error code */
            goto clean;
        }
    }

        /* do connection */
        /* !!! shouldn't need casts */
    if ((errcode = ConnectInstruments (from_inst->pinst_Instrument, (char *)from_out_name, to_inst->pinst_Instrument, (char *)to_in_name)) < 0) {
        PrintParserFileError (file, "Unable to connect %s.%s -> %s.%s.", from_inst_name, from_out_name, to_inst_name, to_in_name);
        /* !!! get operror message? */
        goto clean;
    }

        /* if a knob, need to note that knob has been usurped by connection */
    {
        PatchKnob *to_knob;

        if ((to_knob = FindPatchKnob (to_inst, to_in_name)) != NULL) ReleasePatchKnob (to_knob);
    }

clean:
    return errcode;
}


/*
    AttachSample <attachment symbol> <instrument> <sample> [<fifo name> [<start frame>]]

    Create attachment (named <attachment>) to attach a sample to an instrument.
    fifo name is optional, defaults to first/only fifo in instrument.
*/
static Err cmdproc_AttachSample (const ParserFile *file, Patch *patch, int argc, const char * const argv[])
{
    const char *att_name     = argv[1];
    const char *inst_name    = argv[2];
    const char *samp_name    = argv[3];
    const char *fifo_name    = argc > 4 ? argv[4] : NULL;
    const uint32 start_frame = argc > 5 ? strtoul (argv[5], NULL, 0) : 0;
    PatchSampleAttachment *att = NULL;
    PatchInstrument *inst;
    PatchSample *samp;
    Err errcode = 0;

        /* locate instrument from patch's instrument list */
    if ( (inst = FindPatchInstrument (patch, inst_name)) == NULL ) {
        PrintParserFileError (file, "Instrument \"%s\" undefined.", inst_name);
        errcode = -1;       /* !!! real error code */
        goto clean;
    }

        /* locate sample from patch's sample list */
    if ( (samp = FindPatchSample (patch, samp_name)) == NULL ) {
        PrintParserFileError (file, "Sample \"%s\" undefined.", samp_name);
        errcode = -1;       /* !!! real error code */
        goto clean;
    }

        /* do attachment */
    if ((att = CreatePatchSampleAttachment (att_name, inst, samp, fifo_name, start_frame, &errcode)) == NULL) {
        PrintParserFileError (file, "Unable to attach sample \"%s\" to %s.%s.", samp_name, inst_name, fifo_name ? fifo_name : "(default)");
        /* !!! get operror message? */
        goto clean;
    }

        /* add to SampleAttachmentList */
    if ((errcode = AddPatchSymbol (&patch->patch_SampleAttachmentList, (PatchSymbol *)att)) < 0) {
            /* !!! generalize this error message */
        PrintParserFileError (file, "Sample Attachment \"%s\" redefined.", att_name);
        goto clean;
    }

clean:
    if (errcode < 0) DeletePatchSymbol ((PatchSymbol *)att);
    return errcode;
}


/*
    Tweak <instrument> <knob> <value>

    Adjust a knob.

    !!! BUGS:
        . doesn't check pointer returned by strtol() for junk after number.
        . doesn't do bounds checking on knob_value.
*/
static Err cmdproc_Tweak (const ParserFile *file, Patch *patch, int argc, const char * const argv[])
{
    const char *inst_name  = argv[1];
    const char *knob_name  = argv[2];
    const int32 knob_value = strtol (argv[3], NULL, 0);
    Err errcode = 0;
    PatchInstrument *inst;
    PatchKnob *knob;

  #if DEBUG_LoadInterpreter
    printf ("  Tweak %s.%s %ld\n", inst_name, knob_name, knob_value);
  #endif

        /* locate instrument from patch's instrument list */
    if ((inst = FindPatchInstrument (patch, inst_name)) == NULL) {
            /* !!! this seems like a pretty common error message */
        PrintParserFileError (file, "Instrument \"%s\" undefined.", inst_name);
        errcode = -1;       /* !!! real error code */
        goto clean;
    }

        /* locate knob */
    if ((knob = FindPatchKnob (inst, knob_name)) == NULL) {
        PrintParserFileError (file, "Unknown knob %s.%s.", inst_name, knob_name);
        errcode = -1;       /* !!! real error code */
        goto clean;
    }

        /* tweak knob (!!! currently letting Tweak() catch ungrabbed knob. is this a good idea?) */
    if ((errcode = TweakRawKnob (knob->pknob_Knob, knob_value)) < 0) {
        PrintParserFileError (file, "Unable to tweak knob %s.%s%s.", inst_name, knob_name, IsPatchKnobGrabbed(knob) ? " - TweakRawKnob() failed" : " - instrument connected to it");
        /* !!! get operror message? */
        goto clean;
    }

clean:
    return errcode;
}


#if 0       /* useful for prototyping new commands */
static Err cmdproc_dummy (const ParserFile *file, Patch *patch, int argc, const char * const argv[])
{
    PrintParserFileError (file, "Warning: Command \"%s\" not implemented yet.", argv[0]);

    return 0;
}
#endif


/* -------------------- UnloadPatch() */

/*
    UnloadPatch - Unload a patch file loaded by LoadPatch()

    SYNOPSIS

        Err UnloadPatch (Patch *patch)

    DESCRIPTION

        !!!

    ARGUMENTS

        patch       Patch loaded by LoadPatch() or NULL.

    RETURN VALUE

        0 on success, error code on failure.

    IMPLEMENTATION

    ASSOCIATED FILES

    CAVEATS

    SEE ALSO

        LoadPatch()
*/

Err UnloadPatch (Patch *patch)
{
  #if DEBUG_LoadEntry
    printf ("UnloadPatch() patch=$%p ", patch); printavail(); printf ("\n");
  #endif

    if (patch) {
            /* delete symbols by name space */
        DeletePatchSymbolList (&patch->patch_SampleAttachmentList);     /* delete links between samples and instruments */
        DeletePatchSymbolList (&patch->patch_SampleList);               /* delete samples */
        DeletePatchSymbolList (&patch->patch_InstrumentList);           /* delete instruments */

            /* delete Patch */
        myfree (patch);
    }

  #if DEBUG_LoadEntry
    printf ("  "); printavail(); printf ("\n");
  #endif

    return 0;
}


/* -------------------- Trigger Patch */

/*
    Start all instruments in patch.

    Starts instruments in the order they were defined. Gives up on first
    error encountered while starting. Doesn't stop instruments it
    succeeded in starting on failure.

    Returns 0 on success, error code on failure.
*/
Err StartPatch (Patch *patch, const TagArg *taglist)
{
    struct PatchInstrument *inst;
    Err errcode = 0;

  #if DEBUG_Trigger
    printf ("StartPatch()\n");
  #endif

    for (inst = (PatchInstrument *)FirstNode(&patch->patch_InstrumentList); IsNode(&patch->patch_InstrumentList,inst); inst = (PatchInstrument *)NextNode(inst)) {
        if ((errcode = StartPatchInstrument (inst, taglist)) < 0) goto clean;
    }

clean:
  #if DEBUG_Trigger
    printf ("  err=%ld\n", errcode);
  #endif
    return errcode;
}

/*
    Release all instruments in patch.

    Releases instruments in the reverse order from the order they were
    defined. Releases all instrument regardless of encoutering an error
    midway.

    Returns first non-zero error code it finds, or 0 on success.
*/
Err ReleasePatch (Patch *patch, const TagArg *taglist)
{
    struct PatchInstrument *inst;
    Err errcode = 0;

  #if DEBUG_Trigger
    printf ("ReleasePatch()\n");
  #endif

    for (inst = (PatchInstrument *)LastNode(&patch->patch_InstrumentList); IsNodeB(&patch->patch_InstrumentList,inst); inst = (PatchInstrument *)PrevNode(inst)) {
        const Err terrcode = ReleasePatchInstrument (inst, taglist);

        if (errcode >= 0 && terrcode < 0) errcode = terrcode;
    }

  #if DEBUG_Trigger
    printf ("  err=%ld\n", errcode);
  #endif
    return errcode;
}

/*
    Stop all instruments in patch.

    Stops instruments in the reverse order from the order they were
    defined. Stops all instrument regardless of encoutering an error
    midway.

    Returns first non-zero error code it finds, or 0 on success.
*/
Err StopPatch (Patch *patch, const TagArg *taglist)
{
    struct PatchInstrument *inst;
    Err errcode = 0;

  #if DEBUG_Trigger
    printf ("StopPatch()\n");
  #endif

    for (inst = (PatchInstrument *)LastNode(&patch->patch_InstrumentList); IsNodeB(&patch->patch_InstrumentList,inst); inst = (PatchInstrument *)PrevNode(inst)) {
        const Err terrcode = StopPatchInstrument (inst, taglist);

        if (errcode >= 0 && terrcode < 0) errcode = terrcode;
    }

  #if DEBUG_Trigger
    printf ("  err=%ld\n", errcode);
  #endif
    return errcode;
}


/* -------------------- PatchInstrument */

static Err DestroyPatchInstrument (PatchInstrument *);

/*
    Create a new PatchInstrument.  Loads an instrument and creates a new instrument symbol.
*/
static PatchInstrument *CreatePatchInstrument (const char *symname, const char *filename, bool triggerable, Err *errbuf)
{
    static const PatchSymbolFunctions PatchInstrumentFunctions = {
        (PatchSymbolDestructor)DestroyPatchInstrument,
    };
    PatchInstrument *inst = NULL;
    Err errcode = 0;

  #if DEBUG_LoadInterpreter
    printf ("  CreatePatchInstrument() %s = \"%s\" triggerable=%d\n", symname, filename, triggerable);
  #endif

        /* allocate instrument symbol + file name space */
    if ((inst = (PatchInstrument *)CreatePatchSymbol (symname, sizeof *inst + strlen(filename) + 1, &PatchInstrumentFunctions, &errcode)) == NULL) goto clean;
    strcpy (inst->pinst_TemplateName = (char *)(inst + 1), filename);   /* set template file name */

        /* set attributes */
    if (triggerable) inst->pinst_AttrFlags |= PIATTRF_Triggerable;

        /* load instrument */
    {
            /* !!! this function should take a const filename */
        const Item result = LoadInstrument ((char *)filename, 0, PINST_Priority);

        if (result < 0) {
            errcode = result;
            goto clean;
        }
        inst->pinst_Instrument = result;
    }

        /* attach knob table */
    if ((errcode = AttachKnobTable (inst)) < 0) goto clean;

clean:
    if (errcode < 0) {
        DeletePatchSymbol ((PatchSymbol *)inst); inst = NULL;
        if (errbuf) *errbuf = errcode;
    }

  #if DEBUG_LoadInterpreter
    printf ("    inst=$%p err=%ld\n", inst, errcode);
  #endif

    return inst;
}

/*
    Destroy PatchInstrument loaded by CreatePatchInstrument.

    Called by DeletePatchSymbol().
    inst will never be NULL.
    called exactly once for a given PatchInstrument
    not allowed to free PatchInstrument, only its contents.
*/
static Err DestroyPatchInstrument (PatchInstrument *inst)
{
  #if DEBUG_LoadInterpreter
    printf ("  DestroyPatchInstrument() inst=$%p item=%ld\n", inst, inst->pinst_Instrument);
  #endif

    DetachKnobTable (inst);
    UnloadInstrument (inst->pinst_Instrument);

    return 0;
}


/*
    Trigger PatchInstrument

    @@@ this will eventually turn into a dispatcher function for both PatchInstrument and Patch classes.
*/

static Err StartPatchInstrument (PatchInstrument *inst, const TagArg *taglist)
{
    Err errcode = 0;

  #if DEBUG_Trigger
    printf ("  StartPatchInstrument() \"%s\" state=$%02x", inst->pinst_Symbol.psym_Node.n_Name, inst->pinst_StateFlags);
  #endif

        /* start instrument if it's triggerable or hasn't been started yet */
    if (inst->pinst_AttrFlags & PIATTRF_Triggerable || !(inst->pinst_StateFlags & PISTATEF_Started)) {
        inst->pinst_StateFlags |= PISTATEF_Started;

      #if DEBUG_Trigger
        printf (" started: state=$%02x", inst->pinst_StateFlags);
      #endif

        errcode = StartInstrument (inst->pinst_Instrument, (TagArg *)taglist);  /* !!! unnecessary cast */
    }

  #if DEBUG_Trigger
    printf ("\n");
  #endif

    return errcode;
}

static Err ReleasePatchInstrument (PatchInstrument *inst, const TagArg *taglist)
{
    Err errcode = 0;

  #if DEBUG_Trigger
    printf ("  ReleasePatchInstrument() \"%s\" state=$%02x", inst->pinst_Symbol.psym_Node.n_Name, inst->pinst_StateFlags);
  #endif

        /* release instrument if triggerable */
    if (inst->pinst_AttrFlags & PIATTRF_Triggerable) {
      #if DEBUG_Trigger
        printf (" released");
      #endif
        errcode = ReleaseInstrument (inst->pinst_Instrument, (TagArg *)taglist);    /* !!! unnecessary cast */
    }

  #if DEBUG_Trigger
    printf ("\n");
  #endif

    return errcode;
}

static Err StopPatchInstrument (PatchInstrument *inst, const TagArg *taglist)
{
  #if DEBUG_Trigger
    printf ("  StopPatchInstrument() \"%s\" state=$%02x", inst->pinst_Symbol.psym_Node.n_Name, inst->pinst_StateFlags);
  #endif

    inst->pinst_StateFlags &= ~PISTATEF_Started;

  #if DEBUG_Trigger
    printf (" stopped: state=$%02x\n", inst->pinst_StateFlags);
  #endif

        /* unconditionally stop instrument */
    return StopInstrument (inst->pinst_Instrument, (TagArg *)taglist);      /* !!! unnecessary cast */
}


#if 0       /* @@@ these could be used to build patch derived class methods for PatchInstrument, current flag checking code should become common */

/*
    Trigger PatchInstrument
*/
#if DEBUG_Trigger
  #define _TriggerPatchInstrumentTemplate(_phase) \
    static Err _phase##PatchInstrument (PatchInstrument *inst, const TagArg *taglist) \
    { \
        printf ("  "#_phase"PatchInstrument() \"%s\"\n", inst->pinst_Symbol.psym_Node.n_Name); \
        return _phase##Instrument (inst->pinst_Instrument, (TagArg *)taglist); /* !!! unnecessary cast */ \
    }
#else
  #define _TriggerPatchInstrumentTemplate(_phase) \
    static Err _phase##PatchInstrument (PatchInstrument *inst, const TagArg *taglist) \
    { \
        return _phase##Instrument (inst->pinst_Instrument, (TagArg *)taglist); /* !!! unnecessary cast */ \
    }
#endif

_TriggerPatchInstrumentTemplate(Start)      /* StartPatchInstrument() */
_TriggerPatchInstrumentTemplate(Release)    /* ReleasePatchInstrument() */
_TriggerPatchInstrumentTemplate(Stop)       /* StopPatchInstrument() */

#undef _TriggerPatchInstrumentTemplate

#endif      /* @@@ not used - see if above */


/* -------------------- KnobTable */

/* !!! might want to rethink these a bit in terms of OOP */
/*
    Attach pinst_KnobTable to inst.
*/
static Err AttachKnobTable (PatchInstrument *inst)
{
    Err errcode = 0;

  #if DEBUG_LoadInterpreter
    printf ("    AttachKnobTable() inst=$%p", inst);
  #endif

        /* get nknobs */
    {
        const int32 result = GetNumKnobs (inst->pinst_Instrument);

        if (result < 0) {
            errcode = result;
            goto clean;
        }
        inst->pinst_NKnobs = result;
    }

        /* create table if there are some knobs */
    if (inst->pinst_NKnobs) {

            /* alloc/clear table */
        if ((inst->pinst_KnobTable = (PatchKnob *)myalloc (inst->pinst_NKnobs * sizeof (PatchKnob))) == NULL) {
            errcode = -1;           /* !!! real error code */
            goto clean;
        }

            /* fill out table */
        {
            PatchKnob *knob = inst->pinst_KnobTable;
            int32 i=0;

            for (; i<inst->pinst_NKnobs; knob++, i++) {

                    /* get knob name */
                if ((knob->pknob_Name = GetKnobName (inst->pinst_Instrument, i)) == NULL) {
                    errcode = -1;           /* !!! real error code */
                    goto clean;
                }

                    /* grab knob */
                if ((errcode = GrabPatchKnob (inst->pinst_Instrument, knob)) < 0) goto clean;

            }
        }
    }

clean:
  #if DEBUG_LoadInterpreter
    printf (" tbl=$%p,%ld err=%ld\n", inst->pinst_KnobTable, inst->pinst_NKnobs, errcode);
  #endif

    if (errcode < 0) {
        DetachKnobTable (inst);
    }

    return errcode;
}

/*
    Detach pinst_KnobTable attached by AttachKnobTable().  inst must not be NULL.
*/
static Err DetachKnobTable (PatchInstrument *inst)
{
  #if DEBUG_LoadInterpreter
    printf ("    DetachKnobTable() inst=$%p tbl=$%p,%ld\n", inst, inst->pinst_KnobTable, inst->pinst_NKnobs);
  #endif

        /* free KnobTable if it was attached */
    if (inst->pinst_KnobTable) {
        PatchKnob *knob = inst->pinst_KnobTable;
        int32 nknobs = inst->pinst_NKnobs;

        for (; nknobs--; knob++) ReleasePatchKnob (knob);

        myfree (inst->pinst_KnobTable);
    }

        /* clear pointers since Attach set them */
    inst->pinst_KnobTable = NULL;
    inst->pinst_NKnobs    = 0;

    return 0;
}

/*
    Find a PatchKnob in an instruments KnobTable by name (case insensitive).
*/
static PatchKnob *FindPatchKnob (const PatchInstrument *inst, const char *knobname)
{
    PatchKnob *knob = inst->pinst_KnobTable;
    int32 nknobs    = inst->pinst_NKnobs;

    for (; nknobs--; knob++) {
        if (!strcasecmp (knob->pknob_Name, knobname)) return knob;
    }

    return NULL;
}

/*
    Count # of grabbed knobs
*/
int32 GetNGrabbedPatchKnobs (const PatchInstrument *inst)
{
    PatchKnob *knob = inst->pinst_KnobTable;
    int32 nknobs    = inst->pinst_NKnobs;
    int32 ngrabbed  = 0;

    for (; nknobs--; knob++) {
        if (IsPatchKnobGrabbed(knob)) ngrabbed++;
    }

    return ngrabbed;
}


/* -------------------- PatchKnob management */

/*
    Grab a PatchKnob.  Return 0 on success, error code on failure.

    Handles condition of knob already grabbed, but these functions do not nest.
*/
static Err GrabPatchKnob (Item instrument, PatchKnob *knob)
{
    Err errcode = 0;

    if (!IsPatchKnobGrabbed (knob)) {
        const Item result = GrabKnob (instrument, knob->pknob_Name);

        if (result < 0) errcode          = result;
        else            knob->pknob_Knob = result;
    }

    return errcode;
}

/*
    Release a PatchKnob.  Return 0 on success, error code on failure.
*/
static Err ReleasePatchKnob (PatchKnob *knob)
{
    Err errcode = 0;

    if (IsPatchKnobGrabbed (knob)) {
        errcode = ReleaseKnob (knob->pknob_Knob);
        knob->pknob_Knob = 0;
    }

    return errcode;
}


/* -------------------- PatchSample */

static Err DestroyPatchSample (PatchSample *);
static Err DestroyPatchDelayLine (PatchSample *);

/*
    Create a new PatchSample.  Loads a sample and creates a new sample symbol.
*/
static PatchSample *CreatePatchSample (const char *symname, const char *filename, Err *errbuf)
{
    static const PatchSymbolFunctions PatchSampleFunctions = {
        (PatchSymbolDestructor)DestroyPatchSample,
    };
    PatchSample *samp = NULL;
    Err errcode = 0;

  #if DEBUG_LoadInterpreter
    printf ("  CreatePatchSample() %s = \"%s\"\n", symname, filename);
  #endif

        /* allocate sample symbol + file name space */
    if ((samp = (PatchSample *)CreatePatchSymbol (symname, sizeof *samp + strlen(filename) + 1, &PatchSampleFunctions, &errcode)) == NULL) goto clean;
    strcpy (samp->psamp_FileName = (char *)(samp + 1), filename);       /* set file name */

        /* load sample */
    {
            /* !!! this function should take a const filename */
        const Item result = LoadSample ((char *)filename);

        if (result < 0) {
            errcode = result;
            goto clean;
        }
        samp->psamp_Sample = result;
    }

clean:
    if (errcode < 0) {
        DeletePatchSymbol ((PatchSymbol *)samp); samp = NULL;
        if (errbuf) *errbuf = errcode;
    }

  #if DEBUG_LoadInterpreter
    printf ("    samp=$%p err=%ld\n", samp, errcode);
  #endif

    return samp;
}

/*
    Destroy PatchSample loaded by CreatePatchSample.

    Called by DeletePatchSymbol().
    samp will never be NULL.
    called exactly once for a given PatchSample
    not allowed to free PatchSample, only its contents.
*/
static Err DestroyPatchSample (PatchSample *samp)
{
  #if DEBUG_LoadInterpreter
    printf ("  DestroyPatchSample() samp=$%p item=%ld\n", samp, samp->psamp_Sample);
  #endif

    UnloadSample (samp->psamp_Sample);

    return 0;
}


/*
    Create a new DelayLine PatchSample.  Creates a DelayLine and a new sample symbol.
*/
static PatchSample *CreatePatchDelayLine (const char *symname, uint32 nbytes, uint32 nchannels, bool loop, Err *errbuf)
{
    static const PatchSymbolFunctions PatchDelayLineFunctions = {
        (PatchSymbolDestructor)DestroyPatchDelayLine,
    };
    PatchSample *samp = NULL;
    Err errcode = 0;

  #if DEBUG_LoadInterpreter
    printf ("  CreatePatchDelayLine() %s: nbytes=%lu nchannels=%lu loop=%d\n", symname, nbytes, nchannels, loop);
  #endif

        /* allocate sample symbol */
    if ((samp = (PatchSample *)CreatePatchSymbol (symname, sizeof *samp, &PatchDelayLineFunctions, &errcode)) == NULL) goto clean;

        /* create delay line */
    {
        const Item result = CreateDelayLine (nbytes, nchannels, loop);

        if (result < 0) {
            errcode = result;
            goto clean;
        }
        samp->psamp_Sample = result;
    }

clean:
    if (errcode < 0) {
        DeletePatchSymbol ((PatchSymbol *)samp); samp = NULL;
        if (errbuf) *errbuf = errcode;
    }

  #if DEBUG_LoadInterpreter
    printf ("    samp=$%p err=%ld\n", samp, errcode);
  #endif

    return samp;
}

/*
    Destroy DelayLine PatchSample created by CreatePatchDelayLine.

    Called by DeletePatchSymbol().
    samp will never be NULL.
    called exactly once for a given PatchSample
    not allowed to free PatchSample, only its contents.
*/
static Err DestroyPatchDelayLine (PatchSample *samp)
{
  #if DEBUG_LoadInterpreter
    printf ("  DestroyPatchDelayLine() samp=$%p item=%ld\n", samp, samp->psamp_Sample);
  #endif

    DeleteDelayLine (samp->psamp_Sample);

    return 0;
}


/* -------------------- PatchSampleAttachment */

static Err DestroyPatchSampleAttachment (PatchSampleAttachment *);

/*
    Create a new PatchSampleAttachment.  Attaches a sample to an instrument and creates a new sample attachment symbol.
*/
static PatchSampleAttachment *CreatePatchSampleAttachment (const char *symname, const PatchInstrument *inst, const PatchSample *samp, const char *fifoname, uint32 startframe, Err *errbuf)
{
    static const PatchSymbolFunctions PatchSampleAttachmentFunctions = {
        (PatchSymbolDestructor)DestroyPatchSampleAttachment,
    };
    PatchSampleAttachment *att = NULL;
    Err errcode = 0;

  #if DEBUG_LoadInterpreter
    printf ("  CreatePatchSampleAttachment() \"%s\" = %s <-> %s.%s start=%lu\n", symname, samp->psamp_Symbol.psym_Node.n_Name, inst->pinst_Symbol.psym_Node.n_Name, fifoname ? fifoname : "(default)", startframe);
  #endif

        /* allocate attachment symbol */
    if ((att = (PatchSampleAttachment *)CreatePatchSymbol (symname, sizeof *att, &PatchSampleAttachmentFunctions, &errcode)) == NULL) goto clean;

        /* attach sample */
    {
            /* !!! this function should take a const fifoname */
        const Item result = AttachSample (inst->pinst_Instrument, samp->psamp_Sample, (char *)fifoname);

        if (result < 0) {
            errcode = result;
            goto clean;
        }
        att->psatt_Attachment = result;
    }

        /* set attachment start frame */
    {
        TagArg tags[] = {
            { AF_TAG_START_AT },
            TAG_END
        };
        tags[0].ta_Arg = (void *)startframe;

        if ( (errcode = SetAudioItemInfo (att->psatt_Attachment, tags)) < 0 ) goto clean;
    }

clean:
    if (errcode < 0) {
        DeletePatchSymbol ((PatchSymbol *)att); att = NULL;
        if (errbuf) *errbuf = errcode;
    }

  #if DEBUG_LoadInterpreter
    printf ("    att=$%p err=%ld\n", att, errcode);
  #endif

    return att;
}

/*
    Destroy PatchSampleAttachment created by CreatePatchSampleAttachment.

    Called by DeletePatchSymbol().
    att will never be NULL.
    called exactly once for a given PatchSampleAttachment
    not allowed to free PatchSampleAttachment, only its contents.
*/
static Err DestroyPatchSampleAttachment (PatchSampleAttachment *att)
{
  #if DEBUG_LoadInterpreter
    printf ("  DestroyPatchSampleAttachment() att=$%p item=%ld\n", att, att->psatt_Attachment);
  #endif

    DetachSample (att->psatt_Attachment);

    return 0;
}


/* -------------------- PatchSymbol List */

/*
    Add a PatchSymbol to tail of a Symbol List.

    Returns an error if newsym name isn't unique in symlist.
*/
static Err AddPatchSymbol (List *symlist, PatchSymbol *newsym)
{
    const char *symname = newsym->psym_Node.n_Name;
    Err errcode = 0;

  #if DEBUG_Symbol
    printf ("  AddPatchSymbol() list: $%p %s  sym: $%p %s\n", symlist, symlist->l.n_Name, newsym, symname);
  #endif

        /* trap symbol name already in use */
    if (FindPatchSymbol (symlist, symname)) {
        errcode = -1;   /* !!! real error code */
        goto clean;
    }

        /* add symbol to list */
    AddTail (symlist, (Node *)newsym);

clean:
    return errcode;
}

/*
    Delete an entire list full of PatchSymbols.

    list can be empty, but must not be NULL.
    Empties list, but doesn't actually free the list structure.
    List is a legal empty list when complete.
*/
static Err DeletePatchSymbolList (List *symlist)
{
    PatchSymbol *symbol;

  #if DEBUG_Symbol
    printf ("  DeletePatchSymbolList() list: $%p %s\n", symlist, symlist->l.n_Name);
  #endif

    while ((symbol = (PatchSymbol *)RemTail (symlist)) != NULL) DeletePatchSymbol (symbol);

    return 0;
}


/* -------------------- PatchSymbol */

/*
    Create a new PatchSymbol structure.

    Allocate enough extra space after PatchSymbol to hold symname and copy it.
*/
static PatchSymbol *CreatePatchSymbol (const char *symname, size_t usersize, const PatchSymbolFunctions *functions, Err *errbuf)
{
    const size_t allocsize = usersize + strlen(symname) + 1;
    PatchSymbol *symbol = NULL;
    Err errcode = 0;

  #if DEBUG_Symbol
    printf ("  CreatePatchSymbol() name=%s size=%ld,%ld functions=$%p\n", symname, usersize, allocsize, functions);
  #endif

        /* allocate requested size + space for symbol name */
    if ((symbol = (PatchSymbol *)myalloc (allocsize)) == NULL) {
        errcode = -1;           /* !!! real error code */
        goto clean;
    }

        /* init fields */
    strcpy (symbol->psym_Node.n_Name = (char *)symbol + usersize, symname);
    symbol->psym_Node.n_Size = allocsize;
    symbol->psym_Functions = functions;

clean:
    if (errcode < 0) {
        CloseRes (DeletePatchSymbol, symbol);
        if (errbuf) *errbuf = errcode;
    }

  #if DEBUG_Symbol
    printf ("    symbol=$%p err=%ld\n", symbol, errcode);
  #endif

    return symbol;
}

/*
    Delete PatchSymbol created by CreatePatchSymbol().

    symbol can be NULL or client data (after PatchSymbol structure) can be partially initialized.
    Dispatches destructor for symbol when symbol is non-NULL.
*/
static Err DeletePatchSymbol (PatchSymbol *symbol)
{
  #if DEBUG_Symbol
    printf ("  DeletePatchSymbol() symbol=$%p name=%s functions=$%p\n", symbol, symbol ? symbol->psym_Node.n_Name : "<null>", symbol ? symbol->psym_Functions : NULL);
  #endif

    if (symbol) {
        symbol->psym_Functions->psymfn_Destructor (symbol);
        myfree (symbol);
    }

    return 0;
}



/* -------------------- ParserFile */

/*
    Open a ParserFile.  filename must remain valid for life of returned ParserFile.
*/
static ParserFile *OpenParserFile (const char *filename, Err *errbuf)
{
    ParserFile *parserfile = NULL;
    Err errcode = 0;

        /* alloc/clear/init ParserFile */
    if ((parserfile = (ParserFile *)myalloc (sizeof *parserfile)) == NULL) {
        errcode = -1;       /* !!! real error code */
        goto clean;
    }
    parserfile->pfile_FileName = filename;

        /* open file */
        /* !!! this function should take a const char * */
    if ((parserfile->pfile_Stream = OpenDiskStream ((char *)filename, 0)) == NULL) {
        errcode = -1;       /* !!! real error code */
        goto clean;
    }

clean:
    if (errcode < 0) {
        CloseRes (CloseParserFile, parserfile);
        if (errbuf) *errbuf = errcode;
    }

    return parserfile;
}

/*
    Close ParserFile opened by OpenParserFile().
*/
static Err CloseParserFile (ParserFile *parserfile)
{
    if (parserfile) {
        CloseRes (CloseDiskStream, parserfile->pfile_Stream);
        myfree (parserfile);
    }

    return 0;
}

/*
    Read line from ParserFile.
    Return # of characters read into buffer or error code.
    Increment LineNum if successful.
*/
static int32 GetParserFileLine (ParserFile *parserfile, char *buf, int32 bufsize)
{
    int32 result;

    if ((result = GetStreamLine (parserfile->pfile_Stream, buf, bufsize)) > 0) {
        parserfile->pfile_LineNum++;
    }

    return result;
}


/*
    Print a parser error message
*/
static void PrintParserFileError (const ParserFile *file, const char *errorfmt, ...)
{
    printf ("\"%s\" line %ld: ", file->pfile_FileName, file->pfile_LineNum);

    {
        va_list vargs;

        va_start (vargs, errorfmt);
        vprintf (errorfmt, vargs);
        va_end (vargs);
    }

    printf ("\n");
}


/* -------------------- stream support */

/*
    GetStreamLine - Read a line from a Stream.

    SYNOPSIS

        int32 GetStreamLine (Stream *stream, char *buf, int32 bufsize)

    DESCRIPTION

        Reads line from a Stream one character at a time until one of the
        following happens:
            . a new line character is encountered.
            . end of file is reached.
            . client buffer is full.

        No more than bufsize-1 characters are read from the Stream.  The
        client buffer is always NULL terminated.  If found, the new line
        character is left in the client buffer.  This permits the client
        to determine if the buffer was too short to hold the line or EOF
        was reached w/o a terminating new line character.

        This function behaves more or less the same way as the standard C
        library function fgets().

    ARGUMENTS

        stream      Stream created by OpenDiskStream().

        buf         Pointer to client buffer.

        bufsize     sizeof client buffer.  This size includes the NULL
                    termination.

    RETURN VALUE

        >0: number of characters placed in buf (e.g. strlen(buf)).

        0:  end of file was reached (there wasn't a single character left
            in the file to place in the buffer).

        <0: error code

    IMPLEMENTATION

    ASSOCIATED FILES

    CAVEATS

        stream and buf must be non-NULL.
        bufsize must be > 0.

    SEE ALSO

        LoadPatch()
*/

static int32 GetStreamLine (Stream *stream, char *buf, int32 bufsize)
{
    char *s = buf;
    int32 n = bufsize-1;        /* bufsize-1 to leave room for null termination */
    int32 actual;

    while (n--) {
        if ((actual = ReadDiskStream (stream, s, 1)) < 0) return actual;    /* read character to buf, return on error */
        if (!actual) break;                                                 /* simply leave loop on EOF */
        if (*s == '\r') *s = '\n';                                          /* !!! hack to convert mac EOL to somethine sensible.  causes trouble for pc EOL's though. */
        if (*s++ == '\n') break;                                            /* increment buffer pointer, leave loop if new line */
    }
    *s = '\0';                  /* terminate string */

    return s - buf;             /* return string length */
}


/* -------------------- command line arg processing */

static char *trimline (char *);
static int getargs (char *s, char *argv[], int argvnelts);
static char *breakword (char *s, char **newp);

/*
    Parse command line:
        . strip off comment.
        . build argument array.

    Returns filled out argv[] array and argc as function return value.
*/
static int ParseLine (char *s, char *argv[], int argvnelts)
{
    int argc;

  #if DEBUG_LoadParser
    printf ("    ParseLine() \"%s\" len=%ld\n", s, strlen(s));
  #endif

    argc = getargs (trimline(s), argv, argvnelts);

  #if DEBUG_LoadParser
    {
        int i;

        for (i=0; i<argc; i++) {
            printf ("      %d: $%p \"%s\" %ld\n", i, argv[i], argv[i], strlen(argv[i]));
        }
    }
  #endif

    return argc;
}

/*
    Build argv[] from source string, return # of args.

    Breaks original string bufffer.
*/
static int getargs (char *s, char *argv[], int argvnelts)
{
    char **targv = argv;
    int n = argvnelts-1;        /* leave room for NULL termination of array */

        /* loop until space exhausted or no more words in s */
    while (n-- && (*targv = breakword (s, &s)) != NULL) targv++;

        /* NULL terminate array */
    *targv = NULL;

    return targv - argv;
}


/*
    Break string at first instance of new line, pound sign, or semicolon
    by writing '\0' there.  Returns original buffer pointer.
*/
static char *trimline (char *buf)
{
    char *s;

        /* replace first occurance of EOL or comment char w/ '\0' */
    if ((s = strpbrk(buf, ";#\n")) != NULL) *s = '\0';

    return buf;
}


/*
    SYNOPSIS

        char *breakword (char *s, char **newp)

    ARGUMENTS

        s       Pointer to character to begin string search.  Can point
                to any character in a string including the null
                termination.  Must not be NULL.

        newp    Pointer to variable to store value of s for next pass.
                Can point to same variable containing s.  The result is
                never NULL, but could point to white space or the end of
                the string.

    RETURN VALUE

        Pointer to beginning of a word or NULL if no more found.  Never
        returns a pointer to the end of the string or white space.

    NOTES
        @@@ could be extended to support quotes (although, comment
            location would then have to occur here instead of separate)
*/

static char *breakword (char *s, char **newp)
{
    char *wordp;

        /* skip leading white space */
    s = skipspace(s);

        /* get word pointer, if there is one */
    wordp = *s ? s : NULL;

        /* locate end of word (does nothing if already at end of string) */
    s = skipnspace(s);

        /* if not at end of string, break string and get pointer to next character */
    if (*s) *s++ = '\0';

        /* set pointer to current string location for next call to breakword(). */
    *newp = s;

    return wordp;
}


/* -------------------- utility */

/*
    Locate a named node w/ case insensitivity
*/
static Node *FindNamedNodeNoCase (const List *list, const char *name)
{
    Node *node;

    for (node = FirstNode(list); IsNode(list,node); node = NextNode(node)) {
        if (!strcasecmp (node->n_Name, name)) return node;
    }

    return NULL;
}


/* -------------------- string.h helpers and emulation (!!! publish) */

/*
    Return pointer to first non-whitespace character in string,
    or end of string if none found.
*/
static char *skipspace (char *s)
{
    while (*s && isspace(*s)) s++;
    return s;
}

/*
    Return pointer to first whitespace character in string,
    or end of string if none found.
*/
static char *skipnspace (char *s)
{
    while (*s && !isspace(*s)) s++;
    return s;
}

static char *strpbrk (const char *cs, const char *ct)
{
    for (; *cs; cs++) {
        if (strchr (ct, *cs)) return (char *)cs;
    }

    return NULL;
}

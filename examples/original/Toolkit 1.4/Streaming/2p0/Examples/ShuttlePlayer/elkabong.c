/*  :ts=8 bk=0
 *
 * elkabong.c:	Defense against bug in filesystem version XX.XX.
 *		Nomenclature by Dave Platt.
 *
 * Description Of Bug:
 *	The Opera Filesystem (release versions 1.1 and earlier) have a bug
 *	that may cause synchronous I/O operations to jam, manifesting as a
 *	program lockup.  This bug is triggered by posting a high-priority
 *	asynchronous I/O request to a file, then AbortIO()ing it before it
 *	is processed.  The filesystem's internal priority level is not
 *	reset, and the next I/O request to arrive will be queued but not
 *	processed (since it is believed to be of lower priority).  However,
 *	it's arrival will trigger a recalculation of the internal priority
 *	level, and subsequent I/O requests will be processed, in addition to
 *	releasing and processing the jammed one.
 *
 * Workaround Theory Of Operation:
 *	This source module basically caters to the problem by posting an
 *	asynchronous CMD_READ request which is anticipated to jam.  Then a
 *	second synchronous request is issued which is expected to complete
 *	and un-jam the first.  All subsequent I/O should then behave
 *	normally.
 *
 *	Note:  The file must be open before this operation is done, since
 *	OpenDiskFile() may cause a read of the directory.  Such a read would
 *	jam, and OpenDiskFile() would not complete.
 *
 *	This operation is not performed on filesystem versions later than
 *	XX.XX.
 *
 * Leo L. Schwab					9311.07
 *  Minor sanity check on returned block size.		9311.16
 */

#ifdef LEO
#include <filefolioglue.h>
#include <filefunctions.h>
#include <filesystem.h>
#include <mem.h>
#include <types.h>
#else
#include "Init3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#endif

/***************************************************************************
 * #defines
 */
#define KABONG_VERSION 0 /*  WHAT ARE THESE REALLY???  */
#define KABONG_REVISION 0

#define DEFAULT_BLOCK_SIZE 2048

/***************************************************************************
 * Prototypes.
 */
int32 performElKabong (void *buf);
int32 initElKabong (char *filename);
void closeElKabong (void);
int elKabongRequired (void);

/***************************************************************************
 * Static globals.
 */
static Item file, jamio, unjamio;
static int32 blocksize;

/***************************************************************************
 * Code.
 */

/***************************************************************************
 * Perform filesystem bug workaround.  'buf' points to a buffer suitable
 * for depositing the dummy block (the client may pass in a pointer to an
 * allocated but as-yet unused buffer if desired).  The buffer must be of
 * sufficient size to accomodate a single block on the prevailing volume.
 * If 'buf' is NULL, a temporary buffer is created internally.
 *
 * Returns zero if successful; negative error code if anything went wrong.
 */
int32 performElKabong (buf) void *buf;
{
  IOInfo jaminf, unjaminf;
  void *mybuf = NULL;
  int32 retval;

  if (!elKabongRequired ())
    /*
     * If this version of the filesystem folio is newer than the
     * one with the bug in it, DO NOTHING.
     */
    return (0);

  /*
   * If the client has not passed in their own buffer, we create
   * our own temporary buffer.
   */
  if (!buf)
    {
      if (!(mybuf = AllocMem (blocksize, 0)))
        {
#ifdef DEBUG
          kprintf ("Can't allocate tmp disk buffer.\n");
#endif
          return (-1);
        }
      buf = mybuf;
    }

  /*
   * Setup two identical reads for a single block.
   */
  memset (&jaminf, 0, sizeof (jaminf));
  jaminf.ioi_Command = CMD_READ;
  jaminf.ioi_Recv.iob_Buffer = buf;
  jaminf.ioi_Recv.iob_Len = blocksize;

  unjaminf = jaminf;

  /*
   * The first transaction (SendIO()) will jam, but cause the
   * filesystem to reconsider reality.  The second transaction (DoIO())
   * will go through, and cause the first to be released.  All
   * subsequent I/O will then operate normally.
   */
  if (!(retval = SendIO (jamio, &jaminf)))
    retval = DoIO (unjamio, &unjaminf);

  /*
   * Free our temporary buffer, if present.
   */
  if (mybuf)
    FreeMem (mybuf, blocksize);

  /*
   * Let upstairs know about how we did.
   */
  return (retval);
}

/***************************************************************************
 * Basic housekeeping.
 *****
 * Set up facilities to work around filesystem bug.  'filename' is the name
 * of a file the client wishes to use as a dummy file.  THIS FILE MUST
 * RESIDE ON THE SAME VOLUME ON WHICH YOU ANTICIPATE LOCKUPS.  If 'filename'
 * is NULL, the file containing the current directory is used.
 *
 * Returns zero if successful.  If anything goes wrong, a negative error
 * code is returned and nothing is allocated.
 */
int32
initElKabong (filename)
char *filename;
{
  FileStatus fstat;
  IOInfo ioi;
  int32 retval;
  char *errstr;

  if (!elKabongRequired ())
    /*
     * If this version of the filesystem folio is newer than the
     * one with the bug in it, DO NOTHING.
     */
    return (0);

  if (!filename)
    /*
     * Use the current directory if no preferred filename given.
     */
    filename = ".";

  /*
   * Open the file.
   */
  if ((retval = OpenDiskFile (filename)) < 0)
    {
      errstr = "Can't open kabonging file.\n";
      goto imdeadjim; // Look down.
    }
  file = retval;

  /*
   * Create the I/O requests that will be used to clobber some sense
   * into the filesystem.
   */
  if ((retval = CreateIOReq (NULL, 199, file, 0)) < 0)
    {
      errstr = "Can't create jamming I/O.\n";
      goto imdeadjim; // Look down.
    }
  jamio = retval;

  if ((retval = CreateIOReq (NULL, 199, file, 0)) < 0)
    {
      errstr = "Can't create unjamming I/O.\n";
      goto imdeadjim; // Look down.
    }
  unjamio = retval;

  /*
   * Find the prevailing block size on the filesystem.
   */
  memset (&ioi, 0, sizeof (ioi));
  ioi.ioi_Command = CMD_STATUS;
  ioi.ioi_Recv.iob_Buffer = &fstat;
  ioi.ioi_Recv.iob_Len = sizeof (fstat);
  if (retval = DoIO (jamio, &ioi))
    {
      errstr = "CMD_STATUS failed on kabonging file.\n";
      goto imdeadjim; // Look down.
    }
  if ((blocksize = fstat.fs.ds_DeviceBlockSize) <= 0)
    blocksize = DEFAULT_BLOCK_SIZE;

  /*
   * If there was an error, go here, clean up, and output a diagnostic.
   */
  if (retval)
    {
    imdeadjim:
      closeElKabong ();
#ifdef DEBUG
      kprintf (errstr);
      PrintfSysErr (retval);
#endif
    }

  return (retval);
}

/***************************************************************************
 * Cleanup resources allocated to deal with filesystem bug.
 */
void
closeElKabong ()
{
  if (file > 0)
    CloseDiskFile (file), file = 0;
  if (unjamio > 0)
    DeleteIOReq (unjamio), unjamio = 0;
  if (jamio > 0)
    DeleteIOReq (jamio), jamio = 0;
}

/***************************************************************************
 * This routine checks the version and revision numbers of the filesystem
 * folio against the last version known to have the priority sorting bug.
 * If the version currently running is newer than this, the bug is presumed
 * to have been fixed, and FALSE is returned.  Else, TRUE is returned.
 */

int
elKabongRequired ()
{
  FileFolio *ff;

  //	ff = GetFileFolio ();
  //	return (ff->ff.fn.n_Version < KABONG_VERSION  ||
  //		(ff->ff.fn.n_Version == KABONG_VERSION  &&
  //		 ff->ff.fn.n_Revision <= KABONG_REVISION));

  return true;
}

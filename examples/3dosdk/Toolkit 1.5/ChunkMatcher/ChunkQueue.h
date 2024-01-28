/*******************************************************************************************
 *	File:			ChunkQueue.h
 *
 *	Contains:		Queue of chunk objects.
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __CHUNKQUEUE__
#define __CHUNKQUEUE__

class TChunkListElement;

/*************/
/*  CLASSES  */
/*************/

class TChunkQueue
{
public:
  TChunkQueue ();

  virtual ~TChunkQueue ();

  void Add (TChunk *chunk);
  // Add the specified chunk to the tail of this queue

  TChunk *Remove ();
  // Remove a number of chunks from the head of this queue

  void RemoveAll ();
  // Remove all chunks from this queue

  Boolean IsEmpty () const;
  // Return true if this queue is empty

private:
  friend class TChunkQueueIterator;
  // Iterator is allowed to paw through the queue representation

  TChunkQueue (const TChunkQueue &) {}
  void
  operator= (const TChunkQueue &)
  {
  }
  // Copying and assignment are disallowed

  TChunkListElement *head; // pointer to head of this queue
  TChunkListElement *tail; // pointer to tail of this queue
};

class TChunkQueueIterator
{
public:
  TChunkQueueIterator (const TChunkQueue *chunkQueue);

  virtual ~TChunkQueueIterator ();

  void Reset ();
  // Reset this iterator to the beginning of the queue

  TChunk *GetNext ();
  // Return the next chunk from the queue

private:
  TChunkQueueIterator (const TChunkQueueIterator &) {}
  void
  operator= (const TChunkQueueIterator &)
  {
  }
  // Copying and assignment are disallowed

  const TChunkQueue *iterQueue; // associated chunk queue
  TChunkListElement *next;      // pointer to next element in queue
};

#endif

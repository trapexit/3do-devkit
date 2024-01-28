/*******************************************************************************************
 *	File:			ChunkQueue.cp
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

#include "Chunk.h"
#include "ChunkFileFormat.h"
#include "ChunkQueue.h"
#include "TypeID.h"
#include <files.h>
#include <iostream.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

/*********************/
/*  PRIVATE CLASSES  */
/*********************/

class TChunkListElement
{
private:
  friend class TChunkQueue;
  friend class TChunkQueueIterator;

  TChunkListElement () : chunk (NULL), link (NULL) {}

  TChunkListElement (TChunk *chunkPtr) : chunk (chunkPtr), link (NULL) {}

  ~TChunkListElement () {}

  TChunk *chunk; // pointer to chunk object represented by this list element
  TChunkListElement *link; // link to next element in the list
};

/*****************************/
/*  PUBLIC MEMBER FUNCTIONS  */
/*****************************/

/***************
 * TChunkQueue
 ***************
 * Null constructor
 */

TChunkQueue::TChunkQueue ()

{
  this->head = NULL;
  this->tail = NULL;
}

/****************
 * ~TChunkQueue
 ****************
 * Destructor
 */

TChunkQueue::~TChunkQueue () { this->RemoveAll (); }

/***************
 * TChunkQueue
 ***************
 * Add
 ***************
 * Add the specified chunk to the tail
 * of this queue.
 */

void
TChunkQueue::Add (TChunk *chunk)

{
  TChunkListElement *element; // new element to add to queue

  // Allocate a new element to represent the chunk object

  element = new TChunkListElement (chunk);

  // Add the element to the tail of the queue

  if (element != NULL)
    {
      if (this->tail == NULL)
        // Queue is empty, so head will point at new element also
        this->head = element;
      else
        // Queue is not empty, so link new element at tail
        this->tail->link = element;

      // Update tail to point at new element

      this->tail = element;
    }
}

/***************
 * TChunkQueue
 ***************
 * Remove
 ***************
 * Remove the chunk at the head of this queue.
 * Result is a pointer to the removed chunk.
 */

TChunk *
TChunkQueue::Remove ()

{
  TChunkListElement *element; // element at the head of this queue
  TChunk *chunk;              // chunk represented by the head element

  element = this->head;

  // Short-circuit for an empty queue

  if (element == NULL)
    return NULL;

  if (element == this->tail)
    {
      // One element in queue; transition to empty queue
      this->head = NULL;
      this->tail = NULL;
    }
  else
    // Unlink head element
    this->head = this->head->link;

  // Delete the list element and return
  // the chunk pointer it contained

  chunk = element->chunk;
  delete element;

  return chunk;
}

/***************
 * TChunkQueue
 ***************
 * IsEmpty
 ***************
 * Result indicates whether this queue is empty.
 */

Boolean
TChunkQueue::IsEmpty () const

{
  return this->head == NULL ? true : false;
}

/***********************
 * TChunkQueueIterator
 ***********************
 * Constructor
 ***********************
 *
 */

TChunkQueueIterator::TChunkQueueIterator (const TChunkQueue *chunkQueue)

{
  this->iterQueue = chunkQueue;
  this->next = chunkQueue->head;
}

/***********************
 * TChunkQueueIterator
 ***********************
 * Destructor
 ***********************
 *
 */

TChunkQueueIterator::~TChunkQueueIterator () {}

/***********************
 * TChunkQueueIterator
 ***********************
 * Reset
 ***********************
 * Reset this iterator to the beginning of the queue.
 */

void
TChunkQueueIterator::Reset ()

{
  this->next = this->iterQueue->head;
}

/***********************
 * TChunkQueueIterator
 ***********************
 * GetNext
 ***********************
 * Return the next chunk from the queue.
 */

TChunk *
TChunkQueueIterator::GetNext ()

{
  TChunk *nextChunk; // pointer to next chunk on queue

  nextChunk = NULL;

  if (this->next != NULL)
    {
      nextChunk = this->next->chunk;
      this->next = this->next->link;
    }

  return nextChunk;
}

/***************
 * TChunkQueue
 ***************
 * RemoveAll
 ***************
 * Remove all elements from this queue.
 */

void
TChunkQueue::RemoveAll ()

{
  while (this->Remove () != NULL)
    ;
}

/******************************/
/*  PRIVATE MEMBER FUNCTIONS  */
/******************************/

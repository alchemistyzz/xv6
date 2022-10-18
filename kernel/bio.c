// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKETS 13

uint 
hash(uint n){
  return n%NBUCKETS;
}


struct {
  struct buf buf[NBUF];

  // Linked list of all buffers, through bcacheprev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hash_buckets[NBUCKETS];
  struct spinlock bucketslock[NBUCKETS];
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i=0;i<NBUCKETS;i++){
    initlock(&bcache.bucketslock[i],"bcache.lock");
    bcache.hash_buckets[i].prev = &bcache.hash_buckets[i];
    bcache.hash_buckets[i].next = &bcache.hash_buckets[i];
  }


  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hash_buckets[0].next;
    b->prev = &bcache.hash_buckets[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hash_buckets[0].next->prev = b;
    bcache.hash_buckets[0].next = b;
  }
}

struct buf*
bfind(int i,int needlock)
{//param:
//i:hash_id
//needlock:if having the lock we can protect this atomic process
  if(needlock){
    acquire(&bcache.bucketslock[i]);
  }
  for(struct buf *b =bcache.hash_buckets[i].prev;b!=&bcache.hash_buckets[i];b=b->prev){
    if(b->refcnt==0){
      b->prev->next = b->next;
      b->next->prev = b->prev;
      b->prev = 0;
      b->next = 0;
      if(needlock){
        release(&bcache.bucketslock[i]); 
      }
      return b;
    }
  }
  if(needlock){
    release(&bcache.bucketslock[i]);
  }
  return 0;
};


// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  uint hash_id = hash(blockno);
  struct buf *b;
  acquire(&bcache.bucketslock[hash_id]);
  // Is the block already cached?
  for(b = bcache.hash_buckets[hash_id].next; b != &bcache.hash_buckets[hash_id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucketslock[hash_id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Not cached; recycle an unused buffer.
  
  b = bfind(hash_id,0);
  if(!b)
    for(int i = (hash_id + 1) % NBUCKETS; i != hash_id; i = (i + 1) % NBUCKETS){
        b = bfind(i, 1);
        if(b) break;
    }
      
        
 if(b){
    b->next = bcache.hash_buckets[hash_id].next;
    b->prev = &bcache.hash_buckets[hash_id];
    bcache.hash_buckets[hash_id].next->prev = b;
    bcache.hash_buckets[hash_id].next = b;

    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bcache.bucketslock[hash_id]);
    acquiresleep(&b->lock);
    return b;
  }
  
    panic("bget: no buffers");
}
// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int blockno = hash(b->blockno);
  acquire(&bcache.bucketslock[blockno]);
  b->refcnt--;
  if (b->refcnt == 0) {
      // no one is waiting for it.
      //release b
      b->next->prev = b->prev;
      b->prev->next = b->next;
      //add  b
      b->next = bcache.hash_buckets[blockno].next;
      b->prev = &bcache.hash_buckets[blockno];
      bcache.hash_buckets[blockno].next->prev = b;
      bcache.hash_buckets[blockno].next = b;
  } 
    release(&bcache.bucketslock[blockno]);
}

void
bpin(struct buf *b) {
  int hash_id = hash(b->blockno);
  acquire(&bcache.bucketslock[hash_id]);
  b->refcnt++;
  release(&bcache.bucketslock[hash_id]);
}

void
bunpin(struct buf *b) {
  int hash_id = hash(b->blockno);
  acquire(&bcache.bucketslock[hash_id]);
  b->refcnt--;
  release(&bcache.bucketslock[hash_id]);
}



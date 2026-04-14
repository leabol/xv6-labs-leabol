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

struct hashtable{
  struct buf pbuf;
  struct spinlock bcache_hash_lock;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
  struct hashtable bufmap[NBUCKETS];
} bcache;

static uint
hash(uint dev, uint blockno)
{
  // 1. 合并 dev 和 blockno：用异或混合高位和低位
  uint key = dev ^ (blockno << 16) ^ (blockno >> 16);
  
  // 2. 乘法哈希 + 右移：利用质数乘数打散分布
  key = key * 2654435761U;  
  
  return key % NBUCKETS;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for(int i = 0; i < NBUCKETS; i++){
    initlock(&bcache.bufmap[i].bcache_hash_lock,"bcache_lock");
    struct buf *head = &bcache.bufmap[i].pbuf;
    head->next = head;
    head->prev = head;
  }
  int i = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++, i++){
    initsleeplock(&b->lock, "buffer");
    
    struct buf* head= &bcache.bufmap[i%NBUCKETS].pbuf;
    b->next = head->next;
    b->prev = head;
    head->next->prev = b;
    head->next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *evict_buf;

  uint key = hash(dev,blockno);

  struct buf *head = &bcache.bufmap[key].pbuf;
  acquire(&bcache.bufmap[key].bcache_hash_lock);

  struct buf *curr_buf = head->next;

  // Is the block already cached?
  while (curr_buf != head){
    if(curr_buf->dev == dev && curr_buf->blockno == blockno){
      curr_buf->refcnt++;
      release(&bcache.bufmap[key].bcache_hash_lock);
      acquiresleep(&curr_buf->lock);
      return curr_buf;
    }
    curr_buf = curr_buf->next;
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i = 0; i < NBUCKETS; i++){
    struct buf *bucket_head = &bcache.bufmap[i].pbuf;
    evict_buf = bucket_head->next;  
    while (evict_buf != bucket_head){
      if(evict_buf->refcnt == 0) {
        evict_buf->dev = dev;
        evict_buf->blockno = blockno;
        evict_buf->valid = 0;
        evict_buf->refcnt = 1;

        evict_buf->next->prev = evict_buf->prev;
        evict_buf->prev->next = evict_buf->next;

        evict_buf->next = head->next;
        evict_buf->prev = head;
        head->next->prev = evict_buf;
        head->next = evict_buf;
        release(&bcache.bufmap[key].bcache_hash_lock);
        acquiresleep(&evict_buf->lock);
        return evict_buf;
      }
      evict_buf = evict_buf->next;
    }
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

  uint key = hash(b->dev,b->blockno);

  acquire(&bcache.bufmap[key].bcache_hash_lock);
  b->refcnt--;
  release(&bcache.bufmap[key].bcache_hash_lock);
}

void
bpin(struct buf *b) {
  uint key = hash(b->dev,b->blockno);
  acquire(&bcache.bufmap[key].bcache_hash_lock);
  b->refcnt++;
  release(&bcache.bufmap[key].bcache_hash_lock);
}

void
bunpin(struct buf *b) {
  uint key = hash(b->dev,b->blockno);
  acquire(&bcache.bufmap[key].bcache_hash_lock);
  b->refcnt--;
  release(&bcache.bufmap[key].bcache_hash_lock);
}



#include "mm_alloc.h"
#include <stdlib.h>
#define FALSE 0
#define align4(x) (((((x)-1)>>2)<<2)+4)
s_block_ptr base=NULL;
s_block_ptr get_block (void *p){
  char *tmp;
  tmp = p;
  return (p = tmp -= BLOCK_SIZE ); 
}
s_block_ptr find_block ( s_block_ptr *last , size_t size ){
s_block_ptr b=base;
while (b && !(b->free && b->size >= size )) {
*last = b;
b = b->next;
}
return (b);
}

s_block_ptr extend_heap ( s_block_ptr last , size_t s){
  s_block_ptr b = (s_block_ptr)sbrk(0);
  if ( sbrk ( BLOCK_SIZE + s) == (void *) -1){
    return NULL;
  }
  b->size = s;
  b->next = NULL;
  if (last){
    last ->next = b;
  }
  b->free = FALSE;
  return b;
}
int valid_addr (void *p)
{
  if (base)
    {
      if ( p>base && p< sbrk (0))
	{
	  return (p == ( get_block (p))->ptr );
	}
    }
  return (0);
}

s_block_ptr fusion ( s_block_ptr b){
  if (b->next && b->next ->free ){
    b->size += BLOCK_SIZE + b->next ->size;
    b->next = b->next ->next;
    if (b->next)
      b->next ->prev = b;
  }
  return b;
}
void split_block ( s_block_ptr b, size_t s)
{
  s_block_ptr new;
  new= ( s_block_ptr )(b->data + s);
  new ->size = b->size - s - BLOCK_SIZE ;
  new ->next = b->next;
  new ->prev = b;
  new ->free = 1;
  new ->ptr = new ->data;
  b->size = s;
  b->next = new;
  if (new ->next)
    new ->next ->prev = new;
}

void* mm_malloc(size_t size){
  s_block_ptr b,last;
  size_t s;
  s = align4 (size );
  if (base) {
    last = base;
    b = find_block (& last ,s);
    if (b) {
      if ((b->size - s) >= ( BLOCK_SIZE + 4))
	split_block (b,s);
      b->free =0;
    } else {
      b = extend_heap (last ,s);
      if (!b)
	return (NULL );
    }
  } else {
    b = extend_heap (NULL ,s);
    if (!b)
      return (NULL );
    base = b;
  }
  return (b->data );
}

void copy_block ( s_block_ptr src , s_block_ptr dst)
{
  int
    *sdata ,* ddata ;
  size_t
    i;
  sdata = src ->ptr;
  ddata = dst ->ptr;
  for (i=0; i*4<src ->size && i*4<dst ->size; i++)
    ddata [i] = sdata [i];
}
void* mm_realloc(void* ptr, size_t size){
  size_t s;
  s_block_ptr b, new;
  void *newp;
  if (!ptr)
    return ( malloc (size));
  if ( valid_addr (ptr)){
    s = align4 (size );
      b = get_block (ptr);
      if (b->size >= s){
	if (b->size - s >= ( BLOCK_SIZE + 4))
	  split_block (b,s);
      }
      else
	{
	  if (b->next && b->next ->free && (b->size + BLOCK_SIZE + b->next ->size) >= s){
	    fusion (b);
	    if (b->size - s >= ( BLOCK_SIZE + 4))
	      split_block (b,s);
	  }else{
	    newp = malloc (s);
	    if (! newp)
	      return (NULL );
	    new = get_block (newp );
	    copy_block (b,new );
	    free(ptr);
	    return (newp );
	  }
	}
      return (ptr);
  }
  return (NULL );
}

void mm_free(void* ptr){
  s_block_ptr b;
  if ( valid_addr (ptr)) {
    b = get_block (ptr);
    b->free = 1;
    if(b->prev && b->prev ->free)
      b = fusion (b->prev );
    if (b->next)
      fusion (b);
    else{
      if (b->prev)
	b->prev ->next = NULL;
      else
	base = NULL;
      brk(b);
    }
  }
}

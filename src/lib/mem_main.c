/*
 *  Author:
 *  Sasikanth.V        <sasikanth@email.com>
 *
 *  $Id: mem_main.c,v 1.3 2011/01/23 10:34:04 Sasi Exp $
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include "common_types.h"
#include "list.h"

/* TODO: 1) shrik pool based on the flags 
 *       2) block usage period
 *       3) allocation freq handling
 *       4) expanding the memory pool based on the flags
 *       5) pool id allocation
 *       6) Global pool allocator - implement using mmap, allocate based pages 
 *       			    memory pool will access global pool allocator for memory
 *       			    not malloc
 *       7) more support the mempool debuging display address also
 */

#define   in_range(start, end, size, value)        ((value >= start) && ((value + size) <= end))
#define   is_not_valid_block_offset(offset ,size)  (offset % size)

#define   ALLOCATED                             0x01
#define   MAX_POOL_NAME                         8
#define   MACHINE_ALIGNEMENT                    (sizeof(unsigned long))
#define   MEM_ALIGNED(bytes)                    (bytes % MACHINE_ALIGNEMENT) 
#define   COMPUTE_ADDR_BLOCK(saddr, cnt, size)	(saddr + (cnt * size))

#define   debug_mem_pool(p, fmt)                   if (p->debug_enabled || debug_all)\
							printf ("%s-MEM_POOL_MGR: %s\n", p->pool_name, fmt);

size_t alloc_size = 0;

struct mem_info {
	struct list_head n;
	sync_lock_t  lock;
	char     pool_name[MAX_POOL_NAME];
	void     *saddr;
	void     *addr_cache;
	int      memid;
	int      nblks;
	int      useblks;
	int      fblks;
	int      size;
	int      debug_enabled;
};

static int               add_to_mcb        (struct mem_info *m);
void  *                  tm_calloc         (size_t nmemb, size_t size);
int                      show_mem_pool     (void);
int                      mem_init          (void);
static struct mem_info * get_next_free_mcb (int *memid);
static struct mem_info * get_mem_info      (int memid);
static void   *          alloc_mem         (size_t size);
static void              free_mem          (void *mem, size_t size);
int                      debug_memory_pool (int pool_id, int set);

static LIST_HEAD(hd_mcb);
static unsigned long    pool_index;
static int debug_all = 0;

int mem_pool_create (const char *name, size_t size, int n_blks, int flags UNUSED_PARAM)
{
	uint32_t           bytes = 0;
	int                align = 0;
	int                memid = -1;
	struct mem_info    *mcb   = NULL;


	if (!name || !name[0] || !size  || !n_blks)
		return -1;

	/*Allocate 4 byte blk status to for house-keeping info*/
	size += 4;

	bytes = size * n_blks;  

	align = MEM_ALIGNED (bytes);

	if (align) {
		/*Requested memory is not aligned ,so aligned it 
 		  as per machine aligenment*/
		align += ((MACHINE_ALIGNEMENT - align));
		bytes +=  align;
		//warn ("Memory is not aligned");
	}

	mcb = get_next_free_mcb (&memid); /*XXX:Dynamic or Static - What to do*/

	if (!mcb) {
		printf ("-ERR- : No Free Memory Blks\n");
		return -1;
	}

	strncpy(mcb->pool_name, name, MAX_POOL_NAME);
	mcb->size = size;
	mcb->fblks = n_blks; 
	mcb->useblks = 0;
	mcb->nblks = n_blks;
	mcb->memid = memid;
	create_sync_lock (&mcb->lock);
	sync_unlock (&mcb->lock);
	INIT_LIST_HEAD (&mcb->n);

	if (!(mcb->saddr = alloc_mem (bytes))) {
		debug_mem_pool (mcb, "-ERR- : Insufficent Memory\n");
		return -1;
	}

	mcb->addr_cache = COMPUTE_ADDR_BLOCK (mcb->saddr, 0, mcb->size);

	/*Add to main control block*/
	add_to_mcb (mcb);

	/*Return non-neg mem pool id*/
	return mcb->memid;
}	

int mem_pool_delete (int pool_id)
{
	struct mem_info *p = get_mem_info (pool_id);
	if (!p) {
		return -1;
	}

	sync_lock (&p->lock);
	list_del (&p->n);
	sync_unlock (&p->lock);
	free_mem (p->saddr, p->size);
	free_mem (p, sizeof (*p));
	return 0;
}	

static int add_to_mcb (struct mem_info *m)
{
	list_add_tail (&m->n, &hd_mcb);
	return 0;
}

static struct mem_info * get_next_free_mcb (int *memid)
{
	/*XXX: Re-implement this function*/
	struct mem_info *p = alloc_mem (sizeof(struct mem_info));

	if (!p) {
		return NULL;
	}
	*memid = ++pool_index;

	return p;
}

static struct mem_info * get_mem_info (int memid)
{
	struct list_head *head = &hd_mcb;
	struct list_head *p = NULL;
	struct mem_info  *pmem = NULL;

	list_for_each (p, head) {
		pmem = list_entry (p, struct mem_info, n);
		if (pmem->memid == memid)
			return pmem;
	}
	return NULL;
}


int show_mem_pool (void)
{
        struct list_head *head = &hd_mcb;
        struct list_head *p = NULL;
        struct mem_info  *pmem = NULL;

        cli_printf ("Memory Pool Information\n");
        cli_printf (" %-8s %-8s  %-10s  %-10s   %-10s  %-10s  %-20s\n", 
		"Pool ID", "Pool Name", "Total Blks","Used Blks", "Free Blks", "Size(Bytes)", "Start Addr");
	cli_printf (" %-8s %-8s  %-10s  %-10s   %-10s  %-10s  %-20s\n", 
		"-------", "---------", "-----------", "---------", "--------", "---------", "-----------");
        list_for_each (p, head) 
	{
                pmem = list_entry (p, struct mem_info, n) ;
		sync_lock (&pmem->lock);
	        cli_printf ("   %-8d %-8s  %-10d    %-10d %-10d  %-10d  %-20p\n", pmem->memid,
                        pmem->pool_name,pmem->nblks, pmem->useblks, pmem->fblks, 
			pmem->size - sizeof (uint8_t), pmem->saddr);
		sync_unlock (&pmem->lock);
        }
        return 0;
}

int debug_memory_pool (int pool_id, int set)
{
	if (pool_id) {
		struct mem_info *p = get_mem_info (pool_id);
		if (!p) {
			printf ("%%Error: Invalid memory pool id\n");
			return -1;
		}
		if (set) {
			p->debug_enabled = 1;
		} else 
			p->debug_enabled = 0;
		
	} else  {
		if (set)
			debug_all = 1;
		else 
			debug_all = 0;
	}

	return 0;
}

void * alloc_block (int memid)
{
	struct mem_info *p = get_mem_info (memid);
	int i = 0;
	uint8_t *retaddr = NULL;

	if (!p) {
		warn ("-ERR- : Invalid POOL ID\n");
		return NULL;
	}

	sync_lock (&p->lock);

	if (!p->fblks) {
		debug_mem_pool (p, "No free memory blocks");
		sync_unlock (&p->lock);
		return NULL;
	}

	if (p->addr_cache) {
		*(uint32_t *)p->addr_cache = ALLOCATED;
		retaddr = ((uint8_t *)p->addr_cache + sizeof (uint32_t));
		p->fblks--;
		p->useblks++;
		p->addr_cache = NULL;
		debug_mem_pool (p, "Allocated Cached block");
		sync_unlock (&p->lock);
		return retaddr;
	}

	while (i < p->nblks) {
		retaddr = COMPUTE_ADDR_BLOCK (p->saddr, i, p->size);
		if (*retaddr) {
			i++;
			continue;
		}
		p->fblks--;
		p->useblks++;
		*retaddr = ALLOCATED;
		retaddr += sizeof (uint32_t);
		debug_mem_pool (p, "Allocated NEW block");
		sync_unlock (&p->lock);
		return (void *)retaddr;
	}

	sync_unlock (&p->lock);
	debug_mem_pool (p, "Allocated failed .. No New Block");
	return NULL;	
}

int free_blk (int memid, void *addr)
{
	int             offset = -1;
	struct mem_info *p     = get_mem_info (memid);

	if (!p) {
		warn ("-ERR- : Invalid POOL ID\n");
		return -1;
	}

	sync_lock (&p->lock);

	/*Move pointer 4 byte backwards to access the status flag*/
	addr  = (uint8_t *)addr - sizeof (uint32_t); 

	if (!in_range (p->saddr, (p->saddr + (p->size * p->nblks)), p->size, 
		       addr)) 
		goto invalid_address;

	offset = (((unsigned long)addr - (unsigned long)p->saddr));

	if (is_not_valid_block_offset (offset , p->size)) 
		goto invalid_address;

	memset (addr, 0, p->size);
	
	if (!p->addr_cache)
		p->addr_cache = addr;
	p->fblks++;
	p->useblks--;
        debug_mem_pool (p, "Freed block");
	sync_unlock (&p->lock);
	return 0;

invalid_address:
	printf ("%%ERROR : %s-MEM_POOL_MGR: Trying to free invaild address\n", p->pool_name);
	sync_unlock (&p->lock);
	return -1;
}

/* alloc_mem  is the final routine to allocate memory*/
static void * alloc_mem (size_t size)
{
	return tm_calloc (1, size);
}

static inline void free_mem(void *mem, size_t size)
{
	tm_free (mem, size);
}

void *tm_calloc(size_t nmemb, size_t size)
{
	alloc_size += size;

	return calloc (nmemb, size);
}

void * tm_malloc (size_t size)
{
	return tm_calloc (1, size);
}

void tm_free (void *p , size_t size)
{
	alloc_size -= size;
	free (p);
}


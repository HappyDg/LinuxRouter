/*
 * This file contatins all library funtions prototypes
 */
#ifndef _LIB_PROTO_H
#define _LIB_PROTO_H
#include "task.h"
#include "hashlib.h"

typedef struct event_block 
{
	int32_t         event;
	tskmtx_t        evt_mtx;
	tskcnd_t        evt_cnd;
}EVT_T;


/*TASK*/
retval_t task_create (const char tskname[], int tsk_prio, int sched_alg, int stk_size,
	              void *(*start_routine) (void *), void (*exit_routine) (void),
		      void *arg, tmtaskid_t * rettskid);
tmtaskid_t tsk_selfid (void);
void tsk_delay (int secs, int nsecs);
void tsk_sleep (int secs);
void tsk_mdelay (int msecs);
int evt_rx (tmtaskid_t tskid, int *pevent, int event);
void evt_snd (tmtaskid_t tskid, int event);
retval_t task_delete (char tskname[], tmtaskid_t tskid);
void tsk_cancel (tmtaskid_t task_id);
char * get_tsk_name (tmtaskid_t tskid);
unsigned long tick_start (void);
void tick_end (unsigned long *p, unsigned long start);
unsigned int milli_secs_to_ticks (unsigned int msecs);


/*MEM pool*/
int mem_pool_create (const char *name, size_t size, int n_blks, int flags);
int mem_pool_delete (int pool_id);
void * alloc_block (int memid);
int free_blk (int memid, void *addr);
void *tm_calloc(size_t nmemb, size_t size);
void * tm_malloc (size_t size);
void tm_free (void *p , size_t size);
unsigned int tm_get_ticks_per_second (void); 


/*TIMERS*/

int    setup_timer (void **p, void (*handler) (void *), void *data);
int    mod_timer   (void *p, unsigned int ticks);
int    del_timer   (void *p);
int    stop_timer  (void *p);
void * start_timer (unsigned int ticks, void *data, void (*handler) (void *), int flags);
int timer_pending (void *p);
unsigned int timer_get_remaining_time (void *p);
unsigned int get_secs (void);
unsigned int sys_now (void);

/*HASH*/
struct hash_table * create_hash_table (const char *, int ,int (*cmp)(const uint8_t *, const uint8_t *),
				        int  (*index_gen)(unsigned char *), int ); 
void destroy_hash_table (HASH_TABLE *htbl, void (*free_data) (void *));
void * hash_tbl_lookup (unsigned char *, struct hash_table *);
int  hash_tbl_add (uint8_t *, struct hash_table *, void *);
int  hash_tbl_delete (unsigned char *, struct hash_table *, void (*free_data)(void *));
int  hash_walk (struct hash_table *, void (*callback)(void *));
uint32_t jhash_1word(uint32_t a, uint32_t initval);

/*MSG Q*/

int msg_create_Q (const char *name, int maxmsg, int size);
int msg_rcv (int qid, char **msg, int size);
int msg_send (int qid, void *msg, int size);
int msg_Q_delete (int qid);
int mq_vaild (int qid);
int msg_rcv_timed (int qid, char **msg, int size, unsigned int msecs);

/*LOCK*/
int create_sync_lock (sync_lock_t *slock);
int sync_lock (sync_lock_t *slock);
int sync_unlock (sync_lock_t *slock);
int destroy_sync_lock (sync_lock_t *slock);
int sync_lock_timed_wait (sync_lock_t *slock, int secs, int nanosecs);

uint32_t ip_2_uint32 (uint8_t *ipaddress, int byte_order);
void uint32_2_ipstring (uint32_t ipAddress, uint8_t *addr);
u_char u32ip_masklen (uint32_t netmask);
void u32masklen2ip (int masklen, uint32_t *netmask);
uint32_t u32ipv4_network_addr (uint32_t hostaddr, int masklen);

void send_packet (void *buf, uint16_t port, int len);


/*EVENT Layer*/
int EventInit (EVT_T *p);
int EventDeInit (EVT_T *p);
int EvtRx (EVT_T *evt, int *pevent, int event);
int EvtRx_timed_wait (EVT_T *evt, int *pevent, int event, int secs, int nsecs);
void EvtSnd (EVT_T *evt, int event);
int EvtLock (EVT_T *evt);
int EvtUnLock (EVT_T *evt);
int EvtWaitOn (EVT_T *evt);
int EvtWaitOnTimed (EVT_T *evt, int secs, int nsecs);
void EvtSignal (EVT_T *evt);

/*Pqueue lib*/

unsigned long  pqueue_create (void);
int  pqueue_destroy (unsigned long pcb);
int  pqueue_valid (unsigned long pcb);
int queue_packet (unsigned long qblk , uint8_t *buf, int len);
int dequeue_packet (unsigned long qcb, uint8_t **data, size_t datalen, int secs, int nsecs, int flags);

/*LIB */
uint32_t ip_2_uint32 (uint8_t *ipaddress, int byte_order);
void convert_uint32_str_ip_mask (char *str, uint32_t ip, uint32_t mask);
void convert_uint32_str_ip (char *str, uint32_t ip);
void uint32_2_ipstring (uint32_t ipAddress, uint8_t *addr);
void u32masklen2ip (int masklen, uint32_t *netmask);
u_char u32ip_masklen (uint32_t netmask);
uint32_t u32ipv4_network_addr (uint32_t hostaddr, int masklen);
char*  hex2bin(char *dst, const char *str, int count);
void dump_stack (void);
in_addr_t ipv4_broadcast_addr (in_addr_t hostaddr, int masklen);

const char * inet_ntop(int af, const void *src, char *dst, size_t size);
size_t strlcpy(char *dst, const char *src, size_t siz);

int mcore_init (void);
int get_cpu (void);
long get_max_cpus (void);
int cpu_bind (int cpu);


/*Sock Layer*/
#endif

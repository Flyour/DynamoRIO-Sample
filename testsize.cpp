#include "dr_api.h"
#include <stdio.h>
#include <stddef.h>

#ifdef WINDOWS
# define DISPLAY_STRING(msg) dr_messagebox(msg)
#else
# define DISPLAY_STRING(msg) dr_printf("%s\n", msg)
#endif

#define SCHAR_MIN  -128
#define SCHAR_MAX  127

#define TESTALL(mask, var)  (((mask) & (var)) == (mask))
#define TESTANY(mask, var)  (((mask) & (var)) != 0)

typedef struct bb_counts {
	uint64 blocks;
	uint64 total_size;
} bb_counts;

file_t f;

int test = 0;

static bb_counts counts_as_built;
void *as_built_lock;

static bb_counts counts_dynamic;
void *count_lock;

/* Protected by the as_built_lock */
static uint64 bbs_eflags_saved;

static void
event_exit(void);
static dr_emit_flags_t
event_basic_block(void *drcontext, void *tag, instrlist_t *bb,
	bool for_trace, bool translating);

static void
event_thread_init(void *drcontext);
static void
event_thread_exit(void *drcontext);

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
	/* register events */
	dr_register_exit_event(event_exit);
	dr_register_bb_event(event_basic_block);
	dr_register_thread_init_event(event_thread_init);
	dr_register_thread_exit_event(event_thread_exit);
	/* initialize lock */
	as_built_lock = dr_mutex_create();
	count_lock = dr_mutex_create();

	f = dr_open_file("C:\\Users\\Mr.wang\\Desktop\\DynamoRIO-Windows-7.0.17873-0\\work\\log.txt", DR_FILE_WRITE_APPEND );
//	DR_ASSERT(f != INVALID_FILE);
}


static void
event_exit(void)
{
	 /* Display results - we must first snpritnf the string as on windows
	  * dr_printf(), dr_messagebox() and dr_fprintf() can't print floats. */
	char msg[512];
	int len;
	len = snprintf(msg, sizeof(msg) / sizeof(msg[0]),
		"Number of basic blocks built : %llu \n"
		"     Average size            : %.5f instructions\n"
		"Number of basic blocks executed	: %llu \n"
		"     Average weighted size         : %.5f instructions\n"
		"Num saved eflags			  : %llu \n",
		counts_as_built.blocks,
		counts_as_built.total_size / (double)counts_as_built.blocks,
		counts_dynamic.blocks,
		counts_dynamic.total_size  / (double)counts_dynamic.blocks,
		bbs_eflags_saved);
		//counts_as_built.total_size);
	DR_ASSERT(len > 0);
	msg[sizeof(msg) / sizeof(msg[0]) - 1] = '\0'; /* NUll terminate */
	DISPLAY_STRING(msg);
	
	      /* free mutex */
	dr_close_file(f);
	dr_mutex_destroy(as_built_lock);
	dr_mutex_destroy(count_lock);
}


static void
event_thread_init(void *drcontext)
{
	/* create an instance of our data structure for this thread */
	bb_counts *counts = (bb_counts *)dr_thread_alloc(drcontext, sizeof(bb_counts));
	/* store it in the slot provided in the drcontext */
	dr_set_tls_field(drcontext, counts);
	memset(counts, 0, sizeof(bb_counts));
	DISPLAY_STRING("hello, thread_init");
}


static void
event_thread_exit(void *drcontext)
{
	DISPLAY_STRING("byby, thread_exit");
	bb_counts *counts = (bb_counts *)dr_get_tls_field(drcontext);

	dr_mutex_lock(count_lock);
	counts_dynamic.blocks += counts->blocks;
	counts_dynamic.total_size += counts->total_size;
	dr_mutex_unlock(count_lock);
	dr_thread_free(drcontext, counts, sizeof(bb_counts)); 
}



static dr_emit_flags_t
event_basic_block(void *drcontext, void *tag, instrlist_t *bb,
	bool for_trace, bool translating)
{
	uint num_instructions = 0;
	instr_t *instr;
	instr_t *where = NULL;

	bool eflags_saved = true;
	bb_counts *counts = (bb_counts *)dr_get_tls_field(drcontext);


	     /* count the number of instructions in this block */
	for (instr = instrlist_first(bb); instr != NULL; instr = instr_get_next(instr)) {
		/* Since it doesn't matter where we insert, look for a place
		 * where the eflags are dead. */
		 uint flags = instr_get_arith_flags(instr, DR_QUERY_DEFAULT);
		 /* 这里判断，如果basic block中没有读任何一个arithmetic eflags 
		  * 并且写了6 个 arithmetic eflags ，则说明不需要在 basic block 开头处保存eflags
		  */
		 if (TESTALL(EFLAGS_WRITE_6, flags) && !TESTANY(EFLAGS_READ_6, flags)) {
			 where = instr;
			 eflags_saved = false;
			 //dr_fprintf(f, "eflags_saved = false, flags= %x \n", flags);
		 }
		num_instructions++;

	}

	     /* update the as-built counts */
	dr_mutex_lock(as_built_lock);
	counts_as_built.blocks++;
	counts_as_built.total_size += num_instructions;
	if (eflags_saved)
		bbs_eflags_saved++;
	dr_mutex_unlock(as_built_lock);

	if (eflags_saved) {
		where = instrlist_first(bb);
		dr_save_arith_flags(drcontext, bb, where, SPILL_SLOT_1);
	}

//	/* If all thread-private caches we can use direct addressing. */
//	if (!dr_using_all_private_caches()) {
//		/* Spill a register to get a pointer to our TLS structure. */
//		dr_save_reg(drcontext, bb, where, DR_REG_XDI, SPILL_SLOT_2);
//		dr_insert_read_tls_field(drcontext, bb, where, REG_XDI);
//	}


//
//	/* increment counters */
//	where = instrlist_first(bb);
//
//		char reg[512];
//		int len;
//		len = snprintf(reg, sizeof(reg) / sizeof(reg[0]),
//			//" blocks built : %llu \n"
//			//"total_size	   : %llu \n",
//			"counts address : %p\n"
//			"reg			: %x\n"
//			"orrset			: %x\n"
//			" reg blocks		: %p\n",
//			counts,
//			DR_REG_XDI,
//			offsetof(bb_counts, blocks),
//			OPND_CREATE_MEM32(DR_REG_XDI, offsetof(bb_counts, blocks))
//			);
//		DISPLAY_STRING(reg);
//		test = 1;
//	}
//





#ifdef X86_32
	 instrlist_meta_preinsert(bb, where,
		INSTR_CREATE_add(drcontext,
//			OPND_CREATE_MEM32(DR_REG_XDI, offsetof(bb_counts, blocks)),
			OPND_CREATE_ABSMEM((byte *)&counts->blocks, OPSZ_4),
			OPND_CREATE_INT8(1)));
	 instrlist_meta_preinsert(bb, where, 
		INSTR_CREATE_adc(drcontext,
//			OPND_CREATE_MEM32(DR_REG_XDI, offsetof(bb_counts, blocks) + 4),
			OPND_CREATE_ABSMEM((byte *)&counts->blocks + 4, OPSZ_4),
			OPND_CREATE_INT8(0)));
	 instrlist_meta_preinsert(bb, where,
		INSTR_CREATE_add(drcontext,
//			OPND_CREATE_MEM32(DR_REG_XDI, offsetof(bb_counts, total_size)),
			OPND_CREATE_ABSMEM((byte *)&counts->total_size, OPSZ_4),
			OPND_CREATE_INT_32OR8(num_instructions)));
	 instrlist_meta_preinsert(bb, where,
		INSTR_CREATE_adc(drcontext,
//			OPND_CREATE_MEM32(DR_REG_XDI, offsetof(bb_counts, total_size) + 4),
			OPND_CREATE_ABSMEM((byte *)&counts->total_size + 4, OPSZ_4),
			OPND_CREATE_INT8(0)));

#else
	instrlist_meta_preinsert(bb, where,
		INSTR_CREATE_inc(drcontext,
			OPND_CREATE_MEM64(DR_REG_XDI, offsetof(bb_counts, blocks))));
	instrlist_meta_preinsert(bb, where,
		INSTR_CREATE_add(drcontext,
			OPND_CREATE_MEM64(DR_REG_XDI, offsetof(bb_counts, total_size)),
			OPND_CREATE_INT_32OR8(num_instructions)));
#endif

	/* Restore spilled register */
//	dr_restore_reg(drcontext, bb, where, DR_REG_XDI, SPILL_SLOT_2);
	
	if (eflags_saved) {
		dr_restore_arith_flags(drcontext, bb, where, SPILL_SLOT_1);
	}
	return DR_EMIT_DEFAULT;
}


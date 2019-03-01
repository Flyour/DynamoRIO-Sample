#include "dr_api.h"
//#include "utils.h"

static void
event_exit(void);
static void
event_thread_init(void *drcontext);
static void
event_thread_exit(void *drcontext);
static dr_emit_flags_t
event_basic_block(void *drcontext, void *tag, instrlist_t *bb, 
				 bool for_trace, bool translating);
static void
at_call(app_pc instr_addr, app_pc target_addr);
static void
at_call_ind(app_pc instr_addr, app_pc target_addr);
static void
at_return(app_pc instr_addr, app_pc target_addr);

static int tls_idx;

static client_id_t my_id;

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{

	dr_register_exit_event(event_exit);
	dr_register_bb_event(event_basic_block);
	dr_register_thread_init_event(event_thread_init);
	dr_register_thread_exit_event(event_thread_exit);

	dr_fprintf(STDOUT, "CLIENT START\n");

}


static void
event_exit(void)
{
	dr_fprintf(STDOUT, "CLIENT EXIT\n");
}



static void event_thread_init(void *drcontext)
{
	/* we're going to dump our data to a per-thread file */
	file_t f;
	f = dr_open_file("C:\\Users\\Mr.wang\\Desktop\\DynamoRIO-Windows-7.0.17873-0\\work\\log.txt", DR_FILE_WRITE_APPEND );
	DR_ASSERT( f != INVALID_FILE);

	/* store it in the slot provided in the drcontext */
	dr_set_tls_field(drcontext, (void *)f);

	dr_fprintf(STDOUT, "thread init\n");
}


static void event_thread_exit(void *drcontext)
{
	file_t f = (file_t)(ptr_uint_t) dr_get_tls_field(drcontext);
	dr_close_file(f);
	dr_fprintf(STDOUT, "thread exit\n");
}


static dr_emit_flags_t
event_basic_block(void *drcontext, void *tag, instrlist_t *bb, 
				 bool for_trace, bool translating)
{
	instr_t *instr, *next_instr;

	for (instr = instrlist_first(bb); instr!= NULL; instr = next_instr) {
		next_instr = instr_get_next(instr);
		if (!instr_opcode_valid(instr))
			continue;

		/* instrument calls and returns -- ignore far calls/rets */
		if (instr_is_call_direct(instr)) {
			dr_insert_call_instrumentation(drcontext, bb, instr, (app_pc) at_call);
		} else if (instr_is_call_indirect(instr)) {
			dr_insert_mbr_instrumentation(drcontext, bb, instr, (app_pc)at_call_ind,
										  SPILL_SLOT_1);
		} else if (instr_is_return(instr)) {
			dr_insert_mbr_instrumentation(drcontext, bb, instr, (app_pc)at_return, 
										  SPILL_SLOT_1);
		}
	}
	return DR_EMIT_DEFAULT;
}

static void
at_call(app_pc instr_addr, app_pc target_addr)
{
	file_t f = (file_t)(ptr_uint_t) dr_get_tls_field(dr_get_current_drcontext());
	dr_mcontext_t mc;
	dr_get_mcontext(dr_get_current_drcontext(), &mc, NULL);
	dr_fprintf(f, "CALL @ %p to %p, TOS is %p\n", instr_addr, target_addr, mc.xsp);
}

static void
at_call_ind(app_pc instr_addr, app_pc target_addr)
{
	file_t f = (file_t)(ptr_uint_t) dr_get_tls_field(dr_get_current_drcontext());
	dr_fprintf(f, "CALL INDIRECT @ %p to %p\n", instr_addr, target_addr); 
}

static void
at_return(app_pc instr_addr, app_pc target_addr)
{
	file_t f = (file_t)(ptr_uint_t) dr_get_tls_field(dr_get_current_drcontext());
	dr_fprintf(f, "RETURN @ %p to %p\n", instr_addr, target_addr);
}









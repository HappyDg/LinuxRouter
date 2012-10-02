#include "common_types.h"
#include "cparser.h"
#include "cparser_tree.h"
#include "cli.h"

/**************************************************************/
void  cparser_fsm_reset       (cparser_t *parser);
void  cparser_record_command  (cparser_t *parser, cparser_result_t rc);
void  cparser_print_prompt    (const cparser_t *parser);
int   create_user             (char *username, char *password, int priv_level);
int   user_del                (char *username);
int   show_users              (void);
int   process_lock            (void);
int   process_logout          (void);
int   show_mem_pool           (void);
int   show_cpu_usage          (void);
int   show_mac_table          (void);
int   debug_memory_pool       (int pool_id, int set);
int   set_hostname            (const char *Hostname);
cparser_result_t cparser_set_prompt (cparser_t *parser, const char *prompt);
/**************************************************************/

static cparser_result_t cparser_cmd_enter_privileged_mode (cparser_t *parser, char *buf, int buf_size);


cparser_result_t cparser_cmd_lock(cparser_context_t *context UNUSED_PARAM)
{
	if (process_lock ())
		return CPARSER_NOT_OK;
	return CPARSER_OK;
}
cparser_result_t cparser_cmd_logout(cparser_context_t *context UNUSED_PARAM)
{
	if (process_logout ())
		return CPARSER_NOT_OK;
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_show_users(cparser_context_t *context UNUSED_PARAM)
{
	if (show_users ())
		return CPARSER_NOT_OK;
	return CPARSER_OK;
}
cparser_result_t cparser_cmd_show_mem_pool(cparser_context_t *context UNUSED_PARAM)
{
        if (show_mem_pool ())
                return CPARSER_NOT_OK;
        return CPARSER_OK;
}

cparser_result_t cparser_cmd_show_task_cpu(cparser_context_t *context UNUSED_PARAM)
{
	if (show_cpu_usage ())
		return CPARSER_NOT_OK;
	return CPARSER_OK;
}
cparser_result_t cparser_cmd_enable_privileged_mode(cparser_context_t *context)
{
	char passwd[100];
	int rc;

	if (cparser_is_in_privileged_mode(context->parser)) {
		cli_printf ("Already in privileged mode.\n");
		return CPARSER_NOT_OK;
	}

	/* Request privileged mode password */
	rc = cparser_user_input(context->parser, "Enter password (Enter: 'HELLO'): ", 0,
			passwd, sizeof(passwd), cparser_cmd_enter_privileged_mode);
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_configure_terminal(cparser_context_t *context)
{
	char prompt[CPARSER_MAX_PROMPT];
	/* Enter the submode */
	set_prompt ("(config)");
	get_prompt (prompt);
	set_curr_mode (GLOBAL_CONFIG_MODE);
	return cparser_submode_enter(context->parser, NULL, prompt);
}
cparser_result_t cparser_cmd_config_user_add_username_password_level
                 (cparser_context_t *context UNUSED_PARAM, char **username_ptr, char **password_ptr, int32_t *level_ptr)

{
	if (create_user (*username_ptr, *password_ptr, *level_ptr))
		return CPARSER_NOT_OK;
	return CPARSER_OK;
}
cparser_result_t cparser_cmd_config_user_del_username(cparser_context_t *context UNUSED_PARAM,
    char **username_ptr)
{
	if (user_del (*username_ptr))
		return CPARSER_NOT_OK;
	return CPARSER_OK;
}
cparser_result_t cparser_cmd_end(cparser_context_t *context UNUSED_PARAM)
{
	return CPARSER_OK;
}
cparser_result_t cparser_cmd_exit(cparser_context_t *context)
{
	return cparser_submode_exit(context->parser);
}
cparser_result_t cparser_cmd_disable_privileged_mode(cparser_context_t *context)
{
	if (!cparser_is_in_privileged_mode(context->parser)) {
		cli_printf ("Not in privileged mode.\n");
		return CPARSER_NOT_OK;
	}

	cparser_set_privileged_mode(context->parser, 0);
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_config_end(cparser_context_t *context UNUSED_PARAM)
{
        set_prompt ("");
        set_curr_mode (USER_EXEC_MODE);
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_config_exit(cparser_context_t *context)
{
	if (!exit_mode ())
	{
		return cparser_submode_exit (context->parser);
	}
	return CPARSER_NOT_OK;
}


cparser_result_t cparser_cmd_show_mac_address_table(cparser_context_t *context UNUSED_PARAM)
{
	//if (!show_mac_table ())
	//	return CPARSER_OK;
	return CPARSER_NOT_OK;
}

static cparser_result_t
cparser_cmd_enter_privileged_mode (cparser_t *parser, char *buf, int buf_size)
{
    if (strncmp(buf, "HELLO", buf_size)) {
        cli_printf("\nPassword incorrect. Should enter 'HELLO'.\n");
    } else {
        cli_printf("\nEnter privileged mode.\n");
        cparser_set_privileged_mode(parser, 1);
    }
    return CPARSER_OK;
}

cparser_result_t cparser_cmd_run_script_filename(cparser_context_t *context, char **filename_ptr)
{
	FILE  *fp = fopen  (*filename_ptr, "r");
	int    c = 0;
	int do_echo = 0;

	if (!fp) {
		cli_printf ("Unable to open file :  %s\n", *filename_ptr);
		return CPARSER_NOT_OK;
	}

	/* Reset FSM states and advance to the next line */
	cparser_record_command(context->parser, CPARSER_OK);
	cparser_fsm_reset(context->parser);
	if (!cparser_is_user_input(context->parser, &do_echo)) {
		cparser_print_prompt(context->parser);
	}
	cparser_line_advance(context->parser);	
	while ((c = fgetc (fp)) != EOF) {
        	cparser_input (context->parser, c, CPARSER_CHAR_REGULAR);
	}
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_clear_screen (cparser_context_t *context UNUSED_PARAM)
{
	cli_printf("\033[2J");
	cli_printf("\033[0;0f");
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_debug_mem_pool_poolid(cparser_context_t *context UNUSED_PARAM, int32_t *poolid_ptr)
{
	int rval = -1;

	if (poolid_ptr) {
		rval = debug_memory_pool (*poolid_ptr, 1);
	} else
		rval = debug_memory_pool (0, 1);

	if (rval < 0)
		return CPARSER_NOT_OK;
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_no_debug_mem_pool_poolid(cparser_context_t *context UNUSED_PARAM, int32_t *poolid_ptr)
{
	int rval = -1;

	if (poolid_ptr) {
		rval = debug_memory_pool (*poolid_ptr, 0);
	} else
		rval = debug_memory_pool (0, 0);

	if (rval < 0)
		return CPARSER_NOT_OK;
	return CPARSER_OK;
}
cparser_result_t cparser_cmd_config_set_hostname_hostname(cparser_context_t *context,
    char **hostname_ptr)
{
	char prompt[CPARSER_MAX_PROMPT];

	if (set_hostname (*hostname_ptr))
		return CPARSER_NOT_OK;
	get_prompt (prompt);
	cparser_set_prompt (context->parser, prompt);
	return CPARSER_OK;
}
cparser_result_t cparser_cmd_system_linuxcommands_args (cparser_context_t *context, char **command, char **args)
{
	char linux_cmd[64];
	sprintf (linux_cmd,"%s ", *command);
	if (args && *args)
		strcat (linux_cmd, *args);
	system (linux_cmd);
	return CPARSER_OK;
}

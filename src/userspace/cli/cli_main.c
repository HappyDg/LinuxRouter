/*
 *  Author:
 *  Sasikanth.V        <sasikanth@email.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include "common_types.h"
#include "defs.h"
#include "cparser.h"
#include "cparser_tree.h"
#include <unistd.h>

#define MAX_CLI_SESSION  8

#define MAX_MODE_GROUP  24

#define DEFAULT_USER_NAME  "guest"

struct cli {
	tmtaskid_t taskid;
	int session;
	int priv_level;
	int in;
	int out;
	int port_number;
        int current_priv_level;
	int current_mode;
	int port_no;
	int vlanid;
	char prmpt[MAX_PMP_LEN + 1]; 
	char hostname[MAX_PMP_LEN];
	char prmpt_prev[MAX_PMP_LEN];
	char username[MAX_USER_NAME];
	cparser_t parser;
	void *telnet;
}__attribute__ ((__packed__));


static void spawn_cli_thread (int);
void *cmdinterface(void *unused);
void handle_segfault (int );
int cli_init(const char *prmt);
int cli_session_init(const char *prmt, int this_session, int fd);
int start_cli_task(void);
int set_prompt_internal(char *pmt, int len);
int get_prompt(char *pmt);
void print_prompt (void);
int write_input_on_screen(char c);
void write_string(const char *str);
void handle_segfault(int signo);
int get_curr_priv_level(void);
int get_curr_mode(void);
int set_curr_mode(int mode);
void set_curr_priv_level(int level);
int set_prompt(const char *prmpt_new);
void  cli_set_port(int port);
int cli_get_vlan_id(void);
void cli_set_vlan_id(int vlan_id);
int cli_get_port(void);
int get_current_user_name(char *user);
int set_current_user_name(char *user);
char read_input(void);
void do_mode_change(int mode);
int change_vlan_mode(char **args);
int change_to_interface_mode(char **args);
int change_config_mode(char **args);
int end_mode(char **args);
int exit_mode(void);
void cparser_telnet_io_config (cparser_t *parser);
int cli_start_session (int session);
void cparser_print_prompt (const cparser_t *parser);
void cparser_feed (int session, int ch);
int set_hostname (const char *Hostname);
int telnet_prints (void *telnet, const char *buffer, int len);

extern int show_users (void);
int process_logout(void);
int process_lock (void);
extern int show_cpu_usage (void);
extern int show_mem_pool (void);
void write_config_to_start_up (char *line);
int user_db_init (void);
struct cli * cli_get_new_session (void);
int cli_get_cli_session_id (void);
int cli_telnet_session_init (const char *prmt, int fd, void *data);


struct cli this_cli[MAX_CLI_SESSION];
tmtaskid_t clitskid = 0;
static int gfd = 1;
int login_sucessfull = 0;

struct modes {
	int mode;
	int (*mode_change) (char **args);
};

struct modes mode_p[] = { {GLOBAL_CONFIG_MODE, change_config_mode},
			  {INTERFACE_MODE ,    change_to_interface_mode},
			  {VLAN_CONFIG_MODE, change_vlan_mode},
			  {USER_EXEC_MODE,  end_mode}
			};

static FILE *fp = NULL;

void write_config_to_start_up (char *line)
{
	if (!fp || !line)
		return;
	fwrite (line, 1, strlen (line), fp);
}

int cli_init (const char *prmt) 
{
	int i = 1;
        user_db_init ();
	
	fp = fopen ("startup.cfg", "w");
	
	while (i < MAX_CLI_SESSION) {
		this_cli[i].session = -1;
		i++;
	}
	cli_session_init (prmt, 0, STDOUT_FILENO);
	return 0;
}

struct cli * cli_get_new_session (void)
{
	int i = 1;
	while (i < MAX_CLI_SESSION) {
		if (this_cli[i].session == -1) {
			this_cli[i].session = 1;
			return &this_cli[i];
		}
		i++;
	}

	return NULL;
}
int cli_session_init (const char *prmt, int this_session, int fd) 
{
	memset(&this_cli[this_session].parser, 0, sizeof(this_cli[this_session].parser));
        memcpy (this_cli[this_session].hostname, prmt, strlen(prmt));
	sprintf (this_cli[this_session].prmpt, "%s%s ", prmt, "#");
        strcpy (this_cli[this_session].username, DEFAULT_USER_NAME);
	this_cli[this_session].parser.cfg.root = &cparser_root;
	this_cli[this_session].parser.cfg.ch_complete = '\t';
	this_cli[this_session].parser.cfg.ch_erase = '\b';
	this_cli[this_session].parser.cfg.ch_help = '?';
	this_cli[this_session].parser.cfg.flags = 0;
	cparser_io_config(&this_cli[this_session].parser);
	this_cli[this_session].parser.cfg.fd = fd;
	return 0;
}

int cli_get_cli_session_id (void)
{
	int i = 0;
	tmtaskid_t tskid = tsk_selfid ();

	while (i < MAX_CLI_SESSION) {
		if (this_cli[i].taskid == tskid) {
			return i;
		}
		i++;
	}
	return -1;

}

int cli_telnet_session_init (const char *prmt, int fd, void *data)
{
	struct cli *cli_session = NULL;

	cli_session = cli_get_new_session ();
	
	if (cli_session) {
		memset(&cli_session->parser, 0, sizeof(cli_session->parser));
		memcpy (cli_session->hostname, prmt, strlen(prmt));
		sprintf (cli_session->prmpt, "%s%s ", prmt, "#");
		strcpy (cli_session->username, DEFAULT_USER_NAME);
		cli_session->parser.cfg.root = &cparser_root;
		cli_session->parser.cfg.ch_complete = '\t';
		cli_session->parser.cfg.ch_erase = '\b';
		cli_session->parser.cfg.ch_help = '?';
		cli_session->parser.cfg.flags = 0;
		cparser_telnet_io_config (&cli_session->parser);
		cli_session->parser.cfg.fd = fd;
		cli_session->parser.session_data = data;
		return cli_session->session;
	}
	return -1;
}

int start_cli_task (void)
{
	set_curr_mode (USER_EXEC_MODE);

	/*finally kick-start the shell thread*/
	spawn_cli_thread (0);

	return 0;
}

int cli_start_session (int session)
{
	if (session < 0 || session > MAX_CLI_SESSION)
		return -1;

	this_cli[session].taskid = tsk_selfid ();

	get_prompt (this_cli[session].parser.cfg.prompt);

        cli_set_vlan_id (1);

        cparser_init(&this_cli[session].parser.cfg, &this_cli[session].parser);

	this_cli[session].parser.cfg.io_init(&this_cli[session].parser);

	//show_login_prompt ();

	cparser_print_prompt(&this_cli[session].parser);

	this_cli[session].parser.done = 0;

	return 0;
}

void cparser_feed (int session, int ch)
{
	cparser_input(&this_cli[session].parser, ch, 1);
}

static void spawn_cli_thread (int this_session)
{
	task_create ("CLI", 10, 3, 48000, cmdinterface, NULL, (void *)this_session, &clitskid);
}

void *cmdinterface(void *unused)
{
	int session = (int)unused;
	cparser_result_t rc;

	this_cli[session].taskid = tsk_selfid ();

	get_prompt (this_cli[session].parser.cfg.prompt);

        cli_set_vlan_id (1);

	rc = cparser_init(&this_cli[session].parser.cfg, &this_cli[session].parser);

	cparser_run(&this_cli[session].parser);

	return NULL;
}

int set_prompt_internal (char *pmt, int len)
{
	int this_session = cli_get_cli_session_id ();
	if (len > MAX_PMP_LEN || !pmt || (this_session < 0)) {
		return -1;
	}
	
	sprintf (this_cli[this_session].prmpt, "%s%s ", pmt, "#");
	return 0;
}

int set_hostname (const char *Hostname)
{
      int this_session = cli_get_cli_session_id ();

      if (this_session  < 0)
              return -1;

      sprintf (this_cli[this_session].hostname, "%s", Hostname);

      set_prompt ("(config)");
      get_prompt (this_cli[this_session].parser.cfg.prompt);

      return 0;
}

int  get_prompt (char *pmt)
{
	int this_session = cli_get_cli_session_id ();

	if (!pmt || (this_session < 0))
		return -1;
	memcpy (pmt, this_cli[this_session].prmpt, MAX_PMP_LEN);
	return 0;
}

void print_prompt (void)
{
	int this_session = cli_get_cli_session_id ();
	write(gfd, this_cli[this_session].prmpt, 
			strlen(this_cli[this_session].prmpt));
	fflush (stdout);
}

int write_input_on_screen(char c)
{
	write(gfd,&c,1);
	fflush (stdout);
	return 0;
}

void write_string (const char *str)
{
	int this_session = cli_get_cli_session_id ();

	if (this_session < 0) {
		write (gfd, str, strlen(str));
		return;
	}

	if (this_session == 0) {
		write (this_cli[this_session].parser.cfg.fd , str, strlen(str));
		fflush (stdout);
	} else {
		telnet_prints (this_cli[this_session].parser.session_data , str, strlen(str));
	}
	return;
}


void handle_segfault (int signo)
{
	if (signo) {
		write_string ("ooppppssssssss ....! System crashed ..."
				"Am going to sleep for 10 secs debug the image\n");
		tsk_sleep (10);
		exit (0);
	}
}

int get_curr_priv_level (void)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return -1;
	return this_cli[this_session].current_priv_level;
}

int get_curr_mode (void)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return -1;
	return this_cli[this_session].current_mode;
}

int set_curr_mode (int mode)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return -1;

	this_cli[this_session].current_mode = mode;
	return 0;
}

void set_curr_priv_level (int level)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return;

	this_cli[this_session].current_priv_level = level;
}

int set_prompt (const char *prmpt_new)
{
	char gprompt[MAX_PMP_LEN];

	char sprompt[MAX_PMP_LEN];
	int this_session = cli_get_cli_session_id ();

	if (this_session  < 0)
		return -1;

	memset (sprompt, 0, MAX_PMP_LEN);
	memset (gprompt, 0, MAX_PMP_LEN);

	sprintf (sprompt, "%s%s", this_cli[this_session].hostname, 
		 prmpt_new);
	set_prompt_internal(sprompt, strlen (sprompt));

	return 0;
}

void cli_set_port (int port)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return;

	this_cli[this_session].port_no = port;
}

int cli_get_vlan_id (void)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return -1;
	return this_cli[this_session].vlanid;
}

void cli_set_vlan_id (int vlan_id)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return;
	this_cli[this_session].vlanid = vlan_id;
}
int cli_get_port (void)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return -1;
	return this_cli[this_session].port_no;
}

int get_current_user_name (char *user)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return -1;
	strcpy (user, this_cli[this_session].username);
	return 0;
}
int set_current_user_name (char *user)
{
	int this_session = cli_get_cli_session_id ();
	if (this_session  < 0)
		return -1;
	strcpy (this_cli[this_session].username, user);
	return 0;
}

char read_input (void)
{
        char c = 0;
	int this_session = cli_get_cli_session_id ();

	if (this_session < 0) {
		while (read (fileno(stdin), &c, 1) == -1) {
			continue;
		}
		return c;
	}

	if (this_session == 0) {
		while (read (this_cli[this_session].parser.cfg.fd, &c, 1) == -1) {
			continue;
		}
		return c;
	} else {
		while (read (this_cli[this_session].parser.cfg.fd , &c, 1) == -1)
			continue;
		return c;
	}

        return c;
}

#ifndef CONFIG_OPENSWITCH_TCP_IP
int lwip_read (int fd, char *buf, int size)
{
	return read (fd, buf, size);
}
#endif

void do_mode_change (int mode)
{
	int i = sizeof (mode_p)/ sizeof(struct modes);
	int j = 0;

	while (j < i) {
		if (mode_p[j].mode == mode) {
			mode_p[j].mode_change (NULL);
			return;
		}
		j++;
	}
	return;
}

int change_vlan_mode (char **args)
{
	char prmpt[MAX_PMP_LEN];
	int vlan_id = 0;
	memset (prmpt, 0, sizeof (prmpt));
	if (args) {
		vlan_id = atoi (args[0]);
		cli_set_vlan_id (atoi(args[0]));
	}
	else {
		vlan_id = cli_get_vlan_id ();
	}
	sprintf (prmpt, "%s%d%s","(config-vlan-",vlan_id, ")");
	set_prompt (prmpt);
	set_curr_mode (VLAN_CONFIG_MODE);
	return 0;
}

int change_to_interface_mode (char **args)
{
	char prmpt[MAX_PMP_LEN];
	int port = 0;
	memset (prmpt, 0, sizeof (prmpt));
	if (args) {
		port = atoi (args[0]);
		cli_set_port (atoi(args[0]));
	}
	else {
		port = cli_get_port ();
	}
	sprintf (prmpt, "%s%d%s","(config-if-",port, ")");
	set_prompt (prmpt);
	set_curr_mode (INTERFACE_MODE);
	return 0;
}

int change_config_mode (char **args)
{
	args = args;
	set_prompt ("(config)");
	set_curr_mode (GLOBAL_CONFIG_MODE);
	return 0;
}

int end_mode (char **args)
{
	args = args;
	set_prompt ("");
	set_curr_mode (USER_EXEC_MODE);
	return 0;
}

int exit_mode (void)
{
        int mode = get_curr_mode ();

        switch (mode) {
                case GLOBAL_CONFIG_MODE:
                        do_mode_change (USER_EXEC_MODE);
                        break;
                case INTERFACE_MODE:
                        do_mode_change (GLOBAL_CONFIG_MODE);
                        break;
                case VLAN_CONFIG_MODE:
                        do_mode_change (GLOBAL_CONFIG_MODE);
                        break;
                case USER_EXEC_MODE:
                        break;
		default:
			return -1;
        }
	return 0;
}

int cli_printf  (const char *fmt, ...)
{
	int n, size = 100;
	char *p, *np;
	va_list ap;

	if ((p = malloc(size)) == NULL)
		return 0;

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size) {
			write_string (p);
			free (p);
			return 0;
		}
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n+1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		if ((np = realloc (p, size)) == NULL) {
			free(p);
			return 0;
		} else {
			p = np;
		}
	}
}

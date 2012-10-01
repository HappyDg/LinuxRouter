#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <termio.h>
#include "defs.h"
//#include "cli.h"

#define MAX_TRY 3

int validate_username_password (char *user, char *passwd);
int process_logout(void);
void show_login_prompt(void);
int process_lock(void);
int process_login(void);
void read_username_password(char *pword, int flag);
int write_input_on_screen(char c);
void write_string (const char *str);
int get_current_user_name(char *user);
int set_current_user_name(char *user);
char read_input(void);


int process_logout(void)
{
	return process_login ();
}

void show_login_prompt(void)
{     
	cli_printf("************************************************************\n");
	cli_printf("*       $$      Open Source Switch Solution     $$         *\n");
	cli_printf("************************************************************");
	process_login ();
}

int process_lock (void)
{
	char pword[MAX_USER_PASSWORD];
	char user[MAX_USER_PASSWORD];

	memset (user, 0, sizeof(user));
	memset(pword,0, sizeof(pword));

	get_current_user_name (user);
retry:
	cli_printf("\rEnter Password To UnLock The Console:");
	read_username_password (pword, 0);

	if (validate_username_password (user, pword)  < 0) {
		goto retry;
	}

	cli_printf ("\n");

	return 0;
}

int process_login (void)
{
	char pword[MAX_USER_NAME];
	char user[MAX_USER_PASSWORD];
	int  u1count = 0;

	memset (user, 0, sizeof(user));

	memset (pword, 0, sizeof(pword));

retry:
	if (u1count == MAX_TRY) {
		cli_printf("\n");
		show_login_prompt ();
		u1count = 0;
		goto login;
	}
	else {
login:
		cli_printf ("\nlogin:");
		read_username_password (user, 1);
		cli_printf ("\nPassword:");
		read_username_password (pword, 0);
		if (validate_username_password (user, pword)  < 0) {
			cli_printf ("\nIncorrect Login, Please try again");
			u1count++;
			goto retry;
		}
		set_current_user_name (user);
	}
	cli_printf ("\n");
	return 0;
}

void read_username_password (char *pword, int flag)
{
	int i = 0;
	int ch;
	while ((ch = read_input()) != '\n') {

		if (IS_BACK_SPACE(ch))  {
			if(pword[0] == '\0')
				continue;
			if (i > 0) {
				i--;
				pword[i] = '\0';
				if (flag)
					cli_printf ("\b \b");
			}
		}
		else {
			pword[i++] = (char)ch;
			if (flag)
				write_input_on_screen (ch);

		}
	}

	pword[i] = '\0';

	return;
}

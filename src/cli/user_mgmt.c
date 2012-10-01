#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <termio.h>
#include <ctype.h>
#include "defs.h"

int user_db_init(void);
int create_user(char *username, char *password, int priv_level);
int user_del(char *username);
int show_users(void);
static struct user_db * get_user_info (char *username);
int validate_username_password (char *user, char *passwd);
static int  update_user_info (char *username, char *password, int priv_level);
static int password_validation (char *pswd);
#if 0
static char * encrypt_password (char *password);
static char * decrypt_password (char *password);
#endif

struct user_db {
	char user_name[MAX_USER_NAME];
	char password[MAX_USER_PASSWORD];
	int  priv_level;
	int  status;
};

static struct  user_db userdb[MAX_USERS];

int user_db_init (void)
{
	if (create_user ((char *)"guest", (char *)"Guest1", 5)  < 0) {
		cli_printf ("Default User \"guest\" creation failed\n");
		return -1;
	}
	if (create_user ((char *)"admin", (char *)"Admin123", 0) < 0) {
		cli_printf ("Default User \"admin\" creation failed\n");
		return -1;
	}
	return 0;

}

static struct user_db * get_user_info (char *username)
{
	int i = MAX_USERS;

	if (!username || !username[0]) 
		return NULL;

	if (!strlen (username))
		return NULL;

	while (i--) {
		if (userdb[i].status && 
		    !strcmp (userdb[i].user_name, username))
				return &userdb[i];
	}
	return NULL;
}

int validate_username_password (char *user, char *passwd)
{
	struct user_db * p = get_user_info (user);

	if (!p)
		return -1;

	if (strcmp (p->password, passwd))
		return -1;
	return 0;
}

int create_user (char *username, char *password, int priv_level)
{

	int i = MAX_USERS;

	if (!username || !password) {
		cli_printf("Error!!! Creating Username and Password...\n");
		return -1;
	}

	if (password_validation (password) < 0) {
		cli_printf("Error!!! Passwords should be AlphaNumberic... eg: \"PassWord123\"\n");
		return -1;
	}

	if (priv_level < 0 || priv_level > 5) {
		cli_printf("Error!!! Priority Level Range Between 0 to 5\n");
		return -1;
	}

	if(!update_user_info (username, password, priv_level)) 
		return 0;

	while (i--) {
		if (!userdb[i].status) {
			strncpy (userdb[i].user_name, username, MAX_USER_NAME);
			strncpy (userdb[i].password, password, MAX_USER_PASSWORD);
			if (priv_level >= 0)
				userdb[i].priv_level = priv_level;
			else 
				userdb[i].priv_level = 5;
			userdb[i].status  = 1;
			return 0;
		}
	}
	
	cli_printf("Oops!!! Unable to Create User\n"); 	
	return -1;
}

static int  update_user_info (char *username, char *password, int priv_level)
{

	struct user_db * p = NULL;

	if (!(p = get_user_info (username)))
		return -1;

	if (priv_level >= 0) {
		if (p->priv_level != priv_level)
			p->priv_level = priv_level;
	}

	strncpy (p->password, password, MAX_USER_PASSWORD);

	return 0;
}

static int password_validation (char *pswd)
{
	char is_u = 0;
	char is_l = 0;
	char is_d = 0;
	char c = 0;

	if (!pswd || !pswd[0])
		return -1;

	do {
		c = *pswd;
		if (isspace (c))
			return -1;
		if (!is_u && isupper (c))
			is_u = 1;
		else if (!is_l && islower (c))
			is_l = 1;
		else if (!is_d && isdigit (c))
			is_d = 1;
		pswd++;

	} while (*pswd);

	if (is_d && is_l && is_u)
		return 0;
	return -1;

}

int user_del (char *username)
{
	struct user_db *del = NULL;

	if (!username)
		return -1;

	if (!(del = get_user_info (username))) {
		return -1;
	}

	memset (del, 0, sizeof(struct user_db));

	return 0;
} 


#if 0
static char * encrypt_password (char *password)
{
	password = password;
	return NULL;
}

static char * decrypt_password (char *password)
{
	password = password;
	return NULL;
}
#endif
int show_users (void)
{
	int  i = -1;
	cli_printf (" %-16s   %-16s    %-16s\n","Username","Password","Level");
	cli_printf (" %-16s   %-16s    %-16s\n","--------","--------", "--------");
	while (++i < MAX_USERS) {
		if (userdb[i].status)
			cli_printf (" %-16s   %-16s    %4d\n",
				userdb[i].user_name, "******", userdb[i].priv_level);
	}
	return 0;
}

#ifndef SMPD_H
#define SMPD_H

#ifndef HAVE_WINDOWS_H
#include "smpdconf.h"
#endif
#include "sock.h"
#include <stdio.h>

#define SMPD_LISTENER_PORT               8676

#define SMPD_SUCCESS                        0
#define SMPD_FAIL                          -1

#define SMPD_TRUE                           1
#define SMPD_FALSE                          0

#define SMPD_SERVER_AUTHENTICATION          0
#define SMPD_CLIENT_AUTHENTICATION          1

#define SMPD_MAX_CMD_LENGTH	         8192
#define SMPD_MAX_HOST_LENGTH	           64
#define SMPD_PASSPHRASE_MAX_LENGTH        256
#define SMPD_SALT_VALUE                   "14"
#define SMPD_AUTHENTICATION_STR_LEN       256
#define SMPD_AUTHENTICATION_REJECTED_STR  "FAIL"
#define SMPD_AUTHENTICATION_ACCEPTED_STR  "SUCCESS"
#define SMPD_SMPD_SESSION_STR             "smpd"
#define SMPD_PROCESS_SESSION_STR          "process"
#define SMPD_DEFAULT_PASSPHRASE           "behappy" /* must be less than 13 characers */
#define SMPD_DEFAULT_PASSWORD             "gastroduodenostomy"
#define SMPD_REGISTRY_KEY                 "SOFTWARE\\MPICH\\SMPD"
#define SMPD_CRED_REQUEST                 "credentials"
#define SMPD_NO_CRED_REQUEST              "nocredentials"
#define SMPD_PWD_REQUEST                  "pwd"
#define SMPD_NO_PWD_REQUEST               "nopwd"
#define SMPD_NO_RECONNECT_PORT_STR        "-1"

#define DBS_SUCCESS_STR	                  "DBS_SUCCESS"
#define DBS_FAIL_STR	                  "DBS_FAIL"
#define DBS_END_STR	                  "DBS_END"

#define SMPD_CONSOLE_STR_LENGTH         10*SMPD_MAX_CMD_LENGTH

#define SMPD_DEFAULT_TIMEOUT               45
#define SMPD_SHORT_TIMEOUT                 20

#ifdef HAVE_WINDOWS_H
#define snprintf _snprintf
#endif

extern char g_pszSMPDExe[1024];
extern int  g_bService;
extern int  g_bPasswordProtect;
extern char g_SMPDPassword[100];
extern char g_UserAccount[100];
extern char g_UserPassword[100];

/* smpd */
int smpd_parse_command_args(int *argcp, char **argvp[]);
int smpd_session(sock_set_t set, sock_t sock);
int smpd_start_mgr(sock_set_t set, sock_t sock);
#ifdef HAVE_WINDOWS_H
char *smpd_encode_handle(char *str, HANDLE h);
HANDLE smpd_decode_handle(char *str);
#endif

/* smpd_util */
int smpd_write_string(sock_set_t set, sock_t sock, char *str);
int smpd_read_string(sock_set_t set, sock_t sock, char *str, int maxlen);
int smpd_authenticate(sock_set_t set, sock_t sock, int type);
int smpd_dbg_printf(char *str, ...);
int smpd_err_printf(char *str, ...);
int smpd_get_user_data(char *key, char *value, int value_len);
int smpd_get_smpd_data(char *key, char *value, int value_len);
int smpd_get_user_data_default(char *key, char *value, int value_len);
int smpd_get_smpd_data_default(char *key, char *value, int value_len);
int smpd_set_user_data(char *key, char *value);
int smpd_set_smpd_data(char *key, char *value);
int smpd_server_authenticate(sock_set_t set, sock_t sock, char *passphrase);
int smpd_client_authenticate(sock_set_t set, sock_t sock, char *passphrase);
int smpd_getpid();
char * get_sock_error_string(int error);
int smpd_close_connection(sock_set_t set, sock_t sock);
int smpd_connect_to_smpd(sock_set_t parent_set, sock_t parent_sock, char *host, char *session_type, sock_set_t *set_ptr, sock_t *sock_ptr);
void smpd_get_password(char *password);
void smpd_get_account_and_password(char *account, char *password);
int smpd_get_credentials_from_parent(sock_set_t set, sock_t sock);
int smpd_get_smpd_password_from_parent(sock_set_t set, sock_t sock);
int smpd_get_opt(int *argc, char ***argv, char * flag);
int smpd_get_opt_int(int *argc, char ***argv, char * flag, int *n);
int smpd_get_opt_long(int *argc, char ***argv, char * flag, long *n);
int smpd_get_opt_double(int *argc, char ***argv, char * flag, double *d);
int smpd_get_opt_string(int *argc, char ***argv, char * flag, char *str, int len);


#endif

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void mp_print_options(void)
{
    printf("\n");
    printf("Usage:\n");
    printf("mpiexec -n <maxprocs> [options] executable [args ...]\n");
    printf("mpiexec [options] executable [args ...] : [options] exe [args] : ...\n");
    printf("mpiexec -file <configfile>\n");
    printf("\n");
    printf("options:\n");
    printf("\n");
    printf("standard:\n");
    printf("-n <maxprocs>\n");
    printf("-wdir <working directory>\n");
    printf("-file <filename> -\n");
    printf("       each line contains a complete set of mpiexec options\n");
    printf("       including the executable and arguments\n");
    printf("-host <hostname>\n");
    printf("-soft <Fortran90 triple> - acceptable number of processes up to maxprocs\n");
    printf("       a or a:b or a:b:c where\n");
    printf("       1) a = a\n");
    printf("       2) a:b = a, a+1, a+2, ..., b\n");
    printf("       3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc\n");
    printf("          where a+kc <= b if c>0\n");
    printf("                a+kc >= b if c<0\n");
    printf("-path <search path for executable, ; separated>\n");
    printf("-arch <architecture> - sun, linux, rs6000, ...\n");
    printf("\n");
    printf("extensions:\n");
    printf("-env <variable=value>\n");
    printf("-env <variable=value;variable2=value2;...>\n");
    printf("-hosts <n host1 host2 ... hostn>\n");
    printf("-hosts <n host1 m1 host2 m2 ... hostn mn>\n");
    printf("-machinefile <filename> - one host per line, #commented\n");
    printf("-localonly <numprocs>\n");
    printf("-nompi - processes never call any MPI functions\n");
    printf("-exitcodes - print the exit codes of processes as they exit\n");
    printf("\n");
    printf("examples:\n");
    printf("mpiexec -n 4 cpi\n");
    printf("mpiexec -n 1 -host foo master : -n 8 slave\n");
    printf("\n");
    printf("For a list of all mpiexec options, execute 'mpiexec -help2'\n");
}

void mp_print_extra_options(void)
{
    printf("\n");
    printf("All options to mpiexec:\n");
    printf("\n");
    printf("-n x\n");
    printf("-np x\n");
    printf("  launch x processes\n");
    printf("-localonly x\n");
    printf("-np x -localonly\n");
    printf("  launch x processes on the local machine\n");
    printf("-machinefile filename\n");
    printf("  use a file to list the names of machines to launch on\n");
    printf("-host hostname\n");
    printf("-hosts n host1 host2 ... hostn\n");
    printf("-hosts n host1 m1 host2 m2 ... hostn mn\n");
    printf("  launch on the specified hosts\n");
    printf("  In the second version the number of processes = m1 + m2 + ... + mn\n");
    printf("-map drive:\\\\host\\share\n");
    printf("  map a drive on all the nodes\n");
    printf("  this mapping will be removed when the processes exit\n");
    printf("-dir drive:\\my\\working\\directory\n");
    printf("-wdir /my/working/directory\n");
    printf("  launch processes in the specified directory\n");
    printf("-env var=val\n");
    printf("-env \"var1=val1;var2=val2;var3=val3...\"\n");
    printf("  set environment variables before launching the processes\n");
    printf("-logon\n");
    printf("  prompt for user account and password\n");
    printf("-pwdfile filename\n");
    printf("  read the account and password from the file specified\n");
    printf("  put the account on the first line and the password on the second\n");
    printf("-nocolor\n");
    printf("  don't use process specific output coloring\n");
    printf("-nompi\n");
    printf("  launch processes without the mpi startup mechanism\n");
    printf("-nomapping\n");
    printf("  don't try to map the current directory on the remote nodes\n");
    printf("-nopopup_debug\n");
    printf("  disable the system popup dialog if the process crashes\n");
    printf("-dbg\n");
    printf("  catch unhandled exceptions\n");
    printf("-exitcodes\n");
    printf("  print the process exit codes when each process exits.\n");
    printf("-noprompt\n");
    printf("  prevent mpirun from prompting for user credentials.\n");
    printf("-priority class[:level]\n");
    printf("  set the process startup priority class and optionally level.\n");
    printf("  class = 0,1,2,3,4   = idle, below, normal, above, high\n");
    printf("  level = 0,1,2,3,4,5 = idle, lowest, below, normal, above, highest\n");
    printf("  the default is -priority 1:3\n");
    printf("-localroot\n");
    printf("  launch the root process without smpd if the host is local.\n");
    printf("  (This allows the root process to create windows and be debugged.)\n");
    printf("-iproot\n");
    printf("-noiproot\n");
    printf("  use or not the ip address of the root host instead of the host name.\n");
}

static int strip_args(int *argcp, char **argvp[], int n)
{
    int i;

    if (n+1 > (*argcp))
    {
	printf("Error: cannot strip %d args, only %d left.\n", n, (*argcp)-1);
	return SMPD_FAIL;
    }
    for (i=n+1; i<=(*argcp); i++)
    {
	(*argvp)[i-n] = (*argvp)[i];
    }
    (*argcp) -= n;
    return SMPD_SUCCESS;
}

static int isnumber(char *str)
{
    size_t i, n = strlen(str);
    for (i=0; i<n; i++)
    {
	if (!isdigit(str[i]))
	    return SMPD_FALSE;
    }
    return SMPD_TRUE;
}

int mp_get_next_hostname(char *host)
{
    if (gethostname(host, SMPD_MAX_HOST_LENGTH) == 0)
	return SMPD_SUCCESS;
    return SMPD_FAIL;
}

int mp_get_host_id(char *host, int *id_ptr)
{
    smpd_host_node_t *node;
    static int parent = 0;
    static int id = 1;
    int bit, mask, temp;

    /* look for the host in the list */
    node = smpd_process.host_list;
    while (node)
    {
	if (strcmp(node->host, host) == 0)
	{
	    /* return the id */
	    *id_ptr = node->id;
	    return SMPD_SUCCESS;
	}
	if (node->next == NULL)
	    break;
	node = node->next;
    }

    /* allocate a new node */
    if (node != NULL)
    {
	node->next = (smpd_host_node_t *)malloc(sizeof(smpd_host_node_t));
	node = node->next;
    }
    else
    {
	node = (smpd_host_node_t *)malloc(sizeof(smpd_host_node_t));
	smpd_process.host_list = node;
    }
    if (node == NULL)
    {
	smpd_err_printf("malloc failed to allocate a host node structure\n");
	return SMPD_FAIL;
    }
    strcpy(node->host, host);
    node->parent = parent;
    node->id = id;
    node->next = NULL;

    /* move to the next id and parent */
    id++;

    temp = id >> 2;
    bit = 1;
    while (temp)
    {
	bit <<= 1;
	temp >>= 1;
    }
    mask = bit - 1;
    parent = bit | (id & mask);

    /* return the id */
    *id_ptr = node->id;

    return SMPD_SUCCESS;
}

int mp_get_next_host(smpd_host_node_t **host_node_pptr, smpd_launch_node_t *launch_node)
{
    int result;
    char host[SMPD_MAX_HOST_LENGTH];
    smpd_host_node_t *host_node_ptr;

    if (*host_node_pptr == NULL)
    {
	result = mp_get_next_hostname(host);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to get the next available host name\n");
	    return SMPD_FAIL;
	}
	result = mp_get_host_id(host, &launch_node->host_id);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to get a id for host %s\n", host);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    host_node_ptr = *host_node_pptr;
    if (host_node_ptr->nproc == 0)
    {
	(*host_node_pptr) = (*host_node_pptr)->next;
	free(host_node_ptr);
	host_node_ptr = *host_node_pptr;
	if (host_node_ptr == NULL)
	{
	    smpd_err_printf("no more hosts in the list.\n");
	    return SMPD_FAIL;
	}
    }
    result = mp_get_host_id(host_node_ptr->host, &launch_node->host_id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to get a id for host %s\n", host_node_ptr->host);
	return SMPD_FAIL;
    }
    host_node_ptr->nproc--;
    if (host_node_ptr->nproc == 0)
    {
	(*host_node_pptr) = (*host_node_pptr)->next;
	free(host_node_ptr);
    }

    return SMPD_SUCCESS;
}

#ifndef HAVE_WINDOWS_H

#if 0
int exists(char *filename)
{
    struct stat file_stat;

    if ((stat(filename, &file_stat) < 0) || !(S_ISREG(file_stat.st_mode)))
    {
	return 0; /* no such file, or not a regular file */
    }
    return 1;
}
#endif

int GetFullPathName(const char *filename, int maxlen, char *buf, char **file_part)
{
    char *path = NULL;
    char cwd[SMPD_MAX_EXE_LENGTH] = "";

    getcwd(cwd, SMPD_MAX_EXE_LENGTH);
    path = (char*)malloc((strlen(getenv("PATH")) + 1) * sizeof(char));

    if (cwd[strlen(cwd)-1] != '/')
	strcat(cwd, "/");

    /* add searching of the path and verifying file exists */

    /* for now, just put whatever they give you tacked on to the cwd */
    snprintf(buf, maxlen, "%s%s", cwd, filename);

    free(path);
    return 0;
}

#if 0
/* SEARCH_PATH() - use the given environment, find the PATH variable,
 * search the path for cmd, return a string with the full path to
 * the command.
 *
 * This could probably be done a lot more efficiently.
 *
 * Returns a pointer to a string containing the filename (including
 * path) if successful, or NULL on failure.
 */
char *search_path(char **env, char *cmd, char *cwd, int uid, int gid, char *uname)
{
    int i, len;
    char *tmp, *path, succeeded = 0;
    static char filename[4096];

    for (i=0; env[i]; i++)
    {
	if (strncmp(env[i], "PATH=", 5) != 0)
	    continue;

	len = strlen(env[i])+1;
	if (!(path=(char *)malloc(len * sizeof(char))))
	{
	    return(NULL);
	}
	bcopy(env[i], path, len * sizeof(char));

	/* check for absolute or relative pathnames */
	if ((strncmp(cmd,"./",2)) && (strncmp(cmd,"../",3)) && (cmd[0]!='/'))
	{
	    /* ok, no pathname specified, search for a valid executable */
	    tmp = NULL;
	    for (strtok(path, "="); (tmp = strtok(NULL, ":")); )
	    {
		/* concatenate search path and command */
		/* use cwd if relative path is being used in environment */
		if (!strncmp(tmp,"../",3) || !strncmp(tmp,"./",2) ||
		    !strcmp(tmp,".") || !strcmp(tmp,".."))
		{
		    snprintf(filename, 4096, "%s/%s/%s", cwd, tmp, cmd);
		}
		else
		{
		    snprintf(filename, 4096, "%s/%s", tmp, cmd);
		}
		/* see if file is executable */
		if (is_executable(filename, uname, uid, gid))
		{
		    succeeded=1;
		    break;
		}
	    }
	}
	else
	{
	    /* ok, pathname is specified */
	    if (!(strncmp(cmd,"../",3)) || !(strncmp(cmd,"./",2)))
		snprintf(filename, 4096, "%s/%s", cwd, cmd);
	    else
		strncpy(filename,cmd,4096);
	    if (is_executable(filename, uname, uid, gid))
		succeeded=1;
	}
	return((succeeded) ? filename : NULL);
    }
    return(NULL);
}

/* IS_EXECUTABLE() - checks to see if a given filename refers to a file
 * which a given user could execute
 *
 * Parameters:
 * fn  - pointer to string containing null terminated file name,
 *       including path
 * un  - pointer to string containing user name
 * uid - numeric user id for user
 * gid - numeric group id for user (from password entry)
 *
 * Returns 1 if the file exists and is executable, 0 otherwise.
 *
 * NOTE: This code in and of itself isn't a good enough check unless it
 * is called by a process with its uid/gid set to the values passed in.
 * Otherwise the directory path would not necessarily be traversable by
 * the user.
 */
int is_executable(char *fn, char *un, int uid, int gid)
{
    struct stat file_stat;

    if ((stat(fn, &file_stat) < 0) || !(S_ISREG(file_stat.st_mode)))
    {
	return(0); /* no such file, or not a regular file */
    }

    if (file_stat.st_mode & S_IXOTH)
    {
	return(1); /* other executable */
    }

    if ((file_stat.st_mode & S_IXUSR) && (file_stat.st_uid == uid))
    {
	return(1); /* user executable and user owns file */
    }

    if (file_stat.st_mode & S_IXGRP)
    {
	struct group *grp_info;
	int i;

	if (file_stat.st_gid == gid)
	{
	    return(1); /* group in passwd entry matches, executable */
	}

	/* check to see if user is in this group in /etc/group */
	grp_info = getgrgid(file_stat.st_gid);
	for(i=0; grp_info->gr_mem[i]; i++)
	{
	    if (!strcmp(un, grp_info->gr_mem[i]))
	    {
		return(1); /* group from groups matched, executable */
	    }
	}
    }
    return(0);
}
#endif
#endif

int mp_parse_command_args(int *argcp, char **argvp[])
{
    int cur_rank;
    int argc, next_argc;
    char **next_argv;
    char *exe_ptr;
    int num_args_to_strip;
    int nproc;
    int run_local = SMPD_FALSE;
    char machine_file_name[SMPD_MAX_EXE_LENGTH];
    int use_machine_file = SMPD_FALSE;
    smpd_map_drive_node_t *map_node, *drive_map_list;
    smpd_env_node_t *env_node, *env_list;
    char *equal_sign_pos;
    char wdir[SMPD_MAX_EXE_LENGTH];
    int logon;
    int use_debug_flag;
    char pwd_file_name[SMPD_MAX_EXE_LENGTH];
    int use_pwd_file;
    smpd_host_node_t *host_node_ptr, *host_list, *host_node_iter;
    int no_drive_mapping;
    int n_priority_class, n_priority, use_priorities;
    int index, i;
    char configfilename[SMPD_MAX_FILENAME];
    int use_configfile;
    char exe[SMPD_MAX_EXE_LENGTH]/*, args[SMPD_MAX_ARGS_LENGTH]*/;
    char temp_exe[SMPD_MAX_EXE_LENGTH], *namepart;
    smpd_launch_node_t *launch_node, *launch_node_iter;
    int total;

    smpd_enter_fn("mp_parse_command_args");

    /* check for console options: must be the first option */
    if (strcmp((*argvp)[1], "-console") == 0)
    {
	if (smpd_get_opt_string(argcp, argvp, "-console", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
	{
	    smpd_process.do_console = 1;
	}
	if (smpd_get_opt(argcp, argvp, "-console"))
	{
	    smpd_process.do_console = 1;
	    gethostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	}
	if (smpd_get_opt(argcp, argvp, "-p"))
	{
	    smpd_process.use_process_session = 1;
	}
	if (*argcp != 1)
	{
	    smpd_err_printf("ignoring extra arguments passed with -console option.\n");
	}
	smpd_process.host_list = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
	if (smpd_process.host_list == NULL)
	{
	    smpd_err_printf("unable to allocate a host node.\n");
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
	strcpy(smpd_process.host_list->host, smpd_process.console_host);
	smpd_process.host_list->id = 1;
	smpd_process.host_list->nproc = 0;
	smpd_process.host_list->parent = 0;
	smpd_process.host_list->next = NULL;
	smpd_exit_fn("mp_parse_command_args");
	return SMPD_SUCCESS;
    }

    if (strcmp((*argvp)[1], "-shutdown") == 0)
    {
	smpd_get_opt_string(argcp, argvp, "-shutdown", smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	if (smpd_get_opt(argcp, argvp, "-shutdown"))
	{
	    gethostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	}
	smpd_process.do_console = 1;
	smpd_process.shutdown = 1;
	smpd_process.host_list = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
	if (smpd_process.host_list == NULL)
	{
	    smpd_err_printf("unable to allocate a host node.\n");
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
	strcpy(smpd_process.host_list->host, smpd_process.console_host);
	smpd_process.host_list->id = 1;
	smpd_process.host_list->nproc = 0;
	smpd_process.host_list->parent = 0;
	smpd_process.host_list->next = NULL;
	smpd_exit_fn("mp_parse_command_args");
	return SMPD_SUCCESS;
    }

    if (strcmp((*argvp)[1], "-restart") == 0)
    {
	smpd_get_opt_string(argcp, argvp, "-restart", smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	if (smpd_get_opt(argcp, argvp, "-restart"))
	{
	    gethostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	}
	smpd_process.do_console = 1;
	smpd_process.restart = 1;
	smpd_process.host_list = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
	if (smpd_process.host_list == NULL)
	{
	    smpd_err_printf("unable to allocate a host node.\n");
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
	strcpy(smpd_process.host_list->host, smpd_process.console_host);
	smpd_process.host_list->id = 1;
	smpd_process.host_list->nproc = 0;
	smpd_process.host_list->parent = 0;
	smpd_process.host_list->next = NULL;
	smpd_exit_fn("mp_parse_command_args");
	return SMPD_SUCCESS;
    }

    /* check for mpi options */
    /*
     * Required:
     * -n <maxprocs>
     * -host <hostname>
     * -soft <Fortran90 triple> - represents allowed number of processes up to maxprocs
     *        a or a:b or a:b:c where
     *        1) a = a
     *        2) a:b = a, a+1, a+2, ..., b
     *        3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc
     *           where a+kc <= b if c>0
     *                 a+kc >= b if c<0
     * -wdir <working directory>
     * -path <search path for executable>
     * -arch <architecture> - sun, linux, rs6000, ...
     * -file <filename> - each line contains a complete set of mpiexec options, #commented
     *
     * Extensions:
     * -env <variable=value>
     * -env <variable=value;variable2=value2;...>
     * -hosts <n host1 host2 ... hostn>
     * -hosts <n host1 m1 host2 m2 ... hostn mn>
     * -machinefile <filename> - one host per line, #commented
     * -localonly <numprocs>
     * -nompi - don't require processes to be MPI processes (don't have to call MPI_Init or PMI_Init)
     * -exitcodes - print the exit codes of processes as they exit
     * -verbose - same as setting environment variable to SMPD_DBG_OUTPUT=stdout
     * 
     * Windows extensions:
     * -map <drive:\\host\share>
     * -pwdfile <filename> - account on the first line and password on the second
     * -nomapping - don't copy the current directory mapping on the remote nodes
     * -dbg - debug
     * -noprompt - don't prompt for user credentials, fail with an error message
     * -logon - force the prompt for user credentials
     * -priority <class[:level]> - set the process startup priority class and optionally level.
     *            class = 0,1,2,3,4   = idle, below, normal, above, high
     *            level = 0,1,2,3,4,5 = idle, lowest, below, normal, above, highest
     * -localroot - launch the root process without smpd if the host is local.
     *              (This allows the root process to create windows and be debugged.)
     *
     * Backwards compatibility
     * -np <numprocs>
     * -dir <working directory>
     */

    cur_rank = 0;
    next_argc = *argcp;
    next_argv = *argvp + 1;
    exe_ptr = **argvp;
    do
    {
	/* calculate the current argc and find the next argv */
	argc = 1;
	while ( (*next_argv) != NULL && (**next_argv) != ':')
	{
	    argc++;
	    next_argc--;
	    next_argv++;
	}
	if ( (*next_argv) != NULL && (**next_argv) == ':')
	{
	    (*next_argv) = NULL;
	    next_argv++;
	}
	argcp = &argc;

	/* reset block global variables */
	nproc = 0;
	drive_map_list = NULL;
	env_list = NULL;
	wdir[0] = '\0';
	logon = SMPD_FALSE;
	use_debug_flag = SMPD_FALSE;
	use_pwd_file = SMPD_FALSE;
	host_list = NULL;
	no_drive_mapping = SMPD_FALSE;
	use_priorities = SMPD_FALSE;
	use_configfile = SMPD_FALSE;
	use_machine_file = SMPD_FALSE;

	/* parse the current block */

	/* parse the mpiexec options */
	while ((*argvp)[1] && (*argvp)[1][0] == '-')
	{
	    if ((*argvp)[1][1] == '-')
	    {
		/* double -- option provided, trim it to a single - */
		index = 2;
		while ((*argvp)[1][index] != '\0')
		{
		    (*argvp)[1][index-1] = (*argvp)[1][index];
		    index++;
		}
		(*argvp)[1][index] = '\0';
	    }

	    num_args_to_strip = 1;
	    if ((strcmp(&(*argvp)[1][1], "np") == 0) || (strcmp(&(*argvp)[1][1], "n") == 0))
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no number specified after %s option.\n", (*argvp)[1]);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		nproc = atoi((*argvp)[2]);
		if (nproc < 1)
		{
		    printf("Error: must specify a number greater than 0 after the %s option\n", (*argvp)[1]);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "localonly") == 0)
	    {
		run_local = SMPD_TRUE;
		if (argc > 2)
		{
		    if (isnumber((*argvp)[2]))
		    {
			if (nproc != 0)
			{
			    printf("Error: only one option is allowed to determine the number of processes.\n");
			    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
			    smpd_exit_fn("mp_parse_command_args");
			    return SMPD_FAIL;
			}
			nproc = atoi((*argvp)[2]);
			if (nproc < 1)
			{
			    printf("Error: If you specify a number after -localonly option,\n        it must be greater than 0.\n");
			    smpd_exit_fn("mp_parse_command_args");
			    return SMPD_FAIL;
			}
			num_args_to_strip = 2;
		    }
		}
	    }
	    else if (strcmp(&(*argvp)[1][1], "machinefile") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no filename specified after -machinefile option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strcpy(machine_file_name, (*argvp)[2]);
		use_machine_file = SMPD_TRUE;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "map") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no drive specified after -map option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if ((strlen((*argvp)[2]) > 2) && (*argvp)[2][1] == ':')
		{
		    map_node = (smpd_map_drive_node_t*)malloc(sizeof(smpd_map_drive_node_t));
		    if (map_node == NULL)
		    {
			printf("Error: malloc failed to allocate map structure.\n");
			smpd_exit_fn("mp_parse_command_args");
			return SMPD_FAIL;
		    }
		    map_node->drive = (*argvp)[2][0];
		    strcpy(map_node->share, &(*argvp)[2][2]);
		    map_node->next = drive_map_list;
		    drive_map_list = map_node;
		}
		num_args_to_strip = 2;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "dir") == 0) || (strcmp(&(*argvp)[1][1], "wdir") == 0) )
	    {
		if (argc < 3)
		{
		    printf("Error: no directory after %s option\n", (*argvp)[1]);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strcpy(wdir, (*argvp)[2]);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "env") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no environment variables after -env option\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		env_node = (smpd_env_node_t*)malloc(sizeof(smpd_env_node_t));
		if (env_node == NULL)
		{
		    printf("Error: malloc failed to allocate structure to hold an environment variable.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		equal_sign_pos = strstr((*argvp)[2], "=");
		if (equal_sign_pos == NULL)
		{
		    printf("Error: improper environment variable option. '%s %s' is not in the format '-env var=value'\n",
			(*argvp)[1], (*argvp)[2]);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		*equal_sign_pos = '\0';
		strcpy(env_node->name, (*argvp)[2]);
		strcpy(env_node->value, equal_sign_pos+1);
		env_node->next = env_list;
		env_list = env_node;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "logon") == 0)
	    {
		logon = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "noprompt") == 0)
	    {
		smpd_process.credentials_prompt = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "dbg") == 0)
	    {
		use_debug_flag = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "pwdfile") == 0)
	    {
		use_pwd_file = SMPD_TRUE;
		if (argc < 3)
		{
		    printf("Error: no filename specified after -pwdfile option\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strcpy(pwd_file_name, (*argvp)[2]);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "file") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -file option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strcpy(configfilename, (*argvp)[2]);
		use_configfile = SMPD_TRUE;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "host") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no host specified after -host option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (host_list != NULL)
		{
		    printf("Error: -host option can only be called once and it cannot be combined with -hosts.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		/* create a host list of one and set nproc to -1 to be replaced by
		   nproc after parsing the block */
		host_list = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
		if (host_list == NULL)
		{
		    printf("failed to allocate memory for a host node.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		host_list->next = NULL;
		host_list->nproc = -1;
		strcpy(host_list->host, (*argvp)[2]);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "hosts") == 0)
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (host_list != NULL)
		{
		    printf("Error: -hosts option can only be called once and it cannot be combined with -host.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (argc > 2)
		{
		    if (isnumber((*argvp)[2]))
		    {
			/* initially set nproc to be the number of hosts */
			nproc = atoi((*argvp)[2]);
			if (nproc < 1)
			{
			    printf("Error: You must specify a number greater than 0 after -hosts.\n");
			    smpd_exit_fn("mp_parse_command_args");
			    return SMPD_FAIL;
			}
			num_args_to_strip = 2 + nproc;
			index = 3;
			for (i=0; i<nproc; i++)
			{
			    if (index >= argc)
			    {
				printf("Error: missing host name after -hosts option.\n");
				smpd_exit_fn("mp_parse_command_args");
				return SMPD_FAIL;
			    }
			    host_node_ptr = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
			    if (host_node_ptr == NULL)
			    {
				printf("failed to allocate memory for a host node.\n");
				smpd_exit_fn("mp_parse_command_args");
				return SMPD_FAIL;
			    }
			    host_node_ptr->next = NULL;
			    host_node_ptr->nproc = 1;
			    strcpy(host_node_ptr->host, (*argvp)[index]);
			    index++;
			    if (argc > index)
			    {
				if (isnumber((*argvp)[index]))
				{
				    host_node_ptr->nproc = atoi((*argvp)[index]);
				    index++;
				    num_args_to_strip++;
				}
			    }
			    if (host_list == NULL)
			    {
				host_list = host_node_ptr;
			    }
			    else
			    {
				host_node_iter = host_list;
				while (host_node_iter->next)
				    host_node_iter = host_node_iter->next;
				host_node_iter->next = host_node_ptr;
			    }
			}

			/* adjust nproc to be the actual number of processes */
			host_node_iter = host_list;
			nproc = 0;
			while (host_node_iter)
			{
			    nproc += host_node_iter->nproc;
			    host_node_iter = host_node_iter->next;
			}
		    }
		    else
		    {
			printf("Error: You must specify the number of hosts after the -hosts option.\n");
			smpd_exit_fn("mp_parse_command_args");
			return SMPD_FAIL;
		    }
		}
		else
		{
		    printf("Error: not enough arguments specified for -hosts option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
	    }
	    else if (strcmp(&(*argvp)[1][1], "nocolor") == 0)
	    {
		smpd_process.do_multi_color_output = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nompi") == 0)
	    {
		smpd_process.no_mpi = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nomapping") == 0)
	    {
		no_drive_mapping = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nopopup_debug") == 0)
	    {
#ifdef HAVE_WINDOWS_H
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
	    }
	    /* catch -help, -?, and --help */
	    else if (strcmp(&(*argvp)[1][1], "help") == 0 || (*argvp)[1][1] == '?' || strcmp(&(*argvp)[1][1], "-help") == 0)
	    {
		mp_print_options();
		exit(0);
	    }
	    else if (strcmp(&(*argvp)[1][1], "help2") == 0)
	    {
		mp_print_extra_options();
		exit(0);
	    }
	    else if (strcmp(&(*argvp)[1][1], "exitcodes") == 0)
	    {
		smpd_process.output_exit_codes = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "localroot") == 0)
	    {
		smpd_process.local_root = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "priority") == 0)
	    {
		char *str;
		n_priority_class = atoi((*argvp)[2]);
		str = strchr((*argvp)[2], ':');
		if (str)
		{
		    str++;
		    n_priority = atoi(str);
		}
		smpd_dbg_printf("priorities = %d:%d\n", n_priority_class, n_priority);
		use_priorities = SMPD_TRUE;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "iproot") == 0)
	    {
		smpd_process.use_iproot = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "noiproot") == 0)
	    {
		smpd_process.use_iproot = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "verbose") == 0)
	    {
		smpd_process.verbose = SMPD_TRUE;
		smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
	    }
	    else
	    {
		printf("Unknown option: %s\n", (*argvp)[1]);
	    }
	    strip_args(argcp, argvp, num_args_to_strip);
	}

	if (use_configfile)
	{
	    /* parse configuration file */
	    smpd_err_printf("configuration file parsing not implemented yet.\n");
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
	else
	{
	    /* remaining args are the executable and it's args */
	    if (argc < 2)
	    {
		printf("Error: no executable specified\n");
		smpd_exit_fn("mp_parse_command_args");
		return SMPD_FAIL;
	    }

	    if (!((*argvp)[1][0] == '\\' && (*argvp)[1][1] == '\\') && (*argvp)[1][0] != '/' &&
		!(strlen((*argvp)[1]) > 3 && (*argvp)[1][1] == ':' && (*argvp)[1][2] == '\\') )
	    {
		GetFullPathName((*argvp)[1], SMPD_MAX_EXE_LENGTH, temp_exe, &namepart);
		total = smpd_add_string(exe, SMPD_MAX_EXE_LENGTH, temp_exe);
	    }
	    else
	    {
		total = smpd_add_string(exe, SMPD_MAX_EXE_LENGTH, (*argvp)[1]);
	    }
	    for (i=2; i<argc; i++)
	    {
		total += smpd_add_string(&exe[total], SMPD_MAX_EXE_LENGTH - total, (*argvp)[i]);
	    }
	    smpd_dbg_printf("handling executable:\n%s\n", exe);
	}

	if (nproc == 0)
	{
	    smpd_err_printf("missing num_proc flag: -n, -np, or -hosts.\n");
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
	if (host_list != NULL && host_list->nproc == -1)
	{
	    /* -host specified, replace nproc field */
	    host_list->nproc = nproc;
	}

	for (i=0; i<nproc; i++)
	{
	    /* create a launch_node */
	    launch_node = (smpd_launch_node_t*)malloc(sizeof(smpd_launch_node_t));
	    if (launch_node == NULL)
	    {
		smpd_err_printf("unable to allocate a launch node structure.\n");
		smpd_exit_fn("mp_parse_command_args");
		return SMPD_FAIL;
	    }
	    mp_get_next_host(&host_list, launch_node);
	    launch_node->iproc = cur_rank++;
	    launch_node->env = launch_node->env_data;
	    launch_node->env_data[0] = '\0';
	    strcpy(launch_node->exe, exe);
	    launch_node->next = NULL;
	    if (smpd_process.launch_list == NULL)
		smpd_process.launch_list = launch_node;
	    else
	    {
		launch_node_iter = smpd_process.launch_list;
		while (launch_node_iter->next)
		    launch_node_iter = launch_node_iter->next;
		launch_node_iter->next = launch_node;
	    }
	}

	/* move to the next block */
	*argvp = next_argv - 1;
	if (*next_argv)
	    **argvp = exe_ptr;
    } while (*next_argv);

    /* add nproc to all the launch nodes */
    launch_node_iter = smpd_process.launch_list;
    while (launch_node_iter)
    {
	launch_node_iter->nproc = cur_rank;
	launch_node_iter = launch_node_iter->next;
    }

    smpd_exit_fn("mp_parse_command_args");
    return SMPD_SUCCESS;
}

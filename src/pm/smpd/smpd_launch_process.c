/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <stdlib.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_WINDOWS_H

int smpd_get_user_handle(char *account, char *domain, char *password, HANDLE *handle_ptr)
{
    HANDLE hUser;
    int error;
    int num_tries = 3;

    smpd_enter_fn("smpd_get_user_handle");

    /* logon the user */
    while (!LogonUser(
	account,
	domain,
	password,
	LOGON32_LOGON_INTERACTIVE,
	LOGON32_PROVIDER_DEFAULT,
	&hUser))
    {
	error = GetLastError();
	if (error == ERROR_NO_LOGON_SERVERS)
	{
	    if (num_tries)
		Sleep(250);
	    else
	    {
		*handle_ptr = INVALID_HANDLE_VALUE;
		smpd_exit_fn("smpd_get_user_handle");
		return error;
	    }
	    num_tries--;
	}
	else
	{
	    *handle_ptr = INVALID_HANDLE_VALUE;
	    smpd_exit_fn("smpd_get_user_handle");
	    return error;
	}
    }

    *handle_ptr = hUser;
    smpd_exit_fn("smpd_get_user_handle");
    return SMPD_SUCCESS;
}

static void SetEnvironmentVariables(char *bEnv)
{
    char name[MAX_PATH]="", value[MAX_PATH]="";
    char *pChar;
    
    pChar = name;
    while (*bEnv != '\0')
    {
	if (*bEnv == '=')
	{
	    *pChar = '\0';
	    pChar = value;
	}
	else
	{
	    if (*bEnv == ';')
	    {
		*pChar = '\0';
		pChar = name;
		SetEnvironmentVariable(name, value);
	    }
	    else
	    {
		*pChar = *bEnv;
		pChar++;
	    }
	}
	bEnv++;
    }
    *pChar = '\0';
    SetEnvironmentVariable(name, value);
}

static void RemoveEnvironmentVariables(char *bEnv)
{
    char name[MAX_PATH]="", value[MAX_PATH]="";
    char *pChar;
    
    pChar = name;
    while (*bEnv != '\0')
    {
	if (*bEnv == '=')
	{
	    *pChar = '\0';
	    pChar = value;
	}
	else
	{
	    if (*bEnv == ';')
	    {
		*pChar = '\0';
		pChar = name;
		SetEnvironmentVariable(name, NULL);
	    }
	    else
	    {
		*pChar = *bEnv;
		pChar++;
	    }
	}
	bEnv++;
    }
    *pChar = '\0';
    SetEnvironmentVariable(name, NULL);
}

int smpd_priority_class_to_win_class(int *priorityClass)
{
    *priorityClass = NORMAL_PRIORITY_CLASS;
    return SMPD_SUCCESS;
}

int smpd_priority_to_win_priority(int *priority)
{
    *priority = THREAD_PRIORITY_NORMAL;
    return SMPD_SUCCESS;
}

/* Windows code */

#define USE_LAUNCH_THREADS

#ifdef USE_LAUNCH_THREADS

typedef struct smpd_piothread_arg_t
{
    HANDLE hIn;
    SOCKET hOut;
} smpd_piothread_arg_t;

static int smpd_easy_send(SOCKET sock, char *buffer, int length)
{
    int error;
    int num_sent;

    while ((num_sent = send(sock, buffer, length, 0)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	if (error == WSAEWOULDBLOCK)
	{
            Sleep(0);
	    continue;
	}
	if (error == WSAENOBUFS)
	{
	    /* If there is no buffer space available then split the buffer in half and send each piece separately.*/
	    if (smpd_easy_send(sock, buffer, length/2) == SOCKET_ERROR)
		return SOCKET_ERROR;
	    if (smpd_easy_send(sock, buffer+(length/2), length - (length/2)) == SOCKET_ERROR)
		return SOCKET_ERROR;
	    return length;
	}
	WSASetLastError(error);
	return SOCKET_ERROR;
    }
    
    return length;
}

int smpd_piothread(smpd_piothread_arg_t *p)
{
    char buffer[1024];
    int num_read;
    HANDLE hIn;
    SOCKET hOut;

    hIn = p->hIn;
    hOut = p->hOut;
    free(p);
    p = NULL;

    smpd_dbg_printf("*** entering smpd_piothread ***\n");
    while (1)
    {
	if (!ReadFile(hIn, buffer, 1024, &num_read, NULL))
	{
	    smpd_dbg_printf("ReadFile failed, error %d\n", GetLastError());
	    break;
	}
	if (num_read < 1)
	{
	    smpd_dbg_printf("ReadFile returned %d bytes\n", num_read);
	    break;
	}
	/*smpd_dbg_printf("*** smpd_piothread read %d bytes ***\n", num_read);*/
	if (smpd_easy_send(hOut, buffer, num_read) == SOCKET_ERROR)
	{
	    smpd_dbg_printf("smpd_easy_send of %d bytes failed.\n", num_read);
	    break;
	}
	/*smpd_dbg_printf("*** smpd_piothread wrote %d bytes ***\n", num_read);*/
    }
    smpd_dbg_printf("*** smpd_piothread finishing ***\n");
    FlushFileBuffers((HANDLE)hOut);
    shutdown(hOut, SD_BOTH);
    closesocket(hOut);
    CloseHandle(hIn);
    /*smpd_dbg_printf("*** exiting smpd_piothread ***\n");*/
    return 0;
}

int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, sock_set_t set)
{
    HANDLE hStdin, hStdout, hStderr;
    SOCKET hSockStdinR = INVALID_SOCKET, hSockStdinW = INVALID_SOCKET;
    SOCKET hSockStdoutR = INVALID_SOCKET, hSockStdoutW = INVALID_SOCKET;
    SOCKET hSockStderrR = INVALID_SOCKET, hSockStderrW = INVALID_SOCKET;
    /*HANDLE hPipeStdinR = NULL, hPipeStdinW = NULL;*/
    HANDLE hPipeStdoutR = NULL, hPipeStdoutW = NULL;
    HANDLE hPipeStderrR = NULL, hPipeStderrW = NULL;
    HANDLE hIn, hOut, hErr;
    STARTUPINFO saInfo;
    PROCESS_INFORMATION psInfo;
    void *pEnv=NULL;
    char tSavedPath[MAX_PATH] = ".";
    DWORD launch_flag;
    int nError, result;
    unsigned long blocking_flag;
    sock_t sock_in, sock_out, sock_err;
    SECURITY_ATTRIBUTES saAttr;

    smpd_enter_fn("smpd_launch_process");

    smpd_priority_class_to_win_class(&priorityClass);
    smpd_priority_to_win_priority(&priority);

    /* Save stdin, stdout, and stderr */
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE  || hStderr == INVALID_HANDLE_VALUE)
    {
	nError = GetLastError(); /* This will only be correct if stderr failed */
	smpd_err_printf("GetStdHandle failed, error %d\n", nError);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;;
    }

    /* Create sockets for stdin, stdout, and stderr */
    if (nError = smpd_make_socket_loop_choose(&hSockStdinR, SMPD_FALSE, &hSockStdinW, SMPD_TRUE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (nError = smpd_make_socket_loop_choose(&hSockStdoutR, SMPD_TRUE, &hSockStdoutW, SMPD_FALSE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (nError = smpd_make_socket_loop_choose(&hSockStderrR, SMPD_TRUE, &hSockStderrW, SMPD_FALSE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.lpSecurityDescriptor = NULL;
    saAttr.bInheritHandle = TRUE;

    /* Create the pipes for stdout, stderr */
    if (!CreatePipe(&hPipeStdoutR, &hPipeStdoutW, &saAttr, 0))
    {
	smpd_err_printf("CreatePipe(stdout) failed, error %d\n", GetLastError());
	goto CLEANUP;
    }
    if (!CreatePipe(&hPipeStderrR, &hPipeStderrW, &saAttr, 0))
    {
	smpd_err_printf("CreatePipe(stderr) failed, error %d\n", GetLastError());
	goto CLEANUP;
    }

    /* Make the ends of the pipes that this process will use not inheritable */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinW, GetCurrentProcess(), &hIn, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    /*
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutR, GetCurrentProcess(), &hOut, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrR, GetCurrentProcess(), &hErr, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hPipeStdoutR, GetCurrentProcess(), &hOut, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hPipeStderrR, GetCurrentProcess(), &hErr, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* prevent the socket loops from being inherited */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutR, GetCurrentProcess(), &hSockStdoutR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrR, GetCurrentProcess(), &hSockStderrR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutW, GetCurrentProcess(), &hSockStdoutW, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrW, GetCurrentProcess(), &hSockStderrW, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* make the ends used by the spawned process blocking */
    blocking_flag = 0;
    ioctlsocket(hSockStdinR, FIONBIO, &blocking_flag);
    /*
    blocking_flag = 0;
    ioctlsocket(hSockStdoutW, FIONBIO, &blocking_flag);
    blocking_flag = 0;
    ioctlsocket(hSockStderrW, FIONBIO, &blocking_flag);
    */

    /* Set stdin, stdout, and stderr to the ends of the pipe the created process will use */
    if (!SetStdHandle(STD_INPUT_HANDLE, (HANDLE)hSockStdinR))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    /*
    if (!SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)hSockStdoutW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    if (!SetStdHandle(STD_ERROR_HANDLE, (HANDLE)hSockStderrW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    */
    if (!SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)hPipeStdoutW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    if (!SetStdHandle(STD_ERROR_HANDLE, (HANDLE)hPipeStderrW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }

    /* Create the process */
    memset(&saInfo, 0, sizeof(STARTUPINFO));
    saInfo.cb = sizeof(STARTUPINFO);
    saInfo.hStdInput = (HANDLE)hSockStdinR;
    /*
    saInfo.hStdOutput = (HANDLE)hSockStdoutW;
    saInfo.hStdError = (HANDLE)hSockStderrW;
    */
    saInfo.hStdOutput = (HANDLE)hPipeStdoutW;
    saInfo.hStdError = (HANDLE)hPipeStderrW;
    saInfo.dwFlags = STARTF_USESTDHANDLES;

    SetEnvironmentVariables(process->env);
    pEnv = GetEnvironmentStrings();

    GetCurrentDirectory(MAX_PATH, tSavedPath);
    SetCurrentDirectory(process->dir);

    launch_flag = 
	CREATE_SUSPENDED | CREATE_NO_WINDOW | priorityClass;
    if (dbg)
	launch_flag = launch_flag | DEBUG_PROCESS;

    psInfo.hProcess = INVALID_HANDLE_VALUE;
    if (CreateProcess(
	NULL,
	process->exe,
	NULL, NULL, TRUE,
	launch_flag,
	pEnv,
	NULL,
	&saInfo, &psInfo))
    {
	SetThreadPriority(psInfo.hThread, priority);
	process->pid = psInfo.dwProcessId;
    }
    else
    {
	nError = GetLastError();
	smpd_err_printf("CreateProcess failed, error %d\n", nError);
    }

    FreeEnvironmentStrings((TCHAR*)pEnv);
    SetCurrentDirectory(tSavedPath);
    RemoveEnvironmentVariables(process->env);

    /* make sock structures out of the sockets */
    nError = sock_native_to_sock(set, hIn, NULL, &sock_in);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }
    nError = sock_native_to_sock(set, (SOCK_NATIVE_FD)hSockStdoutR, NULL, &sock_out);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }
    nError = sock_native_to_sock(set, (SOCK_NATIVE_FD)hSockStderrR, NULL, &sock_err);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }

    process->in->sock = sock_in;
    process->out->sock = sock_out;
    process->err->sock = sock_err;
    process->pid = process->in->id = process->out->id = process->err->id = psInfo.dwProcessId;
    sock_set_user_ptr(sock_in, process->in);
    sock_set_user_ptr(sock_out, process->out);
    sock_set_user_ptr(sock_err, process->err);

RESTORE_CLEANUP:
    /* Restore stdin, stdout, stderr */
    SetStdHandle(STD_INPUT_HANDLE, hStdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStderr);

CLEANUP:
    CloseHandle((HANDLE)hSockStdinR);
    /*
    CloseHandle((HANDLE)hSockStdoutW);
    CloseHandle((HANDLE)hSockStderrW);
    */
    CloseHandle((HANDLE)hPipeStdoutW);
    CloseHandle((HANDLE)hPipeStderrW);

    if (psInfo.hProcess != INVALID_HANDLE_VALUE)
    {
	HANDLE hThread;
	smpd_piothread_arg_t *arg_ptr;

	arg_ptr = (smpd_piothread_arg_t*)malloc(sizeof(smpd_piothread_arg_t));
	arg_ptr->hIn = hOut;
	arg_ptr->hOut = hSockStdoutW;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_piothread, arg_ptr, 0, NULL);
	CloseHandle(hThread);
	arg_ptr = (smpd_piothread_arg_t*)malloc(sizeof(smpd_piothread_arg_t));
	arg_ptr->hIn = hErr;
	arg_ptr->hOut = hSockStderrW;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_piothread, arg_ptr, 0, NULL);
	CloseHandle(hThread);

	result = sock_post_read(sock_out, process->out->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	result = sock_post_read(sock_err, process->err->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	ResumeThread(psInfo.hThread);
	process->wait = process->in->wait = process->out->wait = process->err->wait = psInfo.hProcess;
	CloseHandle(psInfo.hThread);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_SUCCESS;
    }

    smpd_exit_fn("smpd_launch_process");
    return SMPD_FAIL;
}

#else

int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, sock_set_t set)
{
    HANDLE hStdin, hStdout, hStderr;
    SOCKET hSockStdinR = INVALID_SOCKET, hSockStdinW = INVALID_SOCKET;
    SOCKET hSockStdoutR = INVALID_SOCKET, hSockStdoutW = INVALID_SOCKET;
    SOCKET hSockStderrR = INVALID_SOCKET, hSockStderrW = INVALID_SOCKET;
    HANDLE hIn, hOut, hErr;
    STARTUPINFO saInfo;
    PROCESS_INFORMATION psInfo;
    void *pEnv=NULL;
    char tSavedPath[MAX_PATH] = ".";
    DWORD launch_flag;
    int nError, result;
    unsigned long blocking_flag;
    sock_t sock_in, sock_out, sock_err;

    smpd_enter_fn("smpd_launch_process");

    smpd_priority_class_to_win_class(&priorityClass);
    smpd_priority_to_win_priority(&priority);

    /* Save stdin, stdout, and stderr */
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE  || hStderr == INVALID_HANDLE_VALUE)
    {
	nError = GetLastError();
	smpd_err_printf("GetStdHandle failed, error %d\n", nError);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;;
    }

    /* Create sockets for stdin, stdout, and stderr */
    if (nError = smpd_make_socket_loop_choose(&hSockStdinR, SMPD_FALSE, &hSockStdinW, SMPD_TRUE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (nError = smpd_make_socket_loop_choose(&hSockStdoutR, SMPD_TRUE, &hSockStdoutW, SMPD_FALSE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (nError = smpd_make_socket_loop_choose(&hSockStderrR, SMPD_TRUE, &hSockStderrW, SMPD_FALSE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* Make the ends of the pipes that this process will use not inheritable */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinW, GetCurrentProcess(), &hIn, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutR, GetCurrentProcess(), &hOut, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrR, GetCurrentProcess(), &hErr, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* make the ends used by the spawned process blocking */
    blocking_flag = 0;
    ioctlsocket(hSockStdinR, FIONBIO, &blocking_flag);
    blocking_flag = 0;
    ioctlsocket(hSockStdoutW, FIONBIO, &blocking_flag);
    blocking_flag = 0;
    ioctlsocket(hSockStderrW, FIONBIO, &blocking_flag);

    /* Set stdin, stdout, and stderr to the ends of the pipe the created process will use */
    if (!SetStdHandle(STD_INPUT_HANDLE, (HANDLE)hSockStdinR))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)hSockStdoutW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    if (!SetStdHandle(STD_ERROR_HANDLE, (HANDLE)hSockStderrW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }

    /* Create the process */
    memset(&saInfo, 0, sizeof(STARTUPINFO));
    saInfo.cb = sizeof(STARTUPINFO);
    saInfo.hStdError = (HANDLE)hSockStderrW;
    saInfo.hStdInput = (HANDLE)hSockStdinR;
    saInfo.hStdOutput = (HANDLE)hSockStdoutW;
    saInfo.dwFlags = STARTF_USESTDHANDLES;

    SetEnvironmentVariables(process->env);
    pEnv = GetEnvironmentStrings();

    GetCurrentDirectory(MAX_PATH, tSavedPath);
    SetCurrentDirectory(process->dir);

    launch_flag = 
	CREATE_SUSPENDED | CREATE_NO_WINDOW | priorityClass;
    if (dbg)
	launch_flag = launch_flag | DEBUG_PROCESS;

    psInfo.hProcess = INVALID_HANDLE_VALUE;
    if (CreateProcess(
	NULL,
	process->exe,
	NULL, NULL, TRUE,
	launch_flag,
	pEnv,
	NULL,
	&saInfo, &psInfo))
    {
	SetThreadPriority(psInfo.hThread, priority);
	process->pid = psInfo.dwProcessId;
    }
    else
    {
	nError = GetLastError();
	smpd_err_printf("CreateProcess failed, error %d\n", nError);
    }

    FreeEnvironmentStrings((TCHAR*)pEnv);
    SetCurrentDirectory(tSavedPath);
    RemoveEnvironmentVariables(process->env);

    /* make sock structures out of the sockets */
    nError = sock_native_to_sock(set, hIn, NULL, &sock_in);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }
    nError = sock_native_to_sock(set, hOut, NULL, &sock_out);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }
    nError = sock_native_to_sock(set, hErr, NULL, &sock_err);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }

    process->in->sock = sock_in;
    process->out->sock = sock_out;
    process->err->sock = sock_err;
    process->pid = process->in->id = process->out->id = process->err->id = psInfo.dwProcessId;
    sock_set_user_ptr(sock_in, process->in);
    sock_set_user_ptr(sock_out, process->out);
    sock_set_user_ptr(sock_err, process->err);

RESTORE_CLEANUP:
    /* Restore stdin, stdout, stderr */
    SetStdHandle(STD_INPUT_HANDLE, hStdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStderr);

CLEANUP:
    CloseHandle((HANDLE)hSockStdinR);
    CloseHandle((HANDLE)hSockStdoutW);
    CloseHandle((HANDLE)hSockStderrW);

    if (psInfo.hProcess != INVALID_HANDLE_VALUE)
    {
	result = sock_post_read(sock_out, process->out->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	result = sock_post_read(sock_err, process->err->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	ResumeThread(psInfo.hThread);
	process->wait = process->in->wait = process->out->wait = process->err->wait = psInfo.hProcess;
	CloseHandle(psInfo.hThread);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_SUCCESS;
    }

    smpd_exit_fn("smpd_launch_process");
    return SMPD_FAIL;
}

#endif

void smpd_parse_account_domain(char *domain_account, char *account, char *domain)
{
    char *pCh, *pCh2;

    smpd_enter_fn("smpd_parse_account_domain");

    pCh = domain_account;
    pCh2 = domain;
    while ((*pCh != '\\') && (*pCh != '\0'))
    {
	*pCh2 = *pCh;
	pCh++;
	pCh2++;
    }
    if (*pCh == '\\')
    {
	pCh++;
	strcpy(account, pCh);
	*pCh2 = L'\0';
    }
    else
    {
	strcpy(account, domain_account);
	domain[0] = '\0';
    }

    smpd_exit_fn("smpd_parse_account_domain");
}

#else

/* Unix code */

int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, sock_set_t set)
{
    int result;
    int stdin_pipe_fds[2], stdout_pipe_fds[2], stderr_pipe_fds[2];
    int pid;
    sock_t sock_in, sock_out, sock_err;
    char args[SMPD_MAX_EXE_LENGTH];
    char *argv[1024];
    char *token;
    int i;

    smpd_enter_fn("smpd_launch_process");

    /* parse the command for arguments */
    args[0] = '\0';
    strcpy(args, process->exe);
    i = 0;
    token = strtok(args, " ");
    while (token)
    {
	argv[i] = token;
	token = strtok(NULL, " ");
	i++;
    }
    argv[i] = NULL;

    /* create pipes for redirecting I/O */
    /*
    pipe(stdin_pipe_fds);
    pipe(stdout_pipe_fds);
    pipe(stderr_pipe_fds);
    */
    socketpair(AF_UNIX, SOCK_STREAM, 0, stdin_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, stdout_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, stderr_pipe_fds);

    pid = fork();
    if (pid < 0)
    {
	smpd_err_printf("fork failed - error %d.\n", errno);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }

    if (pid == 0)
    {
	/* child process */
	smpd_dbg_printf("client is alive and about to redirect io\n");

	close(0); 		  /* close stdin     */
	dup(stdin_pipe_fds[0]);   /* dup a new stdin */
	close(stdin_pipe_fds[0]);
	close(stdin_pipe_fds[1]);

	close(1);		  /* close stdout     */
	dup(stdout_pipe_fds[1]);  /* dup a new stdout */
	close(stdout_pipe_fds[0]);
	close(stdout_pipe_fds[1]);

	close(2);		  /* close stderr     */
	dup(stderr_pipe_fds[1]);  /* dup a new stderr */
	close(stderr_pipe_fds[0]);
	close(stderr_pipe_fds[1]);

	result = -1;
	if (process->dir[0] != '\0')
	    result = chdir( process->dir );
	if (result < 0)
	    chdir( getenv( "HOME" ) );

	/*result = execvp( process->exe, NULL );*/
	result = execvp( argv[0], argv );

	result = errno;
	fprintf(stderr, "error %d, unable to exec '%s'.\n", result, process->exe);
	exit(result);
    }

    /* parent process */
    process->pid = pid;
    close(stdin_pipe_fds[0]);
    close(stdout_pipe_fds[1]);
    close(stderr_pipe_fds[1]);

    /* make sock structures out of the sockets */
    result = sock_native_to_sock(set, stdin_pipe_fds[1], NULL, &sock_in);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_native_to_sock(set, stdout_pipe_fds[0], NULL, &sock_out);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_native_to_sock(set, stderr_pipe_fds[0], NULL, &sock_err);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    process->in->sock = sock_in;
    process->out->sock = sock_out;
    process->err->sock = sock_err;
    process->pid = process->in->id = process->out->id = process->err->id = pid;
    result = sock_set_user_ptr(sock_in, process->in);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_set_user_ptr(sock_out, process->out);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_set_user_ptr(sock_err, process->err);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }

    result = sock_post_read(sock_out, process->out->read_cmd.cmd, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }
    result = sock_post_read(sock_err, process->err->read_cmd.cmd, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }
    process->wait = process->in->wait = process->out->wait = process->err->wait = pid;

    smpd_exit_fn("smpd_launch_process");
    return SMPD_SUCCESS;
}

#endif

int smpd_wait_process(smpd_pwait_t wait, int *exit_code_ptr)
{
#ifdef HAVE_WINDOWS_H
    int result;
    smpd_enter_fn("smpd_wait_process");

    if (WaitForSingleObject(wait, INFINITE) != WAIT_OBJECT_0)
    {
	smpd_err_printf("WaitForSingleObject failed, error %d\n", GetLastError());
	*exit_code_ptr = -1;
	smpd_exit_fn("smpd_wait_process");
	return SMPD_FAIL;
    }
    result = GetExitCodeProcess(wait, exit_code_ptr);
    if (!result)
    {
	smpd_err_printf("GetExitCodeProcess failed, error %d\n", GetLastError());
	*exit_code_ptr = -1;
	smpd_exit_fn("smpd_wait_process");
	return SMPD_FAIL;
    }
    CloseHandle(wait);

    smpd_exit_fn("smpd_wait_process");
    return SMPD_SUCCESS;
#else
    int status;
    smpd_enter_fn("smpd_wait_process");

    smpd_dbg_printf("waiting for process %d\n", wait);
    waitpid(wait, &status, WUNTRACED);
    if (WIFEXITED(status))
    {
	*exit_code_ptr =  WEXITSTATUS(status);
    }
    else
    {
	smpd_err_printf("WIFEXITED(%d) failed, setting exit code to -1\n", wait);
	*exit_code_ptr = -1;
    }

    smpd_exit_fn("smpd_wait_process");
    return SMPD_SUCCESS;
#endif
}

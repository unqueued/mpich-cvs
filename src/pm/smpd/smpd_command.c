/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpiexec.h"
#include "smpd.h"

char * smpd_get_cmd_state_string(smpd_command_state_t state)
{
    static char unknown_str[100];

    switch (state)
    {
    case SMPD_CMD_INVALID:
	return "SMPD_CMD_INVALID";
    case SMPD_CMD_READING_HDR:
	return "SMPD_CMD_READING_HDR";
    case SMPD_CMD_READING_CMD:
	return "SMPD_CMD_READING_CMD";
    case SMPD_CMD_WRITING_CMD:
	return "SMPD_CMD_WRITING_CMD";
    case SMPD_CMD_READY:
	return "SMPD_CMD_READY";
    case SMPD_CMD_HANDLED:
	return "SMPD_CMD_HANDLED";
    }
    sprintf(unknown_str, "unknown state %d", state);
    return unknown_str;
}

SMPD_BOOL smpd_command_to_string(char **str_pptr, int *len_ptr, int indent, smpd_command_t *cmd_ptr)
{
    char indent_str[SMPD_MAX_TO_STRING_INDENT+1];

    if (*len_ptr < 1)
	return SMPD_FALSE;

    if (indent > SMPD_MAX_TO_STRING_INDENT)
	indent = SMPD_MAX_TO_STRING_INDENT;

    memset(indent_str, ' ', indent);
    indent_str[indent] = '\0';

    smpd_snprintf_update(str_pptr, len_ptr, "%sstate: %s\n", indent_str, smpd_get_cmd_state_string(cmd_ptr->state));
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%scmd_str: %s\n", indent_str, cmd_ptr->cmd_str);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%ssrc: %d\n", indent_str, cmd_ptr->src);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%sdest: %d\n", indent_str, cmd_ptr->dest);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%stag: %d\n", indent_str, cmd_ptr->tag);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%swait: %s\n", indent_str, cmd_ptr->wait ? "TRUE" : "FALSE");
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%scmd_hdr_str: %s\n", indent_str, cmd_ptr->cmd_hdr_str);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%slength: %d\n", indent_str, cmd_ptr->length);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%scmd: %s\n", indent_str, cmd_ptr->cmd);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%sfreed: %d\n", indent_str, cmd_ptr->freed);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%siov[0].buf: %p\n", indent_str, cmd_ptr->iov[0].MPID_IOV_BUF);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%siov[0].len: %d\n", indent_str, cmd_ptr->iov[0].MPID_IOV_LEN);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%siov[1].buf: %p\n", indent_str, cmd_ptr->iov[1].MPID_IOV_BUF);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%siov[1].len: %d\n", indent_str, cmd_ptr->iov[1].MPID_IOV_LEN);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%sstdin_read_offset: %d\n", indent_str, cmd_ptr->stdin_read_offset);
    if (*len_ptr < 1) return SMPD_FALSE;
    smpd_snprintf_update(str_pptr, len_ptr, "%snext: %p\n", indent_str, cmd_ptr->next);
    if (*len_ptr < 1) return SMPD_FALSE; /* this misses the case of an exact fit */

    return SMPD_TRUE;
}

int smpd_command_destination(int dest, smpd_context_t **dest_context)
{
    int src, level_bit, sub_tree_mask;

    smpd_enter_fn("smpd_command_destination");

    /* get the source */
    src = smpd_process.id;

    /*smpd_dbg_printf("determining destination context for %d -> %d\n", src, dest);*/

    /* determine the route and return the context */
    if (src == dest)
    {
	*dest_context = NULL;
	smpd_dbg_printf("%d -> %d : returning NULL context\n", src, dest);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    if (src == 0)
    {
	/* this assumes that the root uses the left context for it's only child. */
	if (smpd_process.left_context == NULL)
	{
	    smpd_exit_fn("smpd_command_destination");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.left_context;
	smpd_dbg_printf("%d -> %d : returning left_context\n", src, dest);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    if (dest < src)
    {
	if (smpd_process.parent_context == NULL)
	{
	    smpd_exit_fn("smpd_command_destination");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.parent_context;
	smpd_dbg_printf("%d -> %d : returning parent_context: %d < %d\n", src, dest, dest, src);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    level_bit = 0x1 << smpd_process.level;
    sub_tree_mask = (level_bit << 1) - 1;

    if (( src ^ level_bit ) == ( dest & sub_tree_mask ))
    {
	if (smpd_process.left_context == NULL)
	{
	    smpd_exit_fn("smpd_command_destination");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.left_context;
	smpd_dbg_printf("returning left_context\n", src, dest);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    if ( src == ( dest & sub_tree_mask ) )
    {
	if (smpd_process.right_context == NULL)
	{
	    smpd_exit_fn("smpd_command_destination");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.right_context;
	smpd_dbg_printf("%d -> %d : returning right_context\n", src, dest);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    if (smpd_process.parent_context == NULL)
    {
	smpd_exit_fn("smpd_command_destination");
	return SMPD_FAIL;
    }
    *dest_context = smpd_process.parent_context;
    smpd_dbg_printf("%d -> %d : returning parent_context: fall through\n", src, dest);
    smpd_exit_fn("smpd_command_destination");
    return SMPD_SUCCESS;
}

int smpd_init_command(smpd_command_t *cmd)
{
    smpd_enter_fn("smpd_init_command");

    if (cmd == NULL)
    {
	smpd_exit_fn("smpd_init_command");
	return SMPD_FAIL;
    }

    cmd->cmd_hdr_str[0] = '\0';
    cmd->cmd_str[0] = '\0';
    cmd->cmd[0] = '\0';
    cmd->dest = -1;
    cmd->src = -1;
    cmd->tag = -1;
    cmd->next = NULL;
    cmd->length = 0;
    cmd->wait = SMPD_FALSE;
    cmd->state = SMPD_CMD_INVALID;
    cmd->stdin_read_offset = 0;
    cmd->freed = 0;
    cmd->context = NULL;

    smpd_exit_fn("smpd_init_command");
    return SMPD_SUCCESS;
}

int smpd_parse_command(smpd_command_t *cmd_ptr)
{
    smpd_enter_fn("smpd_parse_command");

    /* get the source */
    if (!smpd_get_int_arg(cmd_ptr->cmd, "src", &cmd_ptr->src))
    {
	smpd_err_printf("no src flag in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }
    if (cmd_ptr->src < 0)
    {
	smpd_err_printf("invalid command src: %d\n", cmd_ptr->src);
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the destination */
    if (!smpd_get_int_arg(cmd_ptr->cmd, "dest", &cmd_ptr->dest))
    {
	smpd_err_printf("no dest flag in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }
    if (cmd_ptr->dest < 0)
    {
	smpd_err_printf("invalid command dest: %d\n", cmd_ptr->dest);
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the command string */
    if (!smpd_get_string_arg(cmd_ptr->cmd, "cmd", cmd_ptr->cmd_str, SMPD_MAX_CMD_STR_LENGTH))
    {
	smpd_err_printf("no cmd string in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the tag */
    smpd_get_int_arg(cmd_ptr->cmd, "tag", &cmd_ptr->tag);

    smpd_exit_fn("smpd_parse_command");
    return SMPD_SUCCESS;
}

int smpd_create_command(char *cmd, int src, int dest, int want_reply, smpd_command_t **cmd_pptr)
{
    smpd_command_t *cmd_ptr;
    char *str;
    int len;
    int result;

    smpd_enter_fn("smpd_create_command");

    cmd_ptr = (smpd_command_t*)malloc(sizeof(smpd_command_t));
    if (cmd_ptr == NULL)
    {
	smpd_err_printf("unable to allocate memory for a command.\n");
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    memset(cmd_ptr, 0, sizeof(smpd_command_t));
    smpd_init_command(cmd_ptr);
    cmd_ptr->src = src;
    cmd_ptr->dest = dest;
    cmd_ptr->tag = smpd_process.cur_tag++;

    if (strlen(cmd) >= SMPD_MAX_CMD_STR_LENGTH)
    {
	smpd_err_printf("command string too long: %s\n", cmd);
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    strcpy(cmd_ptr->cmd_str, cmd);

    str = cmd_ptr->cmd;
    len = SMPD_MAX_CMD_LENGTH;
    result = smpd_add_string_arg(&str, &len, "cmd", cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create the command.\n");
	smpd_free_command(cmd_ptr);
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "src", src);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the src to the command.\n");
	smpd_free_command(cmd_ptr);
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "dest", dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the dest to the command.\n");
	smpd_free_command(cmd_ptr);
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "tag", cmd_ptr->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the command.\n");
	smpd_free_command(cmd_ptr);
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    if (want_reply)
	cmd_ptr->wait = SMPD_TRUE;

    *cmd_pptr = cmd_ptr;
    smpd_exit_fn("smpd_create_command");
    return SMPD_SUCCESS;
}

int smpd_create_command_copy(smpd_command_t *src_ptr, smpd_command_t **cmd_pptr)
{
    smpd_command_t *cmd_ptr;

    smpd_enter_fn("smpd_create_command_copy");

    cmd_ptr = (smpd_command_t*)malloc(sizeof(smpd_command_t));
    if (cmd_ptr == NULL)
    {
	smpd_err_printf("unable to allocate memory for a command.\n");
	smpd_exit_fn("smpd_create_command_copy");
	return SMPD_FAIL;
    }
    
    *cmd_ptr = *src_ptr;
    *cmd_pptr = cmd_ptr;

    smpd_exit_fn("smpd_create_command_copy");
    return SMPD_SUCCESS;
}

int smpd_free_command(smpd_command_t *cmd_ptr)
{
    smpd_enter_fn("smpd_free_command");
    if (cmd_ptr)
    {
	/* this check isn't full-proof because random data might match SMPD_FREE_COOKIE */
	if (cmd_ptr->freed == SMPD_FREE_COOKIE)
	{
	    smpd_err_printf("attempt to free a command more than once.\n");
	    return SMPD_FAIL;
	}
	/* erase the contents to help track down use of freed structures */
	smpd_init_command(cmd_ptr);
	free(cmd_ptr);
    }
    smpd_exit_fn("smpd_free_command");
    return SMPD_SUCCESS;
}

int smpd_create_context(smpd_context_type_t type, MPIDU_Sock_set_t set, MPIDU_Sock_t sock, int id, smpd_context_t **context_pptr)
{
    int result;
    smpd_context_t *context;
    
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	return SMPD_FAIL;
    }
    memset(context, 0, sizeof(smpd_context_t));
    result = smpd_init_context(context, type, set, sock, id);
    if (result != SMPD_SUCCESS)
    {
	*context_pptr = NULL;
	free(context);
	return SMPD_FAIL;
    }
    
    /* add the context to the global list */
    context->next = smpd_process.context_list;
    smpd_process.context_list = context;

    *context_pptr = context;
    return result;
}

int smpd_free_context(smpd_context_t *context)
{
    SMPD_BOOL found = SMPD_FALSE;
    smpd_context_t *iter, *trailer;

    smpd_enter_fn("smpd_free_context");
    if (context)
    {
	/* remove the context from the global list */
	iter = trailer = smpd_process.context_list;
	while (iter)
	{
	    if (iter == context)
	    {
		if (iter == smpd_process.context_list)
		    smpd_process.context_list = smpd_process.context_list->next;
		else
		    trailer->next = iter->next;
		found = SMPD_TRUE;
		break;
	    }
	    if (trailer != iter)
		trailer = trailer->next;
	    iter = iter->next;
	}

	if (context->type == SMPD_CONTEXT_FREED)
	{
	    smpd_err_printf("context already freed.\n");
	    smpd_exit_fn("smpd_free_context");
	    return SMPD_FAIL;
	}

	if (!found)
	{
	    smpd_dbg_printf("freeing a context not in the global list - this should be impossible.  "
		"No context should be allocated without calling smpd_create_context.\n");
	}

	/* this check isn't full-proof because random data might match SMPD_CONTEXT_FREED */
	if (context->type == SMPD_CONTEXT_FREED)
	{
	    smpd_err_printf("attempt to free context more than once.\n");
	    return SMPD_FAIL;
	}
	/* erase the contents to help track down use of freed structures */
	memset(context, 0, sizeof(smpd_context_t));
	smpd_init_context(context, SMPD_CONTEXT_FREED, MPIDU_SOCK_INVALID_SET, MPIDU_SOCK_INVALID_SOCK, -1);
	free(context);
    }
    smpd_exit_fn("smpd_free_context");
    return SMPD_SUCCESS;
}

int smpd_add_command_arg(smpd_command_t *cmd_ptr, char *param, char *value)
{
    char *str;
    int len;
    int result;
    int cmd_length;

    cmd_length = (int)strlen(cmd_ptr->cmd);
    if (cmd_length > SMPD_MAX_CMD_LENGTH)
    {
	smpd_err_printf("invalid cmd string length: %d\n", cmd_length);
	return SMPD_FAIL;
    }

    len = (int)(SMPD_MAX_CMD_LENGTH - cmd_length);
    str = &cmd_ptr->cmd[cmd_length];

    /* make sure there is a space after the last parameter in the command */
    if (cmd_length > 0)
    {
	if (cmd_ptr->cmd[cmd_length-1] != ' ')
	{
	    if (len < 2)
	    {
		smpd_err_printf("unable to add the command parameter: %s=%s\n", param, value);
		return SMPD_FAIL;
	    }
	    cmd_ptr->cmd[cmd_length] = ' ';
	    len--;
	    str++;
	}
    }

    result = smpd_add_string_arg(&str, &len, param, value);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the command parameter: %s=%s\n", param, value);
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

int smpd_add_command_int_arg(smpd_command_t *cmd_ptr, char *param, int value)
{
    char *str;
    int len;
    int result;
    int cmd_length;

    cmd_length = (int)strlen(cmd_ptr->cmd);
    if (cmd_length > SMPD_MAX_CMD_LENGTH)
    {
	smpd_err_printf("invalid cmd string length: %d\n", cmd_length);
	return SMPD_FAIL;
    }

    len = (int)(SMPD_MAX_CMD_LENGTH - cmd_length);
    str = &cmd_ptr->cmd[cmd_length];

    /* make sure there is a space after the last parameter in the command */
    if (cmd_length > 0)
    {
	if (cmd_ptr->cmd[cmd_length-1] != ' ')
	{
	    if (len < 2)
	    {
		smpd_err_printf("unable to add the command parameter: %s=%d\n", param, value);
		return SMPD_FAIL;
	    }
	    cmd_ptr->cmd[cmd_length] = ' ';
	    len--;
	    str++;
	}
    }

    result = smpd_add_int_arg(&str, &len, param, value);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the command parameter: %s=%d\n", param, value);
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

int smpd_forward_command(smpd_context_t *src, smpd_context_t *dest)
{
    int result;
    smpd_command_t *cmd;

    smpd_enter_fn("smpd_forward_command");

    result = smpd_create_command_copy(&src->read_cmd, &cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a copy of the command to forward.\n");
	smpd_exit_fn("smpd_forward_command");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("posting write of forwarded command: \"%s\"\n", cmd->cmd);
    result = smpd_post_write_command(dest, cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of a forwarded command.\n");
	smpd_exit_fn("smpd_forward_command");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_forward_command");
    return SMPD_SUCCESS;
}

int smpd_post_read_command(smpd_context_t *context)
{
    int result;

    smpd_enter_fn("smpd_post_read_command");

    /* post a read for the next command header */
    smpd_dbg_printf("posting a read for a command header on the %s context, sock %d\n", smpd_get_context_str(context), MPIDU_Sock_get_sock_id(context->sock));
    context->read_state = SMPD_READING_CMD_HEADER;
    context->read_cmd.state = SMPD_CMD_READING_HDR;
    result = MPIDU_Sock_post_read(context->sock, context->read_cmd.cmd_hdr_str, SMPD_CMD_HDR_LENGTH, SMPD_CMD_HDR_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the next command header,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_post_read_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_post_read_command");
    return SMPD_SUCCESS;
}

int smpd_post_write_command(smpd_context_t *context, smpd_command_t *cmd)
{
    int result;
    smpd_command_t *iter;

    smpd_enter_fn("smpd_post_write_command");

    smpd_package_command(cmd);
    /*smpd_dbg_printf("command after packaging: \"%s\"\n", cmd->cmd);*/
    cmd->next = NULL;
    cmd->state = SMPD_CMD_WRITING_CMD;
    context->write_state = SMPD_WRITING_CMD;

    if (!context->write_list)
    {
	context->write_list = cmd;
    }
    else
    {
	smpd_dbg_printf("enqueueing write at the end of the list.\n");
	iter = context->write_list;
	while (iter->next)
	    iter = iter->next;
	iter->next = cmd;
	smpd_exit_fn("smpd_post_write_command");
	return SMPD_SUCCESS;
    }

    cmd->iov[0].MPID_IOV_BUF = cmd->cmd_hdr_str;
    cmd->iov[0].MPID_IOV_LEN = SMPD_CMD_HDR_LENGTH;
    cmd->iov[1].MPID_IOV_BUF = cmd->cmd;
    cmd->iov[1].MPID_IOV_LEN = cmd->length;
    /*smpd_dbg_printf("command at this moment: \"%s\"\n", cmd->cmd);*/
    smpd_dbg_printf("smpd_post_write_command on the %s context sock %d: %d bytes for command: \"%s\"\n",
	smpd_get_context_str(context), MPIDU_Sock_get_sock_id(context->sock),
	cmd->iov[0].MPID_IOV_LEN + cmd->iov[1].MPID_IOV_LEN,
	cmd->cmd);
    result = MPIDU_Sock_post_writev(context->sock, cmd->iov, 2, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a write for the next command,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_post_write_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_post_write_command");
    return SMPD_SUCCESS;
}

int smpd_package_command(smpd_command_t *cmd)
{
    int length;

    smpd_enter_fn("smpd_package_command");

    /* create the command header - for now it is simply the length of the command string */
    length = (int)strlen(cmd->cmd) + 1;
    if (length > SMPD_MAX_CMD_LENGTH)
    {
	smpd_err_printf("unable to package invalid command of length %d\n", length);
	smpd_exit_fn("smpd_package_command");
	return SMPD_FAIL;
    }
    snprintf(cmd->cmd_hdr_str, SMPD_CMD_HDR_LENGTH, "%d", length);
    cmd->length = length;

    smpd_exit_fn("smpd_package_command");
    return SMPD_SUCCESS;
}

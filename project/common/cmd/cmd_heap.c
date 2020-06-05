/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cmd_util.h"

extern void heap_get_space(uint8_t **start, uint8_t **end, uint8_t **current);

enum cmd_status cmd_heap_space_exec(char *cmd)
{
	uint8_t *start, *end, *current;

	heap_get_space(&start, &end, &current);
	cmd_write_respond(CMD_STATUS_OK, "heap total %u, use %u, free %u, [%p, %p, %p)",
	                  end - start, current - start, end - current, start, current, end);
	return CMD_STATUS_ACKED;
}

#ifdef __CONFIG_MALLOC_TRACE
extern uint32_t wrap_malloc_heap_info(int verbose);

enum cmd_status cmd_heap_info_exec(char *cmd)
{
	uint32_t used;
	uint8_t *start, *end, *current;

	used = wrap_malloc_heap_info(cmd_atoi(cmd));
	heap_get_space(&start, &end, &current);
	cmd_write_respond(CMD_STATUS_OK, "heap total %u (%u KB), "
	                  "use %u (%u KB), free %u (%u KB)",
	                  end - start, (end - start) / 1024,
	                  used, used / 1024,
	                  end - start - used, (end - start - used) / 1024);
	return CMD_STATUS_ACKED;
}
#endif

static enum cmd_status cmd_heap_help_exec(char *cmd);

static const struct cmd_data g_heap_cmds[] = {
	{ "space",	cmd_heap_space_exec, CMD_DESC("get the heap usage") },
#ifdef __CONFIG_MALLOC_TRACE
	{ "info",	cmd_heap_info_exec, CMD_DESC("info <0|1>, get the heap usage details") },
#endif
	{ "help",	cmd_heap_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_heap_help_exec(char *cmd)
{
	return cmd_help_exec(g_heap_cmds, cmd_nitems(g_heap_cmds), 8);
}

enum cmd_status cmd_heap_exec(char *cmd)
{
	return cmd_exec(cmd, g_heap_cmds, cmd_nitems(g_heap_cmds));
}

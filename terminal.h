/*
 * console.h
 *
 *  Created on: 4 sty 2020
 *      Author: Tojwek
 */

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <stdint.h>

#define TERMINAL_VT100_SEQUENCE_MAX_LEN	32

#define TERMINAL_VT100_CURSOR_UP			"\e[A"
#define TERMINAL_VT100_CURSOR_DOWN			"\e[B"
#define TERMINAL_VT100_CURSOR_FORWARD    	"\e[C"
#define TERMINAL_VT100_CURSOR_BACKWARD		"\e[D"
#define TERMINAL_VT100_CURSOR_DELETE		"\e[3~"
#define TERMINAL_VT100_CURSOR_HOME			"\e[1~"
#define TERMINAL_VT100_CURSOR_END			"\e[4~"

#define TERMINAL_VT100_ERASE_END_OF_LINE	"\e[K"
#define TERMINAL_VT100_ERASE_LINE			"\e[2K"

#define TERMINAL_ASCII_DELETE				127
#define TERMINAL_ASCII_END_OF_TEXT			3

typedef struct _Terminal_t Terminal_t;
typedef void (*Terminal_On_Write_Request_t)(Terminal_t *p_instance, char *p_data, uint32_t data_len);
typedef void (*Terminal_On_Line_Read_t)(Terminal_t *p_instance, char *p_line, uint32_t line_len);

typedef struct _Terminal_History_t
{
	char *p_entries;
	uint32_t max_entries;
	uint32_t entry_max_len;
	int32_t number_of_entries;
	uint32_t last_entry_idx;
	int32_t displayed_entry_no;
} Terminal_History_t;

typedef struct _Terminal_t
{
	char *p_line_buffer;
	uint32_t max_line_len;
	uint32_t current_line_len;
	uint32_t cursor_pos;
	char received_vt100_sequence[TERMINAL_VT100_SEQUENCE_MAX_LEN + 1];
	uint8_t received_vt100_sequence_len;
	char vt100_sequence_to_send[TERMINAL_VT100_SEQUENCE_MAX_LEN];
	Terminal_On_Write_Request_t on_write_request;
	Terminal_On_Line_Read_t on_line_read;
	Terminal_History_t history;
	char *p_prompt;
	uint32_t prompt_len;
} Terminal_t;

void terminal_init(Terminal_t *p_terminal,
		           char *p_line_buffer,
				   uint32_t max_line_len,
				   char *p_history_entries,
				   uint32_t history_max_entries,
				   Terminal_On_Write_Request_t on_write_request,
				   Terminal_On_Line_Read_t on_line_read);

void terminal_feed(Terminal_t *p_terminal, char byte);

void terminal_set_prompt(Terminal_t *p_terminal, char *p_prompt, uint32_t prompt_len);

uint32_t terminal_get_number_of_history_entries(Terminal_t *p_terminal);

char *terminal_get_history_entry(Terminal_t *p_terminal, int32_t entry_no);

void terminal_clear_history(Terminal_t *p_terminal);

#endif /* TERMINAL_H_ */

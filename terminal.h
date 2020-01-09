/*
 * console.h
 *
 *  Created on: 4 sty 2020
 *      Author: Tojwek
 */

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <stdbool.h>

#define TERMINAL_VT100_SEQUENCE_MAX_LEN	32

#define TERMINAL_VT100_CURSOR_UP            "\e[A"
#define TERMINAL_VT100_CURSOR_DOWN          "\e[B"
#define TERMINAL_VT100_CURSOR_FORWARD       "\e[C"
#define TERMINAL_VT100_CURSOR_BACKWARD      "\e[D"
#define TERMINAL_VT100_CURSOR_DELETE        "\e[3~"
#define TERMINAL_VT100_CURSOR_HOME          "\e[1~"
#define TERMINAL_VT100_CURSOR_END           "\e[4~"

#define TERMINAL_VT100_ERASE_END_OF_LINE    "\e[K"
#define TERMINAL_VT100_ERASE_LINE           "\e[2K"

#define TERMINAL_ASCII_DELETE               127
#define TERMINAL_ASCII_END_OF_TEXT          3

typedef struct _Terminal_t Terminal_t;
typedef int (*Terminal_On_Write_Request_t)(Terminal_t *p_instance, char *p_data, int data_len);
typedef void (*Terminal_On_Line_Read_t)(Terminal_t *p_instance, char *p_line, int line_len);
typedef char *(*Terminal_On_Suggestion_Request_t)(Terminal_t *p_instance, char *p_line, int line_len);

typedef struct _Terminal_History_t
{
    char *p_entries;
    int max_entries;
    int entry_max_len;
    int number_of_entries;
    int last_entry_idx;
    int displayed_entry_no;
} Terminal_History_t;

typedef struct _Terminal_t
{
    char *p_line_buffer;
    int max_line_len;
    int current_line_len;
    int cursor_pos;
    char *p_write_buffer;
    int write_buffer_size;
    char received_vt100_sequence[TERMINAL_VT100_SEQUENCE_MAX_LEN + 1];
    int received_vt100_sequence_len;
    Terminal_On_Write_Request_t on_write_request;
    Terminal_On_Line_Read_t on_line_read;
    Terminal_On_Suggestion_Request_t on_suggestion_request;
    Terminal_History_t history;
    char *p_prompt;
    bool echo_disabled;
} Terminal_t;

void terminal_init(Terminal_t *p_terminal,
                   char *p_line_buffer,
                   int max_line_len,
                   char *p_write_buffer,
                   int write_buffer_size,
                   char *p_history_entries,
                   int history_max_entries,
                   Terminal_On_Write_Request_t on_write_request,
                   Terminal_On_Line_Read_t on_line_read,
                   Terminal_On_Suggestion_Request_t on_suggestion_request);

void terminal_feed(Terminal_t *p_terminal, char byte);

int terminal_printf(Terminal_t *p_terminal, const char *p_format, ...);

void terminal_set_prompt(Terminal_t *p_terminal, char *p_prompt);

void terminal_set_echo_disabled(Terminal_t *p_terminal, bool disabled);

int terminal_get_number_of_history_entries(Terminal_t *p_terminal);

char *terminal_get_history_entry(Terminal_t *p_terminal, int entry_no);

void terminal_clear_history(Terminal_t *p_terminal);

#endif /* TERMINAL_H_ */

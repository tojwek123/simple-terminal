/*
 * console.c
 *
 *  Created on: 4 sty 2020
 *      Author: Tojwek
 */

#include "terminal.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static void _history_init(Terminal_History_t *p_history, char *p_entries, uint32_t max_entries, uint32_t entry_max_len)
{
	p_history->p_entries = p_entries;
	p_history->max_entries = max_entries;
	p_history->entry_max_len = entry_max_len;
	p_history->number_of_entries = 0U;
	p_history->last_entry_idx = 0U;
	p_history->displayed_entry_no = -1;
}

static void _history_add_entry(Terminal_History_t *p_history, char *p_entry)
{
	uint32_t new_entry_idx;

	if (0 == p_history->number_of_entries)
	{
		new_entry_idx = 0U;
	}
	else
	{
		new_entry_idx = (p_history->last_entry_idx + 1U) % p_history->max_entries;
	}

	if (p_history->number_of_entries < p_history->max_entries)
	{
		p_history->number_of_entries++;
	}

	strcpy(&p_history->p_entries[new_entry_idx * (p_history->entry_max_len + 1U)], p_entry);
	p_history->last_entry_idx = new_entry_idx;
}

static char *_history_pick_older_entry(Terminal_History_t *p_history)
{
	char *p_entry = NULL;
	int32_t entry_to_display_idx = -1;

	if (p_history->number_of_entries > 0)
	{
		if (p_history->displayed_entry_no + 1 < p_history->number_of_entries)
		{
			p_history->displayed_entry_no++;
		}

		if (p_history->displayed_entry_no < p_history->number_of_entries)
		{
			if ((int32_t)p_history->last_entry_idx - p_history->displayed_entry_no < 0)
			{
				entry_to_display_idx = p_history->max_entries - (p_history->displayed_entry_no - p_history->last_entry_idx);
			}
			else
			{
				entry_to_display_idx = p_history->last_entry_idx - p_history->displayed_entry_no;
			}
		}

		if (-1 != entry_to_display_idx)
		{
			p_entry = &p_history->p_entries[entry_to_display_idx * (p_history->entry_max_len + 1U)];
		}
	}

	return p_entry;
}

static char *_history_pick_newer_entry(Terminal_History_t *p_history)
{
	char *p_entry = NULL;
	int32_t entry_to_display_idx = -1;

	if (p_history->displayed_entry_no >= 0)
	{
		p_history->displayed_entry_no--;
	}

	if (p_history->displayed_entry_no >= 0)
	{
		if ((int32_t)p_history->last_entry_idx - p_history->displayed_entry_no < 0)
		{
			entry_to_display_idx = p_history->max_entries - (p_history->displayed_entry_no - p_history->last_entry_idx);
		}
		else
		{
			entry_to_display_idx = p_history->last_entry_idx - p_history->displayed_entry_no;
		}
	}

	if (-1 != entry_to_display_idx)
	{
		p_entry = &p_history->p_entries[entry_to_display_idx * (p_history->entry_max_len + 1U)];
	}

	return p_entry;
}

static void _history_reset_displayed_entry_no(Terminal_History_t *p_history)
{
	p_history->displayed_entry_no = -1;
}

void _show_prompt(Terminal_t *p_terminal)
{
	if (p_terminal->prompt_len > 0U)
	{
		p_terminal->on_write_request(p_terminal, p_terminal->p_prompt, p_terminal->prompt_len);
	}
}

void terminal_init(Terminal_t *p_terminal,
		           char *p_line_buffer,
				   uint32_t max_line_len,
				   char *p_history_entries,
				   uint32_t history_max_entries,
				   Terminal_On_Write_Request_t on_write_request,
				   Terminal_On_Line_Read_t on_line_read)
{
	p_terminal->p_line_buffer = p_line_buffer;
	p_terminal->max_line_len = max_line_len;
	p_terminal->current_line_len = 0U;
	p_terminal->cursor_pos = 0U;
	p_terminal->received_vt100_sequence_len = 0U;
	p_terminal->on_write_request = on_write_request;
	p_terminal->on_line_read = on_line_read;
	p_terminal->prompt_len = 0U;

	_history_init(&p_terminal->history, p_history_entries, history_max_entries, max_line_len);
}

void terminal_feed(Terminal_t *p_terminal, char byte)
{
	if ('\e' == byte)
	{
		/* Escape character occurred - it's the start of VT100 sequence */
		p_terminal->received_vt100_sequence[0] = byte;
		p_terminal->received_vt100_sequence[1] = '\0';
		p_terminal->received_vt100_sequence_len = 1U;
	}
	else if ('\r' == byte)
	{
		p_terminal->p_line_buffer[p_terminal->current_line_len] = '\0';

		if (p_terminal->current_line_len > 0)
		{
			_history_add_entry(&p_terminal->history, p_terminal->p_line_buffer);
		}
		_history_reset_displayed_entry_no(&p_terminal->history);

		p_terminal->on_line_read(p_terminal, p_terminal->p_line_buffer, p_terminal->current_line_len);

		p_terminal->current_line_len = 0U;
		p_terminal->cursor_pos = 0U;
		p_terminal->received_vt100_sequence_len = 0U;
		p_terminal->on_write_request(p_terminal, "\r\n", 2U);
		_show_prompt(p_terminal);
	}
	else if (TERMINAL_ASCII_END_OF_TEXT == byte)
	{
		p_terminal->current_line_len = 0U;
		p_terminal->cursor_pos = 0U;
		p_terminal->received_vt100_sequence_len = 0U;
		p_terminal->on_write_request(p_terminal, "\r\n", 2U);

		if (p_terminal->prompt_len > 0U)
		{
			p_terminal->on_write_request(p_terminal, p_terminal->p_prompt, p_terminal->prompt_len);
		}

		_history_reset_displayed_entry_no(&p_terminal->history);
	}
	else if (p_terminal->received_vt100_sequence_len > 0U)
	{
		/* VT100 sequence is started, so let's continue */
		p_terminal->received_vt100_sequence[p_terminal->received_vt100_sequence_len] = byte;
		p_terminal->received_vt100_sequence[p_terminal->received_vt100_sequence_len + 1] = '\0';
		p_terminal->received_vt100_sequence_len++;

		bool got_sequence = false;

		if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_UP))
		{
			got_sequence = true;

			char *p_history_entry = _history_pick_older_entry(&p_terminal->history);

			if (NULL != p_history_entry)
			{
				p_terminal->on_write_request(p_terminal, TERMINAL_VT100_ERASE_LINE "\r", sizeof(TERMINAL_VT100_ERASE_LINE));
				_show_prompt(p_terminal);

				p_terminal->current_line_len = strlen(p_history_entry);
				strcpy(p_terminal->p_line_buffer, p_history_entry);
				p_terminal->cursor_pos = p_terminal->current_line_len;

				p_terminal->on_write_request(p_terminal, p_terminal->p_line_buffer, p_terminal->current_line_len);
			}
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_DOWN))
		{
			got_sequence = true;

			char *p_history_entry = _history_pick_newer_entry(&p_terminal->history);

			p_terminal->on_write_request(p_terminal, TERMINAL_VT100_ERASE_LINE "\r", sizeof(TERMINAL_VT100_ERASE_LINE));
			_show_prompt(p_terminal);

			if (NULL != p_history_entry)
			{
				p_terminal->current_line_len = strlen(p_history_entry);
				strcpy(p_terminal->p_line_buffer, p_history_entry);
				p_terminal->cursor_pos = p_terminal->current_line_len;

				p_terminal->on_write_request(p_terminal, p_terminal->p_line_buffer, p_terminal->current_line_len);
			}
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_FORWARD))
		{
			got_sequence = true;

			if (p_terminal->cursor_pos < p_terminal->current_line_len)
			{
				p_terminal->cursor_pos++;
				p_terminal->on_write_request(p_terminal, TERMINAL_VT100_CURSOR_FORWARD, sizeof(TERMINAL_VT100_CURSOR_FORWARD) - 1);
			}
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_BACKWARD))
		{
			got_sequence = true;

			if (p_terminal->cursor_pos > 0)
			{
				p_terminal->cursor_pos--;
				p_terminal->on_write_request(p_terminal, TERMINAL_VT100_CURSOR_BACKWARD, sizeof(TERMINAL_VT100_CURSOR_BACKWARD) - 1);
			}
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_DELETE))
		{
			got_sequence = true;

			if (p_terminal->cursor_pos < p_terminal->current_line_len)
			{
				p_terminal->on_write_request(p_terminal, TERMINAL_VT100_ERASE_END_OF_LINE, sizeof(TERMINAL_VT100_ERASE_END_OF_LINE) - 1);

				if (p_terminal->cursor_pos < p_terminal->current_line_len - 1)
				{

					int sequence_length = sprintf(p_terminal->vt100_sequence_to_send, "\e[%dD", p_terminal->current_line_len - p_terminal->cursor_pos - 1);

					memmove(&p_terminal->p_line_buffer[p_terminal->cursor_pos], &p_terminal->p_line_buffer[p_terminal->cursor_pos + 1], p_terminal->current_line_len - p_terminal->cursor_pos - 1);
					p_terminal->on_write_request(p_terminal, &p_terminal->p_line_buffer[p_terminal->cursor_pos], p_terminal->current_line_len - p_terminal->cursor_pos - 1);
					p_terminal->on_write_request(p_terminal, p_terminal->vt100_sequence_to_send, sequence_length);
				}

				p_terminal->current_line_len--;
			}
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_HOME))
		{
			got_sequence = true;

			if (p_terminal->cursor_pos > 0U)
			{
				int sequence_length = sprintf(p_terminal->vt100_sequence_to_send, "\e[%dD", p_terminal->cursor_pos);
				p_terminal->on_write_request(p_terminal, p_terminal->vt100_sequence_to_send, sequence_length);
				p_terminal->cursor_pos = 0U;
			}
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_END))
		{
			got_sequence = true;

			if (p_terminal->cursor_pos < p_terminal->current_line_len)
			{
				int sequence_length = sprintf(p_terminal->vt100_sequence_to_send, "\e[%dC", p_terminal->current_line_len - p_terminal->cursor_pos);
				p_terminal->on_write_request(p_terminal, p_terminal->vt100_sequence_to_send, sequence_length);
				p_terminal->cursor_pos = p_terminal->current_line_len;
			}
		}
		else if ('~' == p_terminal->received_vt100_sequence[p_terminal->received_vt100_sequence_len - 1])
		{
			got_sequence = true;
		}

		if (got_sequence)
		{
			p_terminal->received_vt100_sequence_len = 0U;
		}
	}
	else if ('\t' == byte)
	{

	}
	else if (TERMINAL_ASCII_DELETE == byte)
	{
		/* Backspace yields this */
		if (p_terminal->cursor_pos > 0)
		{
			p_terminal->on_write_request(p_terminal, &byte, 1U);

			if (p_terminal->cursor_pos < p_terminal->current_line_len)
			{
				memmove(&p_terminal->p_line_buffer[p_terminal->cursor_pos - 1], &p_terminal->p_line_buffer[p_terminal->cursor_pos], p_terminal->current_line_len - p_terminal->cursor_pos);

				/* Prepare VT100 sequence to move back the cursor */
				int sequence_length = sprintf(p_terminal->vt100_sequence_to_send, "\e[%dD", p_terminal->current_line_len - p_terminal->cursor_pos);

				/* Update line part after the cursor and move it to the place where it should be */
				p_terminal->on_write_request(p_terminal, TERMINAL_VT100_ERASE_END_OF_LINE, sizeof(TERMINAL_VT100_ERASE_END_OF_LINE) - 1);
				p_terminal->on_write_request(p_terminal, &p_terminal->p_line_buffer[p_terminal->cursor_pos - 1], p_terminal->current_line_len - p_terminal->cursor_pos);
				p_terminal->on_write_request(p_terminal, p_terminal->vt100_sequence_to_send, sequence_length);
			}

			p_terminal->current_line_len--;
			p_terminal->cursor_pos--;
		}
	}
	else
	{
		if (p_terminal->current_line_len < p_terminal->max_line_len)
		{
			if (p_terminal->cursor_pos == p_terminal->current_line_len)
			{
				/* Cursor is at the end of line */
				p_terminal->p_line_buffer[p_terminal->cursor_pos] = byte;
				p_terminal->on_write_request(p_terminal, &byte, 1U);
			}
			else
			{
				/* Editing a line when cursor is in the middle */
				memmove(&p_terminal->p_line_buffer[p_terminal->cursor_pos + 1], &p_terminal->p_line_buffer[p_terminal->cursor_pos], p_terminal->current_line_len - p_terminal->cursor_pos);
				p_terminal->p_line_buffer[p_terminal->cursor_pos] = byte;

				/* Prepare VT100 sequence to move back the cursor */
				int sequence_length = sprintf(p_terminal->vt100_sequence_to_send, "\e[%dD", p_terminal->current_line_len - p_terminal->cursor_pos);

				/* Update line part after the cursor and move it to the place where it should be */
				p_terminal->on_write_request(p_terminal, &p_terminal->p_line_buffer[p_terminal->cursor_pos], p_terminal->current_line_len - p_terminal->cursor_pos + 1);
				p_terminal->on_write_request(p_terminal, p_terminal->vt100_sequence_to_send, sequence_length);
			}
			p_terminal->current_line_len++;
			p_terminal->cursor_pos++;
		}
	}
}

void terminal_set_prompt(Terminal_t *p_terminal, char *p_prompt, uint32_t prompt_len)
{
	p_terminal->p_prompt = p_prompt;
	p_terminal->prompt_len = prompt_len;

	p_terminal->on_write_request(p_terminal, TERMINAL_VT100_ERASE_LINE "\r", sizeof(TERMINAL_VT100_ERASE_LINE));
	_show_prompt(p_terminal);

	if (p_terminal->current_line_len > 0U)
	{
		p_terminal->on_write_request(p_terminal, p_terminal->p_line_buffer, p_terminal->current_line_len);
	}
	if (p_terminal->cursor_pos < p_terminal->current_line_len)
	{
		int sequence_length = sprintf(p_terminal->vt100_sequence_to_send, "\e[%dD", p_terminal->current_line_len - p_terminal->cursor_pos);
		p_terminal->on_write_request(p_terminal, p_terminal->vt100_sequence_to_send, sequence_length);
	}
}

uint32_t terminal_get_number_of_history_entries(Terminal_t *p_terminal)
{
	return p_terminal->history.number_of_entries;
}

char *terminal_get_history_entry(Terminal_t *p_terminal, int32_t entry_no)
{
	char *p_entry = NULL;

	if (entry_no < p_terminal->history.number_of_entries)
	{
		uint32_t entry_idx;

		if ((int32_t)p_terminal->history.last_entry_idx - entry_no < 0)
		{
			entry_idx = p_terminal->history.max_entries - (entry_no - p_terminal->history.last_entry_idx);
		}
		else
		{
			entry_idx = p_terminal->history.last_entry_idx - entry_no;
		}

		p_entry = &p_terminal->history.p_entries[entry_idx * (p_terminal->history.entry_max_len + 1U)];
	}
	return p_entry;
}


void terminal_clear_history(Terminal_t *p_terminal)
{
	p_terminal->history.number_of_entries = 0U;
	p_terminal->history.displayed_entry_no = -1;
}

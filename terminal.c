/*
 * console.c
 *
 *  Created on: 4 sty 2020
 *      Author: Tojwek
 */

#include "terminal.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static void _history_init(Terminal_History_t *p_history, char *p_entries, int max_entries, int entry_max_len)
{
	p_history->p_entries = p_entries;
	p_history->max_entries = max_entries;
	p_history->entry_max_len = entry_max_len;
	p_history->number_of_entries = 0;
	p_history->last_entry_idx = 0;
	p_history->displayed_entry_no = -1;
}

static void _history_add_entry(Terminal_History_t *p_history, char *p_entry)
{
	int new_entry_idx;

	if (0 == p_history->number_of_entries)
	{
		new_entry_idx = 0;
	}
	else
	{
		new_entry_idx = (p_history->last_entry_idx + 1) % p_history->max_entries;
	}

	if (p_history->number_of_entries < p_history->max_entries)
	{
		p_history->number_of_entries++;
	}

	strcpy(&p_history->p_entries[new_entry_idx * (p_history->entry_max_len + 1)], p_entry);
	p_history->last_entry_idx = new_entry_idx;
}

static int _history_get_entry_idx_by_entry_no(Terminal_History_t *p_history, int entry_no)
{
	int entry_idx = -1;

	if (entry_no < p_history->number_of_entries)
	{
		if (p_history->last_entry_idx - entry_no < 0)
		{
			entry_idx = p_history->max_entries - (entry_no - p_history->last_entry_idx);
		}
		else
		{
			entry_idx = p_history->last_entry_idx - entry_no;
		}
	}
	return entry_idx;
}

static char *_history_pick_older_entry(Terminal_History_t *p_history)
{
	char *p_entry = NULL;
	int entry_to_display_idx = -1;

	if (p_history->number_of_entries > 0)
	{
		if (p_history->displayed_entry_no + 1 < p_history->number_of_entries)
		{
			p_history->displayed_entry_no++;
		}

		entry_to_display_idx = _history_get_entry_idx_by_entry_no(p_history, p_history->displayed_entry_no);

		if (-1 != entry_to_display_idx)
		{
			p_entry = &p_history->p_entries[entry_to_display_idx * (p_history->entry_max_len + 1)];
		}
	}

	return p_entry;
}

static char *_history_pick_newer_entry(Terminal_History_t *p_history)
{
	char *p_entry = NULL;
	int entry_to_display_idx = -1;

	if (p_history->displayed_entry_no >= 0)
	{
		p_history->displayed_entry_no--;
	}

	if (p_history->displayed_entry_no >= 0)
	{
		entry_to_display_idx = _history_get_entry_idx_by_entry_no(p_history, p_history->displayed_entry_no);
	}

	if (-1 != entry_to_display_idx)
	{
		p_entry = &p_history->p_entries[entry_to_display_idx * (p_history->entry_max_len + 1)];
	}

	return p_entry;
}

static void _history_reset_displayed_entry_no(Terminal_History_t *p_history)
{
	p_history->displayed_entry_no = -1;
}

void terminal_init(Terminal_t *p_terminal,
		           char *p_line_buffer,
				   int max_line_len,
				   char *p_write_buffer,
				   int write_buffer_size,
				   char *p_history_entries,
				   int history_max_entries,
				   Terminal_On_Write_Request_t on_write_request,
				   Terminal_On_Line_Read_t on_line_read,
				   Terminal_On_Suggestion_Request_t on_suggestion_request)
{
	p_terminal->p_line_buffer = p_line_buffer;
	p_terminal->p_line_buffer[0] = '\0';
	p_terminal->max_line_len = max_line_len;
	p_terminal->current_line_len = 0;
	p_terminal->cursor_pos = 0;
	p_terminal->p_write_buffer = p_write_buffer;
	p_terminal->write_buffer_size = write_buffer_size;
	p_terminal->received_vt100_sequence_len = 0;
	p_terminal->on_write_request = on_write_request;
	p_terminal->on_line_read = on_line_read;
	p_terminal->on_suggestion_request = on_suggestion_request;
	p_terminal->p_prompt = "";
	p_terminal->echo_disabled = false;

	_history_init(&p_terminal->history, p_history_entries, history_max_entries, max_line_len);
}

void terminal_feed(Terminal_t *p_terminal, char byte)
{
	if ('\e' == byte)
	{
		/* Escape character occurred - it's the start of VT100 sequence */
		p_terminal->received_vt100_sequence[0] = byte;
		p_terminal->received_vt100_sequence[1] = '\0';
		p_terminal->received_vt100_sequence_len = 1;
	}
	else if ('\r' == byte)
	{
		/* User pressed ENTER - there is a new line to process */
		if (p_terminal->current_line_len > 0)
		{
			_history_add_entry(&p_terminal->history, p_terminal->p_line_buffer);
		}
		_history_reset_displayed_entry_no(&p_terminal->history);
		terminal_printf(p_terminal, "\r\n");

		/* Fire a callback to notify that a line was read */
		p_terminal->on_line_read(p_terminal, p_terminal->p_line_buffer, p_terminal->current_line_len);

		/* Reset some variables, so next line can be read again */
		p_terminal->p_line_buffer[0] = '\0';
		p_terminal->current_line_len = 0;
		p_terminal->cursor_pos = 0;
		p_terminal->received_vt100_sequence_len = 0;
		terminal_printf(p_terminal, "%s", p_terminal->p_prompt);
	}
	else if (TERMINAL_ASCII_END_OF_TEXT == byte)
	{
		/* User pressed CTRL+C - ignore line */
		p_terminal->p_line_buffer[0] = '\0';
		p_terminal->current_line_len = 0;
		p_terminal->cursor_pos = 0;
		p_terminal->received_vt100_sequence_len = 0;
		terminal_printf(p_terminal, "\r\n%s", p_terminal->p_prompt);

		_history_reset_displayed_entry_no(&p_terminal->history);
	}
	else if (p_terminal->received_vt100_sequence_len > 0)
	{
		/* VT100 sequence is started, so let's continue */
		p_terminal->received_vt100_sequence[p_terminal->received_vt100_sequence_len] = byte;
		p_terminal->received_vt100_sequence[p_terminal->received_vt100_sequence_len + 1] = '\0';
		p_terminal->received_vt100_sequence_len++;

		bool got_sequence = false;

		/* Check if we already have received valid VT100 sequence */
		if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_UP))
		{
			/* User pressed ARROW UP - show older history entry */
			char *p_history_entry = _history_pick_older_entry(&p_terminal->history);

			if (NULL != p_history_entry)
			{
				terminal_printf(p_terminal, TERMINAL_VT100_ERASE_LINE "\r%s", p_terminal->p_prompt);

				p_terminal->current_line_len = strlen(p_history_entry);
				strcpy(p_terminal->p_line_buffer, p_history_entry);
				p_terminal->cursor_pos = p_terminal->current_line_len;

				terminal_printf(p_terminal, p_terminal->p_line_buffer);
			}
			got_sequence = true;
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_DOWN))
		{
			/* User pressed ARROW DOWN - show newer history entry */
			char *p_history_entry = _history_pick_newer_entry(&p_terminal->history);

			terminal_printf(p_terminal, TERMINAL_VT100_ERASE_LINE "\r%s", p_terminal->p_prompt);

			if (NULL == p_history_entry)
			{
				p_terminal->current_line_len = 0;
				p_terminal->cursor_pos = 0;
			}
			else
			{
				p_terminal->current_line_len = strlen(p_history_entry);
				strcpy(p_terminal->p_line_buffer, p_history_entry);
				p_terminal->cursor_pos = p_terminal->current_line_len;

				terminal_printf(p_terminal, p_terminal->p_line_buffer);
			}
			got_sequence = true;
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_FORWARD))
		{
			/* User pressed RIGHT ARROW - move cursor forward */
			if (p_terminal->cursor_pos < p_terminal->current_line_len)
			{
				p_terminal->cursor_pos++;
				terminal_printf(p_terminal, TERMINAL_VT100_CURSOR_FORWARD);
			}
			got_sequence = true;
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_BACKWARD))
		{
			/* User pressed LEFT ARROW - move cursor backward */
			if (p_terminal->cursor_pos > 0)
			{
				p_terminal->cursor_pos--;
				terminal_printf(p_terminal, TERMINAL_VT100_CURSOR_BACKWARD);
			}
			got_sequence = true;
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_DELETE))
		{
			/* User pressed DELETE - remove character in front of cursor */
			if (p_terminal->cursor_pos < p_terminal->current_line_len)
			{
				terminal_printf(p_terminal, TERMINAL_VT100_ERASE_END_OF_LINE);

				if (p_terminal->cursor_pos < p_terminal->current_line_len - 1)
				{
					memmove(&p_terminal->p_line_buffer[p_terminal->cursor_pos], &p_terminal->p_line_buffer[p_terminal->cursor_pos + 1], p_terminal->current_line_len - p_terminal->cursor_pos);

					terminal_printf(p_terminal, &p_terminal->p_line_buffer[p_terminal->cursor_pos]);
					terminal_printf(p_terminal, "\e[%dD", p_terminal->current_line_len - p_terminal->cursor_pos - 1);
				}

				p_terminal->current_line_len--;
			}
			got_sequence = true;
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_HOME))
		{
			/* User pressed HOME - move cursor to the beginning of the line */
			if (p_terminal->cursor_pos > 0)
			{
				terminal_printf(p_terminal, "\e[%dD", p_terminal->cursor_pos);
				p_terminal->cursor_pos = 0;
			}
			got_sequence = true;
		}
		else if (0 == strcmp(p_terminal->received_vt100_sequence, TERMINAL_VT100_CURSOR_END))
		{
			/* User pressed END - move cursor to the end of the line */
			if (p_terminal->cursor_pos < p_terminal->current_line_len)
			{
				terminal_printf(p_terminal, "\e[%dC", p_terminal->current_line_len - p_terminal->cursor_pos);
				p_terminal->cursor_pos = p_terminal->current_line_len;
			}
			got_sequence = true;
		}
		else if ('~' == p_terminal->received_vt100_sequence[p_terminal->received_vt100_sequence_len - 1])
		{
			/* User pressed special key like F1-F12, INSERT and so on - ignore it */
			got_sequence = true;
		}

		if (got_sequence)
		{
			p_terminal->received_vt100_sequence_len = 0;
		}
	}
	else if ('\t' == byte)
	{
		/* User pressed TAB - show suggestion */
		if (NULL != p_terminal->on_suggestion_request)
		{
			char *p_suggestion = p_terminal->on_suggestion_request(p_terminal, p_terminal->p_line_buffer, p_terminal->current_line_len);

			if (NULL != p_suggestion)
			{
				strcpy(p_terminal->p_line_buffer, p_suggestion);
				p_terminal->current_line_len = strlen(p_suggestion);
				p_terminal->cursor_pos = p_terminal->current_line_len;

				terminal_printf(p_terminal, TERMINAL_VT100_ERASE_LINE "\r%s", p_terminal->p_prompt);
				terminal_printf(p_terminal, p_terminal->p_line_buffer);
			}
		}
	}
	else if (TERMINAL_ASCII_DELETE == byte)
	{
		/* User pressed BACKSPACE - delete character behind cursor*/
		if (p_terminal->cursor_pos > 0)
		{
			terminal_printf(p_terminal, "%c", byte);

			if (p_terminal->cursor_pos < p_terminal->current_line_len)
			{
				memmove(&p_terminal->p_line_buffer[p_terminal->cursor_pos - 1], &p_terminal->p_line_buffer[p_terminal->cursor_pos], p_terminal->current_line_len - p_terminal->cursor_pos);
				p_terminal->p_line_buffer[p_terminal->current_line_len - 1] = '\0';

				terminal_printf(p_terminal, TERMINAL_VT100_ERASE_END_OF_LINE);
				terminal_printf(p_terminal, &p_terminal->p_line_buffer[p_terminal->cursor_pos - 1]);
				terminal_printf(p_terminal, "\e[%dD", p_terminal->current_line_len - p_terminal->cursor_pos);
			}

			p_terminal->current_line_len--;
			p_terminal->cursor_pos--;
		}
	}
	else
	{
		/* User pressed normal key - store it in the line buffer */
		if (p_terminal->current_line_len < p_terminal->max_line_len)
		{
			if (p_terminal->cursor_pos == p_terminal->current_line_len)
			{
				/* Cursor is at the end of line */
				p_terminal->p_line_buffer[p_terminal->cursor_pos] = byte;
				p_terminal->p_line_buffer[p_terminal->cursor_pos + 1] = '\0';
				terminal_printf(p_terminal, "%c", byte);
			}
			else
			{
				/* Editing a line when cursor is in the middle */
				memmove(&p_terminal->p_line_buffer[p_terminal->cursor_pos + 1], &p_terminal->p_line_buffer[p_terminal->cursor_pos], p_terminal->current_line_len - p_terminal->cursor_pos);
				p_terminal->p_line_buffer[p_terminal->cursor_pos] = byte;
				p_terminal->p_line_buffer[p_terminal->current_line_len + 1] = '\0';

				terminal_printf(p_terminal, &p_terminal->p_line_buffer[p_terminal->cursor_pos]);
				terminal_printf(p_terminal, "\e[%dD", p_terminal->current_line_len - p_terminal->cursor_pos);
			}
			p_terminal->current_line_len++;
			p_terminal->cursor_pos++;
		}
	}
}

void terminal_set_prompt(Terminal_t *p_terminal, char *p_prompt)
{
	p_terminal->p_prompt = p_prompt;

	terminal_printf(p_terminal, TERMINAL_VT100_ERASE_LINE "\r%s", p_terminal->p_prompt);

	if (p_terminal->current_line_len > 0)
	{
		terminal_printf(p_terminal, p_terminal->p_line_buffer);
	}
	if (p_terminal->cursor_pos < p_terminal->current_line_len)
	{
		terminal_printf(p_terminal,"\e[%dD", p_terminal->current_line_len - p_terminal->cursor_pos);
	}
}

void terminal_set_echo_disabled(Terminal_t *p_terminal, bool disabled)
{
	p_terminal->echo_disabled = disabled;
}

int terminal_printf(Terminal_t *p_terminal, const char *p_format, ...)
{
	int result = 0;

	if (!p_terminal->echo_disabled)
	{
		va_list args;
		va_start(args, p_format);
		int result = vsnprintf(p_terminal->p_write_buffer, p_terminal->write_buffer_size, p_format, args);
		va_end(args);

		if (result > 0)
		{
			result = p_terminal->on_write_request(p_terminal, p_terminal->p_write_buffer, result);
		}
	}
	return result;
}

int terminal_get_number_of_history_entries(Terminal_t *p_terminal)
{
	return p_terminal->history.number_of_entries;
}

char *terminal_get_history_entry(Terminal_t *p_terminal, int entry_no)
{
	char *p_entry = NULL;
	int entry_idx = _history_get_entry_idx_by_entry_no(&p_terminal->history, entry_no);

	if (entry_idx != -1)
	{
		p_entry = &p_terminal->history.p_entries[entry_idx * (p_terminal->history.entry_max_len + 1)];
	}
	return p_entry;
}


void terminal_clear_history(Terminal_t *p_terminal)
{
	p_terminal->history.number_of_entries = 0;
	p_terminal->history.displayed_entry_no = -1;
}

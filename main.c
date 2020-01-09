/*
 * main.c
 *
 *  Created on: 4 sty 2020
 *      Author: Tojwek
 */
#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <stdarg.h>

#include "terminal.h"

#define MAX_LINE_LENGTH     64
#define MAX_HISTORY_LENGTH  10
#define PROMPT              "$ "

char write_buffer[1024];

char terminal_line_buffer[MAX_LINE_LENGTH + 1];
char terminal_history_buffer[MAX_HISTORY_LENGTH * (MAX_LINE_LENGTH + 1)];

int client_socket = -1;

int on_terminal_write_request(Terminal_t *p_terminal, char *p_data, int data_len)
{
    int result = -1;

    if (-1 != client_socket)
    {
        result = send(client_socket, p_data, data_len, 0);
    }
    return result;
}

void on_terminal_line_read(Terminal_t *p_terminal, char *p_line, int line_len)
{
    if (0 == strcmp(p_line, "history"))
    {
        int number_of_history_entries = terminal_get_number_of_history_entries(p_terminal);

        if (0 == number_of_history_entries)
        {
            terminal_printf(p_terminal, "<No history>\r\n\r\n");
        }
        else
        {
            for (int i = 0U; i < number_of_history_entries; ++i)
            {
                int history_entry_no = number_of_history_entries - i - 1;
                char *p_entry = terminal_get_history_entry(p_terminal, history_entry_no);
                terminal_printf(p_terminal, "%3d. %s\r\n", history_entry_no + 1, p_entry);
            }
            terminal_printf(p_terminal, "\r\n");
        }
    }
    else if (0 == strcmp(p_line, "history clear"))
    {
        terminal_clear_history(p_terminal);
    }
    else if (0 == strcmp(p_line, "echo off"))
    {
        terminal_printf(p_terminal, "echo disabled\r\n\r\n");
        terminal_set_echo_disabled(p_terminal, true);
    }
    else if (0 == strcmp(p_line, "echo on"))
    {
        terminal_set_echo_disabled(p_terminal, false);
        terminal_printf(p_terminal, "echo enabled\r\n\r\n");
    }
}

char *on_terminal_suggestion_request(Terminal_t *p_terminal, char *p_line, int line_len)
{
    char *p_suggestion = NULL;

    if (0 == strncmp(p_line, "history", line_len))
    {
        p_suggestion = "history";
    }
    else if (0 == strncmp(p_line, "history clear", line_len))
    {
        p_suggestion = "history clear";
    }
    else if (0 == strncmp(p_line, "echo o", line_len))
    {
        p_suggestion = "echo o";
    }
    else if (0 == strncmp(p_line, "echo off", line_len))
    {
        p_suggestion = "echo off";
    }
    return p_suggestion;
}

int main()
{
    WSADATA wsaData;
    Terminal_t console;

    terminal_init(&console,
                  terminal_line_buffer,
                  MAX_LINE_LENGTH,
                  write_buffer,
                  sizeof(write_buffer),
                  terminal_history_buffer,
                  MAX_HISTORY_LENGTH,
                  on_terminal_write_request,
                  on_terminal_line_read,
                  on_terminal_suggestion_request);

    /* Initialize winsock */
    if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        perror("Failed to initialize winsock");
    }
    else
    {
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (-1 == server_socket)
        {
            perror("Failed to open server socket");
        }
        else
        {
            int opt = 1;
            if (-1 == setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, sizeof(opt)))
            {
                perror("setsockopt");
            }
            else
            {
                struct sockaddr_in address;
                int addrlen = sizeof(address);

                address.sin_family = AF_INET;
                address.sin_addr.s_addr = INADDR_ANY;
                address.sin_port = htons(6969);

                if (-1 == bind(server_socket, (struct sockaddr*) &address, sizeof(address)))
                {
                    perror("Failed to bind");
                }
                else
                {
                    if (-1 == listen(server_socket, 1))
                    {
                        perror("Failed to listen");
                    }
                    else
                    {
                        printf("Waiting for the client...\n");
                        client_socket = accept(server_socket, (struct sockaddr*) &address, &addrlen);

                        if (-1 == client_socket)
                        {
                            perror("Failed to accept");
                        }
                        else
                        {
                            printf("Client connected\n");

                            terminal_set_prompt(&console, PROMPT);

                            while (1)
                            {
                                char byte;
                                int recv_status = recv(client_socket, &byte, 1, 0);

                                if (0 == recv_status)
                                {
                                    printf("Connection closed by the client\n");
                                    break;
                                }
                                else if (-1 == recv_status)
                                {
                                    perror("Failed to receive");
                                    break;
                                }
                                else
                                {
                                    terminal_feed(&console, byte);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

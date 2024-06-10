/*
 *   Copyright (c) 2024 Anton Kundenko <singaraiona@gmail.com>
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include <stdio.h>
#include <unistd.h>
#include "term.h"
#include "heap.h"
#include "string.h"
#include "error.h"
#include "ops.h"
#include "runtime.h"
#include "format.h"
#include "parse.h"
#include "env.h"

#define COMMANDS_LIST "\
  :?  - Displays help.\n\
  :g  - Use rich graphic formatting: [0|1].\n\
  :q  - Exits the application: [exit code]."

term_p term_create()
{
    term_p term = (term_p)heap_mmap(sizeof(struct term_t));

    if (term == NULL)
        return NULL;

    // Set the terminal to non-canonical mode
    tcgetattr(STDIN_FILENO, &term->oldattr);
    term->newattr = term->oldattr;
    term->newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term->newattr);

    // Initialize the input buffer
    term->buf_len = 0;
    term->buf_pos = 0;

    return term;
}

nil_t term_destroy(term_p term)
{
    // Restore the terminal attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &term->oldattr);

    // Unmap the terminal structure
    heap_unmap(term, sizeof(struct term_t));
}

nil_t term_prompt(term_p term)
{
    unused(term);
    obj_p prompt = NULL_OBJ;

    prompt_fmt_into(&prompt);
    printf("%s", as_string(prompt));
    fflush(stdout);
    drop_obj(prompt);
}

i64_t term_highlight_into(term_p term, obj_p *dst)
{
    u64_t i, j, l, n, c;
    str_p verb;

    n = str_fmt_into(dst, -1, "\r\033[K");
    n += prompt_fmt_into(dst);

    l = term->buf_len;

    for (i = 0; i < l; i++)
    {
        c = 0;
        switch (term->buf[i])
        {
        case '(':
        case ')':
            n += str_fmt_into(dst, -1, "%s%s%c%s", BOLD, WHITE, term->buf[i], RESET);
            break;
        case '[':
        case ']':
            n += str_fmt_into(dst, -1, "%s%s%c%s", BOLD, WHITE, term->buf[i], RESET);
            break;
        case '{':
        case '}':
            n += str_fmt_into(dst, -1, "%s%s%c%s", BOLD, WHITE, term->buf[i], RESET);
            break;
        case ',':
            n += str_fmt_into(dst, -1, "%s%c%s", GRAY, term->buf[i], RESET);
            break;
        case ':':
            j = 1;
            if (i == 0 && l > 1)
            {
                for (; j < l; j++)
                {
                    if (!is_alphanum(term->buf[j]) && term->buf[j] != '?')
                        break;
                }

                i = j - 1;
                c = 1;
            }

            n += str_fmt_into(dst, -1, "%s%.*s%s", GRAY, j, term->buf, RESET);

            break;
        default:
            if ((i == 0 || !is_alpha(term->buf[i - 1])) && is_alpha(term->buf[i]))
            {
                for (j = i + 1; j < l; j++)
                {
                    if (!is_alphanum(term->buf[j]) && term->buf[j] != '-')
                        break;
                }

                // try to find in a verbs list
                verb = env_get_internal_function_lit(term->buf + i, j);
                if (verb != NULL)
                {
                    n += str_fmt_into(dst, -1, "%s%s%s%s", BOLD, GREEN, verb, RESET);
                    i += strlen(verb) - 1;
                    c = 1;
                    break;
                }
            }
            else if (is_op(term->buf[i]))
            {
                n += str_fmt_into(dst, -1, "%s%c%s", BLUE, term->buf[i], RESET);
                c = 1;
            }
            else if (term->buf[i] == '"')
            {
                // string
                for (j = i + 1; j < l; j++)
                {
                    if (term->buf[j] == '"')
                    {
                        n += str_fmt_into(dst, -1, "%s", YELLOW);
                        n += str_fmt_into(dst, -1, "%.*s", j - i + 1, term->buf + i);
                        n += str_fmt_into(dst, -1, "%s", RESET);
                        i = j;
                        c = 1;
                        break;
                    }
                }
            }
            else if (term->buf[i] == '\'')
            {
                // char or symbol
                for (j = i + 1; j < l; j++)
                {
                    if (!is_alphanum(term->buf[j]))
                    {
                        n += str_fmt_into(dst, -1, "%s", TEAL);
                        n += str_fmt_into(dst, -1, "%.*s", j - i + 1, term->buf + i);
                        n += str_fmt_into(dst, -1, "%s", RESET);
                        i = j;
                        c = 1;
                        break;
                    }
                }
            }

            if (c == 0)
                n += str_fmt_into(dst, -1, "%c", term->buf[i]);

            break;
        }
    }

    return n;
}

nil_t term_highlight(term_p term)
{
    obj_p fmt = NULL_OBJ;

    term_highlight_into(term, &fmt);
    printf("%s", as_string(fmt));
    fflush(stdout);
    drop_obj(fmt);
}

nil_t term_backspace(term_p term)
{
    if (term->buf_pos == 0)
        return;
    else if (term->buf_pos == term->buf_len)
    {
        term->buf_pos--;
        term->buf[term->buf_pos] = '\0';
        term->buf_len--;
    }
    else
    {
        memmove(term->buf + term->buf_pos - 1, term->buf + term->buf_pos, term->buf_len - term->buf_pos);
        term->buf_len--;
        term->buf_pos--;
    }

    term_highlight(term);
}

obj_p term_read(term_p term)
{
    c8_t c;
    i64_t exit_code;
    obj_p res = NULL;

    if (read(STDIN_FILENO, &c, 1) == 1)
    {
        // Process the character
        switch (c)
        {
        case '\n': // New line
            // Enter key pressed, process the input buffer
            term->buf[term->buf_len] = '\0';

            if (strncmp(term->buf, ":q", 2) == 0)
            {
                if (term->buf_pos > 2)
                    exit_code = i64_from_str(term->buf + 2, term->buf_len - 3);
                else
                    exit_code = 0;

                poll_exit(runtime_get()->poll, exit_code);
            }
            else if (strncmp(term->buf, ":?", 2) == 0)
            {
                printf("\n%s** Commands list:\n%s%s", YELLOW, COMMANDS_LIST, RESET);
                fflush(stdout);
                res = NULL_OBJ;
            }
            else
                res = (term->buf_len) ? cstring_from_str(term->buf, term->buf_len) : NULL_OBJ;

            term->buf_len = 0;
            term->history_index = 0;

            printf("\n");
            fflush(stdout);

            return res;
        case 127:  // Del
        case '\b': // Backspace
            term_backspace(term);
            return res;
        case '\t': // Tab
            // term_autocomplete(term);
            return res;
        case '\033': // Escape sequence
            if (read(STDIN_FILENO, &c, 1) == 1 && c == '[')
            {
                if (read(STDIN_FILENO, &c, 1) == 1)
                {
                    switch (c)
                    {
                    case 'A': // Up arrow
                        // term_navigate_history(term, -1);
                        break;
                    case 'B': // Down arrow
                        // term_navigate_history(term, 1);
                        break;
                    case 'C': // Right arrow
                        // if (term->buf_pos < strlen(term->buf))
                        // {
                        //     term->buf_pos++;
                        //     term_replace_stdout(term);
                        // }
                        break;
                    case 'D': // Left arrow
                        if (term->buf_pos > 0)
                        {
                            term->buf_pos--;
                        }
                        break;
                    }
                    term_highlight(term);
                }
            }
            return res;
        default:
            // regular character
            if (term->buf_len < TERM_BUF_SIZE - 1)
                term->buf[term->buf_len++] = c;

            term_highlight(term);

            return res;
        }
    }

    return res;
}

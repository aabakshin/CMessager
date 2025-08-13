#ifndef INPUT_C_SENTRY
#define INPUT_C_SENTRY

/*#include "DebugUtils.h"*/												/* Used for print_buffer() function call */
#include "Input.h"
#include "CommandsHistoryList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>


enum
{
			MAX_SYM_CODE_SIZE	=   10,
};


enum
{
			CTRL_C			=			3,
			CTRL_D			=			4,
			BACKSPACE		=		  127,
			CTRL_W			=		   23
};


static int get_str(char* buffer, int buffer_size);
#ifdef DEBUG_INPUT
static void show_dec_sym(char* ch_buf);
#endif
static int check_symbols_cnt(char* buffer, int buffer_size, int* i, int* left_offset, char* read_sym);
static int is_cyrillic_sym(char* ch_buf);
static int ascii_cnt_str(const char* buffer, int buffer_size);
static int handle_alphabet_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);
static int handle_ctrlw_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);
static int handle_newline_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);
static int handle_backspace_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);
static int handle_arrow_left_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);
static int handle_arrow_right_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);
static int handle_arrow_up_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);
static int handle_arrow_down_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);
static int handle_del_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf);


/* Буфер предыдущих отправленных команд */
extern CommandsHistoryList* chl_list;

/* Сохраняет текущую позицию на элемент списка chl_list */
static CommandsHistoryList* cur_pos_list = NULL;

/* UTF-16LE */
const char rus_alpha_codes[] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя";

#ifdef DEBUG_INPUT
static void show_dec_sym(char* ch_buf)
{
	putchar('\n');
	int i;
	for ( i = 0; i < MAX_SYM_CODE_SIZE; i++ )
	{
		printf("%3d ", ch_buf[i]);
	}
	putchar('\n');
	fflush(stdout);
}
#endif

int get_any_key(void)
{
	struct termios t1, t2;
	tcgetattr(0, &t1);
	memcpy(&t2, &t1, sizeof(t1));

	t1.c_lflag &= ~ICANON;
	t1.c_lflag &= ~ISIG;
	t1.c_lflag &= ~ECHO;
	t1.c_cc[VMIN] = 0;
	t1.c_cc[VTIME] = 0;

	tcsetattr(0, TCSANOW, &t1);

	char read_sym[MAX_SYM_CODE_SIZE] = { 0 };
	while ( 1 )
	{
		/* 6 - макс. размер в байтах кода клавиши на клавиатуре(F1-F12) */
		int rc = read(0, read_sym, 6);

		if ( rc < 1 )
			continue;

		break;
	}

	tcsetattr(0, TCSANOW, &t2);

	return 1;
}

int input(char* buffer, int buffer_size)
{
	/* Выключение канонического режима терминала */
	struct termios t1, t2;
	tcgetattr(0, &t1);
	memcpy(&t2, &t1, sizeof(t1));

	t1.c_lflag &= ~ICANON;
	t1.c_lflag &= ~ISIG;
	t1.c_lflag &= ~ECHO;
	t1.c_cc[VMIN] = 0;
	t1.c_cc[VTIME] = 0;

	tcsetattr(0, TCSANOW, &t1);

	int result = get_str(buffer, buffer_size);

	/* восстановление канонического режима */
	tcsetattr(0, TCSANOW, &t2);

	return result;
}

/* Обработка терминального ввода в ручном режиме при помощи termios
 * Реализация некоторых возможностей терминала(backspace, удаление последнего слова, del, стрелка влево-вправо)
 * Получение строки из станд.потока ввода через низкоуровненые функции и
 * обработка содержимого с учётом использования многобайтных символов
 * Кол-во вводимых с стандартного потока ввода символов должно быть не более buffer_size-2
 * Возвращает длину получившейся строки в байтах вместе с символом line feed */
static int get_str(char* buffer, int buffer_size)
{
	if ( (buffer == NULL) || (buffer_size < 2) )
		return -1;

	memset(buffer, 0, buffer_size);

	/* буфер под прочитанный символ */
	char read_sym[MAX_SYM_CODE_SIZE] = { 0 };

	/* текущая длина buffer */
	int i = 0;

	/* смещение слева относительно конца строки */
	int left_offset = 0;

	while ( 1 )
	{
		/* 6 - макс. размер в байтах кода клавиши на клавиатуре(F1-F12) */
		int rc = read(0, read_sym, 6);

		if ( rc < 1 )
			continue;

#ifdef DEBUG_INPUT
		show_dec_sym(read_sym);
#endif

		if ( rc == 1 )
		{
			if ( read_sym[0] == CTRL_C )
			{
				/* завершение программы */
				return -1;
			}

			if ( (read_sym[0] == CTRL_D) || (read_sym[0] == '\n') )
			{
				handle_newline_key(buffer, buffer_size, &i, &left_offset, read_sym);
				break;
			}

			if ( (read_sym[0] == '\b') || (read_sym[0] == BACKSPACE) )
			{
				handle_backspace_key(buffer, buffer_size, &i, &left_offset, read_sym);
				memset(read_sym, 0, sizeof(read_sym));

				continue;
			}

			if ( read_sym[0] == CTRL_W )
			{
				handle_ctrlw_key(buffer, buffer_size, &i, &left_offset, read_sym);
				memset(read_sym, 0, sizeof(read_sym));

				continue;
			}

			/* обработка 1-байтного ascii символа */
			if ( !handle_alphabet_key(buffer, buffer_size, &i, &left_offset, read_sym) )
			{
				break;
			}
			memset(read_sym, 0, sizeof(read_sym));

			if ( check_symbols_cnt(buffer, buffer_size, &i, &left_offset, read_sym) )
				break;
		}
		else if ( rc == 2 )
		{
			/* Обработка 2-байтных значений кириллицы */

			/* вывод кода текущего введённого символа */
			/*printf("\n--> %04x %04x\n", read_sym[0], read_sym[1]);
			fflush(stdout);*/

			if ( is_cyrillic_sym(read_sym) )
			{
				if ( !handle_alphabet_key(buffer, buffer_size, &i, &left_offset, read_sym) )
				{
					break;
				}
			}
			memset(read_sym, 0, sizeof(read_sym));

			if ( check_symbols_cnt(buffer, buffer_size, &i, &left_offset, read_sym) )
				break;
		}
		else if ( rc == 3 )
		{
			/* обработка клавиши ARROW_LEFT с 3-байтным кодом */
			if	(
						( read_sym[0] == 0x1b )		&&			/* 27 */
						( read_sym[1] == 0x5b )		&&			/* 91 */
						( read_sym[2] == 0x44 )					/* 68 */
				)
			{
				handle_arrow_left_key(buffer, buffer_size, &i, &left_offset, read_sym);
			}

			/* обработка клавиши ARROW_RIGHT с 3-байтным кодом */
			else if (
							( read_sym[0] == 0x1b )		&&		/* 27 */
							( read_sym[1] == 0x5b )		&&		/* 91 */
							( read_sym[2] == 0x43 )				/* 67 */
					)
			{
				handle_arrow_right_key(buffer, buffer_size, &i, &left_offset, read_sym);
			}

			/* обработка клавиши ARROW_UP с 3-байтным кодом */
			else if (
							( read_sym[0] == 0x1b )		&&		/* 27 */
							( read_sym[1] == 0x5b )		&&		/* 91 */
							( read_sym[2] == 0x41 )				/* 65 */
					)
			{
				handle_arrow_up_key(buffer, buffer_size, &i, &left_offset, read_sym);
			}

			/* обработка клавиши ARROW_DOWN с 3-байтным кодом */
			else if (
							( read_sym[0] == 0x1b )		&&		/* 27 */
							( read_sym[1] == 0x5b )		&&		/* 91 */
							( read_sym[2] == 0x42 )				/* 66 */
					)
			{
				handle_arrow_down_key(buffer, buffer_size, &i, &left_offset, read_sym);
			}

			memset(read_sym, 0, sizeof(read_sym));
		}
		else if ( rc == 4 )
		{
			/* обработка клавиши DEL с 4-х байтным кодом */
			if
				(
						( read_sym[0] == 0x1b )		&&		/* 27 */
						( read_sym[1] == 0x5b )		&&		/* 91 */
						( read_sym[2] == 0x33 )		&&		/* 51 */
						( read_sym[3] == 0x7e )				/* 126 */
				)
			{
				handle_del_key(buffer, buffer_size, &i, &left_offset, read_sym);
			}

			memset(read_sym, 0, sizeof(read_sym));
		}

		memset(read_sym, 0, sizeof(read_sym));
	}

	/* длина строки buffer в байтах */
	return i;
}

static int check_symbols_cnt(char* buffer, int buffer_size, int* i, int* left_offset, char* read_sym)
{
	int ascii_cnt = ascii_cnt_str(buffer, buffer_size);
	int cyril_cnt = (*i -  ascii_cnt) / 2;
	int total_cnt = ascii_cnt + cyril_cnt;
	int max_symbols_cnt = (MAX_READ_BUF_SIZE - 2) / 2;

	if ( total_cnt >= max_symbols_cnt )
	{
		handle_newline_key(buffer, buffer_size, i, left_offset, read_sym);
		return 1;
	}

	return 0;
}

static int is_cyrillic_sym(char* ch_buf)
{
	int len = strlen(rus_alpha_codes);
	int j;
	for ( j = 0; j < len/2; j++ )
	{
		/* printf("[%3d] %3d %3d\n", j, rus_alpha_codes[j*2], rus_alpha_codes[j*2+1]); */
		if ( ch_buf[0] == rus_alpha_codes[j*2] )
		{
			if ( ch_buf[1] == rus_alpha_codes[j*2+1] )
			{
				return 1;
			}
		}
	}

	return 0;
}

static int ascii_cnt_str(const char* buffer, int buffer_size)
{
	if ( (buffer == NULL) || (buffer[0] == '\0') || (buffer_size < 1) )
	{
		return 0;
	}

	int counter = 0;

	/*print_buffer(buffer, buffer_size);*/

	int i;
	for ( i = 0; buffer[i]; i++ )
	{
		if ( (buffer[i] > 0) && (buffer[i] <= 127) )
			counter++;
	}

	return counter;
}

static int handle_alphabet_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	int cyril_flag = 0;
	if ( is_cyrillic_sym(ch_buf) )
		cyril_flag = 1;

	/*printf("\ncyril_flag = %d\n", cyril_flag);
	fflush(stdout);*/

	int save_pos = 0;

	if ( *left_offset > 0 )
	{
		char buf[buffer_size];
		int last_ch = *i - 1;

		*i -= *left_offset;
		save_pos = *i;

		int x, j = 0;
		for ( x = *i; x <= last_ch; x++ )
		{
			buf[j] = buffer[x];
			j++;
		}
		buf[j] = '\0';

		buffer[*i] = ch_buf[0];
		(*i)++;

		if ( cyril_flag )
		{
			buffer[*i] = ch_buf[1];
			(*i)++;
		}

		for ( x = 0; buf[x]; x++ )
		{
			if ( *i < buffer_size-1 )
			{
				buffer[*i] = buf[x];
				(*i)++;
			}
			else
				break;
		}
	}
	else
	{
		buffer[*i] = ch_buf[0];
		(*i)++;

		if ( cyril_flag )
		{
			buffer[*i] = ch_buf[1];
			(*i)++;
		}
	}


	if ( (*i+2) > buffer_size-2 )
	{
		buffer[buffer_size-2] = '\n';
		buffer[buffer_size-1] = '\0';

		*i = buffer_size-1;

		write(1, &ch_buf[0], 1);
		if ( cyril_flag )
			write(1, &ch_buf[1], 1);

		return 0;
	}


	if ( *left_offset > 0 )
	{
		/*printf("\n*left_offset = %d\n"
				"*i = %d\n"
				"save_pos = %d\n", *left_offset, *i, save_pos);
		fflush(stdout);*/

		int last_ch_pos = *i-1;

		/* вывод содержимого buffer начиная с вставленного элемента */

		for ( int cur_pos = save_pos; cur_pos <= last_ch_pos; cur_pos++ )
		{
			putchar(buffer[cur_pos]);
		}
		fflush(stdout);

		save_pos++;

		int len_in_bytes = *i;
		int ascii_cnt = ascii_cnt_str(buffer, buffer_size);
		int cyril_cnt = (len_in_bytes -  ascii_cnt) / 2;
		int total_cnt = ascii_cnt + cyril_cnt;


		/*printf("\nlen_in_bytes = %d\n"
				"ascii_cnt = %d\n"
				"cyril_cnt = %d\n"
				"total_cnt = %d\n"
				"save_pos = %d\n"
				"*left_offset = %d\n", len_in_bytes, ascii_cnt, cyril_cnt, total_cnt, save_pos, *left_offset);
		fflush(stdout);*/


		/* возвращение курсора в прежнее положение после вставки очереднего символа */

		int prev_left_offset = *left_offset;
		int x = 0;
		while ( x < total_cnt )
		{
			putchar('\b');
			x++;
		}
		fflush(stdout);
		*left_offset = *i;

		if ( cyril_flag )
			save_pos++;

		for ( x = 0; x < save_pos; x++ )
		{
			putchar(buffer[x]);
		}
		fflush(stdout);
		*left_offset = prev_left_offset;
	}
	else
	{
		write(1, &ch_buf[0], 1);
		if ( cyril_flag )
			write(1, &ch_buf[1], 1);
	}

	/* check buffer memory */
	/*print_buffer(buffer, buffer_size);*/

	return 1;
}

static int handle_ctrlw_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	if ( *i < 1 )
		return 0;

	int last_ch = *i - 1;
	int cur_pos = *i - *left_offset;
	int save_pos = cur_pos;


	if ( cur_pos > 0 )
	{
		if ( buffer[cur_pos-1] == ' ' )
		{
			while ( (cur_pos > 0) && (buffer[cur_pos-1] == ' ')  )
				cur_pos--;
		}
		else
		{
			while ( (cur_pos > 0) && (buffer[cur_pos-1] != ' ') )
				cur_pos--;
		}


		int del_bytes = save_pos - cur_pos;

		char buf[buffer_size];
		int x = 0;
		int k;
		for ( k = cur_pos; k < save_pos; k++ )
		{
			buf[x] = buffer[k];
			x++;
		}
		buf[x] = '\0';

		int buf_len = strlen(buf);
		int ascii_cnt = ascii_cnt_str(buf, strlen(buf)+1);
		int cyril_cnt = (buf_len - ascii_cnt) / 2;
		int buf_cnt = ascii_cnt + cyril_cnt;

		/*printf("\ndel_bytes = %d\n"
					"buf_cnt = %d\n", del_bytes, buf_cnt);*/

		for ( k = 1; k <= buf_cnt; k++ )
		{
			printf("\b \b");
			fflush(stdout);
		}


		memset(buf, 0, buffer_size);
		x = 0;
		for ( k = save_pos; k <= last_ch; k++ )
		{
			buf[x] = buffer[k];
			x++;
		}
		buf[x] = '\0';

		/* printf("\nbuf = %s\n", buf); */

		buf_len = strlen(buf);
		ascii_cnt = ascii_cnt_str(buf, strlen(buf)+1);
		cyril_cnt = (buf_len - ascii_cnt) / 2;
		buf_cnt = ascii_cnt + cyril_cnt;

		int v = cur_pos;
		for ( x = 0; buf[x]; x++ )
		{
			buffer[v] = buf[x];
			putchar(buffer[v]);
			v++;
		}
		fflush(stdout);

		for ( x = v; x <= last_ch; x++ )
		{
			putchar(' ');
			buffer[x] = '\0';
		}

		int total_cnt = last_ch - v + 1 + buf_cnt;
		for ( x = 1; x <= total_cnt; x++ )
			putchar('\b');
		fflush(stdout);

		*i -= del_bytes;
	}

	/*print_buffer(buffer, buffer_size);*/

	return 1;
}

static int handle_newline_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	const char nl = '\n';
	write(1, &nl, 1);

	if ( *i < buffer_size-1 )
	{
		buffer[*i] = '\n';
		(*i)++;
		buffer[*i] = '\0';

		return 0;
	}
	buffer[buffer_size-2] = '\n';
	buffer[buffer_size-1] = '\0';

	*i = buffer_size-1;

	return 1;
}

static int handle_backspace_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	if ( (*left_offset >= 0) && (*left_offset < *i) )
	{
		char buf[buffer_size];
		int is_cyril_flag = 0;	// проверка, является ли удаляемый символ кириллическим
		int last_ch_pos = *i - 1;

		if ( *left_offset <= 0 )
		{
			if ( *i >= 2 )
			{
				char sym[3] = { 0 };
				sym[0] = buffer[*i-2];
				sym[1] = buffer[*i-1];
				sym[2] = '\0';

				if ( is_cyrillic_sym(sym) )
					is_cyril_flag = 1;
			}

			printf("%s", "\b \b");
			fflush(stdout);
			buffer[*i-1] = '\0';
			if ( is_cyril_flag )
				buffer[*i-2] = '\0';

			(*i)--;
			if ( is_cyril_flag )
				(*i)--;

			return 1;
		}

		int cur_pos = *i - *left_offset;

		if ( cur_pos >= 2 )
		{
			char sym[3] = { 0 };
			sym[0] = buffer[cur_pos-2];
			sym[1] = buffer[cur_pos-1];
			sym[2] = '\0';

			if ( is_cyrillic_sym(sym) )
				is_cyril_flag = 1;
		}

		int x, z = 0;
		for ( x = cur_pos; x <= last_ch_pos; x++ )
		{
			buf[z] = buffer[x];
			z++;
		}
		buf[z] = '\0';

		//printf("\nbuf = %s\n", buf);

		x = cur_pos - 1;
		if ( is_cyril_flag )
			x--;

		putchar('\b');
		for ( z = 0; buf[z]; z++, x++ )
		{
			buffer[x] = buf[z];
			putchar(buffer[x]);
		}
		putchar(' ');
		putchar('\b');
		buffer[x] = '\0';
		fflush(stdout);


		int buf_len = strlen(buf);
		int ascii_cnt = ascii_cnt_str(buf, strlen(buf)+1);
		int cyril_cnt = (buf_len - ascii_cnt) / 2;
		int total_cnt = ascii_cnt + cyril_cnt;

		for ( x = 1; x <= total_cnt; x++ )
			putchar('\b');
		fflush(stdout);


		if ( *i > 0 )
		{
			(*i)--;

			if ( is_cyril_flag )
				(*i)--;
		}

		/*
		printf("\nis_cyril_flag = %d\n"
				"*i = %d\n"
				"buf_len = %d\n"
				"ascii_cnt = %d\n"
				"cyril_cnt = %d\n"
				"total_cnt = %d\n"
				"cur_pos = %d\n"
				"*left_offset = %d\n", is_cyril_flag, *i, buf_len, ascii_cnt, cyril_cnt, total_cnt, cur_pos, *left_offset);
		fflush(stdout);
		*/
	}

	/* check buffer memory */

	/*printf("\nbuffer:\n");
	for ( int x = 0; x < buffer_size; x++ )
	{
		printf("%4d ", buffer[x]);
		if ( ((x+1) % 10) == 0 )
			putchar('\n');
	}
	putchar('\n');
	fflush(stdout);*/


	return 1;
}

static int handle_arrow_left_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	if ( *left_offset < *i )	/* если текущая позиция буфера не в начале строки - перемещать курсор влево */
	{
		putchar('\b');
		fflush(stdout);

		int x = *i - *left_offset - 1;

		if ( x > 0 )
		{
			char sym[3] =
			{
				buffer[x-1],
				buffer[x],
				'\0'
			};

			if ( is_cyrillic_sym(sym) )
			{
				*left_offset += 2;
			}
			else
			{
				(*left_offset)++;
			}

			return 1;
		}

		if ( x == 0 )
		{
			(*left_offset)++;
		}
	}

	return 1;
}

static int handle_arrow_right_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	if ( *left_offset > 0 )		/* если не конец строки - перемещать курсор вправо */
	{
		int x = *i - *left_offset;

		if ( *left_offset > 1 )
		{
			char sym[3] =
			{
				buffer[x],
				buffer[x+1],
				'\0'
			};

			if ( is_cyrillic_sym(sym) )
			{
				write(1, sym, 2);
				(*left_offset) -= 2;
			}
			else
			{
				putchar(buffer[*i - *left_offset]);
				fflush(stdout);
				(*left_offset)--;
			}

			return 1;
		}

		if ( *left_offset == 1 )
		{
			putchar(buffer[*i - *left_offset]);
			fflush(stdout);
			(*left_offset)--;
		}
	}

	return 1;
}

static int handle_arrow_up_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	if ( chl_list != NULL )
	{
		if ( *i > 0 )
		{
			while ( *i > 0 )
			{
				buffer[*i-1] = 0;
				printf("%s", "\b \b");
				(*i)--;
			}
			fflush(stdout);
		}

		if ( cur_pos_list != NULL )
		{
			if ( cur_pos_list->prev != NULL )
			{
				cur_pos_list = cur_pos_list->prev;
			}
		}
		else
		{
			cur_pos_list = chl_list;
		}


		*i = 0;
		for ( int j = 0; cur_pos_list->command[j]; j++, (*i)++ )
			buffer[*i] = cur_pos_list->command[j];

		printf("%s", cur_pos_list->command);
		fflush(stdout);
	}

	return 1;
}

static int handle_arrow_down_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	if ( chl_list != NULL )
	{
		if ( *i > 0 )
		{
			while ( *i > 0 )
			{
				buffer[*i-1] = 0;
				printf("%s", "\b \b");
				(*i)--;
			}
			fflush(stdout);
		}


		if ( cur_pos_list != NULL )
		{
			if ( cur_pos_list->next != NULL )
			{
				cur_pos_list = cur_pos_list->next;
			}
		}
		else
		{
			cur_pos_list = chl_list;
		}


		*i = 0;
		for ( int j = 0; cur_pos_list->command[j]; j++, (*i)++ )
			buffer[*i] = cur_pos_list->command[j];

		printf("%s", cur_pos_list->command);
		fflush(stdout);
	}

	return 1;
}

static int handle_del_key(char* buffer, int buffer_size, int* i, int* left_offset, char* ch_buf)
{
	if ( *left_offset > 0 )		/* если не конец строки */
	{
		int is_cyril_flag = 0;	/* проверка, является ли удаляемый символ кириллическим */

		char buf[buffer_size];
		int last_ch_pos = *i - 1;
		int cur_pos = *i - *left_offset;

		if ( (cur_pos + 1) < *i )
		{
			char sym[3] =
			{
						buffer[cur_pos],
						buffer[cur_pos+1],
						'\0'
			};

			if ( is_cyrillic_sym(sym) )
			{
				is_cyril_flag = 1;
			}

			/*printf("\nis_cyril_flag = %d\n", is_cyril_flag);*/
		}

		int k = cur_pos + 1;

		if ( is_cyril_flag )
			k++;

		int x = 0;
		for ( ; k <= last_ch_pos; k++ )
		{
			buf[x] = buffer[k];
			x++;
		}
		buf[x] = '\0';

		/* printf("\nbuf = %s\n", buf); */

		x = 0;
		for ( k = cur_pos; buf[x]; x++, k++ )
		{
			buffer[k] = buf[x];
			putchar(buffer[k]);
		}
		buffer[k] = '\0';
		putchar(' ');
		putchar('\b');
		fflush(stdout);


		int buf_len = strlen(buf);
		int ascii_cnt = ascii_cnt_str(buf, strlen(buf)+1);
		int cyril_cnt = (buf_len - ascii_cnt) / 2;
		int total_cnt = ascii_cnt + cyril_cnt;

		for ( x = 1; x <= total_cnt; x++ )
			putchar('\b');
		fflush(stdout);


		if ( *i > 0 )
		{
			(*i)--;

			if ( is_cyril_flag )
				(*i)--;
		}

		(*left_offset)--;

		if ( is_cyril_flag )
			(*left_offset)--;

		/*
		printf("\nbuf = %s\n"
				"buf_len = %d\n"
				"ascii_cnt = %d\n"
				"cyril_cnt = %d\n"
				"total_cnt = %d\n"
				"*i = %d\n"
				"*left_offset = %d\n", buf, buf_len, ascii_cnt, cyril_cnt, total_cnt, *i, *left_offset);
		fflush(stdout);
		*/
	}

	return 1;
}

#endif

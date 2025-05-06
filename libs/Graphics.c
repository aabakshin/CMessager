#ifndef GRAPHICS_C_SENTRY
#define GRAPHICS_C_SENTRY

#include "../includes/Graphics.h"
#include <stdio.h>
#include <string.h>

enum { MAX_STRING_LENGTH			=			52 };

void show_logo(void)
{
	printf("\033c");
	printf("%s",
					"   $$$$$$$$\\  $$$$$$\\  $$$$$$$\\        $$$$$$\\  $$\\   $$\\  $$$$$$\\ $$$$$$$$\\\n"
					"   \\__$$  __|$$  __$$\\ $$  __$$\\      $$  __$$\\ $$ |  $$ |$$  __$$\\\\__$$  __|\n"
					"      $$ |   $$ /  \\__|$$ |  $$ |     $$ /  \\__|$$ |  $$ |$$ /  $$ |  $$ |   \n"
					"      $$ |   $$ |      $$$$$$$  |     $$ |      $$$$$$$$ |$$$$$$$$ |  $$ |    \n"
					"      $$ |   $$ |      $$  ____/      $$ |      $$  __$$ |$$  __$$ |  $$ |    \n"
					"      $$ |   $$ |  $$\\ $$ |           $$ |  $$\\ $$ |  $$ |$$ |  $$ |  $$ |    \n"
					"      $$ |   \\$$$$$$  |$$ |           \\$$$$$$  |$$ |  $$ |$$ |  $$ |  $$ |    \n"
					"      \\__|    \\______/ \\__|            \\______/ \\__|  \\__|\\__|  \\__|  \\__|\n"
					"\n\n");
	fflush(stdout);
}

void print_horizontal_line(int offset, int line_length, char char_line)
{
	int i;
	for ( i = 1; i <= offset; i++ )
		putchar(' ');

	for ( i = 1; i <= line_length; i++ )
		putchar(char_line);

	for ( i = 1; i <= offset; i++ )
		putchar(' ');

	fflush(stdout);
}

void print_greeting_text_frame(const char** text_strings, int text_strings_size)
{
	print_horizontal_line(0, 14, ' ');
	printf("%s\n", "Welcome to authorization page of my simple TCP chat.");
	print_horizontal_line(14, MAX_STRING_LENGTH, '#');
	putchar('\n');

	int i;
	for ( i = 0; i < text_strings_size; i++ )
	{
		int len = strlen(text_strings[i]);

		if ( len > MAX_STRING_LENGTH-4 )
			len = MAX_STRING_LENGTH-4;

		print_horizontal_line(0, 14, ' ');
		print_horizontal_line(0, 2, '#');

		int is_odd = 0;
		int offset_len;
		( ((offset_len = MAX_STRING_LENGTH-4-len) % 2) != 0 ) ? is_odd = 1 : is_odd;
		offset_len /= 2;

		print_horizontal_line(0, offset_len, ' ');
		int j;
		for ( j = 0; j < len; j++ )
			putchar(text_strings[i][j]);
		( is_odd == 1 ) ? print_horizontal_line(0, offset_len+1, ' ') : print_horizontal_line(0, offset_len, ' ');

		print_horizontal_line(0, 2, '#');
		putchar('\n');
	}

	print_horizontal_line(14, MAX_STRING_LENGTH, '#');
	putchar('\n');
	print_horizontal_line(0, 14, ' ');
	printf("%s", "Your answer: ");
	fflush(stdout);
}

#endif

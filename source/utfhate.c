/*
 * utfhate - Utility to find and mark instances of UTF-8 characters in the input stream.
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include <stdio.h>

#define UTFHATE_BUFFER_SIZE 4096

static char line_buffer[UTFHATE_BUFFER_SIZE];
static char marker_buffer[UTFHATE_BUFFER_SIZE];

static int utfhate_consume_utf_char(char **pbuffer);

int main(void) {
	unsigned int line = 1, utf_total = 0;
	
	while (fgets(line_buffer, sizeof(line_buffer), stdin) != NULL) {
		char *offset;
		char *marker_offset = marker_buffer;
		unsigned int utf_found = 0;
		
		for(offset = line_buffer; *offset != '\0'; ++offset, ++marker_offset) {
			if (*offset == '\n') {
				line++;
				*offset = '\0';
				*marker_offset++ = '\n';
				
				break;
			}
			
			if (*offset < 0) {
				*marker_offset = '^';

				if (utfhate_consume_utf_char(&offset) != 0)
					break;
				
				utf_found++;
			} else if (*offset == '\t') {
				*marker_offset = '\t';
			} else {
				*marker_offset = ' ';
			}
		}
		
		if (utf_found > 0) {
			utf_total += utf_found;
			*marker_offset = '\0';
		
			printf("Line %u, %u occurence(s):\n", line, utf_found);
			puts(line_buffer);
			puts(marker_buffer);
		}
	}
	
	printf("UTF-8 characters found: %u\n", utf_total);
	
	return 0;
}

static int utfhate_consume_utf_char(char **pbuffer) {
	char *buffer = *pbuffer;
	
	while (*buffer != '\0') {
		if (*buffer > 0) {
			*pbuffer = buffer - 1;
			
			return 0;
		}
		
		buffer++;
	}
	
	return -1;
}

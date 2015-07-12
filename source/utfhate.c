/*
 * utfhate - Utility to find and mark instances of UTF-8 characters in the input stream.
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include <stdio.h>
#include <string.h>

#define UTFHATE_BUFFER_SIZE 4096
#define UTFHATE_UNUSED(arg) (void)arg
#define UTFHATE_DEFAULT_VERBOSE 0

enum utfhate_command_index {
    UTFHATE_COMMAND_INDEX_SEARCH = 0,
    UTFHATE_COMMAND_INDEX_DELETE,
    UTFHATE_COMMAND_INDEX_REPLACE,
    UTFHATE_COMMAND_INDEX_COUNT
};

enum utfhate_command_count_type {
    UTFHATE_COMMAND_COUNT_TYPE_CHARACTERS = 1,
    UTFHATE_COMMAND_COUNT_TYPE_BYTES = 2,
    UTFHATE_COMMAND_COUNT_TYPE_BOTH = UTFHATE_COMMAND_COUNT_TYPE_BYTES | UTFHATE_COMMAND_COUNT_TYPE_CHARACTERS
};

struct utfhate_options {
    enum utfhate_command_index command;
    
    unsigned int verbose;
    
    FILE * source_stream;
    FILE * destination_stream;
    
    union {
        struct {
            char replacement;
        } as_replace;
        
        struct {
            enum utfhate_command_count_type type;
        } as_count;
    } arguments;
};

struct utfhate_command_option {
    const char *name;
    const char *alternate;
    
    int (*handler)(int *arguments, char ***pargv, struct utfhate_options *options);
    
    const char *help_text;
};

static void utfhate_set_default_options(struct utfhate_options *options);

static void utfhate_print_usage(const char *application);

static int utfhate_process_arguments(
    int argc, 
    char ** argv, 
    struct utfhate_options *options
);

static int utfhate_match_argument(
    const char *argument, 
    int *argc, 
    char ***argv, 
    struct utfhate_options *options
);

static int utfhate_command_option_search(
    int *arguments, 
    char ***pargv, 
    struct utfhate_options *options
);

static int utfhate_command_option_delete(
    int *arguments, 
    char ***pargv, 
    struct utfhate_options *options
);

static int utfhate_command_option_replace(
    int *arguments, 
    char ***pargv, 
    struct utfhate_options *options
);

static int utfhate_command_option_count(
    int *arguments, 
    char ***pargv, 
    struct utfhate_options *options
);

static int utfhate_command_option_verbose(
    int *arguments, 
    char ***pargv, 
    struct utfhate_options *options
);

static int utfhate_command_search(const struct utfhate_options *options);
static int utfhate_command_delete(const struct utfhate_options *options);
static int utfhate_command_replace(const struct utfhate_options *options);
static int utfhate_command_count(const struct utfhate_options *options);

static int utfhate_consume_utf_char(char **pbuffer);

static const struct utfhate_command_option utfhate_command_list[] = {
    { "--search",  "-s", &utfhate_command_option_search,  "Searches for UTF-8 characters and, if found, marks their location."                              },
    { "--delete",  "-d", &utfhate_command_option_delete,  "Deletes all UTF-8 characters found in the input (without destroying the source.)"                },
    { "--replace", "-r", &utfhate_command_option_replace, "Replaces all UTF-8 characters with a specified value."                                           }, 
    { "--count",   "-c", &utfhate_command_option_count,   "Counts the number of UTF-8 characters in the input. Modes: 'chars' (default), 'bytes', 'both'."  },
    { "--verbose", "-v", &utfhate_command_option_verbose, "Enables verbose output command output."                                                          },
    
    { NULL, NULL, NULL, NULL }
};

static char line_buffer[UTFHATE_BUFFER_SIZE];
static char scratch_buffer[UTFHATE_BUFFER_SIZE];

int main(int argc, char **argv) {
    struct utfhate_options options;

    utfhate_set_default_options(&options);
    
    if (utfhate_process_arguments(argc - 1, &argv[1], &options) != 0) {
        utfhate_print_usage(*argv);
        
        return -1;
    }
    
    switch(options.command) {
        case UTFHATE_COMMAND_INDEX_SEARCH:
            return utfhate_command_search(&options);
            
        case UTFHATE_COMMAND_INDEX_DELETE:
            return utfhate_command_delete(&options);
            
        case UTFHATE_COMMAND_INDEX_REPLACE:
            return utfhate_command_replace(&options);
            
        case UTFHATE_COMMAND_INDEX_COUNT:
            return utfhate_command_count(&options);
            
        default:
            break;
    }

    return 0;
}

static void utfhate_set_default_options(struct utfhate_options *options) {
    options->command = UTFHATE_COMMAND_INDEX_SEARCH;
    options->verbose = UTFHATE_DEFAULT_VERBOSE;
    options->source_stream = stdin;
    options->destination_stream = stdout;
    
    return;
}

static void utfhate_print_usage(const char *application) {
    const struct utfhate_command_option *option;
    
    printf("Usage: %s [options] < inputfile > output\n", application);
    puts("Available options:");
    
    for (option = utfhate_command_list; option->name != NULL; ++option) {
        printf("\t%s,\t%s:\t%s\n", option->name, option->alternate, option->help_text);
    }
    
    return;
}

static int utfhate_process_arguments(int argc, char **argv, struct utfhate_options *options) {
    char **argv_end = &argv[argc];

    /* No arguments -> default options */
    if (argc <= 0) {
        return 0;
    }
    
    while (argv < argv_end) {
        char *argument = *argv;
        argv++;
        argc--;
        
        if (utfhate_match_argument(argument, &argc, &argv, options) != 0) {
            return -1;
        }        
    }
    
    return 0;
}

static int utfhate_match_argument(const char *argument, int *argc, char ***argv, struct utfhate_options *options) {
    const struct utfhate_command_option *option;
    
    for (option = utfhate_command_list; option->name != NULL; ++option) {
        if (strcmp(option->name, argument) == 0 || strcmp(option->alternate, argument) == 0) {
            return option->handler(argc, argv, options);
        }
    }
    
    printf("Unrecognised option: %s\n", argument);
    
    return -1;
}

static int utfhate_command_option_search(int *arguments, char ***pargv, struct utfhate_options *options) {
    UTFHATE_UNUSED(arguments);
    UTFHATE_UNUSED(pargv);
    
    options->command = UTFHATE_COMMAND_INDEX_SEARCH;
    return 0;
}

static int utfhate_command_option_delete(int *arguments, char ***pargv, struct utfhate_options *options) {
    UTFHATE_UNUSED(arguments);
    UTFHATE_UNUSED(pargv);
    
    options->command = UTFHATE_COMMAND_INDEX_DELETE;
    return 0;
}

static int utfhate_command_option_replace(int *arguments, char ***pargv, struct utfhate_options *options) {
    if (*arguments < 1 || strlen(**pargv) != 1) {
        puts("Replace requires an argument specifying the replacement character");
        
        return -1;
    }
    
    options->arguments.as_replace.replacement = **pargv[0];
    options->command = UTFHATE_COMMAND_INDEX_REPLACE;
    
    (*pargv)++;
    (*arguments)--;
    
    return 0;
}

static int utfhate_command_option_count(int *arguments, char ***pargv, struct utfhate_options *options) {
    options->arguments.as_count.type = UTFHATE_COMMAND_COUNT_TYPE_CHARACTERS;
    
    if (*arguments > 0) {
        char * argument = **pargv;
        
        if (strcmp(argument, "chars") == 0) {
            options->arguments.as_count.type = UTFHATE_COMMAND_COUNT_TYPE_CHARACTERS;
            
            (*arguments)--;
            (*pargv)++;
        } else if (strcmp(argument, "bytes") == 0) {
            options->arguments.as_count.type = UTFHATE_COMMAND_COUNT_TYPE_BYTES;
            
            (*arguments)--;
            (*pargv)++;
        } else if (strcmp(argument, "both") == 0) {
            options->arguments.as_count.type = UTFHATE_COMMAND_COUNT_TYPE_BOTH;
            
            (*arguments)--;
            (*pargv)++;
        } else if (*argument != '-') {
            printf("Invalid argument '%s' for count. Supported modes are: 'chars', 'bytes' and 'both'\n", argument);
            
            return -1;
        }
    }
    
    options->command = UTFHATE_COMMAND_INDEX_COUNT;
    
    return 0;
}

static int utfhate_command_option_verbose(int *arguments, char ***pargv, struct utfhate_options *options)
{
    UTFHATE_UNUSED(arguments);
    UTFHATE_UNUSED(pargv);
    
    options->verbose = 1;
    
    return 0;
}

static int utfhate_command_search(const struct utfhate_options *options) {
    unsigned int line = 1, utf_total = 0;
    
    while (fgets(line_buffer, sizeof(line_buffer), options->source_stream) != NULL) {
        char *offset, *marker_offset = scratch_buffer;
        unsigned int utf_found = 0;
        
        for (offset = line_buffer; *offset != '\0'; ++offset, ++marker_offset) {
            if (*offset == '\n') {
                line++;
                *offset = '\0';
                *marker_offset++ = '\n';
                
                break;
            }
            
            if ((unsigned char)*offset > 0x80) {
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
            
            fprintf(options->destination_stream, "Line %u, %u occurence(s):\n", line, utf_found);
            fputs(line_buffer, options->destination_stream);
            putc('\n', options->destination_stream);
            fputs(scratch_buffer, options->destination_stream);
        }
    }
    
    if (options->verbose != 0) {
        fprintf(options->destination_stream, "UTF-8 characters found: %u\n", utf_total);
    }
    
    return 0;
}

static int utfhate_command_delete(const struct utfhate_options *options) {
    while (fgets(line_buffer, sizeof(line_buffer), options->source_stream) != NULL) {
        char *offset, *output_offset = scratch_buffer;
        
        for (offset = line_buffer; *offset != '\0'; ++offset, ++output_offset) {
            if ((unsigned char)*offset > 0x80) {
                if (utfhate_consume_utf_char(&offset) != 0)
                    break;
                
                continue;
            }
            
            *output_offset = *offset;
        }
        
        *output_offset = '\0';
        fputs(scratch_buffer, options->destination_stream);
    }
    
    return 0;
}

static int utfhate_command_replace(const struct utfhate_options *options) {
    while (fgets(line_buffer, sizeof(line_buffer), options->source_stream) != NULL) {
        char *offset, *output_offset = scratch_buffer;
        
        for (offset = line_buffer; *offset != '\0'; ++offset, ++output_offset) {
            if ((unsigned char)*offset > 0x80) {
                *output_offset = options->arguments.as_replace.replacement;
                
                if (utfhate_consume_utf_char(&offset) != 0)
                    break;
                
                continue;
            }
            
            *output_offset = *offset;
        }
        
        *output_offset = '\0';
        
        fputs(scratch_buffer, options->destination_stream);
    }
    
    return 0;
}

static int utfhate_command_count(const struct utfhate_options *options) {
    unsigned int utf_chars = 0, utf_bytes = 0;
    
    while (fgets(line_buffer, sizeof(line_buffer), options->source_stream) != NULL) {
        char *offset;
        
        for (offset = line_buffer; *offset != '\0'; ++offset) {
            if ((unsigned char)*offset > 0x80) {
                char * old_offset = offset;
                
                if (utfhate_consume_utf_char(&offset) != 0)
                    break;
                
                utf_bytes += (unsigned int)(offset - old_offset) + 1;
                utf_chars++;
            }
        }
    }
    
    if (options->verbose != 0) {
        if (options->arguments.as_count.type & UTFHATE_COMMAND_COUNT_TYPE_BYTES)
            fprintf(options->destination_stream, "UTF-8 Bytes: %u\n", utf_bytes);
        
        if (options->arguments.as_count.type & UTFHATE_COMMAND_COUNT_TYPE_CHARACTERS)
            fprintf(options->destination_stream, "UTF-8 Characters: %u\n", utf_chars);
        
    } else {
        if (options->arguments.as_count.type & UTFHATE_COMMAND_COUNT_TYPE_BYTES)
            fprintf(options->destination_stream, "%u\n", utf_bytes);
        
        if (options->arguments.as_count.type & UTFHATE_COMMAND_COUNT_TYPE_CHARACTERS)
            fprintf(options->destination_stream, "%u\n", utf_chars);
        
    }
    
    return 0;
}

static int utfhate_consume_utf_char(char **pbuffer) {
    unsigned char *buffer = (unsigned char *)*pbuffer;
    unsigned int expected_length = 1;

    if (*buffer >= 0xC0 && *buffer < 0xE0) {
        expected_length = 2;
    } else if (*buffer >= 0xE0 && *buffer < 0xF0) {
        expected_length = 3;
    } else if (*buffer >= 0xF0 && *buffer < 0xF8) {
        expected_length = 4;
    } else if (*buffer >= 0xF8 && *buffer < 0xFC) {
        expected_length = 5;
    } else if (*buffer >= 0xFC) {
        expected_length = 6;
    }
    
    for (; *buffer != '\0'; ++buffer) {
        if (*buffer < 0x80 || expected_length == 0) {
            *pbuffer = (char *)buffer - 1;
            
            return 0;
        }
        
        --expected_length;
    }
    
    return -1;
}
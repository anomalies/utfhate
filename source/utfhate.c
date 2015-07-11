/*
 * utfhate - Utility to find and mark instances of UTF-8 characters in the input stream.
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include <stdio.h>
#include <string.h>

#define UTFHATE_BUFFER_SIZE 4096
#define UTFHATE_UNUSED(arg) (void)arg

enum utfhate_command_index {
    UTFHATE_COMMAND_INDEX_SEARCH = 0,
    UTFHATE_COMMAND_INDEX_DELETE,
    UTFHATE_COMMAND_INDEX_REPLACE,
    UTFHATE_COMMAND_INDEX_COUNT
};

struct utfhate_options {
    enum utfhate_command_index command;
    
    FILE * source_stream;
    FILE * destination_stream;
    
    union {
        struct {
            const char * replacement;
        } as_replace;
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

static int utfhate_command_search(const struct utfhate_options *options);
static int utfhate_command_delete(const struct utfhate_options *options);
static int utfhate_command_replace(const struct utfhate_options *options);
static int utfhate_command_count(const struct utfhate_options *options);

static int utfhate_consume_utf_char(char **pbuffer);

static const struct utfhate_command_option utfhate_command_list[] = {
    { "--search",  "-s", &utfhate_command_option_search,  "Searches for UTF-8 characters and, if found, marks their location."               },
    { "--delete",  "-d", &utfhate_command_option_delete,  "Deletes all UTF-8 characters found in the input (without destroying the source.)" },
    { "--replace", "-r", &utfhate_command_option_replace, "Replaces all UTF-8 characters with a specified value."                            }, 
    { "--count",   "-c", &utfhate_command_option_count,   "Counts the number of UTF-8 characters in the input."                              },
    
    { NULL, NULL, NULL, NULL }
};

static char line_buffer[UTFHATE_BUFFER_SIZE];
static char marker_buffer[UTFHATE_BUFFER_SIZE];

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
    options->source_stream = stdin;
    options->destination_stream = stdout;
    
    return;
}

static void utfhate_print_usage(const char *application) {
    const struct utfhate_command_option *option;
    
    printf("Usage: %s [options] < inputfile > output\n", application);
    puts("Available options:");
    
    for(option = utfhate_command_list; option->name != NULL; ++option) {
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
    
    for(option = utfhate_command_list; option->name != NULL; ++option) {
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
    if (*arguments < 1) {
        puts("Replace requires an argument specifying the replacement string");
        
        return -1;
    }
    
    options->arguments.as_replace.replacement = **pargv;
    options->command = UTFHATE_COMMAND_INDEX_REPLACE;
    
    (*pargv)++;
    (*arguments)--;
    
    return 0;
}

static int utfhate_command_option_count(int *arguments, char ***pargv, struct utfhate_options *options) {
    UTFHATE_UNUSED(arguments);
    UTFHATE_UNUSED(pargv);
    
    options->command = UTFHATE_COMMAND_INDEX_COUNT;
    
    return 0;
}

static int utfhate_command_search(const struct utfhate_options *options) {
    unsigned int line = 1, utf_total = 0;
    
    while (fgets(line_buffer, sizeof(line_buffer), options->source_stream) != NULL) {
        char *offset, *marker_offset = marker_buffer;
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
            
            fprintf(options->destination_stream, "Line %u, %u occurence(s):\n", line, utf_found);
            fputs(line_buffer, options->destination_stream);
            putc('\n', options->destination_stream);
            fputs(marker_buffer, options->destination_stream);
            putc('\n', options->destination_stream);
        }
    }
    
    fprintf(options->destination_stream, "UTF-8 characters found: %u\n", utf_total);
    
    return 0;
}

static int utfhate_command_delete(const struct utfhate_options *options) {
    UTFHATE_UNUSED(options);
    return 0;
}

static int utfhate_command_replace(const struct utfhate_options *options) {
    UTFHATE_UNUSED(options);
    return 0;
}

static int utfhate_command_count(const struct utfhate_options *options) {
    UTFHATE_UNUSED(options);
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
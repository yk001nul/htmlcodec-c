#ifndef CMDLINE_H
#define CMDLINE_H

#include <stdio.h>
#include <stdlib.h>

/* Parsed command-line options */
typedef struct {
    const char* text;        /* raw text or file path (required) */
    int         file_flag;   /* -f: treat text as file path */
    int         decode_flag; /* -d: decode mode */
    char        format[8];   /* -l: "HTML", "CSS", "JS", "EN" (default "EN") */
    int         ascii_flag;  /* -a: raw ASCII output instead of hexdump */
    const char* outputpath;  /* output file path (NULL = stdout) */
} CmdlineOpts;

/*
 * Parse argc/argv into opts.
 * Returns 0 on success, non-zero on parse error.
 * On error a message is printed to stderr.
 */
int parse_cmdline_opts(int argc, char* argv[], CmdlineOpts* opts);

/*
 * Encode or decode the input described by opts.
 * On success: returns 0, allocates *out_data (caller must free), sets *out_len.
 * On error: returns non-zero, *out_data is NULL.
 */
int process_opts(const CmdlineOpts* opts, unsigned char** out_data, size_t* out_len);

/* Print a hexdump of data to fp in the format:
 *   00000000: 48 65 6c 6c 6f  Hello */
void print_hexdump_to(FILE* fp, const unsigned char* data, size_t len);

/* Print usage guide to stdout */
void print_usage(const char* progname);

#endif /* CMDLINE_H */

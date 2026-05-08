#include "cmdline.h"
#include "nl-en-tokenizer.h"
#include "nl-en-codec.h"
#include "html-tokenizer.h"
#include "html-codec.h"
#include "css-tokenizer.h"
#include "css-codec.h"
#include "cl-javascript-en-tokenizer.h"
#include "cl-javascript-codec.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ── internal helpers ──────────────────────────────────────────────────────── */

static char* read_file_bytes(const char* path, size_t* len_out) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NULL; }
    long sz = ftell(fp);
    if (sz < 0) { fclose(fp); return NULL; }
    rewind(fp);
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(fp); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[n] = '\0';
    if (len_out) *len_out = n;
    return buf;
}

/*
 * Parse a hex string (hex digits only; spaces, colons, and newlines ignored)
 * into a raw byte buffer.  Returns NULL on failure.
 */
static unsigned char* parse_hex_bytes(const char* hex, size_t* out_len) {
    size_t n = strlen(hex);
    unsigned char* buf = (unsigned char*)malloc(n / 2 + 1);
    if (!buf) return NULL;
    size_t bi = 0;
    int nibble_count = 0;
    unsigned char current = 0;
    for (size_t i = 0; i < n; i++) {
        char c = hex[i];
        unsigned char val;
        if (c >= '0' && c <= '9') val = (unsigned char)(c - '0');
        else if (c >= 'a' && c <= 'f') val = (unsigned char)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val = (unsigned char)(c - 'A' + 10);
        else continue; /* skip spaces, colons, addresses, printable section */
        current = (unsigned char)((current << 4) | val);
        nibble_count++;
        if (nibble_count == 2) {
            buf[bi++] = current;
            current = 0;
            nibble_count = 0;
        }
    }
    *out_len = bi;
    return buf;
}

/* ── public API ────────────────────────────────────────────────────────────── */

void print_usage(const char* progname) {
    printf("Usage: %s text [-f] [-d] [-l format] [-a] [outputpath]\n\n", progname);
    printf("  text        Raw text to encode/decode, or a file path when -f is used\n");
    printf("  -f          Treat 'text' as a file path and read its contents\n");
    printf("  -d          Decode mode (default: encode)\n");
    printf("  -l format   Language/codec: HTML, CSS, JS, EN (default: EN)\n");
    printf("  -a          Output raw ASCII bytes instead of a hexdump\n");
    printf("              WARNING: may cause unexpected terminal output without outputpath\n");
    printf("  outputpath  Write result to this file instead of the console\n\n");
    printf("Examples:\n");
    printf("  %s \"Hello world\"                   EN encode, hexdump to console\n", progname);
    printf("  %s mypage.html -f -l HTML           HTML encode file, hexdump\n", progname);
    printf("  %s \"4865...\" -d                    EN decode hex string to console\n", progname);
    printf("  %s input.css -f -l CSS -a out.bin   CSS encode file, raw bytes to file\n", progname);
}

void print_hexdump_to(FILE* fp, const unsigned char* data, size_t len) {
    for (size_t i = 0; i < len; i += 16) {
        fprintf(fp, "%08x: ", (unsigned int)i);
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) fprintf(fp, "%02x ", data[i + j]);
            else              fprintf(fp, "   ");
            if (j == 7) fprintf(fp, " ");
        }
        fprintf(fp, " |");
        for (size_t j = 0; j < 16 && i + j < len; j++) {
            unsigned char c = data[i + j];
            fprintf(fp, "%c", (c >= 32 && c < 127) ? (char)c : '.');
        }
        fprintf(fp, "|\n");
    }
}

int parse_cmdline_opts(int argc, char* argv[], CmdlineOpts* opts) {
    if (!opts) return -1;

    opts->text        = NULL;
    opts->file_flag   = 0;
    opts->decode_flag = 0;
    opts->format[0]   = '\0';
    strncpy(opts->format, "EN", sizeof(opts->format) - 1);
    opts->format[sizeof(opts->format) - 1] = '\0';
    opts->ascii_flag  = 0;
    opts->outputpath  = NULL;

    int positional = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            opts->file_flag = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            opts->decode_flag = 1;
        } else if (strcmp(argv[i], "-a") == 0) {
            opts->ascii_flag = 1;
        } else if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -l requires a format argument (HTML, CSS, JS, EN)\n");
                return -1;
            }
            i++;
            const char* fmt = argv[i];
            if (strcmp(fmt, "HTML") != 0 && strcmp(fmt, "CSS") != 0 &&
                strcmp(fmt, "JS")   != 0 && strcmp(fmt, "EN")  != 0) {
                fprintf(stderr, "Error: unknown format '%s'. Valid formats: HTML, CSS, JS, EN\n", fmt);
                return -1;
            }
            strncpy(opts->format, fmt, sizeof(opts->format) - 1);
            opts->format[sizeof(opts->format) - 1] = '\0';
        } else {
            if (positional == 0) opts->text = argv[i];
            else if (positional == 1) opts->outputpath = argv[i];
            positional++;
        }
    }

    if (!opts->text) return -1;
    return 0;
}

int process_opts(const CmdlineOpts* opts, unsigned char** out_data, size_t* out_len) {
    if (!opts || !out_data || !out_len) return -1;
    *out_data = NULL;
    *out_len  = 0;

    char* input = NULL;
    size_t input_len = 0;
    int input_heap = 0; /* 1 = must free(input) */

    if (opts->file_flag) {
        input = read_file_bytes(opts->text, &input_len);
        if (!input) {
            fprintf(stderr, "Error: cannot read file '%s'\n", opts->text);
            return -1;
        }
        input_heap = 1;
    } else {
        input = (char*)opts->text;
        input_len = strlen(input);
    }

    int result = 0;

    if (!opts->decode_flag) {
        /* ── ENCODE ──────────────────────────────────────────────────── */
        if (strcmp(opts->format, "EN") == 0) {
            NLTokenArray* tok = tokenizeEnglishOpt(input);
            if (!tok) { result = -1; goto cleanup; }
            *out_data = nl_en_encode_opt(tok, tok->count, out_len);
            freeNLTokenArray(tok);

        } else if (strcmp(opts->format, "HTML") == 0) {
            HTMLTokenArray* tok = parseHTML(input);
            if (!tok) { result = -1; goto cleanup; }
            enrichHTMLTokenSubdata(tok);
            *out_data = html_encode_ae_opt(tok, (size_t)tok->count, out_len);
            freeHTMLTokenArray(tok);

        } else if (strcmp(opts->format, "CSS") == 0) {
            CSSTokenArray* tok = parseCSS(input);
            if (!tok) { result = -1; goto cleanup; }
            *out_data = css_encode_opt(tok, (size_t)tok->count, out_len);
            freeCSS(tok);

        } else if (strcmp(opts->format, "JS") == 0) {
            CLJSTokenArray* tok = tokenizeJavaScript(input);
            if (!tok) { result = -1; goto cleanup; }
            *out_data = cljs_encode_ae_opt(tok, tok->count, out_len);
            freeCLJSTokenArray(tok);
        }

    } else {
        /* ── DECODE ──────────────────────────────────────────────────── */
        unsigned char* bin = NULL;
        size_t bin_len = 0;
        int bin_heap = 0;

        if (opts->file_flag) {
            bin     = (unsigned char*)input;
            bin_len = input_len;
        } else {
            bin = parse_hex_bytes(input, &bin_len);
            if (!bin) {
                fprintf(stderr, "Error: invalid hex input\n");
                result = -1;
                goto cleanup;
            }
            bin_heap = 1;
        }

        char* decoded = NULL;
        int   dec_len = 0;

        if (strcmp(opts->format, "EN") == 0) {
            /* init NL patterns before detokenizing */
            NLTokenArray* init = tokenizeEnglishOpt("");
            if (init) freeNLTokenArray(init);

            NLTokenArray* tok = nl_en_decode_opt(bin, bin_len);
            if (!tok) { result = -1; }
            else {
                decoded = detokenizeNLTokenArray(tok, &dec_len);
                freeNLTokenArray(tok);
            }

        } else if (strcmp(opts->format, "HTML") == 0) {
            /* patterns initialized inside html_decode_ae_opt path */
            NLTokenArray* init = tokenizeEnglishOpt("");
            if (init) freeNLTokenArray(init);

            HTMLTokenArray* tok = html_decode_ae_opt(bin, bin_len);
            if (!tok) { result = -1; }
            else {
                decoded = detokenizeHTMLTokenArray(tok, &dec_len);
                freeHTMLTokenArray(tok);
            }

        } else if (strcmp(opts->format, "CSS") == 0) {
            CSSTokenArray* tok = css_decode_opt(bin, bin_len);
            if (!tok) { result = -1; }
            else {
                decoded = detokenizeCSSTokenArray(tok, &dec_len);
                freeCSS(tok);
            }

        } else if (strcmp(opts->format, "JS") == 0) {
            /* init CLJS patterns */
            CLJSTokenArray* init = tokenizeJavaScript("");
            if (init) freeCLJSTokenArray(init);

            CLJSTokenArray* tok = cljs_decode_ae_opt(bin, bin_len);
            if (!tok) { result = -1; }
            else {
                decoded = detokenizeCLJSTokenArray(tok, &dec_len);
                freeCLJSTokenArray(tok);
            }
        }

        if (bin_heap) free(bin);

        if (result == 0 && decoded) {
            *out_data = (unsigned char*)decoded;
            *out_len  = (size_t)dec_len;
        } else if (result == 0) {
            /* decode produced NULL — treat as empty success */
            *out_data = (unsigned char*)calloc(1, 1);
            *out_len  = 0;
        }
    }

cleanup:
    if (input_heap) free(input);
    return result;
}

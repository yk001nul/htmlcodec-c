// htmlcodec-c.c : command-line entry point

#include "htmlcodec-c.h"
#include "cmdline.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    CmdlineOpts opts;
    if (parse_cmdline_opts(argc, argv, &opts) != 0) {
        fprintf(stderr, "Run without arguments to see usage.\n");
        return 1;
    }

    unsigned char* out_data = NULL;
    size_t         out_len  = 0;

    if (process_opts(&opts, &out_data, &out_len) != 0) {
        return 1;
    }

    if (opts.outputpath) {
        FILE* fp = fopen(opts.outputpath, "wb");
        if (!fp) {
            fprintf(stderr, "Error: cannot open output file '%s'\n", opts.outputpath);
            free(out_data);
            return 1;
        }
        if (opts.ascii_flag || opts.decode_flag) {
            fwrite(out_data, 1, out_len, fp);
        } else {
            print_hexdump_to(fp, out_data, out_len);
        }
        fclose(fp);
    } else {
        if (opts.ascii_flag || opts.decode_flag) {
            fwrite(out_data, 1, out_len, stdout);
        } else {
            print_hexdump_to(stdout, out_data, out_len);
        }
    }

    free(out_data);
    return 0;
}

#include "html-codec.h"
#include "nl-en-codec.h"
#include "css-codec.h"
#include "cl-javascript-codec.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Codebook — three segments
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Segment 1: Standard HTML tags (112 entries, indices 0..111) */
static const char* s_tags[] = {
    "a", "abbr", "address", "area", "article", "aside", "audio",
    "b", "base", "bdi", "bdo", "blockquote", "body", "br", "button",
    "canvas", "caption", "cite", "code", "col", "colgroup",
    "data", "datalist", "dd", "del", "details", "dfn", "dialog", "div", "dl", "dt",
    "em", "embed",
    "fieldset", "figcaption", "figure", "footer", "form",
    "h1", "h2", "h3", "h4", "h5", "h6",
    "head", "header", "hgroup", "hr", "html",
    "i", "iframe", "img", "input", "ins",
    "kbd",
    "label", "legend", "li", "link",
    "main", "map", "mark", "menu", "meta", "meter",
    "nav", "noscript",
    "object", "ol", "optgroup", "option", "output",
    "p", "picture", "pre", "progress",
    "q",
    "rp", "rt", "ruby",
    "s", "samp", "script", "search", "section", "select", "slot", "small",
    "source", "span", "strong", "style", "sub", "summary", "sup",
    "table", "tbody", "td", "template", "textarea", "tfoot", "th", "thead",
    "time", "title", "tr", "track",
    "u", "ul",
    "var", "video",
    "wbr"
};
#define TAG_COUNT 112

/* Segment 2: Standard HTML attributes (118 entries, indices 112..229)
 * Note: names that also exist as HTML tags (cite, data, form, label, slot, span,
 * style, title) are replaced to keep the codebook duplicate-free. */
static const char* s_attrs[] = {
    "accept", "accept-charset", "accesskey", "action", "allow", "allowfullscreen",
    "alt", "as", "async", "autocapitalize", "autocomplete", "autofocus", "autoplay",
    "capture", "charset", "checked", "class", "color", "cols", "colspan",
    "content", "contenteditable", "controls", "coords", "crossorigin",
    "datetime", "decoding", "default", "defer", "dir", "dirname", "disabled",
    "download", "draggable",
    "enctype", "enterkeyhint", "exportparts",
    "fetchpriority", "for", "formaction", "formenctype", "formmethod",
    "formnovalidate", "formtarget",
    "headers", "height", "hidden", "high", "href", "hreflang", "http-equiv",
    "id", "imagesizes", "imagesrcset", "inert", "inputmode", "integrity", "is", "ismap", "itemscope",
    "kind",
    "lang", "list", "loading", "loop", "low",
    "max", "maxlength", "media", "method", "min", "minlength", "multiple", "muted",
    "name", "nomodule", "nonce", "novalidate",
    "open", "optimum",
    "part", "pattern", "ping", "placeholder", "playsinline",
    "popover", "popovertarget", "popovertargetaction", "poster", "preload",
    "readonly", "referrerpolicy", "rel", "required", "reversed", "rows", "rowspan",
    "sandbox", "scope", "selected", "shape", "size", "sizes",
    "spellcheck", "src", "srcdoc", "srclang", "srcset", "start", "step",
    "tabindex", "target", "translate", "type",
    "usemap",
    "value",
    "width", "wrap"
};
#define ATTR_COUNT 118
#define ATTR_START TAG_COUNT
#define MIME_START (TAG_COUNT + ATTR_COUNT)

/* Segment 3: Top 255 MIME types (indices 230..484) */
static const char* s_mimes[] = {
    "text/html", "text/plain", "text/css", "text/javascript",
    "application/json", "application/xml", "application/pdf",
    "image/jpeg", "image/png", "image/gif", "image/webp", "image/svg+xml",
    "image/x-icon", "image/avif", "image/bmp", "image/tiff",
    "audio/mpeg", "audio/ogg", "audio/wav", "audio/webm", "audio/aac",
    "audio/flac", "audio/midi", "audio/x-wav", "audio/x-aiff", "audio/x-m4a",
    "video/mp4", "video/ogg", "video/webm", "video/mpeg", "video/mp2t",
    "video/x-flv", "video/x-m4v", "video/x-msvideo", "video/quicktime",
    "video/3gpp", "video/3gpp2", "video/x-ms-wmv", "video/x-ms-asf",
    "application/octet-stream", "application/zip", "application/gzip",
    "application/x-www-form-urlencoded", "multipart/form-data",
    "application/javascript", "application/xhtml+xml",
    "application/atom+xml", "application/rss+xml",
    "application/ld+json", "application/manifest+json", "application/wasm",
    "application/x-ndjson", "application/x-yaml", "text/yaml",
    "application/x-tar", "application/x-zip-compressed",
    "application/x-7z-compressed", "application/x-rar-compressed",
    "application/x-bzip", "application/x-bzip2",
    "application/java-archive", "application/x-shockwave-flash",
    "application/x-httpd-php", "application/x-perl", "application/x-python-code",
    "application/x-dvi", "application/x-latex", "application/postscript",
    "application/x-msdownload", "application/x-dosexec",
    "application/x-apple-diskimage",
    "application/vnd.android.package-archive",
    "application/x-debian-package", "application/x-rpm",
    "application/msword",
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    "application/vnd.ms-excel",
    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    "application/vnd.ms-powerpoint",
    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
    "application/vnd.oasis.opendocument.text",
    "application/vnd.oasis.opendocument.spreadsheet",
    "application/vnd.oasis.opendocument.presentation",
    "application/vnd.google-earth.kml+xml",
    "application/vnd.google-earth.kmz",
    "font/woff", "font/woff2", "font/ttf", "font/otf", "font/eot",
    "application/x-font-ttf", "application/x-font-woff",
    "model/gltf+json", "model/gltf-binary", "model/obj",
    "text/xml", "text/csv", "text/calendar", "text/markdown", "text/rtf",
    "text/vcard", "text/x-vcard", "text/vtt", "text/event-stream",
    "text/tab-separated-values", "text/cache-manifest",
    "text/x-python", "text/x-java-source", "text/x-c", "text/x-c++",
    "text/x-shellscript", "text/x-sql", "text/x-log",
    "image/x-png", "image/x-bitmap", "image/x-xcf", "image/x-psd",
    "application/cache-manifest",
    "audio/x-midi", "application/pkcs8", "application/pkcs10",
    "application/pkcs7-mime", "application/x-pkcs12",
    "application/x-x509-ca-cert", "application/x-x509-user-cert",
    "application/pgp-signature", "application/pgp-encrypted",
    "message/rfc822", "message/partial",
    "multipart/mixed", "multipart/alternative", "multipart/related",
    "multipart/signed", "multipart/encrypted",
    "application/sql", "application/graphql",
    "application/x-protobuf", "application/protobuf",
    "application/msgpack", "application/cbor",
    "application/jose", "application/jose+json",
    "application/jwt", "application/json-patch+json",
    "application/merge-patch+json", "application/problem+json",
    "application/vnd.api+json", "application/hal+json",
    "text/html; charset=utf-8", "text/plain; charset=utf-8",
    "application/json; charset=utf-8",
    "font/collection", "application/x-font-opentype",
    "application/x-font-pcf", "application/x-font-snf",
    "application/x-font-bdf",
    "chemical/x-pdb", "chemical/x-xyz",
    "application/vnd.mozilla.xul+xml",
    "application/vnd.ms-fontobject",
    "application/vnd.ms-cab-compressed",
    "application/vnd.ms-htmlhelp",
    "application/vnd.ms-works",
    "application/vnd.lotus-1-2-3",
    "application/vnd.stardivision.writer",
    "application/vnd.stardivision.calc",
    "application/vnd.sun.xml.writer",
    "application/vnd.sun.xml.calc",
    "application/vnd.kde.kword",
    "application/vnd.kde.kspread",
    "application/vnd.palm",
    "application/vnd.wap.wbxml",
    "application/vnd.wap.wmlc",
    "text/vnd.wap.wml",
    "application/x-nes-rom", "application/x-genesis-rom",
    "application/x-snes-rom", "application/x-gameboy-rom",
    "application/x-gba-rom", "application/x-n64-rom",
    "application/x-stuffit", "application/x-stuffitx",
    "application/x-lzh-compressed", "application/x-lzma",
    "application/x-xz", "application/x-compress",
    "application/zstd", "application/x-snappy-framed",
    "application/x-lz4", "application/x-zstd",
    "video/x-matroska", "video/x-theora",
    "audio/x-speex", "audio/x-opus",
    "image/jp2", "image/jpx", "image/jpm",
    "image/x-portable-bitmap", "image/x-portable-graymap",
    "image/x-portable-pixmap", "image/x-portable-anymap",
    "image/x-rgb", "image/x-sgi",
    "application/x-cpio", "application/x-shar",
    "application/x-sv4cpio", "application/x-sv4crc",
    "application/x-ustar",
    "application/vnd.rn-realmedia",
    "audio/vnd.rn-realaudio", "video/vnd.rn-realvideo",
    "application/x-troff", "application/x-troff-man",
    "application/x-troff-me",
    "text/x-setext", "text/x-uuencode",
    "application/dsptype", "application/x-gtar",
    "application/x-wais-source",
    "application/x-netcdf", "application/x-hdf",
    "audio/basic", "audio/x-pn-realaudio",
    "video/x-sgi-movie",
    "application/x-director",
    "application/x-authorware-bin",
    "application/x-authorware-map",
    "application/x-authorware-seg",
    "application/dxf",
    "application/x-mif",
    "text/coffeescript",
    "text/typescript",
    "application/x-www-form-urlencoded; charset=utf-8",
    "image/svg+xml; charset=utf-8",
    "audio/ogg; codecs=opus",
    "application/vnd.lotus-wordpro",
    "application/vnd.stardivision.draw",
    "application/vnd.stardivision.impress",
    "application/vnd.sun.xml.draw",
    "application/vnd.sun.xml.impress",
    "application/vnd.kde.kpresenter",
    "application/vnd.kde.kivio",
    "application/vnd.kde.karbon",
    "application/vnd.handheld-entertainment+xml",
    "application/vnd.wap.wmlscriptc",
    "text/vnd.wap.wmlscript",
    "application/ld+json; charset=utf-8",
    "application/x-ndjson; charset=utf-8",
    "application/geo+json",
    "application/schema+json",
    "application/vnd.geo+json",
    "text/x-rst",
    "text/x-kotlin",
    "text/x-swift",
    "application/x-ms-application"
};
#define MIME_COUNT 255
#define CODEBOOK_SIZE (TAG_COUNT + ATTR_COUNT + MIME_COUNT)

#define FLAG_RAW CODEBOOK_SIZE

const int HTML_CODEBOOK_TAG_COUNT  = TAG_COUNT;
const int HTML_CODEBOOK_ATTR_START = ATTR_START;
const int HTML_CODEBOOK_ATTR_COUNT = ATTR_COUNT;
const int HTML_CODEBOOK_MIME_START = MIME_START;
const int HTML_CODEBOOK_MIME_COUNT = MIME_COUNT;
const int HTML_CODEBOOK_SIZE       = CODEBOOK_SIZE;

static const char* s_flat[CODEBOOK_SIZE];
static int s_init = 0;

static void codebook_init(void) {
    if (s_init) return;
    for (int i = 0; i < TAG_COUNT; i++)  s_flat[i]            = s_tags[i];
    for (int i = 0; i < ATTR_COUNT; i++) s_flat[ATTR_START+i]  = s_attrs[i];
    for (int i = 0; i < MIME_COUNT; i++) s_flat[MIME_START+i]  = s_mimes[i];
    s_init = 1;
}

const char* HTML_CODEBOOK[CODEBOOK_SIZE];

void html_codebook_init(void) {
    if (s_init) return;
    codebook_init();
    for (int i = 0; i < CODEBOOK_SIZE; i++) HTML_CODEBOOK[i] = s_flat[i];
}

static int find_tag(const char* name) {
    for (int i = 0; i < TAG_COUNT; i++)
        if (strcmp(s_tags[i], name) == 0) return i;
    return FLAG_RAW;
}

static int find_attr(const char* name) {
    for (int i = 0; i < ATTR_COUNT; i++)
        if (strcmp(s_attrs[i], name) == 0) return ATTR_START + i;
    return FLAG_RAW;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Bit I/O helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

static void set_bit(unsigned char* buf, size_t pos, unsigned char v) {
    if (v) buf[pos/8] |=  (unsigned char)(1u << (pos%8));
    else   buf[pos/8] &= (unsigned char)~(1u << (pos%8));
}
static unsigned char get_bit(const unsigned char* buf, size_t pos) {
    return (buf[pos/8] >> (pos%8)) & 1u;
}
static void set_bits(unsigned char* buf, size_t s, size_t n, unsigned int v) {
    for (size_t i = 0; i < n; i++)
        set_bit(buf, s+i, (unsigned char)((v>>i)&1u));
}
static unsigned int get_bits(const unsigned char* buf, size_t s, size_t n) {
    unsigned int r = 0;
    for (size_t i = 0; i < n; i++) r |= (unsigned int)get_bit(buf, s+i) << i;
    return r;
}

static void write_bytes_bp(unsigned char* buf, size_t* bp,
                            const unsigned char* src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        set_bits(buf, *bp, 8, (unsigned int)src[i]);
        *bp += 8;
    }
}
static int read_bytes_bp(const unsigned char* buf, size_t* bp, size_t total_bits,
                          unsigned char* dst, size_t len) {
    if (*bp + len * 8 > total_bits) return 0;
    for (size_t i = 0; i < len; i++) {
        dst[i] = (unsigned char)get_bits(buf, *bp, 8);
        *bp += 8;
    }
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Step 4 — Codebook AE helpers (WNC-style renormalized arithmetic coding)
 * ═══════════════════════════════════════════════════════════════════════════ */

#define CBAE_SCALE 65536u
#define CBAE_TOP   0x80000000u
#define CBAE_QRTR  0x40000000u

static void cbae_emit(unsigned char* buf, size_t* bp, unsigned int bit) {
    set_bit(buf, *bp, (unsigned char)(bit & 1u));
    (*bp)++;
}

static void cbae_renorm_enc(uint32_t* lo, uint32_t* hi, int* pend,
                             unsigned char* buf, size_t* bp) {
    while (1) {
        if (*hi < CBAE_TOP) {
            cbae_emit(buf, bp, 0);
            for (int k = 0; k < *pend; k++) cbae_emit(buf, bp, 1);
            *pend = 0;
            *lo = *lo << 1;
            *hi = (*hi << 1) | 1u;
        } else if (*lo >= CBAE_TOP) {
            cbae_emit(buf, bp, 1);
            for (int k = 0; k < *pend; k++) cbae_emit(buf, bp, 0);
            *pend = 0;
            *lo = (*lo - CBAE_TOP) << 1;
            *hi = (*hi - CBAE_TOP) << 1 | 1u;
        } else if (*lo >= CBAE_QRTR && *hi < (CBAE_TOP | CBAE_QRTR)) {
            (*pend)++;
            *lo = (*lo - CBAE_QRTR) << 1;
            *hi = (*hi - CBAE_QRTR) << 1 | 1u;
        } else {
            break;
        }
    }
}

static void cbae_flush_enc(uint32_t lo, int pend, unsigned char* buf, size_t* bp) {
    pend++;
    if (lo < CBAE_QRTR) {
        cbae_emit(buf, bp, 0);
        for (int k = 0; k < pend; k++) cbae_emit(buf, bp, 1);
    } else {
        cbae_emit(buf, bp, 1);
        for (int k = 0; k < pend; k++) cbae_emit(buf, bp, 0);
    }
}

static unsigned int cbae_read_bit(const unsigned char* buf, size_t pos, size_t total) {
    return (pos < total) ? (unsigned int)get_bit(buf, pos) : 0u;
}

static void cbae_renorm_dec(uint32_t* lo, uint32_t* hi, uint32_t* code,
                             const unsigned char* buf, size_t* bp, size_t total) {
    while (1) {
        if (*hi < CBAE_TOP) {
            *lo   = *lo << 1;
            *hi   = (*hi << 1) | 1u;
            *code = (*code << 1) | cbae_read_bit(buf, (*bp)++, total);
        } else if (*lo >= CBAE_TOP) {
            *lo   = (*lo - CBAE_TOP) << 1;
            *hi   = (*hi - CBAE_TOP) << 1 | 1u;
            *code = (*code - CBAE_TOP) << 1 | cbae_read_bit(buf, (*bp)++, total);
        } else if (*lo >= CBAE_QRTR && *hi < (CBAE_TOP | CBAE_QRTR)) {
            *lo   = (*lo - CBAE_QRTR) << 1;
            *hi   = (*hi - CBAE_QRTR) << 1 | 1u;
            *code = (*code - CBAE_QRTR) << 1 | cbae_read_bit(buf, (*bp)++, total);
        } else {
            break;
        }
    }
}

static void cbae_cum_bounds(const uint32_t* row, size_t vocab_size, size_t sym,
                             uint32_t* out_lo, uint32_t* out_hi) {
    uint32_t total = 0;
    for (size_t j = 0; j < vocab_size; j++) total += row[j];
    uint32_t cum = 0;
    for (size_t j = 0; j < vocab_size; j++) {
        uint32_t c = row[j];
        if (j == sym) {
            *out_lo  = (uint32_t)((uint64_t)cum * CBAE_SCALE / total);
            *out_hi  = (j + 1 == vocab_size)
                     ? CBAE_SCALE
                     : (uint32_t)((uint64_t)(cum + c) * CBAE_SCALE / total);
            return;
        }
        cum += c;
    }
    *out_lo = *out_hi = 0;
}

static int cbae_find_sym_for_scaled(const uint32_t* row, size_t vocab_size, uint32_t scaled) {
    uint32_t total = 0;
    for (size_t j = 0; j < vocab_size; j++) total += row[j];
    uint32_t cum = 0;
    for (size_t j = 0; j < vocab_size; j++) {
        uint32_t c  = row[j];
        uint32_t lo = (uint32_t)((uint64_t)cum * CBAE_SCALE / total);
        uint32_t hi = (j + 1 == vocab_size)
                    ? CBAE_SCALE
                    : (uint32_t)((uint64_t)(cum + c) * CBAE_SCALE / total);
        if (scaled >= lo && scaled < hi) return (int)j;
        cum += c;
    }
    return -1;
}

/* Encode n_syms codebook flag values using order-1 adaptive AE.
 * Returns heap-allocated AE payload buffer; *out_bytes set to size. */
static unsigned char* cb_ae_encode(const uint16_t* syms, size_t n_syms,
                                    const uint16_t* vocab, size_t vocab_size,
                                    size_t* out_bytes) {
    if (n_syms == 0 || vocab_size == 0) { *out_bytes = 0; return NULL; }

    size_t ae_bits = n_syms * 32 + 64;
    unsigned char* buf = (unsigned char*)calloc((ae_bits + 7) / 8, 1);
    if (!buf) { *out_bytes = 0; return NULL; }

    /* Count table: (vocab_size+1) rows × vocab_size cols, Laplace init */
    uint32_t* ct = (uint32_t*)malloc((vocab_size + 1) * vocab_size * sizeof(uint32_t));
    if (!ct) { free(buf); *out_bytes = 0; return NULL; }
    for (size_t k = 0; k < (vocab_size + 1) * vocab_size; k++) ct[k] = 1u;

    size_t bp   = 0;
    uint32_t lo = 0, hi = 0xFFFFFFFFu;
    int      pend = 0;
    size_t   ctx  = vocab_size; /* start-of-sequence sentinel */

    for (size_t i = 0; i < n_syms; i++) {
        int sym = -1;
        for (size_t j = 0; j < vocab_size; j++) {
            if (vocab[j] == syms[i]) { sym = (int)j; break; }
        }
        if (sym < 0) { free(ct); free(buf); *out_bytes = 0; return NULL; }

        uint32_t* row = ct + ctx * vocab_size;
        uint32_t s_lo, s_hi;
        cbae_cum_bounds(row, vocab_size, (size_t)sym, &s_lo, &s_hi);

        uint64_t range = (uint64_t)(hi - lo) + 1;
        hi = lo + (uint32_t)(range * s_hi / CBAE_SCALE) - 1;
        lo = lo + (uint32_t)(range * s_lo / CBAE_SCALE);

        cbae_renorm_enc(&lo, &hi, &pend, buf, &bp);
        row[(size_t)sym]++;
        ctx = (size_t)sym;
    }
    cbae_flush_enc(lo, pend, buf, &bp);
    free(ct);

    *out_bytes = (bp + 7) / 8;
    return buf;
}

/* Decode n_syms codebook flag values from AE payload buf.
 * Writes decoded flag values (not vocab indices) into out_syms.
 * Returns 1 on success, 0 on error. */
static int cb_ae_decode(const unsigned char* buf, size_t buf_bytes,
                         const uint16_t* vocab, size_t vocab_size,
                         size_t n_syms, uint16_t* out_syms) {
    if (!buf || buf_bytes == 0 || vocab_size == 0 || n_syms == 0) return 0;

    size_t total_bits = buf_bytes * 8;
    size_t bp = 0;

    uint32_t* ct = (uint32_t*)malloc((vocab_size + 1) * vocab_size * sizeof(uint32_t));
    if (!ct) return 0;
    for (size_t k = 0; k < (vocab_size + 1) * vocab_size; k++) ct[k] = 1u;

    /* Prime code register: read 32 bits MSB-first */
    uint32_t code = 0;
    for (int b = 31; b >= 0; b--)
        code |= (uint32_t)cbae_read_bit(buf, bp++, total_bits) << b;

    uint32_t lo = 0, hi = 0xFFFFFFFFu;
    size_t   ctx = vocab_size;

    for (size_t i = 0; i < n_syms; i++) {
        uint32_t* row    = ct + ctx * vocab_size;
        uint64_t  range  = (uint64_t)(hi - lo) + 1;
        uint32_t  scaled = (uint32_t)(((uint64_t)(code - lo + 1) * CBAE_SCALE - 1) / range);

        int sym = cbae_find_sym_for_scaled(row, vocab_size, scaled);
        if (sym < 0) { free(ct); return 0; }

        uint32_t s_lo, s_hi;
        cbae_cum_bounds(row, vocab_size, (size_t)sym, &s_lo, &s_hi);

        hi = lo + (uint32_t)(range * s_hi / CBAE_SCALE) - 1;
        lo = lo + (uint32_t)(range * s_lo / CBAE_SCALE);

        cbae_renorm_dec(&lo, &hi, &code, buf, &bp, total_bits);

        out_syms[i] = vocab[sym];
        row[(size_t)sym]++;
        ctx = (size_t)sym;
    }

    free(ct);
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Step 6 — VLC length fields
 * 1 byte (8 bits) for lengths 0–127; 2 bytes (16 bits) for lengths 128–16383
 * ═══════════════════════════════════════════════════════════════════════════ */

static void vlc_write(unsigned char* buf, size_t* bp, size_t len) {
    if (len < 128u) {
        set_bits(buf, *bp, 8, (unsigned int)len);
        *bp += 8;
    } else {
        set_bits(buf, *bp, 8, (unsigned int)((len & 0x7Fu) | 0x80u));
        *bp += 8;
        set_bits(buf, *bp, 8, (unsigned int)((len >> 7) & 0x7Fu));
        *bp += 8;
    }
}

static size_t vlc_read(const unsigned char* buf, size_t* bp, size_t total_bits) {
    if (*bp + 8u > total_bits) return 0;
    unsigned int b0 = get_bits(buf, *bp, 8);
    *bp += 8;
    if (!(b0 & 0x80u)) return (size_t)b0;
    if (*bp + 8u > total_bits) return 0;
    unsigned int b1 = get_bits(buf, *bp, 8);
    *bp += 8;
    return (size_t)((b0 & 0x7Fu) | ((b1 & 0x7Fu) << 7));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Step 2 — whitespace-only predicate
 * ═══════════════════════════════════════════════════════════════════════════ */

static int is_whitespace_only(const char* s) {
    if (!s || !s[0]) return 1;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return 0;
    }
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Step 3 — attribute-value dictionary
 * ═══════════════════════════════════════════════════════════════════════════ */

#define DICT_MAX     32
#define DICT_VAL_MAX 128   /* 7-bit length field → max 127 chars + NUL */

typedef struct {
    char val[DICT_VAL_MAX];
    int  freq;
    int  len;
} AttrFreqEntry;

#define FREQ_TABLE_MAX 512

static int build_attr_dict(const HTMLTokenArray* arr, size_t count,
                            char dict_vals[DICT_MAX][DICT_VAL_MAX]) {
    AttrFreqEntry* tbl = (AttrFreqEntry*)calloc(FREQ_TABLE_MAX, sizeof(AttrFreqEntry));
    if (!tbl) return 0;
    int tbl_count = 0;

    for (size_t t = 0; t < count; t++) {
        const HTMLToken* tok = &arr->tokens[t];
        if (tok->type == 0) continue;
        for (int a = 0; a < tok->data.tag.attrCount; a++) {
            const HTMLAttribute* attr = &tok->data.tag.attributes[a];
            if (attr->subdataType != HTML_SUBDATA_NONE) continue;
            size_t vl = strlen(attr->value);
            if (vl == 0 || vl >= (size_t)DICT_VAL_MAX) continue;

            int found = -1;
            for (int f = 0; f < tbl_count; f++) {
                if (strcmp(tbl[f].val, attr->value) == 0) { found = f; break; }
            }
            if (found >= 0) {
                tbl[found].freq++;
            } else if (tbl_count < FREQ_TABLE_MAX) {
                memcpy(tbl[tbl_count].val, attr->value, vl + 1);
                tbl[tbl_count].freq = 1;
                tbl[tbl_count].len  = (int)vl;
                tbl_count++;
            }
        }
    }

    int used[FREQ_TABLE_MAX];
    memset(used, 0, (size_t)tbl_count * sizeof(int));
    int dict_size = 0;
    for (int d = 0; d < DICT_MAX; d++) {
        int best = -1, best_score = 0;
        for (int f = 0; f < tbl_count; f++) {
            if (used[f] || tbl[f].freq <= 1) continue;
            int score = tbl[f].freq * tbl[f].len;
            if (score > best_score) { best_score = score; best = f; }
        }
        if (best < 0) break;
        used[best] = 1;
        memcpy(dict_vals[dict_size], tbl[best].val, (size_t)tbl[best].len + 1);
        dict_size++;
    }
    free(tbl);
    return dict_size;
}

static int dict_lookup(const char* val,
                        char dict_vals[DICT_MAX][DICT_VAL_MAX], int dict_size) {
    for (int i = 0; i < dict_size; i++)
        if (strcmp(dict_vals[i], val) == 0) return i;
    return -1;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * nl_tokens_to_string — reconstruct a text string from NLTokenArray
 * ═══════════════════════════════════════════════════════════════════════════ */

static void apply_case(char* s, size_t len, int cs) {
    if (len == 0) return;
    if (cs == 1) {
        for (size_t k = 0; k < len; k++) s[k] = (char)toupper((unsigned char)s[k]);
    } else if (cs == 2) {
        s[0] = (char)toupper((unsigned char)s[0]);
        for (size_t k = 1; k < len; k++) s[k] = (char)tolower((unsigned char)s[k]);
    } else if (cs == 3) {
        for (size_t k = 0; k < len-1; k++) s[k] = (char)tolower((unsigned char)s[k]);
        s[len-1] = (char)toupper((unsigned char)s[len-1]);
    }
}

static void nl_tokens_to_string(const NLTokenArray* src, char* out, size_t max) {
    size_t pos = 0;
    if (!src || max == 0) return;
    for (size_t i = 0; i < src->count && pos + 1 < max; i++) {
        const NLToken* t = &src->tokens[i];
        if (!t->isPattern) {
            out[pos++] = (char)(unsigned char)t->flag;
        } else {
            const char* pat = NULL;
            if (t->flag < NL_EN_PATTERN_COUNT)
                pat = NL_EN_PATTERNS[t->flag];
            else if (t->flag < (unsigned short)NL_EN_OPT_PATTERN_COUNT)
                pat = NL_EN_WORD_PATTERNS[t->flag - NL_EN_PATTERN_COUNT];
            if (pat) {
                size_t plen = strlen(pat);
                if (pos + plen >= max) plen = max - pos - 1;
                memcpy(out + pos, pat, plen);
                apply_case(out + pos, plen, t->caseStyle);
                pos += plen;
            }
        }
    }
    out[pos] = '\0';
}

/* ═══════════════════════════════════════════════════════════════════════════
 * html_encode_ae_opt  (Steps 1–6)
 *
 * Bitstream layout:
 *   10 bits : token count
 *   [Step 3] 5 bits dict_size; per entry: 7 bits str_len + str_len×8 bits chars
 *   [Step 4] 12 bits cb_sym_count; 9 bits cb_vocab_size;
 *            cb_vocab_size×9 bits symbol values (0..485);
 *            VLC cb_ae_bytes; cb_ae_bytes bytes AE payload
 *   [Step 1+2+5] 10 bits text_tok_count; text_tok_count×10 bits nl_token_count;
 *                VLC global_nl_bytes; global_nl_bytes bytes NL payload
 *   Per token (2 bits type first):
 *     type==0 text : 2 bits subdataType;
 *                    if subdataType>0: VLC sub_bytes + sub_bytes bytes sub-codec
 *     type==1/2 tag: (no 9-bit flag — flag in AE stream);
 *                    if FLAG_RAW: 6 bits len + len×8 bits ASCII name;
 *                    1 bit selfClosing; 5 bits attrCount;
 *                    per attr: (no 9-bit flag — in AE stream);
 *                    if FLAG_RAW: 6 bits len + len×8 bits ASCII name;
 *                    2 bits subdataType;
 *                    if NONE: 1 dict_hit + (5 idx | 7 len + raw bytes)
 *                    if CSS/JS/NL: 7 bits sub_len + sub_len bytes sub-codec
 * ═══════════════════════════════════════════════════════════════════════════ */

unsigned char* html_encode_ae_opt(const HTMLTokenArray* arr, size_t count,
                                  size_t* outSize) {
    if (!arr || count == 0 || count > (size_t)arr->count) {
        *outSize = 0;
        return NULL;
    }
    html_codebook_init();

    /* ── Step 3: Build attr-value dictionary ─────────────────────────────── */
    char dict_vals[DICT_MAX][DICT_VAL_MAX];
    int  dict_size = build_attr_dict(arr, count, dict_vals);

    /* ── Pass 1: collect cb_syms[] (Step 4) and merged NL (Steps 1+2+5) ─── */

    /* Codebook symbols: at most count + count×HTML_MAX_ATTR_COUNT entries */
    size_t cb_syms_cap = count * ((size_t)HTML_MAX_ATTR_COUNT + 1u);
    uint16_t* cb_syms = (uint16_t*)malloc(cb_syms_cap * sizeof(uint16_t));
    if (!cb_syms) { *outSize = 0; return NULL; }
    size_t cb_sym_count = 0;

    NLTokenArray* merged_nl = (NLTokenArray*)calloc(1, sizeof(NLTokenArray));
    if (!merged_nl) { free(cb_syms); *outSize = 0; return NULL; }

    uint16_t text_nl_counts[HTML_MAX_TOKENS];
    memset(text_nl_counts, 0, sizeof(text_nl_counts));
    int text_tok_count = 0;

    for (size_t t = 0; t < count; t++) {
        const HTMLToken* tok = &arr->tokens[t];

        if (tok->type == 0) {
            /* Step 1+2+5: skip whitespace-only and subdataType!=NONE from NL */
            uint16_t nl_cnt = 0;
            int skip = is_whitespace_only(tok->data.text.content)
                    || tok->subdataType != HTML_SUBDATA_NONE;

            if (!skip && tok->data.text.textTokenArray &&
                tok->data.text.textTokenArray->count > 0) {
                size_t want = tok->data.text.textTokenArray->count;
                size_t cap  = (size_t)NL_EN_MAX_TOKENS - merged_nl->count;
                if (want > cap) want = cap;
                if (want > 0) {
                    memcpy(&merged_nl->tokens[merged_nl->count],
                           tok->data.text.textTokenArray->tokens,
                           want * sizeof(NLToken));
                    merged_nl->count += want;
                    nl_cnt = (uint16_t)want;
                }
            }

            if (text_tok_count < HTML_MAX_TOKENS)
                text_nl_counts[text_tok_count] = nl_cnt;
            text_tok_count++;

        } else {
            /* Step 4: collect tag and attr flag symbols */
            uint16_t tf = (uint16_t)find_tag(tok->data.tag.name);
            if (cb_sym_count < cb_syms_cap) cb_syms[cb_sym_count++] = tf;

            int ac = tok->data.tag.attrCount;
            if (ac > 31) ac = 31;
            for (int a = 0; a < ac; a++) {
                uint16_t af = (uint16_t)find_attr(tok->data.tag.attributes[a].name);
                if (cb_sym_count < cb_syms_cap) cb_syms[cb_sym_count++] = af;
            }
        }
    }

    /* ── Step 4: Build codebook vocab and AE-encode symbols ─────────────── */
    /* Vocab in first-appearance order; max CODEBOOK_SIZE+1 = 486 entries */
    uint16_t cb_vocab[CODEBOOK_SIZE + 1];
    size_t   cb_vocab_size = 0;

    for (size_t i = 0; i < cb_sym_count; i++) {
        int found = 0;
        for (size_t j = 0; j < cb_vocab_size; j++) {
            if (cb_vocab[j] == cb_syms[i]) { found = 1; break; }
        }
        if (!found && cb_vocab_size < (size_t)(CODEBOOK_SIZE + 1))
            cb_vocab[cb_vocab_size++] = cb_syms[i];
    }

    size_t         cb_ae_bytes = 0;
    unsigned char* cb_ae_buf   = NULL;
    if (cb_sym_count > 0 && cb_vocab_size > 0)
        cb_ae_buf = cb_ae_encode(cb_syms, cb_sym_count,
                                  cb_vocab, cb_vocab_size, &cb_ae_bytes);

    /* ── Step 1: Encode global NL ─────────────────────────────────────────── */
    size_t         global_nl_bytes = 0;
    unsigned char* global_nl_buf   = NULL;
    if (merged_nl->count > 0)
        global_nl_buf = nl_en_encode_opt(merged_nl, merged_nl->count,
                                          &global_nl_bytes);
    free(merged_nl);

    /* ── Allocate output buffer ─────────────────────────────────────────── */
    size_t buf_bits = 50000u
                    + count * 2000u
                    + (cb_ae_bytes + global_nl_bytes + 16u) * 8u;
    unsigned char* buffer = (unsigned char*)calloc((buf_bits + 7) / 8, 1);
    if (!buffer) {
        free(cb_syms); free(cb_ae_buf); free(global_nl_buf);
        *outSize = 0; return NULL;
    }

    size_t bp = 0;

    /* 10-bit token count */
    set_bits(buffer, bp, 10, (unsigned int)count); bp += 10;

    /* ── Step 3: Dict header ────────────────────────────────────────────── */
    set_bits(buffer, bp, 5, (unsigned int)dict_size); bp += 5;
    for (int d = 0; d < dict_size; d++) {
        size_t slen = strlen(dict_vals[d]);
        set_bits(buffer, bp, 7, (unsigned int)slen); bp += 7;
        write_bytes_bp(buffer, &bp, (const unsigned char*)dict_vals[d], slen);
    }

    /* ── Step 4: Codebook AE section ─────────────────────────────────────── */
    set_bits(buffer, bp, 12, (unsigned int)cb_sym_count);  bp += 12;
    set_bits(buffer, bp, 9,  (unsigned int)cb_vocab_size); bp += 9;
    for (size_t i = 0; i < cb_vocab_size; i++) {
        set_bits(buffer, bp, 9, (unsigned int)cb_vocab[i]); bp += 9;
    }
    vlc_write(buffer, &bp, cb_ae_bytes);
    if (cb_ae_bytes > 0 && cb_ae_buf)
        write_bytes_bp(buffer, &bp, cb_ae_buf, cb_ae_bytes);
    free(cb_ae_buf);

    /* ── Step 1: Global NL section ──────────────────────────────────────── */
    set_bits(buffer, bp, 10, (unsigned int)text_tok_count); bp += 10;
    for (int i = 0; i < text_tok_count; i++) {
        set_bits(buffer, bp, 10, (unsigned int)text_nl_counts[i]); bp += 10;
    }
    vlc_write(buffer, &bp, global_nl_bytes);  /* Step 6: VLC instead of 13-bit */
    if (global_nl_bytes > 0 && global_nl_buf)
        write_bytes_bp(buffer, &bp, global_nl_buf, global_nl_bytes);
    free(global_nl_buf);

    /* ── Per-token loop ─────────────────────────────────────────────────── */
    size_t cb_sym_idx = 0; /* tracks which cb_syms[] entry the current tag/attr uses */

    for (size_t t = 0; t < count; t++) {
        const HTMLToken* tok = &arr->tokens[t];

        set_bits(buffer, bp, 2, (unsigned int)tok->type); bp += 2;

        if (tok->type == 0) {
            /* Text token — NL content is in the global stream */
            set_bits(buffer, bp, 2, (unsigned int)tok->subdataType); bp += 2;

            if (tok->subdataType != HTML_SUBDATA_NONE) {
                unsigned char* subbuf = NULL;
                size_t sub_bytes = 0;
                if (tok->subdataType == HTML_SUBDATA_CSS && tok->subdata.css)
                    subbuf = css_encode_opt(tok->subdata.css,
                                            (size_t)tok->subdata.css->count,
                                            &sub_bytes);
                else if (tok->subdataType == HTML_SUBDATA_JS && tok->subdata.js)
                    subbuf = cljs_encode_ae_opt(tok->subdata.js,
                                                tok->subdata.js->count,
                                                &sub_bytes);
                else if (tok->subdataType == HTML_SUBDATA_NL && tok->subdata.nl)
                    subbuf = nl_en_encode_opt(tok->subdata.nl,
                                              tok->subdata.nl->count,
                                              &sub_bytes);
                vlc_write(buffer, &bp, sub_bytes);  /* Step 6: VLC */
                if (sub_bytes > 0 && subbuf)
                    write_bytes_bp(buffer, &bp, subbuf, sub_bytes);
                free(subbuf);
            }

        } else {
            /* Tag token — flag value is in the codebook AE stream (Step 4) */
            int tag_flag = (cb_sym_idx < cb_sym_count)
                         ? (int)cb_syms[cb_sym_idx++] : FLAG_RAW;

            /* Step 4: no 9-bit flag written here; written into AE pre-stream */
            if (tag_flag == FLAG_RAW) {
                size_t nlen = strlen(tok->data.tag.name);
                if (nlen > 63u) nlen = 63u;
                set_bits(buffer, bp, 6, (unsigned int)nlen); bp += 6;
                write_bytes_bp(buffer, &bp,
                               (const unsigned char*)tok->data.tag.name, nlen);
            }

            set_bits(buffer, bp, 1,
                     tok->data.tag.selfClosing ? 1u : 0u); bp += 1;

            int ac = tok->data.tag.attrCount;
            if (ac < 0) ac = 0;
            if (ac > 31) ac = 31;
            set_bits(buffer, bp, 5, (unsigned int)ac); bp += 5;

            for (int a = 0; a < ac; a++) {
                const HTMLAttribute* attr = &tok->data.tag.attributes[a];

                /* Step 4: attr flag in AE stream */
                int af = (cb_sym_idx < cb_sym_count)
                       ? (int)cb_syms[cb_sym_idx++] : FLAG_RAW;

                if (af == FLAG_RAW) {
                    size_t alen = strlen(attr->name);
                    if (alen > 63u) alen = 63u;
                    set_bits(buffer, bp, 6, (unsigned int)alen); bp += 6;
                    write_bytes_bp(buffer, &bp,
                                   (const unsigned char*)attr->name, alen);
                }

                set_bits(buffer, bp, 2, (unsigned int)attr->subdataType); bp += 2;

                if (attr->subdataType == HTML_SUBDATA_NONE) {
                    int didx = dict_lookup(attr->value, dict_vals, dict_size);
                    if (didx >= 0) {
                        set_bits(buffer, bp, 1, 1u); bp += 1;
                        set_bits(buffer, bp, 5, (unsigned int)didx); bp += 5;
                    } else {
                        set_bits(buffer, bp, 1, 0u); bp += 1;
                        size_t vl = strlen(attr->value);
                        if (vl > 127u) vl = 127u;
                        set_bits(buffer, bp, 7, (unsigned int)vl); bp += 7;
                        if (vl > 0)
                            write_bytes_bp(buffer, &bp,
                                           (const unsigned char*)attr->value, vl);
                    }
                } else {
                    /* CSS/JS/NL sub-codec attribute (kept at 7-bit length) */
                    unsigned char* avbuf = NULL;
                    size_t av_bytes = 0;
                    if (attr->subdataType == HTML_SUBDATA_CSS && attr->subdata.css)
                        avbuf = css_encode_opt(attr->subdata.css,
                                               (size_t)attr->subdata.css->count,
                                               &av_bytes);
                    else if (attr->subdataType == HTML_SUBDATA_JS && attr->subdata.js)
                        avbuf = cljs_encode_ae_opt(attr->subdata.js,
                                                   attr->subdata.js->count,
                                                   &av_bytes);
                    else if (attr->subdataType == HTML_SUBDATA_NL && attr->subdata.nl)
                        avbuf = nl_en_encode_opt(attr->subdata.nl,
                                                 attr->subdata.nl->count,
                                                 &av_bytes);
                    if (av_bytes > 127u) av_bytes = 127u;
                    set_bits(buffer, bp, 7, (unsigned int)av_bytes); bp += 7;
                    if (av_bytes > 0 && avbuf)
                        write_bytes_bp(buffer, &bp, avbuf, av_bytes);
                    free(avbuf);
                }
            }
        }
    }

    free(cb_syms);
    *outSize = (bp + 7) / 8;
    return buffer;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * html_decode_ae_opt  (Steps 1–6)
 * ═══════════════════════════════════════════════════════════════════════════ */

HTMLTokenArray* html_decode_ae_opt(const unsigned char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) {
        HTMLTokenArray* empty = (HTMLTokenArray*)calloc(1, sizeof(HTMLTokenArray));
        return empty;
    }
    html_codebook_init();

    size_t total_bits = bufferSize * 8;
    size_t bp = 0;

    if (bp + 10 > total_bits) return NULL;
    size_t count = get_bits(buffer, bp, 10); bp += 10;
    if (count > HTML_MAX_TOKENS) count = HTML_MAX_TOKENS;

    /* ── Step 3: Read attr-value dictionary ─────────────────────────────── */
    if (bp + 5 > total_bits) return NULL;
    unsigned int dict_size_u = get_bits(buffer, bp, 5); bp += 5;
    if (dict_size_u > DICT_MAX) dict_size_u = DICT_MAX;
    int dict_size = (int)dict_size_u;

    char dict_vals[DICT_MAX][DICT_VAL_MAX];
    memset(dict_vals, 0, sizeof(dict_vals));
    for (int d = 0; d < dict_size; d++) {
        if (bp + 7 > total_bits) return NULL;
        unsigned int slen = get_bits(buffer, bp, 7); bp += 7;
        if (slen >= DICT_VAL_MAX) slen = DICT_VAL_MAX - 1;
        if (slen > 0 && !read_bytes_bp(buffer, &bp, total_bits,
                                        (unsigned char*)dict_vals[d], slen))
            return NULL;
        dict_vals[d][slen] = '\0';
    }

    /* ── Step 4: Read codebook AE section ───────────────────────────────── */
    if (bp + 12 > total_bits) return NULL;
    size_t cb_sym_count = get_bits(buffer, bp, 12); bp += 12;

    if (bp + 9 > total_bits) return NULL;
    size_t cb_vocab_size = get_bits(buffer, bp, 9); bp += 9;
    if (cb_vocab_size > (size_t)(CODEBOOK_SIZE + 1))
        cb_vocab_size = (size_t)(CODEBOOK_SIZE + 1);

    uint16_t cb_vocab[CODEBOOK_SIZE + 1];
    memset(cb_vocab, 0, sizeof(cb_vocab));
    for (size_t i = 0; i < cb_vocab_size; i++) {
        if (bp + 9 > total_bits) return NULL;
        cb_vocab[i] = (uint16_t)get_bits(buffer, bp, 9); bp += 9;
    }

    size_t cb_ae_bytes = vlc_read(buffer, &bp, total_bits);

    /* Pre-decode all codebook symbols into dec_syms[] */
    uint16_t* dec_syms = NULL;
    if (cb_sym_count > 0 && cb_ae_bytes > 0) {
        unsigned char* ae_payload = (unsigned char*)malloc(cb_ae_bytes);
        if (!ae_payload) return NULL;
        if (!read_bytes_bp(buffer, &bp, total_bits, ae_payload, cb_ae_bytes)) {
            free(ae_payload); return NULL;
        }
        dec_syms = (uint16_t*)malloc(cb_sym_count * sizeof(uint16_t));
        if (!dec_syms) { free(ae_payload); return NULL; }
        if (!cb_ae_decode(ae_payload, cb_ae_bytes, cb_vocab, cb_vocab_size,
                           cb_sym_count, dec_syms)) {
            free(ae_payload); free(dec_syms); return NULL;
        }
        free(ae_payload);
    } else if (cb_sym_count > 0) {
        /* cb_sym_count > 0 but no AE bytes — skip raw bytes if any */
        if (cb_ae_bytes > 0) {
            if (bp + cb_ae_bytes * 8u > total_bits) return NULL;
            bp += cb_ae_bytes * 8u;
        }
        /* Cannot decode; allocate empty array to avoid null deref */
        dec_syms = (uint16_t*)calloc(cb_sym_count, sizeof(uint16_t));
    } else {
        /* cb_sym_count == 0; skip any AE bytes */
        if (cb_ae_bytes > 0) {
            if (bp + cb_ae_bytes * 8u > total_bits) return NULL;
            bp += cb_ae_bytes * 8u;
        }
    }

    /* ── Step 1: Read global NL section ─────────────────────────────────── */
    if (bp + 10 > total_bits) { free(dec_syms); return NULL; }
    unsigned int text_tok_count = get_bits(buffer, bp, 10); bp += 10;
    if (text_tok_count > HTML_MAX_TOKENS) text_tok_count = HTML_MAX_TOKENS;

    uint16_t text_nl_counts[HTML_MAX_TOKENS];
    memset(text_nl_counts, 0, sizeof(text_nl_counts));
    for (unsigned int i = 0; i < text_tok_count; i++) {
        if (bp + 10 > total_bits) break;
        text_nl_counts[i] = (uint16_t)get_bits(buffer, bp, 10); bp += 10;
    }

    size_t global_nl_bytes = vlc_read(buffer, &bp, total_bits);  /* Step 6 */

    NLTokenArray* merged_nl = NULL;
    if (global_nl_bytes > 0) {
        unsigned char* nlbuf = (unsigned char*)malloc(global_nl_bytes);
        if (!nlbuf) { free(dec_syms); return NULL; }
        if (!read_bytes_bp(buffer, &bp, total_bits, nlbuf, global_nl_bytes)) {
            free(nlbuf); free(dec_syms); return NULL;
        }
        merged_nl = nl_en_decode_opt(nlbuf, global_nl_bytes);
        free(nlbuf);
    }

    HTMLTokenArray* arr = (HTMLTokenArray*)calloc(1, sizeof(HTMLTokenArray));
    if (!arr) { freeNLTokenArray(merged_nl); free(dec_syms); return NULL; }

    int    text_tok_idx = 0;
    size_t nl_offset    = 0;
    size_t cb_sym_idx   = 0;

    for (size_t t = 0; t < count; t++) {
        HTMLToken* tok = &arr->tokens[t];
        tok->type                      = 0;
        tok->subdataType               = HTML_SUBDATA_NONE;
        tok->subdata.css               = NULL;
        tok->data.text.textTokenArray  = NULL;
        tok->data.text.content[0]      = '\0';

        if (bp + 2 > total_bits) goto done;
        tok->type = (int)get_bits(buffer, bp, 2); bp += 2;

        if (tok->type == 0) {
            /* Reconstruct text from global NL slice */
            int ttidx = text_tok_idx++;
            if (ttidx < (int)text_tok_count) {
                size_t nl_cnt = text_nl_counts[ttidx];
                if (nl_cnt > 0 && merged_nl && nl_offset < merged_nl->count) {
                    size_t avail = merged_nl->count - nl_offset;
                    if (nl_cnt > avail) nl_cnt = avail;

                    NLTokenArray* slice = (NLTokenArray*)calloc(1, sizeof(NLTokenArray));
                    if (slice) {
                        memcpy(slice->tokens,
                               &merged_nl->tokens[nl_offset],
                               nl_cnt * sizeof(NLToken));
                        slice->count = nl_cnt;
                        nl_offset   += nl_cnt;
                        nl_tokens_to_string(slice, tok->data.text.content,
                                            HTML_MAX_TEXT_CONTENT);
                        tok->data.text.textTokenArray = slice;
                    }
                }
            }

            if (bp + 2 > total_bits) { arr->count++; goto done; }
            tok->subdataType = (HTMLSubdataType)get_bits(buffer, bp, 2); bp += 2;

            if (tok->subdataType != HTML_SUBDATA_NONE) {
                /* Step 6: VLC sub_bytes */
                size_t sub_bytes = vlc_read(buffer, &bp, total_bits);
                if (sub_bytes > 0) {
                    unsigned char* subbuf = (unsigned char*)malloc(sub_bytes);
                    if (!subbuf) { arr->count++; goto done; }
                    if (!read_bytes_bp(buffer, &bp, total_bits, subbuf, sub_bytes)) {
                        free(subbuf); arr->count++; goto done;
                    }
                    if (tok->subdataType == HTML_SUBDATA_CSS)
                        tok->subdata.css = css_decode_opt(subbuf, sub_bytes);
                    else if (tok->subdataType == HTML_SUBDATA_JS)
                        tok->subdata.js = cljs_decode_ae_opt(subbuf, sub_bytes);
                    else if (tok->subdataType == HTML_SUBDATA_NL)
                        tok->subdata.nl = nl_en_decode_opt(subbuf, sub_bytes);
                    free(subbuf);
                }

                /* Step 5: reconstruct content from NL sub-codec */
                if (tok->subdataType == HTML_SUBDATA_NL && tok->subdata.nl)
                    nl_tokens_to_string(tok->subdata.nl, tok->data.text.content,
                                        HTML_MAX_TEXT_CONTENT);
            }

        } else {
            /* Tag token — consume next pre-decoded flag (Step 4) */
            tok->data.tag.selfClosing = 0;
            tok->data.tag.attrCount   = 0;
            tok->data.tag.name[0]     = '\0';

            unsigned int fv = (dec_syms && cb_sym_idx < cb_sym_count)
                            ? (unsigned int)dec_syms[cb_sym_idx++]
                            : (unsigned int)FLAG_RAW;

            if (fv == (unsigned int)FLAG_RAW) {
                if (bp + 6 > total_bits) goto done;
                size_t nlen = get_bits(buffer, bp, 6); bp += 6;
                if (nlen >= HTML_MAX_TAG_NAME) nlen = HTML_MAX_TAG_NAME - 1;
                if (!read_bytes_bp(buffer, &bp, total_bits,
                                   (unsigned char*)tok->data.tag.name, nlen))
                    goto done;
                tok->data.tag.name[nlen] = '\0';
            } else if (fv < (unsigned int)TAG_COUNT) {
                strncpy(tok->data.tag.name, s_tags[fv], HTML_MAX_TAG_NAME - 1);
                tok->data.tag.name[HTML_MAX_TAG_NAME - 1] = '\0';
            }

            if (bp + 1 > total_bits) goto done;
            tok->data.tag.selfClosing = (int)get_bits(buffer, bp, 1); bp += 1;

            if (bp + 5 > total_bits) goto done;
            int ac = (int)get_bits(buffer, bp, 5); bp += 5;
            if (ac > HTML_MAX_ATTR_COUNT) ac = HTML_MAX_ATTR_COUNT;
            tok->data.tag.attrCount = ac;

            for (int a = 0; a < ac; a++) {
                HTMLAttribute* attr = &tok->data.tag.attributes[a];
                attr->name[0]     = '\0';
                attr->value[0]    = '\0';
                attr->subdataType = HTML_SUBDATA_NONE;
                attr->subdata.css = NULL;

                /* Step 4: consume pre-decoded attr flag */
                unsigned int afv = (dec_syms && cb_sym_idx < cb_sym_count)
                                 ? (unsigned int)dec_syms[cb_sym_idx++]
                                 : (unsigned int)FLAG_RAW;

                if (afv == (unsigned int)FLAG_RAW) {
                    if (bp + 6 > total_bits) goto done;
                    size_t alen = get_bits(buffer, bp, 6); bp += 6;
                    if (alen >= HTML_MAX_ATTR_NAME) alen = HTML_MAX_ATTR_NAME - 1;
                    if (!read_bytes_bp(buffer, &bp, total_bits,
                                       (unsigned char*)attr->name, alen))
                        goto done;
                    attr->name[alen] = '\0';
                } else if (afv >= (unsigned int)ATTR_START &&
                           afv < (unsigned int)MIME_START) {
                    const char* aname = s_attrs[afv - ATTR_START];
                    strncpy(attr->name, aname, HTML_MAX_ATTR_NAME - 1);
                    attr->name[HTML_MAX_ATTR_NAME - 1] = '\0';
                }

                if (bp + 2 > total_bits) goto done;
                attr->subdataType = (HTMLSubdataType)get_bits(buffer, bp, 2); bp += 2;

                if (attr->subdataType == HTML_SUBDATA_NONE) {
                    if (bp + 1 > total_bits) goto done;
                    unsigned int dict_hit = get_bits(buffer, bp, 1); bp += 1;
                    if (dict_hit) {
                        if (bp + 5 > total_bits) goto done;
                        unsigned int didx = get_bits(buffer, bp, 5); bp += 5;
                        if ((int)didx < dict_size) {
                            size_t vl = strlen(dict_vals[didx]);
                            if (vl >= HTML_MAX_ATTR_VALUE) vl = HTML_MAX_ATTR_VALUE - 1;
                            memcpy(attr->value, dict_vals[didx], vl);
                            attr->value[vl] = '\0';
                        }
                    } else {
                        if (bp + 7 > total_bits) goto done;
                        size_t av = get_bits(buffer, bp, 7); bp += 7;
                        if (av > 0) {
                            unsigned char* avbuf = (unsigned char*)malloc(av + 1);
                            if (!avbuf) goto done;
                            if (!read_bytes_bp(buffer, &bp, total_bits, avbuf, av)) {
                                free(avbuf); goto done;
                            }
                            avbuf[av] = 0;
                            size_t vl = av < (size_t)(HTML_MAX_ATTR_VALUE - 1)
                                      ? av : (size_t)(HTML_MAX_ATTR_VALUE - 1);
                            memcpy(attr->value, avbuf, vl);
                            attr->value[vl] = '\0';
                            free(avbuf);
                        }
                    }
                } else {
                    /* CSS/JS/NL sub-codec attribute */
                    if (bp + 7 > total_bits) goto done;
                    size_t av = get_bits(buffer, bp, 7); bp += 7;
                    if (av > 0) {
                        unsigned char* avbuf = (unsigned char*)malloc(av + 1);
                        if (!avbuf) goto done;
                        if (!read_bytes_bp(buffer, &bp, total_bits, avbuf, av)) {
                            free(avbuf); goto done;
                        }
                        avbuf[av] = 0;
                        if (attr->subdataType == HTML_SUBDATA_CSS)
                            attr->subdata.css = css_decode_opt(avbuf, av);
                        else if (attr->subdataType == HTML_SUBDATA_JS)
                            attr->subdata.js = cljs_decode_ae_opt(avbuf, av);
                        else if (attr->subdataType == HTML_SUBDATA_NL)
                            attr->subdata.nl = nl_en_decode_opt(avbuf, av);
                        free(avbuf);
                    }
                }
            }
        }

        arr->count++;
    }

done:
    freeNLTokenArray(merged_nl);
    free(dec_syms);
    return arr;
}

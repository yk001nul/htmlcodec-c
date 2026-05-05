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

/* Sentinel value stored in 9-bit flag to indicate a raw (unknown) name follows */
#define FLAG_RAW CODEBOOK_SIZE

/* Public constants */
const int HTML_CODEBOOK_TAG_COUNT  = TAG_COUNT;
const int HTML_CODEBOOK_ATTR_START = ATTR_START;
const int HTML_CODEBOOK_ATTR_COUNT = ATTR_COUNT;
const int HTML_CODEBOOK_MIME_START = MIME_START;
const int HTML_CODEBOOK_MIME_COUNT = MIME_COUNT;
const int HTML_CODEBOOK_SIZE       = CODEBOOK_SIZE;

/* Build and expose the flat codebook array */
static const char* s_flat[CODEBOOK_SIZE];
static int s_init = 0;

static void codebook_init(void) {
    if (s_init) return;
    for (int i = 0; i < TAG_COUNT; i++)  s_flat[i]           = s_tags[i];
    for (int i = 0; i < ATTR_COUNT; i++) s_flat[ATTR_START+i] = s_attrs[i];
    for (int i = 0; i < MIME_COUNT; i++) s_flat[MIME_START+i] = s_mimes[i];
    s_init = 1;
}

const char* HTML_CODEBOOK[CODEBOOK_SIZE]; /* filled by html_codebook_init() */

void html_codebook_init(void) {
    if (s_init) return;
    codebook_init();
    for (int i = 0; i < CODEBOOK_SIZE; i++) HTML_CODEBOOK[i] = s_flat[i];
}

/* Lookup helpers */
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
 * html_encode_ae_opt
 * ═══════════════════════════════════════════════════════════════════════════ */

unsigned char* html_encode_ae_opt(const HTMLTokenArray* arr, size_t count,
                                  size_t* outSize) {
    if (!arr || count == 0 || count > (size_t)arr->count) {
        *outSize = 0;
        return NULL;
    }
    html_codebook_init();

    /* Generous worst-case buffer:
     * 10 (count) + per_token:
     *   text: 2+12+4095*8+2+12+4095*8 ≈ 65624 bits
     *   tag:  2+9+6+63*8+1+5 + 32*(9+6+63*8+2+7+127*8) ≈ 50000 bits
     * Use 70000 per token. */
    size_t buf_bits  = 10u + count * 70000u;
    size_t buf_bytes = (buf_bits + 7) / 8;
    unsigned char* buffer = (unsigned char*)calloc(buf_bytes, 1);
    if (!buffer) { *outSize = 0; return NULL; }

    size_t bp = 0;

    /* 10-bit token count */
    set_bits(buffer, bp, 10, (unsigned int)count); bp += 10;

    for (size_t t = 0; t < count; t++) {
        const HTMLToken* tok = &arr->tokens[t];

        set_bits(buffer, bp, 2, (unsigned int)tok->type); bp += 2;

        if (tok->type == 0) {
            /* Text token */
            unsigned char* nlbuf = NULL;
            size_t nl_bytes = 0;
            if (tok->data.text.textTokenArray &&
                tok->data.text.textTokenArray->count > 0) {
                nlbuf = nl_en_encode_opt(tok->data.text.textTokenArray,
                                         tok->data.text.textTokenArray->count,
                                         &nl_bytes);
            }
            if (nl_bytes > 4095u) nl_bytes = 4095u;
            set_bits(buffer, bp, 12, (unsigned int)nl_bytes); bp += 12;
            if (nl_bytes > 0 && nlbuf) write_bytes_bp(buffer, &bp, nlbuf, nl_bytes);
            free(nlbuf);

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
                if (sub_bytes > 4095u) sub_bytes = 4095u;
                set_bits(buffer, bp, 12, (unsigned int)sub_bytes); bp += 12;
                if (sub_bytes > 0 && subbuf)
                    write_bytes_bp(buffer, &bp, subbuf, sub_bytes);
                free(subbuf);
            }

        } else {
            /* Tag token */
            int tag_flag = find_tag(tok->data.tag.name);
            unsigned int fv = (unsigned int)tag_flag; /* FLAG_RAW or 0..111 */
            set_bits(buffer, bp, 9, fv); bp += 9;

            if (fv == (unsigned int)FLAG_RAW) {
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

                int af = find_attr(attr->name);
                unsigned int afv = (unsigned int)af;
                set_bits(buffer, bp, 9, afv); bp += 9;

                if (afv == (unsigned int)FLAG_RAW) {
                    size_t alen = strlen(attr->name);
                    if (alen > 63u) alen = 63u;
                    set_bits(buffer, bp, 6, (unsigned int)alen); bp += 6;
                    write_bytes_bp(buffer, &bp,
                                   (const unsigned char*)attr->name, alen);
                }

                set_bits(buffer, bp, 2, (unsigned int)attr->subdataType); bp += 2;

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
                else {
                    size_t vl = strlen(attr->value);
                    if (vl > 0) {
                        avbuf = (unsigned char*)malloc(vl);
                        if (avbuf) { memcpy(avbuf, attr->value, vl); av_bytes = vl; }
                    }
                }
                if (av_bytes > 127u) av_bytes = 127u;
                set_bits(buffer, bp, 7, (unsigned int)av_bytes); bp += 7;
                if (av_bytes > 0 && avbuf)
                    write_bytes_bp(buffer, &bp, avbuf, av_bytes);
                free(avbuf);
            }
        }
    }

    *outSize = (bp + 7) / 8;
    return buffer;
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
 * html_decode_ae_opt
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

    HTMLTokenArray* arr = (HTMLTokenArray*)calloc(1, sizeof(HTMLTokenArray));
    if (!arr) return NULL;

    for (size_t t = 0; t < count; t++) {
        HTMLToken* tok = &arr->tokens[t];
        tok->type = 0;
        tok->subdataType = HTML_SUBDATA_NONE;
        tok->subdata.css = NULL;
        tok->data.text.textTokenArray = NULL;
        tok->data.text.content[0] = '\0';

        if (bp + 2 > total_bits) goto done;
        tok->type = (int)get_bits(buffer, bp, 2); bp += 2;

        if (tok->type == 0) {
            /* Text token */
            if (bp + 12 > total_bits) goto done;
            size_t nl_bytes = get_bits(buffer, bp, 12); bp += 12;

            if (nl_bytes > 0) {
                unsigned char* nlbuf = (unsigned char*)malloc(nl_bytes);
                if (!nlbuf) goto done;
                if (!read_bytes_bp(buffer, &bp, total_bits, nlbuf, nl_bytes)) {
                    free(nlbuf); goto done;
                }
                tok->data.text.textTokenArray = nl_en_decode_opt(nlbuf, nl_bytes);
                free(nlbuf);
                if (tok->data.text.textTokenArray)
                    nl_tokens_to_string(tok->data.text.textTokenArray,
                                        tok->data.text.content,
                                        HTML_MAX_TEXT_CONTENT);
            }

            if (bp + 2 > total_bits) { arr->count++; goto done; }
            tok->subdataType = (HTMLSubdataType)get_bits(buffer, bp, 2); bp += 2;

            if (tok->subdataType != HTML_SUBDATA_NONE) {
                if (bp + 12 > total_bits) { arr->count++; goto done; }
                size_t sub_bytes = get_bits(buffer, bp, 12); bp += 12;
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
            }

        } else {
            /* Tag token */
            tok->data.tag.selfClosing = 0;
            tok->data.tag.attrCount   = 0;
            tok->data.tag.name[0]     = '\0';

            if (bp + 9 > total_bits) goto done;
            unsigned int fv = get_bits(buffer, bp, 9); bp += 9;

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
                attr->name[0]   = '\0';
                attr->value[0]  = '\0';
                attr->subdataType = HTML_SUBDATA_NONE;
                attr->subdata.css = NULL;

                if (bp + 9 > total_bits) goto done;
                unsigned int afv = get_bits(buffer, bp, 9); bp += 9;

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

                if (bp + 7 > total_bits) goto done;
                size_t av = get_bits(buffer, bp, 7); bp += 7;

                if (av > 0) {
                    unsigned char* avbuf = (unsigned char*)malloc(av + 1);
                    if (!avbuf) goto done;
                    if (!read_bytes_bp(buffer, &bp, total_bits, avbuf, av)) {
                        free(avbuf); goto done;
                    }
                    avbuf[av] = 0;
                    if (attr->subdataType == HTML_SUBDATA_NONE) {
                        size_t vl = av < (size_t)(HTML_MAX_ATTR_VALUE - 1)
                                  ? av : (size_t)(HTML_MAX_ATTR_VALUE - 1);
                        memcpy(attr->value, avbuf, vl);
                        attr->value[vl] = '\0';
                    } else if (attr->subdataType == HTML_SUBDATA_CSS) {
                        attr->subdata.css = css_decode_opt(avbuf, av);
                    } else if (attr->subdataType == HTML_SUBDATA_JS) {
                        attr->subdata.js = cljs_decode_ae_opt(avbuf, av);
                    } else if (attr->subdataType == HTML_SUBDATA_NL) {
                        attr->subdata.nl = nl_en_decode_opt(avbuf, av);
                    }
                    free(avbuf);
                }
            }
        }

        arr->count++;
    }

done:
    return arr;
}

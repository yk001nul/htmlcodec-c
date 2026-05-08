#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "html-tokenizer.h"

void parseHTMLAttributes(const char* attrString, HTMLAttribute* attrs, int* attrCount) {
    *attrCount = 0;
    int i = 0, len = strlen(attrString);

    while (i < len && *attrCount < HTML_MAX_ATTR_COUNT) {
        // Skip whitespace
        while (i < len && isspace(attrString[i])) i++;
        if (i >= len) break;

        // Parse attribute name
        int nameStart = i;
        while (i < len && attrString[i] != '=' && !isspace(attrString[i])) i++;
        int nameLen = i - nameStart;
        if (nameLen == 0) break;

        strncpy(attrs[*attrCount].name, &attrString[nameStart],
            nameLen < HTML_MAX_ATTR_NAME ? nameLen : HTML_MAX_ATTR_NAME - 1);
        attrs[*attrCount].name[nameLen < HTML_MAX_ATTR_NAME ? nameLen : HTML_MAX_ATTR_NAME - 1] = '\0';
        for (int p = 0; attrs[*attrCount].name[p]; p++) {
            attrs[*attrCount].name[p] = tolower((unsigned char)attrs[*attrCount].name[p]);
        }
        attrs[*attrCount].subdataType = HTML_SUBDATA_NONE;
        attrs[*attrCount].subdata.css = NULL;

        // Skip '='
        while (i < len && isspace(attrString[i])) i++;
        if (i >= len || attrString[i] != '=') break;
        i++;

        // Skip whitespace and opening quote
        while (i < len && isspace(attrString[i])) i++;
        char quote = attrString[i];
        if (quote != '"' && quote != '\'') break;
        i++;

        // Parse attribute value
        int valueStart = i;
        while (i < len && attrString[i] != quote) i++;
        int valueLen = i - valueStart;

        strncpy(attrs[*attrCount].value, &attrString[valueStart],
            valueLen < HTML_MAX_ATTR_VALUE ? valueLen : HTML_MAX_ATTR_VALUE - 1);
        attrs[*attrCount].value[valueLen < HTML_MAX_ATTR_VALUE ? valueLen : HTML_MAX_ATTR_VALUE - 1] = '\0';

        (*attrCount)++;
        i++; // skip closing quote
    }
}

static int isValidTagName(const char* name) {
    if (!name || name[0] == '\0') return 0;
    if (!isalpha((unsigned char)name[0])) return 0;
    for (int i = 1; name[i]; i++) {
        if (!(isalnum((unsigned char)name[i]) || name[i] == '-' || name[i] == ':' || name[i] == '.')) return 0;
    }
    return 1;
}

static void appendTextToken(HTMLTokenArray* result, const char* text, int textLen) {
    if (!result || textLen <= 0) return;

    if (result->count > 0 && result->tokens[result->count - 1].type == 0) {
        int curLen = (int)strlen(result->tokens[result->count - 1].data.text.content);
        int toCopy = textLen;
        if (toCopy > HTML_MAX_TEXT_CONTENT - 1 - curLen) {
            toCopy = HTML_MAX_TEXT_CONTENT - 1 - curLen;
        }
        if (toCopy > 0) {
            strncat(result->tokens[result->count - 1].data.text.content, text, toCopy);
        }
    } else {
        HTMLToken* token = &result->tokens[result->count++];
        token->type = 0;
        token->subdataType = HTML_SUBDATA_NONE;
        token->subdata.css = NULL;
        token->data.text.textTokenArray = NULL;
        int copyLen = textLen;
        if (copyLen > HTML_MAX_TEXT_CONTENT - 1) copyLen = HTML_MAX_TEXT_CONTENT - 1;
        strncpy(token->data.text.content, text, copyLen);
        token->data.text.content[copyLen] = '\0';
    }
}

HTMLTokenArray* parseHTML(const char* html) {
    HTMLTokenArray* result = (HTMLTokenArray*)malloc(sizeof(HTMLTokenArray));
    result->count = 0;

    int i = 0, len = strlen(html);

    while (i < len && result->count < HTML_MAX_TOKENS) {
        // Text content
        if (html[i] != '<') {
            int textStart = i;
            while (i < len && html[i] != '<') i++;
            int textLen = i - textStart;

            HTMLToken* token = &result->tokens[result->count++];
            token->type = 0; // text
            token->subdataType = HTML_SUBDATA_NONE;
            token->subdata.css = NULL;
            token->data.text.textTokenArray = NULL;
            strncpy(token->data.text.content, &html[textStart],
                textLen < HTML_MAX_TEXT_CONTENT ? textLen : HTML_MAX_TEXT_CONTENT - 1);
            token->data.text.content[textLen < HTML_MAX_TEXT_CONTENT ? textLen : HTML_MAX_TEXT_CONTENT - 1] = '\0';
        }
        // Tag content
        else if (html[i] == '<') {
            int ltIndex = i;
            i++; // skip '<'

            // Handle comments
            if (i + 2 < len && strncmp(&html[i], "!--", 3) == 0) {
                i += 3;
                while (i + 1 < len && strncmp(&html[i], "--", 2) != 0) i++;
                if (i + 2 < len) i += 3; // skip '-->'
                continue;
            }

            // Find closing >
            int tagStart = i;
            while (i < len && html[i] != '>') i++;
            if (i >= len) {
                // malformed tag, treat from '<' to end as literal text
                int txtLen = len - ltIndex;
                appendTextToken(result, &html[ltIndex], txtLen);
                i = len;
                continue;
            }

            int tagLen = i - tagStart;
            char tag[256];
            strncpy(tag, &html[tagStart], tagLen < 256 ? tagLen : 255);
            tag[tagLen < 256 ? tagLen : 255] = '\0';
            i++; // skip '>'

            int isSelfClosing = (tagLen > 0 && tag[tagLen - 1] == '/');
            int isClosing = (tagLen > 0 && tag[0] == '/');

            // Extract tag name
            int nameStart = isClosing ? 1 : 0;
            int nameEnd = nameStart;
            while (nameEnd < strlen(tag) && !isspace(tag[nameEnd]) && tag[nameEnd] != '/') nameEnd++;
            int nameLen = nameEnd - nameStart;

            if (nameLen == 0) {
                int invalidEnd = i;
                while (invalidEnd < len && html[invalidEnd] != '<') invalidEnd++;
                appendTextToken(result, &html[ltIndex], invalidEnd - ltIndex);
                i = invalidEnd;
                continue;
            }

            char tagName[HTML_MAX_TAG_NAME];
            strncpy(tagName, &tag[nameStart], nameLen < HTML_MAX_TAG_NAME ? nameLen : HTML_MAX_TAG_NAME - 1);
            tagName[nameLen < HTML_MAX_TAG_NAME ? nameLen : HTML_MAX_TAG_NAME - 1] = '\0';

            // Convert to lowercase
            for (int j = 0; tagName[j]; j++) tagName[j] = tolower((unsigned char)tagName[j]);

            if (!isValidTagName(tagName)) {
                int invalidEnd = i;
                while (invalidEnd < len && html[invalidEnd] != '<') invalidEnd++;
                appendTextToken(result, &html[ltIndex], invalidEnd - ltIndex);
                i = invalidEnd;
                continue;
            }

            HTMLToken* token = &result->tokens[result->count++];
            token->type = isClosing ? 2 : 1;
            token->subdataType = HTML_SUBDATA_NONE;
            token->subdata.css = NULL;
            strcpy(token->data.tag.name, tagName);
            token->data.tag.selfClosing = isSelfClosing;

            // Parse attributes
            char attrString[512];
            int attrLen = strlen(tag) - nameEnd - (isSelfClosing ? 1 : 0);
            strncpy(attrString, &tag[nameEnd], attrLen < 512 ? attrLen : 511);
            attrString[attrLen < 512 ? attrLen : 511] = '\0';
            parseHTMLAttributes(attrString, token->data.tag.attributes, &token->data.tag.attrCount);
        }
    }

    enrichHTMLTokenSubdata(result);
    return result;
}

static int startsWithOn(const char* name) {
    return name && strlen(name) >= 2 && name[0] == 'o' && name[1] == 'n';
}

void enrichHTMLTokenSubdata(HTMLTokenArray* tokens) {
    if (!tokens) return;

    // Populate textTokenArray for all text tokens
    for (int i = 0; i < tokens->count; i++) {
        HTMLToken* token = &tokens->tokens[i];
        if (token->type != 0) continue;
        if (token->data.text.content[0] != '\0') {
            token->data.text.textTokenArray = tokenizeEnglishOpt(token->data.text.content);
        }
    }

    // Text tokens preceding closing tag can be CSS/JS/NL depending on tag name
    for (int i = 0; i < tokens->count; i++) {
        HTMLToken* token = &tokens->tokens[i];
        if (token->type != 0) continue;

        int j = i + 1;
        while (j < tokens->count && tokens->tokens[j].type == 0) {
            j++;
        }

        if (j >= tokens->count) continue;

        HTMLToken* next = &tokens->tokens[j];
        if (next->type == 2) {
            if (strcmp(next->data.tag.name, "style") == 0) {
                token->subdata.css = parseCSS(token->data.text.content);
                token->subdataType = HTML_SUBDATA_CSS;
            } else if (strcmp(next->data.tag.name, "script") == 0) {
                token->subdata.js = tokenizeJavaScript(token->data.text.content);
                token->subdataType = HTML_SUBDATA_JS;
            } else {
                token->subdata.nl = tokenizeEnglishOpt(token->data.text.content);
                token->subdataType = HTML_SUBDATA_NL;
            }
        }
    }

    // Attributes may contain inline CSS/JS
    for (int i = 0; i < tokens->count; i++) {
        HTMLToken* token = &tokens->tokens[i];
        if (token->type == 0) continue;

        for (int a = 0; a < token->data.tag.attrCount; a++) {
            HTMLAttribute* attr = &token->data.tag.attributes[a];
            if (strcmp(attr->name, "style") == 0) {
                attr->subdata.css = parseCSS(attr->value);
                attr->subdataType = HTML_SUBDATA_CSS;
            } else if (startsWithOn(attr->name)) {
                attr->subdata.js = tokenizeJavaScript(attr->value);
                attr->subdataType = HTML_SUBDATA_JS;
            }
        }
    }
}

char* detokenizeHTMLTokenArray(const HTMLTokenArray* arr, int* cumLen) {
    if (!arr || !cumLen) return NULL;
    /* Per token: text up to HTML_MAX_TEXT_CONTENT, or tag with all attrs at max size. */
    size_t bufSize = (size_t)arr->count *
        (HTML_MAX_TEXT_CONTENT + HTML_MAX_TAG_NAME +
         (size_t)HTML_MAX_ATTR_COUNT * (HTML_MAX_ATTR_NAME + HTML_MAX_ATTR_VALUE + 8) + 8) + 1;
    char* result = (char*)calloc(bufSize, 1);
    if (!result) return NULL;
    *cumLen = 0;

    for (int i = 0; i < arr->count; i++) {
        if (*cumLen >= (int)bufSize - 1) break;
        const HTMLToken* token = &arr->tokens[i];

        if (token->type == 0) {
            /* Text token: reconstruct via NL detokenizer. */
            if (token->data.text.textTokenArray != NULL &&
                token->data.text.textTokenArray->count > 0) {
                int nlLen = 0;
                char* nlStr = detokenizeNLTokenArray(token->data.text.textTokenArray, &nlLen);
                if (nlStr) {
                    size_t copy = (size_t)nlLen;
                    if (*cumLen + (int)copy > (int)bufSize - 1)
                        copy = (size_t)((int)bufSize - 1 - *cumLen);
                    memcpy(result + *cumLen, nlStr, copy);
                    *cumLen += (int)copy;
                    free(nlStr);
                }
            } else if (token->data.text.content[0] != '\0') {
                /* Fallback: textTokenArray absent, use raw content. */
                size_t contentLen = strlen(token->data.text.content);
                size_t copy = contentLen;
                if (*cumLen + (int)copy > (int)bufSize - 1)
                    copy = (size_t)((int)bufSize - 1 - *cumLen);
                memcpy(result + *cumLen, token->data.text.content, copy);
                *cumLen += (int)copy;
            }
        } else if (token->type == 1) {
            /* Open tag: <name attr1="val1" attr2="val2"> or <name/> */
            if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '<';

            {
                const char* name = token->data.tag.name;
                size_t nameLen = strlen(name);
                size_t copy = nameLen;
                if (*cumLen + (int)copy > (int)bufSize - 1)
                    copy = (size_t)((int)bufSize - 1 - *cumLen);
                memcpy(result + *cumLen, name, copy);
                *cumLen += (int)copy;
            }

            for (int a = 0; a < token->data.tag.attrCount; a++) {
                const HTMLAttribute* attr = &token->data.tag.attributes[a];

                if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = ' ';

                {
                    size_t attrNameLen = strlen(attr->name);
                    size_t copy = attrNameLen;
                    if (*cumLen + (int)copy > (int)bufSize - 1)
                        copy = (size_t)((int)bufSize - 1 - *cumLen);
                    memcpy(result + *cumLen, attr->name, copy);
                    *cumLen += (int)copy;
                }

                if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '=';
                if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '"';

                if (attr->subdataType == HTML_SUBDATA_CSS && attr->subdata.css) {
                    int cssLen = 0;
                    char* cssStr = detokenizeCSSTokenArray(attr->subdata.css, &cssLen);
                    if (cssStr) {
                        size_t copy = (size_t)cssLen;
                        if (*cumLen + (int)copy > (int)bufSize - 1)
                            copy = (size_t)((int)bufSize - 1 - *cumLen);
                        memcpy(result + *cumLen, cssStr, copy);
                        *cumLen += (int)copy;
                        free(cssStr);
                    }
                } else if (attr->subdataType == HTML_SUBDATA_JS && attr->subdata.js) {
                    int jsLen = 0;
                    char* jsStr = detokenizeCLJSTokenArray(attr->subdata.js, &jsLen);
                    if (jsStr) {
                        size_t copy = (size_t)jsLen;
                        if (*cumLen + (int)copy > (int)bufSize - 1)
                            copy = (size_t)((int)bufSize - 1 - *cumLen);
                        memcpy(result + *cumLen, jsStr, copy);
                        *cumLen += (int)copy;
                        free(jsStr);
                    }
                } else {
                    /* NONE or NL attribute: use raw stored value. */
                    size_t valLen = strlen(attr->value);
                    size_t copy = valLen;
                    if (*cumLen + (int)copy > (int)bufSize - 1)
                        copy = (size_t)((int)bufSize - 1 - *cumLen);
                    memcpy(result + *cumLen, attr->value, copy);
                    *cumLen += (int)copy;
                }

                if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '"';
            }

            if (token->data.tag.selfClosing) {
                if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '/';
                if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '>';
            } else {
                if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '>';
            }
        } else if (token->type == 2) {
            /* Close tag: </name> */
            if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '<';
            if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '/';

            {
                const char* name = token->data.tag.name;
                size_t nameLen = strlen(name);
                size_t copy = nameLen;
                if (*cumLen + (int)copy > (int)bufSize - 1)
                    copy = (size_t)((int)bufSize - 1 - *cumLen);
                memcpy(result + *cumLen, name, copy);
                *cumLen += (int)copy;
            }

            if (*cumLen < (int)bufSize - 1) result[(*cumLen)++] = '>';
        }
    }

    result[*cumLen] = '\0';
    return result;
}

void freeHTMLTokenArray(HTMLTokenArray* arr) {
    if (!arr) return;

    for (int i = 0; i < arr->count; i++) {
        HTMLToken* token = &arr->tokens[i];
        if (token->type == 0 && token->data.text.textTokenArray) {
            freeNLTokenArray(token->data.text.textTokenArray);
            token->data.text.textTokenArray = NULL;
        }
        if (token->subdataType == HTML_SUBDATA_CSS && token->subdata.css) {
            freeCSS(token->subdata.css);
            token->subdata.css = NULL;
        } else if (token->subdataType == HTML_SUBDATA_JS && token->subdata.js) {
            freeCLJSTokenArray(token->subdata.js);
            token->subdata.js = NULL;
        } else if (token->subdataType == HTML_SUBDATA_NL && token->subdata.nl) {
            freeNLTokenArray(token->subdata.nl);
            token->subdata.nl = NULL;
        }

        if (token->type != 0) {
            for (int a = 0; a < token->data.tag.attrCount; a++) {
                HTMLAttribute* attr = &token->data.tag.attributes[a];
                if (attr->subdataType == HTML_SUBDATA_CSS && attr->subdata.css) {
                    freeCSS(attr->subdata.css);
                    attr->subdata.css = NULL;
                } else if (attr->subdataType == HTML_SUBDATA_JS && attr->subdata.js) {
                    freeCLJSTokenArray(attr->subdata.js);
                    attr->subdata.js = NULL;
                } else if (attr->subdataType == HTML_SUBDATA_NL && attr->subdata.nl) {
                    freeNLTokenArray(attr->subdata.nl);
                    attr->subdata.nl = NULL;
                }
            }
        }
    }

    free(arr);
}

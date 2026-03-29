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
            strncpy(token->data.text.content, &html[textStart],
                textLen < HTML_MAX_TEXT_CONTENT ? textLen : HTML_MAX_TEXT_CONTENT - 1);
            token->data.text.content[textLen < HTML_MAX_TEXT_CONTENT ? textLen : HTML_MAX_TEXT_CONTENT - 1] = '\0';
        }
        // Tag content
        else if (html[i] == '<') {
            i++; // skip '<'

            // Handle comments
            if (i + 2 < len && strncmp(&html[i], "!--", 3) == 0) {
                i += 3;
                while (i + 1 < len && strncmp(&html[i], "--", 2) != 0) i++;
                i += 3; // skip '-->'
                continue;
            }

            // Extract tag
            int tagStart = i;
            while (i < len && html[i] != '>') i++;
            int tagLen = i - tagStart;
            char tag[256];
            strncpy(tag, &html[tagStart], tagLen < 256 ? tagLen : 255);
            tag[tagLen < 256 ? tagLen : 255] = '\0';
            i++; // skip '>'

            int isSelfClosing = (tag[tagLen - 1] == '/');
            int isClosing = (tag[0] == '/');

            // Extract tag name
            int nameStart = isClosing ? 1 : 0;
            int nameEnd = nameStart;
            while (nameEnd < strlen(tag) && !isspace(tag[nameEnd]) && tag[nameEnd] != '/') nameEnd++;
            int nameLen = nameEnd - nameStart;

            char tagName[HTML_MAX_TAG_NAME];
            strncpy(tagName, &tag[nameStart], nameLen < HTML_MAX_TAG_NAME ? nameLen : HTML_MAX_TAG_NAME - 1);
            tagName[nameLen < HTML_MAX_TAG_NAME ? nameLen : HTML_MAX_TAG_NAME - 1] = '\0';

            // Convert to lowercase
            for (int j = 0; tagName[j]; j++) tagName[j] = tolower(tagName[j]);

            HTMLToken* token = &result->tokens[result->count++];
            token->type = isClosing ? 2 : 1;
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

    return result;
}

void freeHTMLTokenArray(HTMLTokenArray* arr) {
    free(arr);
}

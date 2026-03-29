#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "css-tokenizer.h"

void parseCSS(const char* css, CSSTokenArray* result) {
    result->count = 0;
    int i = 0, len = strlen(css);

    while (i < len && result->count < CSS_MAX_TOKENS) {
        // Skip whitespace
        while (i < len && isspace(css[i])) i++;
        if (i >= len) break;

        // Handle comments
        if (i + 1 < len && css[i] == '/' && css[i + 1] == '*') {
            int commentStart = i;
            i += 2;
            while (i + 1 < len && !(css[i] == '*' && css[i + 1] == '/')) i++;
            if (i + 1 < len) i += 2; // skip '*/'

            int commentLen = i - commentStart;
            CSSToken* token = &result->tokens[result->count++];
            token->type = 2; // comment
            strncpy(token->data.comment.content, &css[commentStart],
                commentLen < CSS_MAX_PROPERTY_VALUE ? commentLen : CSS_MAX_PROPERTY_VALUE - 1);
            token->data.comment.content[commentLen < CSS_MAX_PROPERTY_VALUE ? commentLen : CSS_MAX_PROPERTY_VALUE - 1] = '\0';
            continue;
        }

        // Handle at-rules (@media, @import, etc.)
        if (css[i] == '@') {
            int ruleStart = i;
            while (i < len && css[i] != '{' && css[i] != ';') i++;

            int ruleLen = i - ruleStart;
            CSSToken* token = &result->tokens[result->count++];
            token->type = 1; // at-rule
            strncpy(token->data.atRule.rule, &css[ruleStart],
                ruleLen < CSS_MAX_SELECTOR_LEN ? ruleLen : CSS_MAX_SELECTOR_LEN - 1);
            token->data.atRule.rule[ruleLen < CSS_MAX_SELECTOR_LEN ? ruleLen : CSS_MAX_SELECTOR_LEN - 1] = '\0';

            // Skip to end of rule
            if (i < len && css[i] == '{') {
                int braceCount = 1;
                i++;
                while (i < len && braceCount > 0) {
                    if (css[i] == '{') braceCount++;
                    else if (css[i] == '}') braceCount--;
                    i++;
                }
            } else if (i < len && css[i] == ';') {
                i++;
            }
            continue;
        }

        // Parse selector rule
        int selectorStart = i;
        while (i < len && css[i] != '{') i++;
        int selectorLen = i - selectorStart;

        // Trim trailing whitespace from selector
        while (selectorLen > 0 && isspace(css[selectorStart + selectorLen - 1])) {
            selectorLen--;
        }

        if (i >= len || selectorLen == 0) break;

        CSSToken* token = &result->tokens[result->count++];
        token->type = 0; // selector rule
        strncpy(token->data.rule.selector, &css[selectorStart],
            selectorLen < CSS_MAX_SELECTOR_LEN ? selectorLen : CSS_MAX_SELECTOR_LEN - 1);
        token->data.rule.selector[selectorLen < CSS_MAX_SELECTOR_LEN ? selectorLen : CSS_MAX_SELECTOR_LEN - 1] = '\0';

        i++; // skip '{'

        // Parse properties
        token->data.rule.propertyCount = 0;
        while (i < len && css[i] != '}' && token->data.rule.propertyCount < CSS_MAX_PROPERTIES) {
            // Skip whitespace
            while (i < len && isspace(css[i])) i++;
            if (i >= len || css[i] == '}') break;

            // Parse property name
            int propNameStart = i;
            while (i < len && css[i] != ':' && css[i] != '}') i++;
            int propNameLen = i - propNameStart;

            // Trim whitespace from property name
            while (propNameLen > 0 && isspace(css[propNameStart + propNameLen - 1])) {
                propNameLen--;
            }

            if (propNameLen == 0 || i >= len || css[i] != ':') break;

            strncpy(token->data.rule.properties[token->data.rule.propertyCount].name,
                &css[propNameStart],
                propNameLen < CSS_MAX_PROPERTY_NAME ? propNameLen : CSS_MAX_PROPERTY_NAME - 1);
            token->data.rule.properties[token->data.rule.propertyCount].name[
                propNameLen < CSS_MAX_PROPERTY_NAME ? propNameLen : CSS_MAX_PROPERTY_NAME - 1] = '\0';

            i++; // skip ':'

            // Parse property value
            while (i < len && isspace(css[i])) i++;
            int propValueStart = i;
            while (i < len && css[i] != ';' && css[i] != '}') i++;
            int propValueLen = i - propValueStart;

            // Trim whitespace from property value
            while (propValueLen > 0 && isspace(css[propValueStart + propValueLen - 1])) {
                propValueLen--;
            }

            strncpy(token->data.rule.properties[token->data.rule.propertyCount].value,
                &css[propValueStart],
                propValueLen < CSS_MAX_PROPERTY_VALUE ? propValueLen : CSS_MAX_PROPERTY_VALUE - 1);
            token->data.rule.properties[token->data.rule.propertyCount].value[
                propValueLen < CSS_MAX_PROPERTY_VALUE ? propValueLen : CSS_MAX_PROPERTY_VALUE - 1] = '\0';

            token->data.rule.propertyCount++;

            if (i < len && css[i] == ';') i++; // skip ';'
        }

        if (i < len && css[i] == '}') i++; // skip '}'
    }
}

void freeCSS(CSSTokenArray* arr) {
    if (arr != NULL) {
        free(arr);
    }
}



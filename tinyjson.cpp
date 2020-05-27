/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft GPL 2.0
 */ 

#include "tinyjson.h"
#include <stdio.h>
#include <assert.h>    /* assert() */
#include <stdlib.h>    /* NULL, strtod(),  malloc(), realloc(), free()*/
#include <errno.h>     /* errno, ERANGE */
#include <math.h>      /* HUGE_VAL */
#include <cstring>      /* memcpy() */
//解析空白
static void TinyParseWhiteSpace(TinyContext* context) {
    const char *p = context->json;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    context->json = p;
}

static int TinyParseLiteral(TinyContext* context, TinyValue* value, const char* check, TinyType type) {
    assert(*context->json == check[0]);
    size_t i;
    for(i = 1; check[i] != '\0'; i++) {
        if(context->json[i] != check[i]) return TINY_PARSE_INVALID_VALUE;
    }
    context->json += i;
    value->type = type;
    return TINY_PARSE_OK;
}

static bool isDigit1To9(const char t) {
    return t >= '1' && t <= '9'; 
}

static bool isDigit(const char t) {
    return t >= '0' && t <= '9'; 
}

static int TinyParseNumber(TinyContext* context, TinyValue* value) {
    const char* p = context->json;
    if(*p == '-') p++;
    if(*p == '0') p++;
    else {
        if(!isDigit1To9(*p)) return TINY_PARSE_INVALID_VALUE;
        for(p++; isDigit(*p); p++);
    }
    if(*p == '.') {
        p++;
        if(!isDigit(*p)) return TINY_PARSE_INVALID_VALUE;
        for(p++; isDigit(*p); p++);
    }
    if(*p == 'e' || *p == 'E') {
        p++;
        if(*p == '-' || *p == '+') p++;
        if(!isDigit(*p)) return TINY_PARSE_INVALID_VALUE;
        for(p++; isDigit(*p); p++);
    }
    if(*p == 'e' || *p == 'E' || *p == '.' || isDigit1To9(*p)) return TINY_PARSE_INVALID_VALUE;
    value->num = strtod(context->json, NULL);
    //溢出的时候 errno = ERANGE  
    errno = 0;
    if(value->num == HUGE_VAL || value->num == -HUGE_VAL || errno == ERANGE) {

        return TINY_PARSE_NUMBER_TOO_BIG;
    }
    value->type = TINY_NUMBER;
    context->json = p;
    return TINY_PARSE_OK;
}

static int TinyParseValue(TinyContext* context, TinyValue* value) {
    switch(*context->json) {
        case 'n': return TinyParseLiteral(context, value, "null", TINY_NULL);
        case 't': return TinyParseLiteral(context, value, "true", TINY_TRUE);
        case 'f': return TinyParseLiteral(context, value, "false", TINY_FALSE);
        default: return TinyParseNumber(context, value);
        case '\0': return TINY_PARSE_EXPECT_VALUE;
    }
}

int TinyParse(TinyValue *value, const char* json) {
    TinyContext context;
    int ret;

    assert(value != NULL);
    context.json = json;
    value->type = TINY_NULL;
    TinyParseWhiteSpace(&context);
    ret = TinyParseValue(&context, value);

    if(ret == TINY_PARSE_OK) {
        TinyParseWhiteSpace(&context);
        if(*context.json != '\0') {
            ret = TINY_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

void TinySetNull(TinyValue* value) {
    TinyFree(value);
}

TinyType TinyGetType(const TinyValue* value) {
    assert(value != NULL);
    return value->type;
}

double TinyGetNumber(const TinyValue* value) {
    assert(value != NULL && value->type == TINY_NUMBER);
    return value->num;
}

void TinySetNumber(TinyValue* value, double num) {
    assert(value != NULL);
    value->num = num;
    value->type = TINY_NUMBER;
}

bool TinyGetBoolean(const TinyValue* value) {
    assert(value != NULL && (value->type == TINY_TRUE || value->type == TINY_FALSE));
    return value->type;
}

void TinySetBoolen(TinyValue* value, bool flag) {
    assert(value != NULL);
    if(flag == true) value->type = TINY_TRUE;
    else value->type = TINY_FALSE;
}

const char* TinyGetString(const TinyValue* value) {
    assert(value != NULL && value->type == TINY_STRING && value->str != NULL);
    return value->str;
}
size_t TinyGetStringLength(const TinyValue* value) {
    assert(value != NULL && value->type == TINY_STRING && value->str != NULL);
    return value->len;
}

void TinySetString(TinyValue *value, const char* str, size_t len) {
    assert(value != NULL && (str != NULL || len == 0));
    TinyFree(value);
    value->str = new char[len + 1];
    memcpy(value->str, str, len);
    value->str[len] = '\0';
    value->len = len;
    value->type = TINY_STRING;
}

void TinyInitValue(TinyValue *value) {
    value->type = TINY_NULL;
}

void TinyFree(TinyValue *value) {
    assert(value != NULL);
    if(value->type == TINY_STRING) {
        delete value->str;
    }
    value->type = TINY_NULL;
}








/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft GPL 2.0
 */ 

#include "tinyjson.h"
#include <stdio.h>
#include <assert.h>    /* assert() */
#include <stdlib.h>    /* NULL, strtod() */
#include <errno.h>     /* errno, ERANGE */
#include <math.h>      /* HUGE_VAL */

//解析空白
static void TinyParseWhiteSpace(TinyContext* context) {
    const char *p = context->json;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    context->json = p;
}

static int TinyParseLiteral(TinyContext* context, TinyValue* val, const char* check, TinyType type) {
    assert(*context->json == check[0]);
    size_t i;
    for(i = 1; check[i] != '\0'; i++) {
        if(context->json[i] != check[i]) return TINY_PARSE_INVALID_VALUE;
    }
    context->json += i;
    val->type = type;
    return TINY_PARSE_OK;
}

static bool isDigit1To9(const char t) {
    return t >= '1' && t <= '9'; 
}

static bool isDigit(const char t) {
    return t >= '0' && t <= '9'; 
}

static int TinyParseNumber(TinyContext* context, TinyValue* val) {
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
    val->num = strtod(context->json, NULL);
    //溢出的时候 errno = ERANGE  
    errno = 0;
    if(val->num == HUGE_VAL || val->num == -HUGE_VAL || errno == ERANGE) {

        return TINY_PARSE_NUMBER_TOO_BIG;
    }
    val->type = TINY_NUMBER;
    context->json = p;
    return TINY_PARSE_OK;
}

static int TinyParseValue(TinyContext* context, TinyValue* val) {
    switch(*context->json) {
        case 'n': return TinyParseLiteral(context, val, "null", TINY_NULL);
        case 't': return TinyParseLiteral(context, val, "true", TINY_TRUE);
        case 'f': return TinyParseLiteral(context, val, "false", TINY_FALSE);
        default: return TinyParseNumber(context, val);
        case '\0': return TINY_PARSE_EXPECT_VALUE;
    }
}

int TinyParse(TinyValue *val, const char* json) {
    TinyContext context;
    int ret;

    assert(val != NULL);
    context.json = json;
    val->type = TINY_NULL;
    TinyParseWhiteSpace(&context);
    ret = TinyParseValue(&context, val);

    if(ret == TINY_PARSE_OK) {
        TinyParseWhiteSpace(&context);
        if(*context.json != '\0') {
            ret = TINY_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

TinyType TinyGetType(const TinyValue* val) {
    assert(val != NULL);
    return val->type;
}

double TinyGetNumber(const TinyValue* val) {
    assert(val != NULL && val->type == TINY_NUMBER);
    return val->num;
}
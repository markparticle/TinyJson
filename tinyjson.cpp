/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft GPL 2.0
 */ 

#include "tinyjson.h"
#include <assert.h>
#include <stdlib.h>
#include <cstring>

//解析空白
static void TinyParseWhiteSpace(TinyContext* context) {
    const char *p = context->json;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    context->json = p;
}

static int TinyParseLiteral(TinyContext* context, TinyValue* val, const char* check, TinyType type) {
    assert(*context->json == check[0]);
    int cnt = 0;
    int len = strlen(check);
    for(int i = 0; check[i] != '\0'; i++) {
        if(context->json[i] != check[i]) return TINY_PARSE_INVALID_VALUE;
        cnt ++;
    }
    if(cnt != len) return TINY_PARSE_EXPECT_VALUE;
    context->json += cnt;
    val->type = type;
    return TINY_PARSE_OK;
}

static int TinyParseValue(TinyContext* context, TinyValue* val) {
    switch(*context->json) {
        case 'n': return TinyParseLiteral(context, val, "null", TINY_NULL);
        case 't': return TinyParseLiteral(context, val, "true", TINY_TRUE);
        case 'f': return TinyParseLiteral(context, val, "false", TINY_FALSE);
        case '\0': return TINY_PARSE_EXPECT_VALUE;
        default: return TINY_PARSE_INVALID_VALUE;
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
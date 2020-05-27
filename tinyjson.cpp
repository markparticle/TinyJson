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
    if (*p == 'e' || *p == 'E' || *p == '.' || isDigit1To9(*p)) return TINY_PARSE_INVALID_VALUE;
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

static void* TinyContextPush(TinyContext* context, size_t size) {
    void* ret;
    assert(size > 0);
    //开辟新空间
    if(context->top + size  >= context->size) {
        //初始化
        if(context->size == 0) {
            context->size = TINY_STACK_SIZE;
        }
        //如果超过缓冲空间，增大size
        while(context->top + size  >= context->size) {
            context->size += context->size >> 1; /* context->size * 1.5 */
        }
        //分配空间
        context->strStack = (char*)realloc(context->strStack, context->size);
    }
    //返回原top起点
    ret = context->strStack + context->top;
    context->top += size;
    return ret;
}

static void* TinyContextPop(TinyContext* context, size_t size) {
    assert(context->top >= size);
    context->top -= size;
    return context->strStack + context->top;
}

static void TinyPutC(TinyContext* context, const char ch) {
    char* top = (char*)TinyContextPush(context, sizeof(char));
    // 写入stack
    *top = ch;
}

static int TinyParseString(TinyContext* context, TinyValue* value) {
    size_t head = context->top;
    size_t len;
    const char* p;

    assert(*context->json == '\"');
    context->json++;
    
    p = context->json;
    while(true) {
        char ch = *p++;
        switch (ch) {
            case '\"':
            {
                //字符串结束
                len = context->top - head;
                const char* str = (const char*)TinyContextPop(context, len);
                TinySetString(value, str, len);
                context->json = p;
                return TINY_PARSE_OK;
            }
            case '\0':
            {
                context->top = head;
                return TINY_PARSE_MISS_QUOTATION_MARK;
            }
            case '\\':
                switch(*p++) {
                    case '\"': { TinyPutC(context, '\"'); break; }
                    case '\\': { TinyPutC(context, '\\'); break; }
                    case '/': { TinyPutC(context, '/'); break; }
                    case 'b': { TinyPutC(context, '\b'); break; }
                    case 'f': { TinyPutC(context, '\f'); break; }
                    case 'n': { TinyPutC(context, '\n'); break; }
                    case 'r': { TinyPutC(context, '\r'); break; }
                    case 't': { TinyPutC(context, '\t'); break; }
                    default:
                    {
                        context->top = head;
                        return TINY_PARSE_INVALID_STRING_ESCAPE;
                    }
                }
            default:
            {
                //不合法的字符是 %x00 至 %x1F
                if((unsigned char)ch < 0x20) {
                    context->top = head;
                    return TINY_PARSE_INVALID_STRING_CHAR;
                }
                TinyPutC(context, ch);
            }
        }
    }
}

static int TinyParseValue(TinyContext* context, TinyValue* value) {
    switch(*context->json) {
        case 'n': return TinyParseLiteral(context, value, "null", TINY_NULL);
        case 't': return TinyParseLiteral(context, value, "true", TINY_TRUE);
        case 'f': return TinyParseLiteral(context, value, "false", TINY_FALSE);
        default: return TinyParseNumber(context, value);
        case '\"': return TinyParseString(context, value);
        case '\0': return TINY_PARSE_EXPECT_VALUE;
    }
}

int TinyParse(TinyValue *value, const char* json) {
    assert(value != NULL);
    TinyContext context;
    int ret;

    TinyInitValue(value);
    //初始化context
    context.json = json;
    context.strStack = NULL;
    context.size = context.top = 0;

    TinyParseWhiteSpace(&context);
    ret = TinyParseValue(&context, value);

    if(ret == TINY_PARSE_OK) {
        TinyParseWhiteSpace(&context);
        if(*context.json != '\0') {
            value->type = TINY_NULL;
            ret = TINY_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(context.top == 0);
    free(context.strStack);
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
    TinyFree(value);
    value->num = num;
    value->type = TINY_NUMBER;
}

bool TinyGetBoolean(const TinyValue* value) {
    assert(value != NULL && (value->type == TINY_TRUE || value->type == TINY_FALSE));
    return value->type == TINY_TRUE;
}

void TinySetBoolen(TinyValue* value, bool flag) {
    TinyFree(value);
    if(flag) value->type = TINY_TRUE;
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
        delete[] value->str;
    }
    value->type = TINY_NULL;
}








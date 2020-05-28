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

//读取4位六进制
static const char* TinyParseHex4(const char* str, unsigned* u) {
    int i;
    *u = 0;
    for(i = 0; i < 4; i++) {
        char ch = *str++;
        //u左移4位
        //16进制数占4位二进制
        *u <<= 4;
        if     (ch >= '0' && ch <= '9')  *u |= ch - '0';
        else if(ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
        else if(ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        else return NULL;
    }
    return str;
}

static void TinyEncodeUtf8(TinyContext* context, unsigned u) {
    assert(u >= 0x0000 && u <= 0x10FFFF);
    // 1, 2, 3, 4字节数
    // 0xff : 1111 1111  避免一些编译器的警告误判

    /* 编码规则
    ** 0000 0000-0000 007F | 0xxxxxxx
    ** 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    ** 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    ** 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    */
    if(u <= 0x7f) {
        TinyPutC(context, u & 0xff);
    }
    else if(u <= 0x7ff) {
        // 0xc0 : 1100 0000
        TinyPutC(context, 0xc0 | ((u >> 6) & 0xff));
        // 0x80 : 1000 0000
        TinyPutC(context, 0x80 | (u         & 0x3f));
    }
    else if(u <= 0xffff) {
        //oxffff : 1111 1111 1111 1111 
        TinyPutC(context, 0xE0 | ((u >> 12) & 0xFF));
        TinyPutC(context, 0x80 | ((u >>  6) & 0x3F));
        TinyPutC(context, 0x80 | ( u        & 0x3F));
    }
    else if(u <= 0x10fffff) {
       //Unicode的最大码位为0x10FFFF       
        TinyPutC(context, 0xF0 | ((u >> 18) & 0xFF));
        TinyPutC(context, 0x80 | ((u >> 12) & 0x3F));
        TinyPutC(context, 0x80 | ((u >>  6) & 0x3F));
        TinyPutC(context, 0x80 | ( u        & 0x3F));
    }
}

#define STRING_ERROR(ret) do { context->top = head; return ret; } while(0)

static int TinyParseString(TinyContext* context, TinyValue* value) {
    size_t head = context->top;
    size_t len;
    const char* p;
    unsigned u, u2;
    
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
                    case 'u': {
                        //codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
                        p = TinyParseHex4(p, &u);
                        if(p == NULL) {
                            STRING_ERROR(TINY_PARSE_INVALID_UNICODE_HEX);
                        }
                        if(u >= 0xD800 && u <= 0xDBFF) {
                            if(*p++ != '\\') { 
                                STRING_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE); 
                            }
                            if(*p++ != 'u') { 
                                STRING_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE); 
                            }
                            //高代理项
                            p = TinyParseHex4(p, &u2);
                            if(p == NULL) {
                                STRING_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE);
                            }
                            if(u2 < 0xDC00 || u2 > 0xDFFF) {
                                STRING_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE);
                            }
                            u = (((u - 0xD800) << 10) || (u2 - 0xDC00)) + 0x10000;
                        }
                        TinyEncodeUtf8(context, u);
                        break;
                    }
                    default:
                    {
                        STRING_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE);
                    }
                }
                break;
            case '\0':
            {
                STRING_ERROR(TINY_PARSE_MISS_QUOTATION_MARK); 
            }
            default:
            {
                //非转义（unescaped）的字符，（0 ~ 31 是不合法的编码单元）
                //不合法字符 %x00 至 %x1F
                if((unsigned char)ch < 0x20) {
                    STRING_ERROR(TINY_PARSE_INVALID_STRING_CHAR);
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
        case '"': return TinyParseString(context, value);
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








/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft GPL 2.0
 */ 

#include "tinyjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdio.h>   /* sprintf() */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

static int TinyParseValue(TinyContext* context, TinyValue* value);

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
        context->stack = (char*)realloc(context->stack, context->size);
    }
    //返回 元素开始的 位置
    ret = context->stack + context->top;
    context->top += size;
    return ret;
}

static void* TinyContextPop(TinyContext* context, size_t size) {
    assert(context->top >= size);
    context->top -= size;
    return context->stack + context->top;
}

static void TinyPutC(TinyContext* context, const char ch) {
    char* top = (char*)TinyContextPush(context, sizeof(char));
    // 写入stack
    *top = ch;
}

static void TinyPutS(TinyContext* context, const char* str, size_t len) {
    memcpy(TinyContextPush(context, len), str, len);
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
    // 0xff : 1111 1111  避免一些编译器的警告误判
    // 变长编码
    /* 编码规则
    ** 0000 0000-0000 007F | 0xxxxxxx
    ** 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
                             0xC0
    ** 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
                             0xE0     0x80     0x80 
    ** 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    **                       0xF0
    */                       
    if(u <= 0x7f) {
        //最高位为0
        TinyPutC(context, u & 0xff);
    }
    else if(u <= 0x7ff) {
        //最高位为1100
        TinyPutC(context, 0xc0 | ((u >> 6) & 0xff));
        TinyPutC(context, 0x80 | (u         & 0x3f));  
    }
    else if(u <= 0xffff) {
        //最高位为1110
        // 填入第一部分 左移12位 
        TinyPutC(context, 0xE0 | ((u >> 12) & 0xFF)); 
        // 填入第二部分 左移6位
        TinyPutC(context, 0x80 | ((u >>  6) & 0x3F)); 
        // 填入第三部分 
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

static int TinyParseStringRaw(TinyContext* context, char** str, size_t* len) {
    size_t head = context->top;
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
                *len = context->top - head;
                *str = (char*)TinyContextPop(context, *len);
                context->json = p;
                return TINY_PARSE_OK;
            }
            case '\\':
                //解析转义符和utf8字符
                switch(*p++) {
                    case '\"': TinyPutC(context, '\"'); break;
                    case '\\': TinyPutC(context, '\\'); break;
                    case '/': TinyPutC(context, '/'); break;
                    case 'b': TinyPutC(context, '\b'); break;
                    case 'f': TinyPutC(context, '\f'); break;
                    case 'n': TinyPutC(context, '\n'); break;
                    case 'r': TinyPutC(context, '\r'); break;
                    case 't': TinyPutC(context, '\t'); break;
                    case 'u': {
                        p = TinyParseHex4(p, &u);
                        if(p == NULL) {
                            STRING_ERROR(TINY_PARSE_INVALID_UNICODE_HEX);
                        }
                        // surrogate pair \uXXXX\uYYYY 
                        // 扩展字符而使用的编码方式 (两个UTF-16编码)来表示一个字符
                        if(u >= 0xD800 && u <= 0xDBFF) {
                            // 第一个码点是 U+D800 至 U+DBFF 正确的的高代理项
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
                            //第二个码 U+DC00 至 U+DFFF 的低代理项（low surrogate）
                            if(u2 < 0xDC00 || u2 > 0xDFFF) {
                                STRING_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE);
                            }
                            //  ‭0001 0000 0000 0000 0000‬  0x10000
                            //  ‭     1101 1000 0000 0000  0xD8
                            //       ‭1101 1100 0000 0000‬  0xDc
                            //codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
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

static int TinyParseString(TinyContext* context, TinyValue* value) {
    int ret;
    char* str;
    size_t len;
    ret = TinyParseStringRaw(context, &str, &len);
    if(ret == TINY_PARSE_OK) {
        TinySetString(value, str, len);
    }
    return ret;
}

static int TinyParseArray(TinyContext* context, TinyValue* value) {
    size_t size = 0;
    int ret;

    assert(*context->json == '[');
    context->json++;

    TinyParseWhiteSpace(context);
    if(*context->json == ']') {
        context->json++;
        value->type = TINY_ARRAY;
        value->size = 0;
        value->array = NULL;
        return TINY_PARSE_OK;
    }

    while(true) {
        TinyValue element;
        TinyInitValue(&element);
        ret = TinyParseValue(context, &element);
        if(ret != TINY_PARSE_OK) {
            break;
        }
        memcpy(TinyContextPush(context, sizeof(TinyValue)), &element, sizeof(TinyValue));
        size++;
        TinyParseWhiteSpace(context);
        if(*context->json == ',') {
            context->json++;
            TinyParseWhiteSpace(context);
        }
        else if(*context->json == ']') {
            context->json++;
            value->type = TINY_ARRAY;
            value->size = size;
            size = size * sizeof(TinyValue);
            value->array = (TinyValue*)malloc(size);
            memcpy(value->array, TinyContextPop(context, size), size);
            return TINY_PARSE_OK;
        } else {
            ret = TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    //失败后需要释放压入缓存区的元素
    for(size_t i = 0; i < size; i++) {
        TinyFree((TinyValue*)TinyContextPop(context, sizeof(TinyValue)));
    }
    return ret;
}

static int TinyParseObject(TinyContext* context, TinyValue* value) {
    size_t size;
    TinyMember m;
    int ret;

    assert(*context->json == '{');
    context->json++;

    TinyParseWhiteSpace(context);
    if(*context->json == '}') {
        context->json++;
        value->type = TINY_OBJECT;
        value->member = 0;
        value->msize = 0;
        return TINY_PARSE_OK;
    }

    m.key = NULL;
    size = 0;
    while(true) {
        char * str;
        TinyInitValue(&m.value);

        // 1. parse key
        if(*context->json != '"') {
            ret = TINY_PARSE_MISS_KEY;
            break;
        }
        ret = TinyParseStringRaw(context, &str, &m.kLen);
        if(ret != TINY_PARSE_OK) {
            break;
        }
        m.key = (char*)malloc(m.kLen + 1);
        memcpy(m.key, str, m.kLen);
        m.key[m.kLen] = '\0';

        // 2. parse colon
        TinyParseWhiteSpace(context);
        if(*context->json != ':') {
            ret = TINY_PARSE_MISS_COLON;
            break;
        }
        context->json++;

        // 3. parse value
        TinyParseWhiteSpace(context);
        ret = TinyParseValue(context, &m.value);
        if(ret != TINY_PARSE_OK) {
            break;
        }
        // 写入缓冲区
        memcpy(TinyContextPush(context, sizeof(TinyMember)), &m, sizeof(TinyMember));
        size++;
        m.key = NULL;

        // 4. parse  comma / right-curly-brace
        TinyParseWhiteSpace(context);
        if(*context->json == ',') {
            context->json++;
            TinyParseWhiteSpace(context);
        }
        else if (*context->json == '}') {
            context->json++;
            value->type = TINY_OBJECT;
            value->msize = size;
            size_t s = sizeof(TinyMember) * size;
            value->member = (TinyMember*)malloc(s);
            memcpy(value->member, TinyContextPop(context, s), s);
            return TINY_PARSE_OK;
        } else {
            ret = TINY_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    
    free(m.key);
    for(size_t i = 0; i < size; i++) {
        TinyMember* m = (TinyMember*) TinyContextPop(context, sizeof(TinyMember));
        free(m->key);
        TinyFree(&m->value);
    }
    value->type = TINY_NULL;
    return ret;
}

static int TinyParseValue(TinyContext* context, TinyValue* value) {
    switch(*context->json) {
        case 'n': return TinyParseLiteral(context, value, "null", TINY_NULL);
        case 't': return TinyParseLiteral(context, value, "true", TINY_TRUE);
        case 'f': return TinyParseLiteral(context, value, "false", TINY_FALSE);
        default: return TinyParseNumber(context, value);
        case '"': return TinyParseString(context, value);
        case '[': return TinyParseArray(context, value);
        case '{': return TinyParseObject(context, value);
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
    context.stack = NULL;
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
    free(context.stack);
    return ret;
}

static void TinyStringifyString(TinyContext* context, const char *str, size_t len) {
    static const char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                      'A', 'B', 'C', 'D', 'E', 'F'};
    size_t size = len * 6 + 2; //  "\u00xx"
    char* head, *p;

    assert(str != NULL);
    p = head = (char*)TinyContextPush(context, size);
    *p++ = '"';
    for(size_t i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)str[i];
        switch (ch)
        {
        case '\"': *p++ = '\\'; *p++ = '\"'; break;
        case '\\': *p++ = '\\'; *p++ = '\\'; break;
        case '\b': *p++ = '\\'; *p++ = 'b'; break;
        case '\f': *p++ = '\\'; *p++ = 'f'; break;
        case '\n': *p++ = '\\'; *p++ = 'n'; break;
        case '\r': *p++ = '\\'; *p++ = 'r'; break;
        case '\t': *p++ = '\\'; *p++ = 't'; break;
        default:
            // 小于0x20的字符需要转义为\u00xx的形式
            if(ch < 0x20) {
                char buff[7];
                sprintf(buff, "\\u%04X", ch); // 按16进制输出(大写)，最小输出宽度为4个字符
                *p++ = '\\'; 
                *p++ = 'u'; 
                *p++ = '0'; 
                *p++ = '0';
                *p++ = hexDigits[ch >> 4];
                *p++ = hexDigits[ch & 15];
            } else {
                *p++ = str[i];
            }
            break;
        }
    }
    *p++ = '"';
    context->top -= size - (p - head);     //对齐
}

static void TinyStringifyValue(TinyContext* context, const TinyValue* value) {
    switch (value->type)
    {
    case TINY_NULL: TinyPutS(context, "null", 4); break;
    case TINY_FALSE: TinyPutS(context, "false", 5); break;
    case TINY_TRUE: TinyPutS(context, "true", 4); break;
    case TINY_STRING: TinyStringifyString(context, value->str, value->len); break;
    case TINY_NUMBER:
        {
             char* buff = (char*)TinyContextPush(context, 32);
             int len = sprintf(buff, "%.17g", value->num);
             context->top -= 32 - len;
        }
        break;
    case TINY_ARRAY:
        {
            TinyPutC(context, '[');
            for(size_t i = 0; i < value->size; i++) {
                if(i > 0) TinyPutC(context, ',');
                TinyStringifyValue(context, &value->array[i]);
            }
            TinyPutC(context, ']');
        }
        break;
    case TINY_OBJECT:
        {
            TinyPutC(context, '{');
            for(size_t i = 0; i < value->msize; i++) {
                if(i > 0) TinyPutC(context, ',');
                TinyStringifyString(context, value->member[i].key, value->member[i].kLen);
                TinyPutC(context, ':');
                TinyStringifyValue(context, &value->member[i].value);
            }
            TinyPutC(context, '}');
        }
        break;
    default:
        assert(0 && "invalid type");
        break;
    }
}

char* TinyStringify(const TinyValue* value, size_t* len) {
    TinyContext context;
    assert(value != NULL);

    context.size = TINY_STACK_SIZE;
    context.stack = (char*)malloc(context.size);
    context.top = 0;

    TinyStringifyValue(&context, value);
    if(len > 0) *len = context.top;
    TinyPutC(&context, '\0');

    return context.stack;
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
    value->str = (char*)malloc(len + 1);
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
    switch (value->type)
    {
    case TINY_STRING:
        free(value->str);
        break;
    case TINY_ARRAY:
        //释放元素
        for(size_t i = 0; i < value->size; i++) {
            TinyFree(&value->array[i]);
        }
        //释放数组
        free(value->array);
        break;
    case TINY_OBJECT:
        for(size_t i = 0; i < value->msize; i++){
            free(value->member[i].key);
            TinyFree(&value->member[i].value);
        }
        free(value->member);
        break;
    default:
        break;
    }
    value->type = TINY_NULL;
}

size_t TinyGetArraySize(const TinyValue* value) {
    assert(value != NULL && value->type == TINY_ARRAY);
    return value->size;
}

TinyValue* TinyGetArrayElement(const TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_ARRAY);
    assert(index < value->size);
    return &value->array[index];
}

size_t TinyGetObjectSize(const TinyValue* value) {
    assert(value != NULL && value->type == TINY_OBJECT);
    return value->msize;
}

const char* TinyGetObjectKey(const TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_OBJECT);
    assert(index < value->msize);
    return value->member[index].key;
}

size_t TinyGetObjectKeyLength(const TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_OBJECT);
    assert(index < value->msize);
    return value->member[index].kLen;
}

TinyValue* TinyGetObjectValue(const TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_OBJECT);
    assert(index < value->msize);
    return &value->member[index].value;
}
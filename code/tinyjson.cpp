/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft Apache 2.0
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
        TinySetArray(value, 0);
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
            TinySetArray(value, size);
            value->size = size;
            memcpy(value->array, TinyContextPop(context,  size * sizeof(TinyValue)),  size * sizeof(TinyValue));
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
        TinySetObject(value, 0);
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
            TinySetObject(value, size);
            value->osize = size;
            memcpy(value->object, TinyContextPop(context, sizeof(TinyMember) * size), sizeof(TinyMember) * size);
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
            for(size_t i = 0; i < value->osize; i++) {
                if(i > 0) TinyPutC(context, ',');
                TinyStringifyString(context, value->object[i].key, value->object[i].kLen);
                TinyPutC(context, ':');
                TinyStringifyValue(context, &value->object[i].value);
            }
            TinyPutC(context, '}');
        }
        break;
    default:
        assert(0 && "invalid type");
        break;
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

void TinyInitValue(TinyValue *value) {
    value->type = TINY_NULL;
}

void TinyFree(TinyValue *value) {
    assert(value != NULL);
    switch (value->type)
    {
    case TINY_STRING:
        free(value->str);
        value->len = 0;
        break;
    case TINY_ARRAY:
        //释放元素
        for(size_t i = 0; i < value->size; i++) {
            TinyFree(&value->array[i]);
        }
        //释放数组
        free(value->array);
        value->size = 0;
        break;
    case TINY_OBJECT:
        for(size_t i = 0; i < value->osize; i++){
            free(value->object[i].key);
            TinyFree(&value->object[i].value);
        }
        free(value->object);
        value->osize = 0;
        break;
    default:
        break;
    }
    value->type = TINY_NULL;
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
    assert(value != NULL);
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
    TinyFree(value);
    value->num = num;
    value->type = TINY_NUMBER;
}

bool TinyGetBoolean(const TinyValue* value) {
    assert(value != NULL && (value->type == TINY_TRUE || value->type == TINY_FALSE));
    return value->type == TINY_TRUE;
}

void TinySetBoolen(TinyValue* value, bool flag) {
    assert(value != NULL);
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

void TinySetArray(TinyValue* value, size_t capacity) {
    assert(value != NULL);
    TinyFree(value);
    value->type = TINY_ARRAY;
    value->size = 0;
    value->capacity = capacity;
    if(capacity > 0) {
        value->array = (TinyValue*)malloc(capacity * sizeof(TinyValue));
    } else {
        value->array = NULL;
    }
}

size_t TinyGetArrayCapacity(TinyValue* value) {
    assert(value != NULL && value->type == TINY_ARRAY);
    return value->capacity;
}

size_t TinyGetArraySize(const TinyValue* value) {
    assert(value != NULL && value->type == TINY_ARRAY);
    return value->size;
}

void TinyReserveArray(TinyValue* value, size_t capacity) {
    assert(value != NULL && value->type == TINY_ARRAY);
    if(value->capacity < capacity) {
        value->capacity = capacity;
        value->array = (TinyValue*) realloc(value->array, capacity * sizeof(TinyValue));
    }
}

void TinyShrinkArray(TinyValue* value) {
    assert(value != NULL && value->type == TINY_ARRAY);
    if(value->capacity > value->size) {
        value->capacity = value->size;
        value->array = (TinyValue*) realloc(value->array, value->capacity * sizeof(TinyValue));
    }
}

TinyValue* TinyPushBackArrayElement(TinyValue *value) {
    assert(value != NULL && value->type == TINY_ARRAY);
    if(value->size == value->capacity) {
        if(value->capacity == 0) {
             TinyReserveArray(value, 1);
        } else {
            TinyReserveArray(value, value->capacity * 2);
        }
    }
    TinyInitValue(&value->array[value->size]);
    return &value->array[value->size++];
} 

void TinyPopBackArrayElement(TinyValue* value) {
    assert(value != NULL && value->type == TINY_ARRAY && value->size > 0);
    TinyFree(&value->array[--value->size]);
}

TinyValue* TinyGetArrayElement(const TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_ARRAY && index < value->size);
    return &value->array[index];
}

TinyValue* TinyInsertArrayElement(TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_ARRAY && index < value->size);
    if(value->size + 1 > value->capacity) {
        TinyPushBackArrayElement(value);      
    } else {
        value->size++;
    }
    for(size_t i = value->size - 1; i > index; i--) {
        value->array[i] = value->array[i - 1];
    }
    return &value->array[index];
}

void TinyEraseArrayElement(TinyValue* value, size_t index, size_t count) {
    size_t i;
    assert(value != NULL && value->type == TINY_ARRAY);
    assert(count >= 0 && count + index <= value->size );

    for(i = index; i + count < value->size; i++) {
        value->array[i] = value->array[i + count];
    }
    for(; i < value->size; i++) {
        TinyFree(&value->array[i]);
    }
    value->size -= count;
}

void TinyClearArray(TinyValue* value) {
    assert(value != NULL && value->type == TINY_ARRAY);
    TinyEraseArrayElement(value, 0, value->size);
}

size_t TinyGetObjectSize(const TinyValue* value) {
    assert(value != NULL && value->type == TINY_OBJECT);
    return value->osize;
}

const char* TinyGetObjectKey(const TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_OBJECT && index < value->osize);
    return value->object[index].key;
}

size_t TinyGetObjectKeyLength(const TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_OBJECT && index < value->osize);
    return value->object[index].kLen;
}

TinyValue* TinyGetObjectValue(const TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_OBJECT && index < value->osize);
    return &value->object[index].value;
}

size_t TinyFindObjectIndex(const TinyValue* value, const char* key, size_t klen) {
    assert(value != NULL && value->type == TINY_OBJECT && key != NULL);
    for(size_t i = 0; i < value->osize; i++) {
        if(value->object[i].kLen == klen && memcmp(value->object[i].key, key, klen) == 0) {
            return i;
        }
    }
    return TINY_KEY_NOT_EXIST;
}

TinyValue* TinyFindObjectValue(const TinyValue* value, const char* key, size_t klen) {
    size_t index = TinyFindObjectIndex(value, key, klen);
    if(index == TINY_KEY_NOT_EXIST) return NULL;
    return &value->object[index].value;
}

TinyValue* TinySetObjectValue(TinyValue* value, const char* key, size_t klen) {
    assert(value != NULL && value->type == TINY_OBJECT && key != NULL && klen != 0);
    size_t index = TinyFindObjectIndex(value, key, klen);

    if(index != TINY_KEY_NOT_EXIST) {
        return &value->object[index].value;
    }

    if(value->osize == value->ocapacity) {
        if(value->ocapacity == 0) {
             TinyReserveObject(value, 1);
        } else {
            TinyReserveObject(value, value->ocapacity * 2);
        }
    }

    TinyMember &m = value->object[value->osize++];
    m.kLen = klen;
    m.key = (char*)malloc(klen + 1);
    memcpy(m.key, key, klen);
    TinyInitValue(&m.value);
    return &m.value;
}

void TinySetObject(TinyValue* value, size_t capacity) {
    assert(value != NULL);
    TinyFree(value);
    value->type = TINY_OBJECT;
    value->osize = 0;
    value->ocapacity = capacity;
    if(capacity > 0) {
        value->object = (TinyMember*)malloc(capacity * sizeof(TinyMember));
    } else {
        value->object = NULL;
    }
}

size_t TinyGetObjectCapacity(const TinyValue* value) {
    assert(value != NULL && value->type == TINY_OBJECT);
    return value->ocapacity;
}

void TinyReserveObject(TinyValue* value, size_t capacity) {
    assert(value != NULL && value->type == TINY_OBJECT);
    if(value->ocapacity < capacity) {
        value->ocapacity = capacity;
        value->object = (TinyMember*) realloc(value->object, capacity * sizeof(TinyMember));
    }
}

void TinyShrinkObject(TinyValue* value) {
    assert(value != NULL && value->type == TINY_OBJECT);
    if(value->ocapacity > value->osize) {
        value->ocapacity = value->osize;
        value->object = (TinyMember*) realloc(value->object, value->ocapacity * sizeof(TinyMember));
    }
}

void TinyClearObject(TinyValue* value) {
    assert(value != NULL && value->type == TINY_OBJECT);
    for(size_t i = 0; i < value->osize; i++) {
        free(value->object[i].key);
        TinyFree(&value->object[i].value);
    }
    value->size = 0;
}

void TinyRemoveObjectValue(TinyValue* value, size_t index) {
    assert(value != NULL && value->type == TINY_OBJECT && index < value->osize);
    for(size_t i = index + 1; i < value->osize; i++) {
        memcpy(value->object[i - 1].key, value->object[i].key, value->object[i].kLen);
        value->object[i - 1].value = value->object[i].value;
        value->object[i - 1].kLen = value->object[i].kLen;
    }
    value->osize--;
    free(value->object[value->osize].key);
    TinyFree(&value->object[value->osize].value);
}

bool TinyIsEqual(const TinyValue* lhs, const TinyValue* rhs) {
    assert(lhs != NULL && rhs != NULL);
    if(lhs->type != rhs->type) return false;
    switch(lhs->type) {
        case TINY_STRING:
            return (lhs->len == rhs->len && memcmp(lhs->str, rhs->str, lhs->len) == 0);
        case TINY_NUMBER:
            return lhs->num == rhs->num;
        case TINY_ARRAY:
            if(lhs->size != rhs->size) return false;
            for(size_t i = 0; i < lhs->size; i++) {
                if(TinyIsEqual(&lhs->array[i], &rhs->array[i]) == false) {
                    return false;
                }
            }
            return true;
        case TINY_OBJECT:
            if(lhs->osize != rhs->osize) return false;
            for(size_t i = 0; i < lhs->osize; i++) {
                TinyValue* t = TinyFindObjectValue(rhs, lhs->object[i].key, lhs->object[i].kLen);
                if(t == NULL || TinyIsEqual(t, &lhs->object[i].value) == false) return false;         
            }
            return true;
        default:
            return true;
    }
}

void TinyCopy(TinyValue* dst, const TinyValue* src) {
    assert(src != NULL && dst != NULL && src != dst);
    TinyInitValue(dst);
    switch (src->type)
    {
    case TINY_STRING:
        TinySetString(dst, src->str, src->len);
        break;
    case TINY_ARRAY:
        TinySetArray(dst, src->size);
        dst->size = src->size;
        for(size_t i = 0; i < src->size; i++) {
            TinyCopy(&dst->array[i], &src->array[i]);
        }
        dst->type = src->type;
        break;
    case TINY_OBJECT:
        TinySetObject(dst, src->osize);
        dst->osize = src->osize;
        for(size_t i = 0; i < src->osize; i++) {
            TinyMember &m = dst->object[i];
            m.kLen = src->object[i].kLen;
            m.key = (char*)malloc(m.kLen + 1);
            memcpy(dst->object[i].key, src->object[i].key, m.kLen);
            TinyCopy(&m.value, &src->object[i].value);      
        }
        dst->type = src->type;
        break;
    default:
        memcpy(dst, src, sizeof(TinyValue));
        dst->type = src->type;
        break;
    }
}

void TinyMove(TinyValue* dst, TinyValue* src) {
    assert(dst != NULL && src != NULL && src != dst);
    TinyFree(dst);
    memcpy(dst, src, sizeof(TinyValue));
    TinyInitValue(src);
}

void TinySwap(TinyValue* lhs, TinyValue* rhs) {
    assert(lhs != NULL && rhs != NULL);
    if(lhs != rhs) {
        TinyValue tmp;
        memcpy(&tmp, lhs, sizeof(TinyValue));
        memcpy(lhs, rhs, sizeof(TinyValue));
        memcpy(rhs, &tmp, sizeof(TinyValue));
    }
}
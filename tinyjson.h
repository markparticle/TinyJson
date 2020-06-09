/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft GPL 2.0
 */ 

#ifndef TINYJSON_H
#define TINYJSON_H

#include <stddef.h> /* size_t */

const size_t TINY_STACK_SIZE = 256;

typedef struct TinyValue TinyValue; 
typedef struct TinyMember TinyMember; 

enum TinyType {
    TINY_NULL,
    TINY_FALSE,
    TINY_TRUE,
    TINY_NUMBER,
    TINY_STRING,
    TINY_ARRAY,
    TINY_OBJECT,
};

struct TinyValue {
    union {
        struct {
            TinyMember* member;
            size_t msize;
        };
        struct {
            char *str;
            size_t len;
        };
        struct {
            TinyValue* array;
            size_t size;
        };
        double num;
    };
    TinyType type;
};

struct TinyMember {
    char* key;
    size_t kLen;
    TinyValue value;
};

struct TinyContext {
    const char* json;
    char * stack;
    size_t size, top;
};

enum TinyParseReact{
    TINY_PARSE_OK = 0,
    TINY_PARSE_EXPECT_VALUE,
    TINY_PARSE_INVALID_VALUE,
    TINY_PARSE_ROOT_NOT_SINGULAR,

    TINY_PARSE_NUMBER_TOO_BIG,

    TINY_PARSE_MISS_QUOTATION_MARK,       //缺少”
    TINY_PARSE_INVALID_STRING_CHAR,       //非法字符
    TINY_PARSE_INVALID_STRING_ESCAPE,     //非法的转码符

    TINY_PARSE_INVALID_UNICODE_HEX,       //不符合4位十六进制数字
    TINY_PARSE_INVALID_UNICODE_SURROGATE, //范围不正确 U+0000 ~ U+10FFFF

    TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,

    TINY_PARSE_MISS_KEY,
    TINY_PARSE_MISS_COLON,
    TINY_PARSE_MISS_COMMA_OR_CURLY_BRACKET,

    TINY_STRINGIFY_OK,
};

void TinyInitValue(TinyValue *value);
void TinyFree(TinyValue *value);

int TinyParse(TinyValue *value, const char* json);
char* TinyStringify(const TinyValue* value, size_t* len);

TinyType TinyGetType(const TinyValue* value);
bool TinyGetBoolean(const TinyValue* value);
double TinyGetNumber(const TinyValue* value);

const char* TinyGetString(const TinyValue* value);
size_t TinyGetStringLength(const TinyValue* value);

size_t TinyGetArraySize(const TinyValue* value);
TinyValue* TinyGetArrayElement(const TinyValue* value, size_t index);

size_t TinyGetObjectSize(const TinyValue* value);
const char* TinyGetObjectKey(const TinyValue* value, size_t index);
size_t TinyGetObjectKeyLength(const TinyValue* value, size_t index);
TinyValue* TinyGetObjectValue(const TinyValue* value, size_t index);

void TinySetNull(TinyValue* value);
void TinySetBoolen(TinyValue* value, bool flag);
void TinySetNumber(TinyValue* value, double num);
void TinySetString(TinyValue* value, const char* str, size_t len);

#endif // TINYJSON_H
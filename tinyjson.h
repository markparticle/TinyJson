/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft GPL 2.0
 */ 

#ifndef TINYJSON_H
#define TINYJSON_H
#include <stdlib.h>

const size_t TINY_STACK_SIZE = 256;

enum TinyType {
    TINY_NULL,
    TINY_FALSE,
    TINY_TRUE,
    TINY_NUMBER,
    TINY_STRING,
    TINY_ARRAY,
    TINY_OBJECT
};

struct TinyValue {
    union {
        struct {
            char *str;
            size_t len;
        };
        double num;
    };
    TinyType type;
};

struct TinyContext {
    const char* json;
    char * strStack;
    size_t size, top;
};

enum TinyParseReact{
    TINY_PARSE_OK = 0,
    TINY_PARSE_EXPECT_VALUE,
    TINY_PARSE_INVALID_VALUE,
    TINY_PARSE_ROOT_NOT_SINGULAR,
    TINY_PARSE_NUMBER_TOO_BIG,
    TINY_PARSE_MISS_QUOTATION_MARK,
};

void TinyFree(TinyValue *value);

int TinyParse(TinyValue *value, const char* json);

void TinyInitValue(TinyValue *value);

TinyType TinyGetType(const TinyValue* value);
bool TinyGetBoolean(const TinyValue* value);
double TinyGetNumber(const TinyValue* value);
const char* TinyGetString(const TinyValue* value);
size_t TinyGetStringLength(const TinyValue* value);

void TinySetNull(TinyValue* value);
void TinySetBoolen(TinyValue* value, bool flag);
void TinySetNumber(TinyValue* value, double num);
void TinySetString(TinyValue* value, const char* str, size_t len);



#endif // TINYJSON_H
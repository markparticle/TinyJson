/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft GPL 2.0
 */ 

#ifndef TINYJSON_H
#define TINYJSON_H

enum TinyType {
    TINY_NULL,
    TINY_FALSE,
    TINY_TRUE,
    TINY_NUMBER,
    TINY_SIRING,
    TINY_ARRAY,
    TINY_OBJECT
};

struct TinyValue {
    //type == TINY_NUMBER 
    double num;
    TinyType type;
};

struct TinyContext {
    const char* json;
};

enum TinyParseReact{
    TINY_PARSE_OK = 0,
    TINY_PARSE_EXPECT_VALUE,
    TINY_PARSE_INVALID_VALUE,
    TINY_PARSE_ROOT_NOT_SINGULAR,
    TINY_PARSE_NUMBER_TOO_BIG
};

int TinyParse(TinyValue *v, const char* json);

TinyType TinyGetType(const TinyValue* val);
double TinyGetNumber(const TinyValue* val);


#endif // TINYJSON_H
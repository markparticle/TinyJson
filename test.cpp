/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft GPL 2.0
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tinyjson.h"

static int testCount = 0;
static int testPass = 0;
static int mainRet = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        testCount++;\
        if(equality)\
            testPass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            mainRet = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")


static void TestParseNull() {
    TinyValue val;
    val.type = TINY_FALSE;
    EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&val, "null"));
    EXPECT_EQ_INT(TINY_NULL, TinyGetType(&val));
}

static void TestParseFalse() {
    TinyValue val;
    val.type = TINY_TRUE;
    EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&val, "false"));
    EXPECT_EQ_INT(TINY_FALSE, TinyGetType(&val));
}

static void TestParseTrue() {
    TinyValue val;
    val.type = TINY_FALSE;
    EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&val, "true"));
    EXPECT_EQ_INT(TINY_TRUE, TinyGetType(&val));
}

static void TestParseExpectValue() {
    TinyValue val;
    val.type = TINY_FALSE;
    EXPECT_EQ_INT(TINY_PARSE_EXPECT_VALUE, TinyParse(&val, ""));
    EXPECT_EQ_INT(TINY_NULL, TinyGetType(&val));

    val.type = TINY_FALSE;
    EXPECT_EQ_INT(TINY_PARSE_EXPECT_VALUE, TinyParse(&val, " "));
    EXPECT_EQ_INT(TINY_NULL, TinyGetType(&val));
}

static void TestParseInvalidValue() {
    TinyValue val;
    val.type = TINY_FALSE;
    EXPECT_EQ_INT(TINY_PARSE_INVALID_VALUE, TinyParse(&val, "nul"));
    EXPECT_EQ_INT(TINY_NULL, TinyGetType(&val));
    
    val.type = TINY_FALSE;
    EXPECT_EQ_INT(TINY_PARSE_INVALID_VALUE, TinyParse(&val, "?"));
    EXPECT_EQ_INT(TINY_NULL, TinyGetType(&val));
}

static void TestParseRootNotSingular() {
    TinyValue val;
    val.type = TINY_FALSE;
    EXPECT_EQ_INT(TINY_PARSE_ROOT_NOT_SINGULAR, TinyParse(&val, "null x"));
    EXPECT_EQ_INT(TINY_NULL, TinyGetType(&val));
}

static void TestParse() {
    TestParseNull();
    TestParseFalse();
    TestParseExpectValue();
    TestParseInvalidValue();
    TestParseRootNotSingular();
    TestParseTrue();
}

int main() {
    TestParse();
    printf("%d/%d (%3.2f%%) passed!\n", testPass, testCount, 100.0 * testPass / testCount);
    return mainRet;
}
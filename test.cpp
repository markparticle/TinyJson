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

#define TESTPARSE(react, valType, json)\
    do {\
        TinyValue val;\
        val.type = TINY_FALSE;\
        EXPECT_EQ_INT(react, TinyParse(&val, json));\
        EXPECT_EQ_INT(valType, TinyGetType(&val));\
    } while(0)


static void TestParse() {
    TESTPARSE(TINY_PARSE_OK, TINY_NULL, "null");
    TESTPARSE(TINY_PARSE_OK, TINY_FALSE, "false");
    TESTPARSE(TINY_PARSE_OK, TINY_TRUE, "true");
    TESTPARSE(TINY_PARSE_EXPECT_VALUE, TINY_NULL, "");
    TESTPARSE(TINY_PARSE_EXPECT_VALUE, TINY_NULL, " ");
    TESTPARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "nul");
    TESTPARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "NULL");
    TESTPARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "?");
    TESTPARSE(TINY_PARSE_ROOT_NOT_SINGULAR, TINY_NULL, "null x");
}

int main() {
    TestParse();
    printf("%d/%d (%3.2f%%) passed!\n", testPass, testCount, 100.0 * testPass / testCount);
    return mainRet;
}
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
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17lf")

#define TEST_PARSE(react, valType, json)\
    do {\
        TinyValue val;\
        EXPECT_EQ_INT(react, TinyParse(&val, json));\
        EXPECT_EQ_INT(valType, TinyGetType(&val));\
    } while(0)

#define TEST_PARSE_NUMBER(react, expect, json)\
    do {\
        TinyValue val;\
        EXPECT_EQ_INT(react, TinyParse(&val, json));\
        EXPECT_EQ_INT(TINY_NUMBER, TinyGetType(&val));\
        EXPECT_EQ_DOUBLE(expect, TinyGetNumber(&val));\
    } while(0)

static void TestParse() {
    TEST_PARSE(TINY_PARSE_OK, TINY_NULL, "null");
    TEST_PARSE(TINY_PARSE_OK, TINY_FALSE, "false");
    TEST_PARSE(TINY_PARSE_OK, TINY_TRUE, "true");
    TEST_PARSE(TINY_PARSE_EXPECT_VALUE, TINY_NULL, "");
    TEST_PARSE(TINY_PARSE_EXPECT_VALUE, TINY_NULL, " ");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "nul");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "NULL");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "?");
    TEST_PARSE(TINY_PARSE_ROOT_NOT_SINGULAR, TINY_NULL, "null x");
}

static void TestParseNumberToBig() {
#if 1
    TEST_PARSE(TINY_PARSE_NUMBER_TOO_BIG, TINY_NULL, "1e309");
    TEST_PARSE(TINY_PARSE_NUMBER_TOO_BIG, TINY_NULL, "-1e309");
    TEST_PARSE(TINY_PARSE_NUMBER_TOO_BIG, TINY_NULL, "1e30000009");
    TEST_PARSE(TINY_PARSE_NUMBER_TOO_BIG, TINY_NULL, "-1e3000009");
#endif
}

static void TestParseNumber() {
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 0.0, "0");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 0.0, "-0");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 0.0, "-0.0");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 1.0, "1");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, -1.0, "-1");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 1.5, "1.5");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, -1.5, "-1.5");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 3.1415926, "3.1415926");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 1E10, "1E10");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 1e10, "1e10");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 1E-10, "1E-10");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 1E+10, "1E+10");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, -1E-10, "-1E-10");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, -3.1544e-10, "-3.1544e-10");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 3.1544e-10, "3.1544e-10");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 0.0, "1e-100000");

    //the smallest number > 1
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 1.0000000000000002, "1.0000000000000002");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 4.9406564584124654e-324, "4.9406564584124654e-324");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, -4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 2.2250738585072009e-308, "2.2250738585072009e-308");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, -2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 2.2250738585072014e-308, "2.2250738585072014e-308");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, -2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, 1.7976931348623157e+308, "1.7976931348623157e+308");
    TEST_PARSE_NUMBER(TINY_PARSE_OK, -1.7976931348623157e+308, "-1.7976931348623157e+308");

#if 1
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "+0");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "+1");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, ".123");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "1.");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "0123");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "1.1.1");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "1e1e1");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "1e1.1");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "1.1e1e2");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "INF");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "inf");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "NAN");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "nan");
#endif
}


int main() {
    TestParse();
    TestParseNumber();
    TestParseNumberToBig();
    printf("%d/%d (%3.2f%%) passed!\n", testPass, testCount, 100.0 * testPass / testCount);
    return mainRet;
}
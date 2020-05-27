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
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17f")
#define EXPECT_EQ_SINGLE(expect, actual, aLength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == aLength && \
    memcmp(expect, actual, aLength) == 0, expect, actual, "%s")
#define EXPECT_EQ_TRUE(actual) EXPECT_EQ_BASE((actual == true), "true", "false", "%s");
#define EXPECT_EQ_FALSE(actual) EXPECT_EQ_BASE((actual == false), "false", "true", "%s");

#define TEST_PARSE(expectReact, expectValType, json)\
    do {\
        TinyValue value;\
        TinyInitValue(&value);\
        EXPECT_EQ_INT(expectReact, TinyParse(&value, json));\
        EXPECT_EQ_INT(expectValType, TinyGetType(&value));\
        TinyFree(&value);\
    } while(0)

#define TEST_PARSE_NUMBER(expectReact, expectNum, json)\
    do {\
        TinyValue value;\
        EXPECT_EQ_INT(expectReact, TinyParse(&value, json));\
        EXPECT_EQ_INT(TINY_NUMBER, TinyGetType(&value));\
        EXPECT_EQ_DOUBLE(expectNum, TinyGetNumber(&value));\
        TinyFree(&value);\
    } while(0)

#define TEST_STRING(expect, json) \
    do{\
        TinyValue value;\
        TinyInitValue(&value);\
        EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&value, json));\
        EXPECT_EQ_INT(TINY_STRING, TinyGetType(&value));\
        EXPECT_EQ_SINGLE(expect, TinyGetString(&value), TinyGetStringLength(&value));\
        TinyFree(&value);\
    } while(0)

static void TestParseOk() {
    TEST_PARSE(TINY_PARSE_OK, TINY_NULL, "null");
    TEST_PARSE(TINY_PARSE_OK, TINY_FALSE, "false");
    TEST_PARSE(TINY_PARSE_OK, TINY_TRUE, "true");
    TEST_PARSE(TINY_PARSE_OK, TINY_STRING, "\"helloword!\"");
}

static void TestParseInvalidValue() {
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "nul");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "NULL");
    TEST_PARSE(TINY_PARSE_INVALID_VALUE, TINY_NULL, "?");

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
}

static void TestParseExceptValue() {
    TEST_PARSE(TINY_PARSE_EXPECT_VALUE, TINY_NULL, "");
    TEST_PARSE(TINY_PARSE_EXPECT_VALUE, TINY_NULL, " ");
}

static void TestParseRootNoSingular() {
    TEST_PARSE(TINY_PARSE_ROOT_NOT_SINGULAR, TINY_NULL, "null x");
    TEST_PARSE(TINY_PARSE_ROOT_NOT_SINGULAR, TINY_NULL, "0x0");
    TEST_PARSE(TINY_PARSE_ROOT_NOT_SINGULAR, TINY_NULL, "0x123");
}

static void TestParseNumberToBig() {
#if 1
    TEST_PARSE(TINY_PARSE_NUMBER_TOO_BIG, TINY_NULL, "1e309");
    TEST_PARSE(TINY_PARSE_NUMBER_TOO_BIG, TINY_NULL, "-1e309");
    TEST_PARSE(TINY_PARSE_NUMBER_TOO_BIG, TINY_NULL, "1e30000009");
    TEST_PARSE(TINY_PARSE_NUMBER_TOO_BIG, TINY_NULL, "-1e3000009");
#endif
}

static void TestParseString() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
#if 0
    TEST_STRING("Hello\nWord", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
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
}

static void TestAccessBool() {
    TinyValue value;
    TinyInitValue(&value);
    TinySetBoolen(&value, 0);
    EXPECT_EQ_FALSE(TinyGetBoolean(&value));

    TinySetBoolen(&value, true);
    EXPECT_EQ_TRUE(TinyGetBoolean(&value));
    TinyFree(&value);
}

static void TestAccessNull() {
    TinyValue value;
    TinyInitValue(&value);
    TinySetString(&value, "a", 1);
    TinySetNull(&value);
    EXPECT_EQ_INT(TINY_NULL,TinyGetType(&value));
    TinyFree(&value);
}

static void TestParseMissingQuotationMark() {
    TEST_PARSE(TINY_PARSE_MISS_QUOTATION_MARK, TINY_NULL, "\"");
    TEST_PARSE(TINY_PARSE_MISS_QUOTATION_MARK, TINY_NULL, "\"abc");
}

static void TestParseInvalidStringEscape() {
    TEST_PARSE(TINY_PARSE_INVALID_STRING_ESCAPE, TINY_NULL, "\"\\v\"");
    TEST_PARSE(TINY_PARSE_INVALID_STRING_ESCAPE, TINY_NULL, "\"\\'\"");
    TEST_PARSE(TINY_PARSE_INVALID_STRING_ESCAPE, TINY_NULL, "\"\\v\"");
    TEST_PARSE(TINY_PARSE_INVALID_STRING_ESCAPE, TINY_NULL, "\"\\0\"");
    TEST_PARSE(TINY_PARSE_INVALID_STRING_ESCAPE, TINY_NULL, "\"\\x12\"");
}

static void TestParseInvalidStringChar() {
    TEST_PARSE(TINY_PARSE_INVALID_STRING_CHAR, TINY_NULL, "\"\x01\"");
    TEST_PARSE(TINY_PARSE_INVALID_STRING_CHAR, TINY_NULL, "\"\x1F\"");
}

static void TestAccessNumber() {
    TinyValue value;
    TinyInitValue(&value);
    TinySetNumber(&value, 123.0);
    EXPECT_EQ_DOUBLE(123.0, TinyGetNumber(&value));
    TinyFree(&value);
}

static void TestAccessString() {
    TinyValue value;
    TinyInitValue(&value);
    TinySetString(&value, "", 0);
    EXPECT_EQ_SINGLE("", TinyGetString(&value), TinyGetStringLength(&value));
    TinySetString(&value, "Hello", 5);
    EXPECT_EQ_SINGLE("Hello", TinyGetString(&value), TinyGetStringLength(&value));
    TinyFree(&value);
}

int main() {

    TestParseOk();
    TestParseNumber();
    TestParseString();

    TestParseNumberToBig();
    TestParseExceptValue();
    TestParseRootNoSingular();
    TestParseInvalidValue();
    TestParseInvalidStringChar();
    TestParseInvalidStringEscape();
    TestParseMissingQuotationMark();

    TestAccessString();
    TestAccessNumber();
    TestAccessBool();
    TestAccessNull();

    printf("%d/%d (%3.2f%%) passed!\n", testPass, testCount, 100.0 * testPass / testCount);
    return mainRet;
}
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

#define EXPECT_EQ_STRING(expect, actual, aLength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == aLength && \
    memcmp(expect, actual, aLength) == 0, expect, actual, "%s")

#define EXPECT_EQ_TRUE(actual) EXPECT_EQ_BASE(((actual) == true), "true", "false", "%s");

#define EXPECT_EQ_FALSE(actual) EXPECT_EQ_BASE(((actual) == false), "false", "true", "%s");

#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu");

#define TEST_PARSE(expectReact, expectValType, json)\
    do {\
        TinyValue value;\
        TinyInitValue(&value);\
        EXPECT_EQ_INT(expectReact, TinyParse(&value, json));\
        EXPECT_EQ_INT(expectValType, TinyGetType(&value));\
        TinyFree(&value);\
    } while(0)

#define TEST_PARSE_ERROR(expectReact, json)\
    do {\
        TEST_PARSE(expectReact, TINY_NULL, json);\
    }  while(0)

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
        EXPECT_EQ_STRING(expect, TinyGetString(&value), TinyGetStringLength(&value));\
        TinyFree(&value);\
    } while(0)


static void TestParseOk() {
    TEST_PARSE(TINY_PARSE_OK, TINY_NULL, "null");
    TEST_PARSE(TINY_PARSE_OK, TINY_FALSE, "false");
    TEST_PARSE(TINY_PARSE_OK, TINY_TRUE, "true");
    TEST_PARSE(TINY_PARSE_OK, TINY_STRING, "\"helloword!\"");
}

static void TestParseInvalidValue() {
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "nul");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "NULL");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "?");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "+0");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "+1");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, ".123");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "1.");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "0123");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "1.1.1");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "1e1e1");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "1e1.1");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "1.1e1e2");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "INF");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "inf");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "NAN");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "nan");
}

static void TestParseExceptValue() {
    TEST_PARSE_ERROR(TINY_PARSE_EXPECT_VALUE, "");
    TEST_PARSE_ERROR(TINY_PARSE_EXPECT_VALUE, " ");
}

static void TestParseRootNoSingular() {
    TEST_PARSE_ERROR(TINY_PARSE_ROOT_NOT_SINGULAR, "null x");
    TEST_PARSE_ERROR(TINY_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_PARSE_ERROR(TINY_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void TestParseNumberToBig() {
#if 1
    TEST_PARSE_ERROR(TINY_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_PARSE_ERROR(TINY_PARSE_NUMBER_TOO_BIG, "-1e309");
    TEST_PARSE_ERROR(TINY_PARSE_NUMBER_TOO_BIG, "1e30000009");
    TEST_PARSE_ERROR(TINY_PARSE_NUMBER_TOO_BIG, "-1e3000009");
#endif
}

static void TestParseString() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");

    TEST_STRING("Hello\nWord", "\"Hello\\nWord\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
#if 1
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
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

static void TestParseMissingQuotationMark() {
    TEST_PARSE_ERROR(TINY_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void TestParseInvalidStringEscape() {
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void TestParseInvalidStringChar() {
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void TestParseInvalidUnicodeHex() {
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void TestParseInvalidUnicodeSurrodate() {
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void TestParseArray() {
    TinyValue value;
    TinyInitValue(&value);
    EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&value, "[ null, false, true, 123, \"abc\" ]"));
    EXPECT_EQ_INT(TINY_ARRAY, TinyGetType(&value));
    EXPECT_EQ_SIZE_T(5,  TinyGetArraySize(&value));
    EXPECT_EQ_INT(TINY_NULL, TinyGetType(TinyGetArrayElement(&value, 0)));
    EXPECT_EQ_INT(TINY_FALSE, TinyGetType(TinyGetArrayElement(&value, 1)));
    EXPECT_EQ_INT(TINY_TRUE, TinyGetType(TinyGetArrayElement(&value, 2)));
    EXPECT_EQ_INT(TINY_NUMBER, TinyGetType(TinyGetArrayElement(&value, 3)));
    EXPECT_EQ_INT(TINY_STRING, TinyGetType(TinyGetArrayElement(&value, 4)));
    EXPECT_EQ_DOUBLE(123.0, TinyGetNumber(TinyGetArrayElement(&value, 3)));
    EXPECT_EQ_STRING("abc", TinyGetString(TinyGetArrayElement(&value, 4)), 
            TinyGetStringLength(TinyGetArrayElement(&value, 4)));
    TinyFree(&value);

    TinyInitValue(&value);
    EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&value, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(TINY_ARRAY, TinyGetType(&value));
    EXPECT_EQ_SIZE_T(4,  TinyGetArraySize(&value));
    for(size_t i = 0; i < 4; i++) {
        TinyValue* arr = TinyGetArrayElement(&value, i);
        EXPECT_EQ_INT(TINY_ARRAY, TinyGetType(arr));
        EXPECT_EQ_SIZE_T(i,  TinyGetArraySize(arr));
        for(size_t j = 0; j < i; j++) {
            TinyValue* e = TinyGetArrayElement(arr, j);
            EXPECT_EQ_INT(TINY_NUMBER, TinyGetType(e));
            EXPECT_EQ_DOUBLE((double)j, TinyGetNumber(e));
        }
    }
    TinyFree(&value);
}

static void TestParseMissCommaOrSquareBracket(){
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1, 2");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[[]]");
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
    EXPECT_EQ_STRING("", TinyGetString(&value), TinyGetStringLength(&value));
    TinySetString(&value, "Hello", 5);
    EXPECT_EQ_STRING("Hello", TinyGetString(&value), TinyGetStringLength(&value));
    TinyFree(&value);
}

static void TestParse() {
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

    TestParseInvalidUnicodeSurrodate();
    TestParseInvalidUnicodeHex();

    TestParseArray();
    TestParseMissCommaOrSquareBracket();
}

static void TestAccess() {
    TestAccessString();
    TestAccessNumber();
    TestAccessBool();
    TestAccessNull();
}

int main() {
    TestParse();
    TestAccess();
    printf("%d/%d (%3.2f%%) passed!\n", testPass, testCount, 100.0 * testPass / testCount);
    return mainRet;
}
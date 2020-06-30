/*
 * @Author       : mark
 * @Date         : 2020-05-26
 * @copyleft Apache 2.0
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../code/tinyjson.h"

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

#define EXPECT_TRUE(actual) EXPECT_EQ_BASE(((actual) == true), "true", "false", "%s");

#define EXPECT_FALSE(actual) EXPECT_EQ_BASE(((actual) == false), "false", "true", "%s");

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
    do {\
        TinyValue value;\
        TinyInitValue(&value);\
        EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&value, json));\
        EXPECT_EQ_INT(TINY_STRING, TinyGetType(&value));\
        EXPECT_EQ_STRING(expect, TinyGetString(&value), TinyGetStringLength(&value));\
        TinyFree(&value);\
    } while(0)

#define TEST_ROUNDTRIP(json) \
    do {\
        TinyValue value;\
        size_t len;\
        TinyInitValue(&value);\
        EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&value, json));\
        char* json2 = TinyStringify(&value, &len);\
        EXPECT_EQ_STRING(json, json2, len);\
        TinyFree(&value);\
        free(json2);\
    } while(0)

#define TEST_EQUAL(json1, json2, equality) \
    do {\
        TinyValue v1, v2;\
        TinyInitValue(&v1);\
        TinyInitValue(&v2);\
        EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&v1, json1));\
        EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&v2, json2));\
        EXPECT_EQ_INT(equality, TinyIsEqual(&v1, &v2));\
        TinyFree(&v1);\
        TinyFree(&v2);\
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

    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "[1,]");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_VALUE, "[\"a\", nul]");
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
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_PARSE_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
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

static void TestParseObject() {
    TinyValue value;
    TinyInitValue(&value);
    EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&value, "{ }"));
    EXPECT_EQ_INT(TINY_OBJECT, TinyGetType(&value));
    EXPECT_EQ_SIZE_T(0,  TinyGetObjectSize(&value));
    TinyFree(&value);
    TinyInitValue(&value);
    EXPECT_EQ_INT(TINY_PARSE_OK, TinyParse(&value, 
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(TINY_OBJECT, TinyGetType(&value));
    EXPECT_EQ_SIZE_T(7, TinyGetObjectSize(&value));
    EXPECT_EQ_STRING("n", TinyGetObjectKey(&value, 0), TinyGetObjectKeyLength(&value, 0));
    EXPECT_EQ_INT(TINY_NULL,   TinyGetType(TinyGetObjectValue(&value, 0)));
    EXPECT_EQ_STRING("f", TinyGetObjectKey(&value, 1), TinyGetObjectKeyLength(&value, 1));
    EXPECT_EQ_INT(TINY_FALSE,  TinyGetType(TinyGetObjectValue(&value, 1)));
    EXPECT_EQ_STRING("t", TinyGetObjectKey(&value, 2), TinyGetObjectKeyLength(&value, 2));
    EXPECT_EQ_INT(TINY_TRUE,   TinyGetType(TinyGetObjectValue(&value, 2)));
    EXPECT_EQ_STRING("i", TinyGetObjectKey(&value, 3), TinyGetObjectKeyLength(&value, 3));
    EXPECT_EQ_INT(TINY_NUMBER, TinyGetType(TinyGetObjectValue(&value, 3)));
    EXPECT_EQ_DOUBLE(123.0, TinyGetNumber(TinyGetObjectValue(&value, 3)));
    EXPECT_EQ_STRING("s", TinyGetObjectKey(&value, 4), TinyGetObjectKeyLength(&value, 4));
    EXPECT_EQ_INT(TINY_STRING, TinyGetType(TinyGetObjectValue(&value, 4)));
    EXPECT_EQ_STRING("abc", TinyGetString(TinyGetObjectValue(&value, 4)), TinyGetStringLength(TinyGetObjectValue(&value, 4)));
    EXPECT_EQ_STRING("a", TinyGetObjectKey(&value, 5), TinyGetObjectKeyLength(&value, 5));
    EXPECT_EQ_INT(TINY_ARRAY, TinyGetType(TinyGetObjectValue(&value, 5)));
    EXPECT_EQ_SIZE_T(3, TinyGetArraySize(TinyGetObjectValue(&value, 5)));
    for (size_t i = 0; i < 3; i++) {
        TinyValue* e = TinyGetArrayElement(TinyGetObjectValue(&value, 5), i);
        EXPECT_EQ_INT(TINY_NUMBER, TinyGetType(e));
        EXPECT_EQ_DOUBLE(i + 1.0, TinyGetNumber(e));
    }
    EXPECT_EQ_STRING("o", TinyGetObjectKey(&value, 6), TinyGetObjectKeyLength(&value, 6));
    {
        TinyValue* o = TinyGetObjectValue(&value, 6);
        EXPECT_EQ_INT(TINY_OBJECT, TinyGetType(o));
        for (size_t i = 0; i < 3; i++) {
            TinyValue* ov = TinyGetObjectValue(o, i);
            EXPECT_TRUE(('1' + (int)i) == TinyGetObjectKey(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, TinyGetObjectKeyLength(o, i));
            EXPECT_EQ_INT(TINY_NUMBER, TinyGetType(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, TinyGetNumber(ov));
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

static void TestParseMissKey() {
    TEST_PARSE_ERROR(TINY_PARSE_MISS_KEY, "{:1,");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_KEY, "{1:1,");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_KEY, "{true:1,");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_KEY, "{false:1,");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_KEY, "{null:1,");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_KEY, "{[]:1,");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_KEY, "{{}:1,");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_KEY, "{\"a\":1,");
}

static void TestParseMissColon() {
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COLON, "{\"a\"}");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COLON, "{\"a\", \"b\"}");
}

static void TestParseMissCommaOrCurlyBracket() {
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\": 1");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\": 1]");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\": 1 \"b\"");
    TEST_PARSE_ERROR(TINY_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\": {}");
}

static void TestAccessBool() {
    TinyValue value;
    TinyInitValue(&value);
    TinySetBoolen(&value, 0);
    EXPECT_FALSE(TinyGetBoolean(&value));

    TinySetBoolen(&value, true);
    EXPECT_TRUE(TinyGetBoolean(&value));
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

static void TestAccessArray() {
    TinyValue a, e;
    TinyInitValue(&a);
    for(size_t j = 0; j <= 5; j += 5) {
        TinySetArray(&a, j);
        EXPECT_EQ_SIZE_T(0, TinyGetArraySize(&a));
        EXPECT_EQ_SIZE_T(j, TinyGetArrayCapacity(&a));
        for(size_t i = 0; i < 10; i++) {
            TinyInitValue(&e);
            TinySetNumber(&e, i);
            TinyMove(TinyPushBackArrayElement(&a), &e);
            TinyFree(&e);
        }
    }
    EXPECT_EQ_SIZE_T(10, TinyGetArraySize(&a));
    for(size_t i = 0; i < 10; i++) {
        EXPECT_EQ_DOUBLE((double)i, TinyGetNumber(TinyGetArrayElement(&a, i)));
    }

    TinyPopBackArrayElement(&a);
    EXPECT_EQ_SIZE_T(9, TinyGetArraySize(&a));
    for(size_t i = 0; i < 9; i++) {
        EXPECT_EQ_DOUBLE((double)i, TinyGetNumber(TinyGetArrayElement(&a, i)));
    }

    TinyEraseArrayElement(&a, 4, 0);
    EXPECT_EQ_SIZE_T(9, TinyGetArraySize(&a));
    for(size_t i = 0; i < 9; i++) {
        EXPECT_EQ_DOUBLE((double)i, TinyGetNumber(TinyGetArrayElement(&a, i)));
    }

    TinyEraseArrayElement(&a, 8, 1);
    EXPECT_EQ_SIZE_T(8, TinyGetArraySize(&a));
    for(size_t i = 0; i < 8; i++) {
        EXPECT_EQ_DOUBLE((double)i, TinyGetNumber(TinyGetArrayElement(&a, i)));
    }

    TinyEraseArrayElement(&a, 0, 2);
    EXPECT_EQ_SIZE_T(6, TinyGetArraySize(&a));
    for(size_t i = 0; i < 6; i++) {
        EXPECT_EQ_DOUBLE((double)i + 2, TinyGetNumber(TinyGetArrayElement(&a, i)));
    }

    for(size_t i = 0; i < 2; i++) {
        TinyInitValue(&e);
        TinySetNumber(&e, i);
        TinyMove(TinyInsertArrayElement(&a, i), &e);
        TinyFree(&e);
    }
    EXPECT_EQ_SIZE_T(8, TinyGetArraySize(&a));
    for(size_t i = 0; i < 8; i++) {
        EXPECT_EQ_DOUBLE((double)i, TinyGetNumber(TinyGetArrayElement(&a, i)));
    }

    EXPECT_TRUE(TinyGetArrayCapacity(&a) > 8);
    TinyShrinkArray(&a);
    EXPECT_TRUE(TinyGetArrayCapacity(&a) == 8);
    EXPECT_TRUE(TinyGetArraySize(&a) == 8);
    for(size_t i = 0; i < 8; i++) {
        EXPECT_EQ_DOUBLE((double)i, TinyGetNumber(TinyGetArrayElement(&a, i)));
    }

    TinySetString(&e, "Hello", 5);
    TinyMove(TinyPushBackArrayElement(&a), &e);   /* Test if element is freed */

    size_t i = TinyGetArrayCapacity(&a);
    TinyClearArray(&a);
    EXPECT_EQ_SIZE_T(0, TinyGetArraySize(&a));
    EXPECT_EQ_SIZE_T(i, TinyGetArrayCapacity(&a));
    TinyShrinkArray(&a);
    EXPECT_EQ_SIZE_T(0, TinyGetArrayCapacity(&a));

    TinyFree(&a);
}

static void TestAccessObject() {
    TinyValue o, v, *pv;
    size_t i, j, index;

    TinyInitValue(&o);
    for(j = 0; j <= 5; j += 5) {
        TinySetObject(&o, j);
        EXPECT_EQ_SIZE_T(0, TinyGetObjectSize(&o));
        EXPECT_EQ_SIZE_T(j, TinyGetObjectCapacity(&o));
        for(i = 0; i < 10; i++) {
            char key[] = "a";
            key[0] += i;
            TinyInitValue(&v);
            TinySetNumber(&v, i);
            TinyMove(TinySetObjectValue(&o, key, 1), &v);
            //TinyFree(&v);
        }
        EXPECT_EQ_SIZE_T(10, TinyGetObjectSize(&o));
        for(i = 0; i < 10; i++) {
            char key[] = "a";
            key[0] += i;
            index = TinyFindObjectIndex(&o, key, 1);
            EXPECT_TRUE(index != TINY_KEY_NOT_EXIST);
            pv = TinyGetObjectValue(&o, index);
            EXPECT_EQ_DOUBLE((double)i, TinyGetNumber(pv));
        }
    }
    index = TinyFindObjectIndex(&o, "j", 1);
    EXPECT_TRUE(index != TINY_KEY_NOT_EXIST);
    TinyRemoveObjectValue(&o, index);
    index = TinyFindObjectIndex(&o, "j", 1);
    EXPECT_TRUE(index == TINY_KEY_NOT_EXIST);
    EXPECT_EQ_SIZE_T(9, TinyGetObjectSize(&o));

    index = TinyFindObjectIndex(&o, "a", 1);
    EXPECT_TRUE(index != TINY_KEY_NOT_EXIST);
    TinyRemoveObjectValue(&o, index);
    index = TinyFindObjectIndex(&o, "a", 1);
    EXPECT_TRUE(index == TINY_KEY_NOT_EXIST);
    EXPECT_EQ_SIZE_T(8, TinyGetObjectSize(&o));

    EXPECT_TRUE(TinyGetObjectCapacity(&o) > 8);
    TinyShrinkObject(&o);
    EXPECT_EQ_SIZE_T(8, TinyGetObjectCapacity(&o));
    EXPECT_EQ_SIZE_T(8, TinyGetObjectSize(&o));
    for(i = 0; i < 8; i++) {
        char key[] = "a";
        key[0] += i + 1;
        EXPECT_EQ_DOUBLE((double)i + 1, TinyGetNumber(TinyGetObjectValue(&o, TinyFindObjectIndex(&o, key, 1))));
    }

    TinySetString(&v, "Hello", 5);
    TinyMove(TinySetObjectValue(&o, "World", 5), &v);
    //TinyFree(&v);
    pv = TinyFindObjectValue(&o, "World", 5);
    EXPECT_TRUE(pv != NULL);
    EXPECT_EQ_STRING("Hello", TinyGetString(pv), TinyGetStringLength(pv));

    i = TinyGetObjectCapacity(&o);
    TinyClearObject(&o);
    EXPECT_EQ_SIZE_T(0, TinyGetObjectSize(&o));
    EXPECT_EQ_SIZE_T(i, TinyGetObjectCapacity(&o));
    TinyShrinkObject(&o);
    EXPECT_EQ_SIZE_T(0, TinyGetObjectCapacity(&o));

    TinyFree(&o);
}

static void TestStringifyNumber() {
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("-1e+20");
    TEST_ROUNDTRIP("1.234e+20");
    TEST_ROUNDTRIP("1.234e-20");

    TEST_ROUNDTRIP("1.0000000000000002");       /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324");   /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308"); /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308"); /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void TestStringifyString() {
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void TestStringifyArray() {
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void TestStringifyObject() {
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

static void TestParse() {
    TestParseOk();
    TestParseNumber();
    TestParseString();
    // error
    // number
    TestParseNumberToBig();
    TestParseExceptValue();
    TestParseRootNoSingular();
    // string
    TestParseInvalidValue();
    TestParseInvalidStringChar();
    TestParseInvalidStringEscape();
    TestParseMissingQuotationMark();

    TestParseInvalidUnicodeSurrodate();
    TestParseInvalidUnicodeHex();
    // array
    TestParseArray();
    TestParseMissCommaOrSquareBracket();
    // object
    TestParseMissKey();
    TestParseMissColon();
    TestParseMissCommaOrCurlyBracket();
    TestParseObject();
}

static void TestAccess() {
    TestAccessString();
    TestAccessNumber();
    TestAccessBool();
    TestAccessNull();
    TestAccessArray();
    TestAccessObject();
}

static void TestStringify() {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    TestStringifyNumber();
    TestStringifyString();
    TestStringifyArray();
    TestStringifyObject();
}

static void TestEqual() {
    TEST_EQUAL("true", "true", 1);
    TEST_EQUAL("true", "false", 0);
    TEST_EQUAL("false", "false", 1);
    TEST_EQUAL("null", "null", 1);
    TEST_EQUAL("null", "0", 0);
    TEST_EQUAL("123", "123", 1);
    TEST_EQUAL("123", "456", 0);
    TEST_EQUAL("\"abc\"", "\"abc\"", 1);
    TEST_EQUAL("\"abc\"", "\"abcd\"", 0);
    TEST_EQUAL("[]", "[]", 1);
    TEST_EQUAL("[]", "null", 0);
    TEST_EQUAL("[1,2,3]", "[1,2,3]", 1);
    TEST_EQUAL("[1,2,3]", "[1,2,3,4]", 0);
    TEST_EQUAL("[1, 2, true, null, \"hello\"]", "[1, 2, true, null, \"hello\"]", 1);
    TEST_EQUAL("[[]]", "[[]]", 1);
    TEST_EQUAL("{}", "{}", 1);
    TEST_EQUAL("{}", "null", 0);
    TEST_EQUAL("{}", "[]", 0);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}", 1);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"b\":2,\"a\":1}", 1);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":3}", 0);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2,\"c\":3}", 0);
    TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":{}}}}", 1);
    TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":[]}}}", 0);
}

static void TestCopy() {
    TinyValue v1, v2;
    TinyInitValue(&v1);
    TinyParse(&v1, "{\"t\":true,\"f\":false,\"n\":null,\"d\":1.5,\"a\":[1,2,3]}");
    TinyInitValue(&v2);
    TinyCopy(&v2, &v1);
    EXPECT_TRUE(TinyIsEqual(&v1, &v2));
    TinyFree(&v1);
    TinyFree(&v2);
}

static void TestMove() {
    TinyValue v1, v2, v3;
    TinyInitValue(&v1);
    TinyInitValue(&v2);
    TinyInitValue(&v3);
    TinyParse(&v1, "{\"t\":true,\"f\":false,\"n\":null,\"d\":1.5,\"a\":[1,2,3]}");
    TinyCopy(&v2, &v1);
    TinyMove(&v3, &v2);
    EXPECT_EQ_INT(TINY_NULL, TinyGetType(&v2));
    EXPECT_TRUE(TinyIsEqual(&v3, &v1));
    TinyFree(&v1);
    TinyFree(&v2);
    TinyFree(&v3);
}

static void TestSwap() {
    TinyValue v1, v2;
    TinyInitValue(&v1);
    TinyInitValue(&v2);
    TinySetString(&v1, "Hello", 5);
    TinySetString(&v2, "world!", 6);
    TinySwap(&v2, &v1);
    EXPECT_EQ_STRING("world!", TinyGetString(&v1), TinyGetStringLength(&v1));
    EXPECT_EQ_STRING("Hello", TinyGetString(&v2), TinyGetStringLength(&v2));
    TinyFree(&v1);
    TinyFree(&v2);
}

int main() {
    TestParse();
    TestAccess();
    TestStringify();
    TestEqual();
    TestCopy();
    TestMove();
    TestSwap();
    printf("%d/%d (%3.2f%%) passed!\n", testPass, testCount, 100.0 * testPass / testCount);
    return mainRet;
}
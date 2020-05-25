/*
 * @Author       : mark
 * @Date         : 2020-05-25
 * @copyleft GPL 2.0
 */ 
#include "json_generator.h"
#include <cmath>

int main()
{
    auto writer = new JsonGenerator;
    writer->StartArray ();

    writer->StartShortObject ();
        writer->KeyValue ("name", "mark");
        writer->KeyValue ("age", 3);
    writer->EndObject ();

    writer->StartObject ();
        writer->KeyValue ("skills", "c++");
        writer->KeyValue ("skills","python");
        writer->KeyValue ("skills","php");
        writer->KeyValue ("skills","java");
        writer->KeyValue ("url", "http://baidu.com");

        writer->Key ("path");
        writer->StartArray ();
        writer->Value ("web");
        writer->Value ("algorithm");
        writer->Value ("linux");
        writer->EndArray ();

        writer->Key ("short-array");
        writer->StartShortArray ();
        writer->Value (1);
        writer->Value ((uint64_t)0xabcdef123456);
        writer->Value (M_PI);
        writer->EndContainer ();

    writer->EndObject (),
    writer->Value (false);
    writer->EndArray ();
}

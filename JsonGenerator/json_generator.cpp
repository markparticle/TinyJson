/*
 * @Author       : mark
 * @Date         : 2020-05-25
 * @copyleft GPL 2.0
 */ 

#include "json_generator.h"
#include <cmath>
using std::endl;
using std::string;

void JsonGenerator::Indent()
{   
    int n = initIdentDepth_ + containerStack_.size();
    for(int i = 0; i < n; i++)
    {
        Write() << indent_;
    }
}

void JsonGenerator::StartContainer(ContainerType type, ContainerLayout layout)
{
    if(isForceDefaultLayout_) {
        layout = defaultLayout_;
    }
    else if (layout == INHERIT) {
        if(!containerStack_.empty()) {
            layout = containerStack_.top()->layout;
        }
        else {
            layout = defaultLayout_;
        }
    }
    StartChild();
    containerStack_.push(new Container(type, layout));

    char res;
    if(type == ContainerType::OBJECT) {
        res = '{';
    } else res = '[';
    Write() << res;
}

//对象，元素，键值的缩进和标点符号问题
void JsonGenerator::StartChild(bool isKey)
{
    if(containerStack_.empty()) {
        if(initIdentDepth_ > 0) {
            Indent();
        }
        return;
    }

    Container *container = containerStack_.top();

    //如果是数组或对象中的值，添加“,”
    if(container->childCount > 0 && 
        (container->type == ARRAY ||
        (container->type == OBJECT && container->isKey == false))) {
        Write() << ",";
        if(container->layout == SINGLE_LINE) {
            Write() << containerPadding_;
        } else {
            Write() << endl;
            Indent();
        }
    } else if(container->childCount == 0) {
        Write() << containerPadding_;
        if(container->layout == MULTI_LINE) {
            Write() << endl;
            Indent();
        }
    }
    container->isKey = isKey;
    container->childCount++;
}

void JsonGenerator::WriteString(const char *str)
{
    Write() << "\"";
    for(int i = 0; str[i] != 0; i++) {
        WriteChar(str[i]);
    }
    Write() << "\"";
}

void JsonGenerator::WriteChar(char c)
{
    switch(c) {
    case '"': 
        Write () << "\\\""; 
        break;
    case '\\': 
        Write () << "\\\\"; 
        break;
    case '\b': 
        Write () << "\\b"; 
        break;
    case '\f': 
        Write () << "\\f"; 
        break;
    case '\n': 
        Write () << "\\n"; 
        break;
    case '\r': 
        Write () << "\\r"; 
        break;
    case '\t': 
        Write () << "\\t"; 
        break;
    default:
        Write() << c;
        break;
    }
}

void JsonGenerator::EndContainer()
{
    Container *container = containerStack_.top();
    containerStack_.pop();
    if(container->childCount > 0) {
        if(container->layout == MULTI_LINE) {
            Write() << endl;
            Indent();
        } else {
            Write() << containerPadding_;
        }
    }
    char res;
    if(container->type == OBJECT) {
        res = '}';
    } else res = ']';
    Write() << res;
    delete container;
}

void JsonGenerator::NullValue()
{
    StartChild();
    Write() << "null";
}

void JsonGenerator::Value(string value)
{
    StartChild();
    WriteString(value.c_str()); 
}

void JsonGenerator::Value(bool value)
{
    StartChild();
    Write() << (value ? "true" : "false");
}

void JsonGenerator::Value(const char *value) {
    StartChild();
    WriteString(value); 
}

void JsonGenerator::Key(const char *key)
{
    StartChild(true);
    WriteString(key);
    Write() << keyPaddingLeft_ << ":" << keyPaddingRight_;
}

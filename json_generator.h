/*
 * @Author       : mark
 * @Date         : 2020-05-25
 * @copyleft GPL 2.0
 */ 
#ifndef GENERATOR_H
#define GENERATOR_H

#include <stack>
#include <iostream>
#include <string>

class JsonGenerator
{
public:

    JsonGenerator(): 
        writer_(nullptr),
        initIdentDepth_(0),
        indent_(" "),
        containerPadding_(" "),
        keyPaddingLeft_(""),
        keyPaddingRight_(" "),
        defaultLayout_(MULTI_LINE),
        isForceDefaultLayout_(false)
    {}

    enum ContainerType 
    {
        ARRAY,
        OBJECT,
    };

    enum ContainerLayout
    {
        INHERIT,
        MULTI_LINE,
        SINGLE_LINE,
    };

    struct Container
    {
        ContainerType type;
        ContainerLayout layout;
        bool isKey;
        int childCount;
        Container(ContainerType type, ContainerLayout layout):
            type(type),
            layout(layout),
            isKey(false),
            childCount(0)
        {}
    };

    
    //元素生成
    void StartContainer(ContainerType type, ContainerLayout layout);
    void StartChild(bool isKey = false);
    void EndContainer();

    void StartShortArray () 
    { 
        StartContainer (ARRAY, SINGLE_LINE); 
    }

    void StartArray(ContainerLayout layout = INHERIT)
    {   
         StartContainer(ARRAY, layout);
    }

    void EndArray () 
    { 
        EndContainer (); 
    }

    void StartObject(ContainerLayout layout = INHERIT) 
    { 
        StartContainer (OBJECT, layout); 
    }
    void StartShortObject() 
    { 
        StartContainer (OBJECT, SINGLE_LINE); 
    }
    void EndObject () { EndContainer (); }

    //键值
    void Value(bool value);
    void Value(std::string value);
    void Value(const char *value);
    void NullValue();
    void Key(const char *key);

	template<typename T>
    void Value (T value) { 
        StartChild(); 
        Write () << value; 
    }

    template<typename T>
    void KeyValue (const char *key, T value) {
        Key(key);
        Value(value);
    }

    void KeyNullValue (const char *key) { 
        Key (key); 
        NullValue (); 
    }

    std::ostream& Write()
    {
        if(writer_ == nullptr)
        {
            return std::cout;
        }
        return *writer_;
    }
    
    void WriteChar(const char c);
    void WriteString(const char *str);

    void Indent();

    const char* GetIndent() { return indent_; }
    void SetIndent(const char *indent_) { indent_ = indent_; }



private:

    std::ostream *writer_;
    int initIdentDepth_;
    const char *indent_;
    const char *containerPadding_;
    const char *keyPaddingLeft_;
    const char *keyPaddingRight_;
    ContainerLayout defaultLayout_;
    bool isForceDefaultLayout_;
    std::stack<Container *> containerStack_;
};

#endif //GENERATOR_H

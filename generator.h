/*
 * @Author       : mark
 * @Date         : 2020-05-25
 * @copyleft GPL 2.0
 */ 

#include <stack>
#include <ostream>

class JsonGenerator
{
public:
    JsonGenerator(): 
        generator_(nullptr),
        initIdentDepth_(0),
        indent(" "),
        containerPadding_(" "),
        keyPaddingLeft_(""),
        keyPaddingRight_(" "),
        defaultLayout_(MULTI_LINE),
        forceDefaultLayout_(false)
    {}

    std::ostream& Write()
    {
        if(generator_ == nullptr)
        {
            return std::cout;
        }
        return *generator_;
    }

    enum ContainerType 
    {
        ARRAY,
        OBJECT,
    }

    enum ContainerLayout
    {
        INHERIT,
        MULTI_LINE,
        SINGLE_LINE,
    }

private:

    std::ostream *generator_;
    int initIdentDepth_;
    const char *ident_;
    const char *containerPadding_;
    const char *keyPaddingLeft_;
    const char *keyPaddingRight_;
    ContainerLayout defaultLayout_;
    bool forceDefaultLayout_;
    
    std::stack<Container *> depth_;

    void Indent_();
    struct Container_
    {
        ContainerType type;
        ContainerLayout layout;
        bool isKey;
        int childCount;

        Container (ContainerType type, ContainerLayout layout):
            type(type),
            layout(layout),
            isKey(false),
            childCount(0)
        {}
    }

}





void CompressedOutput ()
{

}


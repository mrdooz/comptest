// Protobuf plugin to generate AntTweakBar bindings

// protoc --proto_path=/projects/protobuf-2.5.0/src --proto_path=. --cpp_out=. *.proto
// protoc --plugin=protoc-gen-ANT=Debug/comptest.exe --ANT_out=. --proto_path=/projects/protobuf-2.5.0/src --proto_path=. *.proto

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"
#include <boost/algorithm/string/replace.hpp>
#include "anttweak.pb.h"

#pragma warning(disable: 4996)

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "libprotoc.lib")
#pragma comment(lib, "libprotobuf.lib")
#endif

using namespace std;

string ExpandToNamespace(const string& str)
{
  string res;
  for (size_t i = 0; i < str.size(); ++i)
  {
    if (str[i] == '.')
    {
      res += "::";
    }
    else
    {
      res += str[i];
    }
  }
  return res;
}

const char getSetTemplate[] = 
  "\t// Add {{FIELD_NAME}}\n\tTwAddVarCB(bar, \"{{FIELD_NAME}}\", {{TW_TYPE}},\n"\
  "\t\t[](const void* value, void* data) { (({{FIELD_TYPE}}*)(data))->set_{{FIELD_NAME}}(*({{CPP_TYPE}}*)value); },\n"\
  "\t\t[](void* value, void* data) { *({{CPP_TYPE}}*)value = (({{FIELD_TYPE}}*)(data))->{{FIELD_NAME}}(); }, data, nullptr);\n\n";

const char getSetRepeatedTemplate[] =
  "\t// Add {{FIELD_NAME}}\n\tTwAddVarCB(bar, \"{{FIELD_NAME}}\", {{TW_TYPE}},\n"\
  "\t\t[](const void* value, void* data) { memcpy((({{FIELD_TYPE}}*)(data))->mutable_{{FIELD_NAME}}()->mutable_data(), value, {{REPEAT_COUNT}} * sizeof({{CPP_TYPE}})); },\n"\
  "\t\t[](void* value, void* data) { memcpy(value, (({{FIELD_TYPE}}*)(data))->{{FIELD_NAME}}().data(), {{REPEAT_COUNT}} * sizeof({{CPP_TYPE}})); }, data, nullptr);\n\n";

string ApplyTemplate(const string& twType, const string& cppType, const string& fieldName, const string& fieldType, int repeatCount = 0)
{
  string res(repeatCount > 0 ? getSetRepeatedTemplate : getSetTemplate);
  boost::algorithm::replace_all(res, "{{TW_TYPE}}", twType);
  boost::algorithm::replace_all(res, "{{CPP_TYPE}}", cppType);
  boost::algorithm::replace_all(res, "{{FIELD_TYPE}}", fieldType);
  boost::algorithm::replace_all(res, "{{FIELD_NAME}}", fieldName);
  if (repeatCount > 0)
  {
    char buf[16];
    sprintf(buf, "%d", repeatCount);
    boost::algorithm::replace_all(res, "{{REPEAT_COUNT}}", buf);
  }
  return res;
}


class CodeGenerator : public google::protobuf::compiler::CodeGenerator
{
public:
  virtual bool Generate(
      const google::protobuf::FileDescriptor *file,
      const std::string &parameter,
      google::protobuf::compiler::OutputDirectory *output_directory,
      std::string *error) const;
};


bool CodeGenerator::Generate(
    const google::protobuf::FileDescriptor *file,
    const std::string &parameter,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error) const
{
  FILE* f = fopen("tjong", "wt");

  for (int i = 0; i < file->message_type_count(); ++i)
  {
    const google::protobuf::Descriptor* desc = file->message_type(i);
    // create a twbar per message

    string typeName = ExpandToNamespace(desc->full_name());

    fprintf(f, "void Bind%s(%s* data)\n{\n", desc->name().c_str(), typeName.c_str());
    fprintf(f, "\tTwBar* bar = TwNewBar(\"%s\");\n", desc->full_name().c_str());
//    TwAddVarRW(myBar, "xx", TW_TYPE_INT32, &xx, 0);
    //fprintf(f, "name: %s\n", desc->name().c_str());
    for (int j = 0; j < desc->field_count(); ++j)
    {
      const google::protobuf::FieldDescriptor* fieldDesc = desc->field(j);
      const char* fieldName = fieldDesc->name().c_str();

      const google::protobuf::FieldOptions& options = fieldDesc->options();
      bool col3f = options.HasExtension(anttweak::color3f) && options.GetExtension(anttweak::color3f);
      bool col4f = options.HasExtension(anttweak::color4f) && options.GetExtension(anttweak::color4f);
      bool dir3f = options.HasExtension(anttweak::dir3f) && options.GetExtension(anttweak::dir3f);
      int repeatCount = 0;

      // add bindings for the types we support
      switch (fieldDesc->cpp_type())
      {
      case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        fprintf(f, "%s", ApplyTemplate("TW_TYPE_BOOLCPP", "bool", fieldName, typeName, repeatCount).c_str());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        fprintf(f, "%s", ApplyTemplate("TW_TYPE_INT32", "int32_t", fieldName, typeName, repeatCount).c_str());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        fprintf(f, "%s", ApplyTemplate("TW_TYPE_UINT32", "uint32_t", fieldName, typeName, repeatCount).c_str());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
          string twType = "TW_TYPE_FLOAT";

          if (col3f) { repeatCount = 3; twType = "TW_TYPE_COLOR3F"; }
          if (col4f) { repeatCount = 4; twType = "TW_TYPE_COLOR4F"; }
          if (dir3f) { repeatCount = 3; twType = "TW_TYPE_DIR3F"; }
          fprintf(f, "%s", ApplyTemplate(twType, "float", fieldName, typeName, repeatCount).c_str());
          break;
        }
      }
    }
    fprintf(f, "}\n");
  }

  fclose(f);
  return true;
}

int main(int argc, char** argv)
{
  CodeGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}


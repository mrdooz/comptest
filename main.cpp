// Protobuf plugin to generate AntTweakBar bindings

// protoc --proto_path=/projects/protobuf-2.5.0/src --proto_path=. --cpp_out=. *.proto
// protoc --plugin=protoc-gen-ANT=Debug/comptest.exe --ANT_out=. --proto_path=/projects/protobuf-2.5.0/src --proto_path=. *.proto

#pragma warning(disable: 4996)

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"
#include <boost/algorithm/string/replace.hpp>
#include "anttweak.pb.h"

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "libprotoc.lib")
#pragma comment(lib, "libprotobuf.lib")
#endif

using namespace std;

//------------------------------------------------------------------------------
string ExpandToNamespace(const string& str, vector<string>* splits)
{
  string res;
  string tmp;
  for (size_t i = 0; i < str.size(); ++i)
  {
    if (str[i] == '.')
    {
      if (splits)
        splits->push_back(tmp);
      tmp.clear();
      res += "::";
    }
    else
    {
      tmp += str[i];
      res += str[i];
    }
  }
  return res;
}

//------------------------------------------------------------------------------
const char getSetTemplate[] = 
  "\t// Add {{FIELD_NAME}}\n\tTwAddVarCB(bar, \"{{FIELD_NAME}}\", {{TW_TYPE}},\n"\
  "\t\t[](const void* value, void* data) { Cfg* cfg = (Cfg*)data; cfg->data->set_{{FIELD_NAME}}(*({{CPP_TYPE}}*)value); if (cfg->dirty) *cfg->dirty = true; },\n"\
  "\t\t[](void* value, void* data) { Cfg* cfg = (Cfg*)data; *({{CPP_TYPE}}*)value = cfg->data->{{FIELD_NAME}}(); }, (void*)&cfg, {{CLIENT_DATA}});\n\n";

const char getSetRepeatedTemplate[] =
  "\t// Add {{FIELD_NAME}}\n\tTwAddVarCB(bar, \"{{FIELD_NAME}}\", {{TW_TYPE}},\n"\
  "\t\t[](const void* value, void* data) { Cfg* cfg = (Cfg*)data; memcpy(cfg->data->mutable_{{FIELD_NAME}}()->mutable_data(), value, {{REPEAT_COUNT}} * sizeof({{CPP_TYPE}})); },\n"\
  "\t\t[](void* value, void* data) { Cfg* cfg = (Cfg*)data; memcpy(value, cfg->data->{{FIELD_NAME}}().data(), {{REPEAT_COUNT}} * sizeof({{CPP_TYPE}})); }, (void*)&cfg, {{CLIENT_DATA}});\n\n";

//------------------------------------------------------------------------------
string ApplyTemplate(
    const string& twType,
    const string& cppType,
    const string& fieldName,
    const string& msgType,
    int repeatCount,
    const string& clientData)
{
  string res(repeatCount > 0 ? getSetRepeatedTemplate : getSetTemplate);
  boost::algorithm::replace_all(res, "{{TW_TYPE}}", twType);
  boost::algorithm::replace_all(res, "{{CPP_TYPE}}", cppType);
  boost::algorithm::replace_all(res, "{{FIELD_NAME}}", fieldName);
  boost::algorithm::replace_all(res, "{{MSG_TYPE}}", msgType);
  boost::algorithm::replace_all(res, "{{CLIENT_DATA}}", clientData.empty() ? "nullptr" : (string("\"") + clientData + string("\"")));
  if (repeatCount > 0)
  {
    char buf[16];
    sprintf(buf, "%d", repeatCount);
    boost::algorithm::replace_all(res, "{{REPEAT_COUNT}}", buf);
  }
  return res;
}

//------------------------------------------------------------------------------
string TabToSpace(const char* str)
{
  string res(str);
  boost::algorithm::replace_all(res, "\t", "  ");
  return res;
}

//------------------------------------------------------------------------------
class CodeGenerator : public google::protobuf::compiler::CodeGenerator
{
public:
  virtual bool Generate(
      const google::protobuf::FileDescriptor *file,
      const std::string &parameter,
      google::protobuf::compiler::OutputDirectory *output_directory,
      std::string *error) const;
};


//------------------------------------------------------------------------------
bool CodeGenerator::Generate(
    const google::protobuf::FileDescriptor *file,
    const std::string &parameter,
    google::protobuf::compiler::OutputDirectory *output_directory,
    std::string *error) const
{

  if (file->message_type_count() == 0)
    return true;

  string cppFile(file->name());
  string hppFile(file->name());
  string header(file->name());
  // replace extension with "_bindings.cpp"
  size_t extPos = cppFile.find('.');
  if (extPos == string::npos)
    return false;

  cppFile.replace(extPos, cppFile.size() - extPos + 1, "_bindings.cpp");
  hppFile.replace(extPos, hppFile.size() - extPos + 1, "_bindings.hpp");
  header.replace(extPos, header.size() - extPos + 1, ".pb.h");

  FILE* fCpp = fopen(cppFile.c_str(), "wt");
  FILE* fHpp = fopen(hppFile.c_str(), "wt");

  fprintf(fHpp, "#pragma once\n");
  fprintf(fHpp, "#include \"%s\"\n", header.c_str());

  fprintf(fCpp, "#include \"%s\"\n", hppFile.c_str());

  // assume that all messages live in the same namespace, so use the first
  // message for determining namespaces etc
  vector<string> splits;
  const google::protobuf::Descriptor* desc = file->message_type(0);
  string typeName = ExpandToNamespace(desc->full_name(), &splits);

  string nsOpen;
  string nsClose;
  for (const string& str : splits)
  {
    nsOpen.append("namespace ");
    nsOpen.append(str);
    nsOpen.append("\n{\n");

    nsClose.append("}\n");
  }

  // write opening namespace
  if (!nsOpen.empty())
  {
    fprintf(fCpp, "%s", nsOpen.c_str());
    fprintf(fHpp, "%s", nsOpen.c_str());
  }

  for (int i = 0; i < file->message_type_count(); ++i)
  {
    const google::protobuf::Descriptor* desc = file->message_type(i);

    // create a twbar per message
    string msgType = ExpandToNamespace(desc->full_name(), nullptr);

    // add declaration to the hpp
    fprintf(fHpp, "void Bind%s(%s* data, bool *dirty);\n", desc->name().c_str(), msgType.c_str());

    // add definition to the cpp
    fprintf(fCpp, "void Bind%s(%s* data, bool *dirty)\n{\n", desc->name().c_str(), msgType.c_str());

    // print the Cfg struct
    fprintf(fCpp, TabToSpace("\tstruct Cfg\n\t{\n\t\t%s* data;\n\t\t\tbool *dirty;\n\t};\n\n").c_str(), desc->name().c_str());
    fprintf(fCpp, TabToSpace("\tstatic Cfg cfg;\n\tcfg.data = data;\n\tcfg.dirty = dirty;\n\n").c_str());

    fprintf(fCpp, "#if WITH_ANT_TWEAK_BAR\n");

    fprintf(fCpp, TabToSpace("\tTwBar* bar = TwNewBar(\"%s\");\n").c_str(), desc->full_name().c_str());
    for (int j = 0; j < desc->field_count(); ++j)
    {
      const google::protobuf::FieldDescriptor* fieldDesc = desc->field(j);
      const char* fieldName = fieldDesc->name().c_str();

      // check for extensions
      const google::protobuf::FieldOptions& options = fieldDesc->options();
      bool col3f = options.HasExtension(anttweak::color3f) && options.GetExtension(anttweak::color3f);
      bool col4f = options.HasExtension(anttweak::color4f) && options.GetExtension(anttweak::color4f);
      bool dir3f = options.HasExtension(anttweak::dir3f) && options.GetExtension(anttweak::dir3f);

      char clientData[256] = {0};
      char* clientDataPtr = clientData;
      if (options.HasExtension(anttweak::minF))   clientDataPtr += sprintf(clientDataPtr, "min=%f ", options.GetExtension(anttweak::minF));
      if (options.HasExtension(anttweak::maxF))   clientDataPtr += sprintf(clientDataPtr, "max=%f ", options.GetExtension(anttweak::maxF));
      if (options.HasExtension(anttweak::stepF))  clientDataPtr += sprintf(clientDataPtr, "step=%f ", options.GetExtension(anttweak::stepF));

      if (options.HasExtension(anttweak::minI))   clientDataPtr += sprintf(clientDataPtr, "min=%d ", options.GetExtension(anttweak::minI));
      if (options.HasExtension(anttweak::maxI))   clientDataPtr += sprintf(clientDataPtr, "max=%d ", options.GetExtension(anttweak::maxI));
      if (options.HasExtension(anttweak::stepI))  clientDataPtr += sprintf(clientDataPtr, "step=%d ", options.GetExtension(anttweak::stepI));

      bool noBind = options.HasExtension(anttweak::nobind) && options.GetExtension(anttweak::nobind);
      if (noBind)
        continue;

      int repeatCount = 0;

      // add bindings for the types we support
      switch (fieldDesc->cpp_type())
      {
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
          fputs(TabToSpace(ApplyTemplate("TW_TYPE_BOOLCPP", "bool", fieldName, msgType, repeatCount, "").c_str()).c_str(), fCpp);
          break;

        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
          fputs(TabToSpace(ApplyTemplate("TW_TYPE_INT32", "int32_t", fieldName, msgType, repeatCount, clientData).c_str()).c_str(), fCpp);
          break;

        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
          fputs(TabToSpace(ApplyTemplate("TW_TYPE_UINT32", "uint32_t", fieldName, msgType, repeatCount, clientData).c_str()).c_str(), fCpp);
          break;

        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
          {
            string twType = "TW_TYPE_FLOAT";

            if (col3f) { repeatCount = 3; twType = "TW_TYPE_COLOR3F"; }
            if (col4f) { repeatCount = 4; twType = "TW_TYPE_COLOR4F"; }
            if (dir3f) { repeatCount = 3; twType = "TW_TYPE_DIR3F"; }
            fputs(TabToSpace(ApplyTemplate(twType, "float", fieldName, msgType, repeatCount, repeatCount == 0 ? clientData : "").c_str()).c_str(), fCpp);
            break;
          }
      }
    }

    fprintf(fCpp, "#endif\n");

    fprintf(fCpp, "}\n");
  }

  if (!nsClose.empty())
  {
    fprintf(fCpp, "%s", nsClose.c_str());
    fprintf(fHpp, "%s", nsClose.c_str());
  }

  fclose(fCpp);
  fclose(fHpp);
  return true;
}

//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
  CodeGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}


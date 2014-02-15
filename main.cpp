// Protobuf plugin to generate AntTweakBar bindings

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/compiler/plugin.h"

using namespace std;

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
  fprintf(f, "name: %s\n", file->name().c_str());
  fprintf(f, "message type count: %d\n", file->message_type_count());

  for (int i = 0; i < file->message_type_count(); ++i)
  {
    const google::protobuf::Descriptor* desc = file->message_type(i);
    fprintf(f, "name: %s\n", desc->name().c_str());
    for (int j = 0; j < desc->field_count(); ++j)
    {
      const google::protobuf::FieldDescriptor* fieldDesc = desc->field(j);
      fprintf(f, "\tfield name: %s (%s%s)\n", fieldDesc->name().c_str(), fieldDesc->is_repeated()
     ? "[rep] " : "", fieldDesc->cpp_type_name());
    }
  }

  fclose(f);
  return true;
}

int main(int argc, char** argv)
{
  CodeGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}


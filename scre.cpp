#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>

inline std::string chunk_number_to_filename(const size_t chunk_number, const std::string & output_file_prefix)
{
  return output_file_prefix + "_chunk" + std::to_string(chunk_number) + ".cpp";
}

inline std::string get_ns_name(const std::string & input_filename)
{
  std::smatch m;
  auto        fname =
    std::regex_replace(input_filename, std::regex(R"reg((.*[/\\]))reg", std::regex_constants::ECMAScript), "");
  fname = std::regex_replace(fname, std::regex(R"(\.)", std::regex_constants::ECMAScript), "_");

  std::regex_search(fname, m, std::regex(R"reg(([a-zA-Z0-9_]+))reg", std::regex_constants::ECMAScript));
  return m[1];
}

void process_chunk(const unsigned char * data,
                   size_t                bytes_to_read,
                   const std::string &   output_file_prefix,
                   const size_t          chunk_number,
                   const std::string &   ns_name)
{
  const std::string chunk_outname(chunk_number_to_filename(chunk_number, output_file_prefix));
  if(std::filesystem::exists(chunk_outname))
    throw(std::logic_error(chunk_outname + std::string(" already exists")));

  std::ofstream of(chunk_outname);

  of << "namespace " << ns_name << "{unsigned char * get_part" << std::to_string(chunk_number)
     << "(){static unsigned char chunk[] = {";

  of << std::hex << std::showbase;
  while(bytes_to_read-- > 1)
    of << static_cast<unsigned int>(*data++) << ',';
  of << static_cast<unsigned int>(*data);
  of << "}; return reinterpret_cast<unsigned char *>(chunk);}}";
}

void generate_allocator_file(const std::string &  output_file_prefix,
                             const unsigned short chunk_size,
                             const size_t         full_chunk_count,
                             const size_t         file_size,
                             const std::string &  ns_name)
{
  const std::string allocator_fname(output_file_prefix + "_allocator.cpp");
  if(std::filesystem::exists(allocator_fname))
    throw(std::logic_error(allocator_fname + std::string(" already exists")));

  std::ofstream allocator_file(allocator_fname);

  allocator_file << "#include <string>" << std::endl
                 << "#include <utility>" << std::endl
                 << "namespace " << ns_name << " {";

  for(int i = 0; i < full_chunk_count; i++)
    allocator_file << "unsigned char * get_part" << std::to_string(i) << "();" << std::endl;

  const size_t last_chunk_size = file_size - full_chunk_count * chunk_size;
  if(last_chunk_size)
    allocator_file << "unsigned char * get_part" << std::to_string(full_chunk_count) << "();" << std::endl;

  allocator_file << "std::pair<const char *, size_t> allocate_resource(){"
                    "unsigned char * res = new unsigned char["
                 << file_size << "];";

  for(int i = 0; i < full_chunk_count; i++)
    allocator_file << "memcpy(res + " << std::to_string(i * chunk_size) << ", "
                   << "get_part" << std::to_string(i) << "(), " << std::to_string(chunk_size) << ");" << std::endl;

  if(last_chunk_size)
    allocator_file << "memcpy(res + " << std::to_string(full_chunk_count * chunk_size) << ", "
                   << "get_part" << std::to_string(full_chunk_count) << "(), " << std::to_string(last_chunk_size)
                   << ");" << std::endl;

  allocator_file << "return std::make_pair(reinterpret_cast<const char *>(res), " << file_size << ");}}";

  allocator_file << R"delim(
//example usage:
//#include <iostream>
//#include <fstream>
//void main()
//{
//	std::ofstream f("resource.res", std::ios::binary);
//	auto res = resource_namespace::allocate_resource();
//	f.write(res.first, res.second);
//}
	)delim";
};

const size_t get_file_size(std::ifstream & f)
{
  f.ignore(std::numeric_limits<std::streamsize>::max());
  const size_t result = f.gcount();
  f.clear();
  f.seekg(0, std::ios_base::beg);
  return result;
}

void process(const std::string &  input_filename,
             const std::string &  output_file_prefix,
             const unsigned short chunk_size)
{
  if(!std::filesystem::exists(input_filename))
    throw(std::logic_error(input_filename + std::string(" doesn't exist")));
  if(!std::filesystem::is_regular_file(input_filename))
    throw(std::logic_error(input_filename + std::string(" isn't a regular file")));
  std::ifstream input_file{input_filename, std::ios::binary};
  if(!input_file.is_open())
    throw(std::logic_error(std::string("can't open ") + input_filename));

  const size_t file_size        = get_file_size(input_file);
  const size_t full_chunk_count = file_size / chunk_size;

  auto ns_name = get_ns_name(output_file_prefix);

  auto chunk_memory = std::make_unique<unsigned char[]>(chunk_size);

  for(size_t cchunk = 0; cchunk < full_chunk_count; cchunk++)
  {
    input_file.read(reinterpret_cast<char *>(chunk_memory.get()), chunk_size);
    process_chunk(chunk_memory.get(), chunk_size, output_file_prefix, cchunk, ns_name);
  }

  const size_t last_chunk_size = file_size - full_chunk_count * chunk_size;
  if(last_chunk_size)
  {
    input_file.read(reinterpret_cast<char *>(chunk_memory.get()), last_chunk_size);
    process_chunk(chunk_memory.get(), last_chunk_size, output_file_prefix, full_chunk_count, ns_name);
  }

  generate_allocator_file(output_file_prefix, chunk_size, full_chunk_count, file_size, ns_name);
}

int main(const int argc, const char ** argv)
{
  const char * description = R"(scre - simple c++ resource embedder.

Description:
Generates arrays on the stack with resource content, function to get heap-allocated pointer to the resource and all the needed headers.

Usage: <input_file> [output_file_prefix] [chunk_size in KBs])";

  if(!(argc >= 2 && argc <= 4))
  {
    std::cerr << description << '\n';
    return 1;
  }
  std::string input_file{argv[1]};
  auto        output_prefix = std::filesystem::path{input_file}.replace_extension("");
  if(argc > 2)
    output_prefix = argv[2];

  if(!output_prefix.has_filename())
    output_prefix = output_prefix.replace_filename(std::filesystem::path{input_file}.filename());
  std::string output_file{output_prefix.string()};
  uint16_t    chunk_size = 32;
  if(argc == 4)
    std::ifstream{argv[3]} >> chunk_size;

  try
  {
    process(input_file, output_file, 1024 * chunk_size);
  }
  catch(std::exception & e)
  {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  }
  catch(...)
  {
    std::cerr << "error: exception of unknown type!" << std::endl;
  }
  return 0;
}

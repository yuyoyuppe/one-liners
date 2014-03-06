#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <iostream>
#include <fstream>
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
	auto fname = std::regex_replace(input_filename, std::regex(R"reg((.*[/\\]))reg", std::regex_constants::ECMAScript), "");
	std::regex_search(fname, m, std::regex(R"reg(([a-zA-Z0-9_]+))reg", std::regex_constants::ECMAScript));
	return m[1];
}

void process_chunk(const unsigned char * data, size_t bytes_to_read, const std::string & output_file_prefix, const size_t chunk_number, const std::string & ns_name)
{
	const std::string chunk_outname(chunk_number_to_filename(chunk_number, output_file_prefix));
	if(boost::filesystem::exists(chunk_outname))
		throw(std::logic_error(chunk_outname + std::string(" is already exist")));

	std::ofstream of(chunk_outname);

	of << "namespace " << ns_name << "{unsigned char * get_part" << std::to_string(chunk_number) << "(){static unsigned char chunk[] = {";

	of << std::hex << std::showbase;
	while(bytes_to_read-- > 1) of << unsigned int(*data++) << ',';
	of << unsigned int(*data);
	of << "}; return reinterpret_cast<unsigned char *>(chunk);}}";
}

void generate_allocator_file(const std::string & output_file_prefix, const unsigned short chunk_size, const size_t full_chunk_count, const size_t file_size, const std::string & ns_name)
{
	const std::string allocator_fname(output_file_prefix + "_allocator.cpp");
	if(boost::filesystem::exists(allocator_fname))
		throw(std::logic_error(allocator_fname + std::string(" is already exist")));
	
	std::ofstream allocator_file(allocator_fname);

	allocator_file << "#include <string>" << std::endl << "#include <utility>" << std::endl << "namespace " << ns_name << " {";

	for(int i = 0; i < full_chunk_count; i++)
		allocator_file << "unsigned char * get_part" << std::to_string(i) << "();" << std::endl;

	const size_t last_chunk_size = file_size - full_chunk_count * chunk_size;
	if(last_chunk_size)
		allocator_file << "unsigned char * get_part" << std::to_string(full_chunk_count) << "();" << std::endl;

	allocator_file << "std::pair<const char *, size_t> allocate_resource(){"
	                  "unsigned char * res = new unsigned char[" << file_size << "];";
	
	for(int i = 0; i < full_chunk_count; i++)
		allocator_file << "memcpy(res + " << std::to_string(i * chunk_size) << ", " 
			<< "get_part" << std::to_string(i) << "(), " 
			<< std::to_string(chunk_size) << ");" << std::endl;

	if(last_chunk_size)
		allocator_file << "memcpy(res + " << std::to_string(full_chunk_count * chunk_size) << ", "
			<< "get_part" << std::to_string(full_chunk_count) << "(), "
			<< std::to_string(last_chunk_size) << ");" << std::endl;

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

void process(const std::string & input_filename, const std::string & output_file_prefix, const unsigned short chunk_size)
{
	if(!boost::filesystem::exists(input_filename)) 
		throw(std::logic_error(input_filename + std::string(" doesn't exist")));
	if(!boost::filesystem::is_regular_file(input_filename)) 
		throw(std::logic_error(input_filename + std::string(" isn't a regular file")));

	boost::interprocess::file_mapping fm(input_filename.c_str(), boost::interprocess::read_only);
	boost::interprocess::mapped_region ifr(fm, boost::interprocess::read_only);

	auto ifp = static_cast<unsigned char *>(ifr.get_address());
	const size_t file_size = ifr.get_size();
	const size_t full_chunk_count = file_size / chunk_size;

	auto ns_name = get_ns_name(input_filename);

	for(size_t cchunk = 0; cchunk < full_chunk_count; cchunk++) 
		process_chunk(ifp + cchunk * chunk_size, chunk_size, output_file_prefix, cchunk, ns_name);

	const size_t last_chunk_size = file_size - full_chunk_count * chunk_size;
	if(last_chunk_size) 
		process_chunk(ifp + full_chunk_count * chunk_size, last_chunk_size, output_file_prefix, full_chunk_count, ns_name);

	generate_allocator_file(output_file_prefix, chunk_size, full_chunk_count, file_size, ns_name);
}

void main(const int argc, const char ** argv)
{
	try 
	{
		boost::program_options::options_description desc("scre - simple c++ resource embedder.\n\nDescription:\nGenerates arrays on the stack with resource content, " 
		                                                 "function to get heap-allocated pointer to the resource and all the needed headers.\n\nUsage");
		desc.add_options()
			("help", "shows this help")
			("input", boost::program_options::value<std::string>(), "path to a file which will be embedded")
			("size", boost::program_options::value<unsigned short>()->default_value(500), "stack chunk size in KBs")
			("output", boost::program_options::value<std::string>()->default_value("generated"), "output filenames prefix");

		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);

		if(vm.count("help")) 
		{
			std::cout << desc << std::endl;
			return;
		}

		if(vm.count("input")) 
			process(vm["input"].as<std::string>(), vm["output"].as<std::string>(), 1024 * vm["size"].as<unsigned short>());
		else 
			std::cout << "error: you must specify the input file to compress! See --help for instructions" << std::endl;;
	}
	catch(std::exception & e) 
	{
		std::cerr << "error: " << e.what() << std::endl;
		return;
	}
	catch(...) 
	{
		std::cerr << "error: exception of unknown type!" << std::endl;
	}
}

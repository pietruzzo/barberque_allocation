#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "plp_translator.hpp"

// Return values:
#define PLP_ERROR   1
#define PLP_SUCCESS 0

// Internal use only:
#define HOST 0
#define MDEV 1

namespace bbque::tools {

int PLPTranslator::parse(const std::string &filename) noexcept{
	

	try {
		std::string filename_local;
		filename_local = this->explore_systems(filename);
		explore_localsys(filename_local);
	} catch (std::runtime_error e) {
		std::cerr << e.what() << std::endl;
		return PLP_ERROR;
	}

	return PLP_ERROR;
	return PLP_SUCCESS;
}

std::string PLPTranslator::explore_systems(const std::string &filename) {
	// Open the systems.xml file  and search for the 
	// local node.
	std::ifstream systems_file(filename);
	if(!systems_file)
		throw std::runtime_error("System file is not accessible.");

	// Read all characters to a local buffer
	std::stringstream buffer;
	buffer << systems_file.rdbuf();
	systems_file.close();
	std::string content(buffer.str());

	systems_doc.parse<0>(&content[0]);

	rapidxml::xml_node<> * root_node;
	root_node = systems_doc.first_node("systems");

	// Search all systems to find the local one
	for (rapidxml::xml_node<> * node = root_node->first_node("include");
		 node;
		 node->next_sibling()) {

		if ( node->first_attribute("local") && 
             node->first_attribute("local")->value() == std::string("true")) {
			return node->value();
		}

	}

	throw std::runtime_error("Local system not found.");
}

void PLPTranslator::explore_localsys(const std::string &filename) {
	// Open the systems.xml file  and search for the 
	// local node.
	std::ifstream systems_file(filename);
	if(!systems_file)
		throw std::runtime_error("Local system file is not accessible.");

	// Read all characters to a local buffer
	std::stringstream buffer;
	buffer << systems_file.rdbuf();
	systems_file.close();
	std::string content(buffer.str());

	localsys_doc.parse<0>(&content[0]);

	// Now explore it!
	rapidxml::xml_node<> * root_node;
	root_node = systems_doc.first_node("system");

	// For all cpu groups
	for (rapidxml::xml_node<> * node = root_node->first_node("cpu");
		 node;
		 node->next_sibling()) {

		std::string curr_mem;

		// Get the memory group of this cpu group
		if ( node->first_attribute("mem_id")) {
			curr_mem = node->first_attribute("mem_id")->value();
		} else {
			throw std::runtime_error("Missing mandatory mem_id argument in <cpu>");		
		}

		for (rapidxml::xml_node<> * pe = node->first_node("pe");
		     pe;
		     pe->next_sibling()) {

			if ( ! pe->first_attribute("id")) {
				throw std::runtime_error("Missing mandatory id argument in <pe>");		
			}

			if ( pe->first_attribute("managed") &&
                 pe->first_attribute("managed")->value() == std::string("false")) {
				add_pe(HOST, pe->first_attribute("id")->value(), curr_mem);
			} else {
				add_pe(MDEV, pe->first_attribute("id")->value(), curr_mem);
			}
		}

	}
	
}

void PLPTranslator::add_pe(int type, const std::string &pe_id,
                           const std::string &memory const std::string &quota) {
	

}

std::string PLPTranslator::get_output() const noexcept {
	return std::string("") + 

		   "# BarbequeRTRM Root Container"
		   "group bbque {"
		   "    perm {"
           "        task {"
		   "            uid = " + data.uid  + ";"
		   "            gid = " + data.guid + ";"
		   "        }"
		   "        admin {"
		   "            uid = " + data.uid  + ";"
		   "            gid = " + data.guid + ";"
		   "        }"
	       "    }"

"# This enables configuring a system so that several independent jobs can share"
"# common kernel data, such as file system pages, while isolating each job's"
"# user allocation in its own cpuset.  To do this, construct a large hardwall"
"# cpuset to hold all the jobs, and construct child cpusets for each individual"
"# job which are not hardwall cpusets."
           "    cpuset {"
		   "        cpuset.cpus = \"" + data.plat_cpus + "\";"
		   "        cpuset.mems = \"" + data.plat_mems + "\";"
		   "        cpuset.cpu_exclusive = \"1\";"
		   "        cpuset.mem_exclusive = \"1\";"
           ;
}


} // bbque::tools

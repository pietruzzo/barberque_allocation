#include "rxml_ploader.h"

#include "bbque/utils/logging/logger.h"
#include "bbque/utils/utility.h"
#include "bbque/config.h"

#include <boost/program_options.hpp>
#include <fstream>
#include <string>

#define CURRENT_VERSION "1.0"

namespace po = boost::program_options;


namespace bbque {
namespace plugins {

/** Set true it means the plugin has read its options in the config file*/
bool RXMLPlatformLoader::configured = false;

/** Recipes directory */
std::string RXMLPlatformLoader::platforms_dir = "";

/** Map of options (in the Barbeque config file) for the plugin */
po::variables_map xmlploader_opts_value;

RXMLPlatformLoader::~RXMLPlatformLoader() {

}

RXMLPlatformLoader::RXMLPlatformLoader() : initialized(false)
{
    // Get a logger
    logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
    assert(logger);

    logger->Debug("Built RXML PlatformLoader object @%p", (void*)this);

    // Save the hostname of the current machine for future use.
    local_hostname[1023] = '\0';
    gethostname(local_hostname, 1023);

}

RXMLPlatformLoader::ExitCode_t RXMLPlatformLoader::loadPlatformInfo() noexcept {

    if (this->initialized) {
        logger->Warn("RXMLPlatformLoader already initialized (I will ignore"
                        "the replicated loadPlatformInfo() call)");
        return PL_SUCCESS;
    }

    std::string   path(platforms_dir + "/systems.xml");
    std::ifstream xml_file(path);

    if (unlikely(!xml_file.good())) {
        logger->Error("systems.xml file not found, inaccessible or empty.");
        return PL_NOT_FOUND;
    }

    std::stringstream buffer;
    buffer << xml_file.rdbuf();
    xml_file.close();

    if (unlikely(!buffer.good())) {
        logger->Error("Reading of systems.xml failed.");
        return PL_NOT_FOUND;
    }

    std::string xmlstr(buffer.str());

    try {
        doc.parse<0>(&xmlstr[0]);    // Note: c++11 guarantees that the memory is
                                     // sequentially allocated inside the string,
                                     // so no problem here.
    } catch(const rapidxml::parse_error &e) {
        logger->Error("XML syntax error near %.10s: %s.", e.where<char>(), e.what());
        return PL_SYNTAX_ERROR;
    } catch (const std::runtime_error &e) {
        logger->Error("Generic error: %s.", e.what());
        return PL_GENERIC_ERROR;
    }

    ExitCode_t error = PL_GENERIC_ERROR;
    try {
        error = ParseDocument();
    } catch(const rapidxml::parse_error &e) {
        logger->Error("XML syntax error near %.10s: %s.", e.where<char>(), e.what());
    } catch(const PlatformLoaderEXC &e) {
        logger->Error("XML not valid: %s.", e.what());
    } catch(const std::runtime_error &e) {
        logger->Error("Generic error: %s.", e.what());
    } catch(...) {
        logger->Error("Generic error.");
    }

    if (error != PL_SUCCESS) {
        // Clean the platform description in case of error
        this->pd = pp::PlatformDescription();
        return error;
    }


    this->initialized = true;
    return PL_SUCCESS;
}

const pp::PlatformDescription &RXMLPlatformLoader::getPlatformInfo() const {
    if (!this->initialized) {
        throw PlatformLoaderEXC("getPlatformInfo() called before initialization.");
    }

    return this->pd;
}

pp::PlatformDescription &RXMLPlatformLoader::getPlatformInfo() {
    if (!this->initialized) {
        throw PlatformLoaderEXC("getPlatformInfo() called before initialization.");
    }

    return this->pd;
}

// =======================[ Static plugin interface ]=========================

void * RXMLPlatformLoader::Create(PF_ObjectParams *params) {
    if (!Configure(params))
        return nullptr;

    return new RXMLPlatformLoader();
}

int32_t RXMLPlatformLoader::Destroy(void *plugin) {
    if (!plugin)
        return -1;
    delete (RXMLPlatformLoader *)plugin;
    return 0;
}

bool RXMLPlatformLoader::Configure(PF_ObjectParams * params) {

    if (configured)
        return true;

    // Declare the supported options
    po::options_description xmlploader_opts_desc("RXML Platform Loader Options");
    xmlploader_opts_desc.add_options()
        (MODULE_CONFIG".platform_dir", po::value<std::string>
         (&platforms_dir)->default_value(BBQUE_PATH_PREFIX "/" BBQUE_PATH_PILS),
         "platform folder")
    ;

    // Get configuration params
    PF_Service_ConfDataIn data_in;
    data_in.opts_desc = &xmlploader_opts_desc;
    PF_Service_ConfDataOut data_out;
    data_out.opts_value = &xmlploader_opts_value;

    PF_ServiceData sd;
    sd.id = MODULE_NAMESPACE;
    sd.request = &data_in;
    sd.response = &data_out;

    int32_t response =
        params->platform_services->InvokeService(PF_SERVICE_CONF_DATA, sd);

    if (response!=PF_SERVICE_DONE)
        return false;

    if (daemonized)
        syslog(LOG_INFO, "Using RXMLPlatformLoader platform folder [%s]",
                platforms_dir.c_str());
    else
        fprintf(stdout, FI("Using RXMLPlatformLoader platform folder [%s]\n"),
                platforms_dir.c_str());

    return true;
}

// =======================[ XML releated methods ]=========================


rapidxml::xml_node<> * RXMLPlatformLoader::GetFirstChild(rapidxml::xml_node<> * parent, const char* name, bool mandatory) const {

    rapidxml::xml_node<> * child = parent->first_node(name, 0, true);

    if (child == 0) {
        if (mandatory) {
            logger->Error("Missing mandatory <%s> tag.", name);
            throw PlatformLoaderEXC("Missing mandatory tag.");
        } else {
            return nullptr;
        }
    }

    return child;
}

rapidxml::xml_attribute<> * RXMLPlatformLoader::GetFirstAttribute(rapidxml::xml_node<> * tag, const char* name, bool mandatory) const {

    rapidxml::xml_attribute<> * attr= tag->first_attribute(name, 0, true);

    if (attr == 0) {
        if (mandatory) {
            logger->Error("Missing argument '%s' in <%s> tag.", name, tag->name());
            throw PlatformLoaderEXC("Missing mandatory argument.");
        } else {
            return nullptr;
        }
    }

    return attr;
}



RXMLPlatformLoader::ExitCode_t RXMLPlatformLoader::ParseDocument() {

    node_ptr root    = this->GetFirstChild(&doc,"systems", true);
    attr_ptr version = this->GetFirstAttribute(root, "version", true);

    if (strcmp(version->value(), CURRENT_VERSION) != 0) {
        logger->Error("Version mismatch: my version is " CURRENT_VERSION " but systems.xml"
                      "has version %s.", version->value());
        return PL_GENERIC_ERROR;
    }

    assert(root);

    bool local_found = false;
    node_ptr include_sys = this->GetFirstChild(root,"include", true) ;
    while(include_sys != NULL) {
        bool curr_local = PL_SUCCESS == this->ParseSystemDocument(include_sys->value());


        if (local_found && curr_local) {
            // That's bad. I found two local systems,
            // this is not possible.
            logger->Error("More than one local system found!");
            return PL_LOGIC_ERROR;
        }

        if (!local_found) {
            // If the loal is not found yet, check if the current is local
            local_found = curr_local;
        }


        include_sys = include_sys->next_sibling("include");
    }

    if (!local_found) {
        logger->Error("No local system found in systems.xml.");
        return PL_LOGIC_ERROR;
    }

    return PL_SUCCESS;
}

RXMLPlatformLoader::ExitCode_t RXMLPlatformLoader::ParseSystemDocument(const char* name) {
    logger->Info("Loading %s platform file...", name);

    std::string   path(platforms_dir + "/" + name);
    std::ifstream xml_file(path);

    if (unlikely(!xml_file.good())) {
        logger->Error("%s file not found, inaccessible or empty.", name);
        return PL_NOT_FOUND;
    }

    std::stringstream buffer;
    buffer << xml_file.rdbuf();
    xml_file.close();

    if (unlikely(!buffer.good())) {
        logger->Error("Reading of %s failed.", name);
        return PL_NOT_FOUND;
    }

    std::string xmlstr(buffer.str());

    rapidxml::xml_document<> inc_doc;
    // Not try{}catch{} it, leave it throwing, managed at upper levels
    inc_doc.parse<0>(&xmlstr[0]);    // Note: c++11 guarantees that the memory is
                                     // sequentially allocated inside the string,
                                     // so no problem here.

    node_ptr root     = this->GetFirstChild(&inc_doc, "system", true);

    // For the root tag, we have two parameters: hostname and address.
    // The last is mandatory only for remote systems, and ignored for
    // local ones
    attr_ptr hostname = this->GetFirstAttribute(root, "hostname", true);

    bool is_local = 0 == strcmp(local_hostname, hostname->value());

    // The address is mandatory only if the system is remote.
    attr_ptr address  = this->GetFirstAttribute(root, "address", !is_local);
    logger->Debug("Parsing system %s at address %s", hostname->value(), address->value() ? address->value() : "`localhost`");

    pp::PlatformDescription::System sys;
    sys.SetLocal(is_local);
    sys.SetHostname(hostname->value());
    if (address) {
        sys.SetNetAddress(address->value());
        if(is_local) {
            logger->Warn("Address specified in a local system (I will ignore it)");
        }
    }

    ///   <memory>
    RXMLPlatformLoader::ExitCode_t ec = PL_SUCCESS;
    ec = ParseMemories(root, sys);
    if (ec != PL_SUCCESS)
        return ec;

    ///                                                             ///
    ///                             <cpu>                           ///
    ///                                                             ///

    node_ptr cpu_tag = this->GetFirstChild(root,"cpu") ;
    while( cpu_tag != NULL ) {
        pp::PlatformDescription::CPU cpu;
        attr_ptr arch_attr     = this->GetFirstAttribute(cpu_tag, "arch",     true);
        attr_ptr id_attr       = this->GetFirstAttribute(cpu_tag, "id",       true);
        attr_ptr socket_id_attr= this->GetFirstAttribute(cpu_tag, "socket_id",true);
        attr_ptr mem_id_attr   = this->GetFirstAttribute(cpu_tag, "mem_id",   true);

        const char * arch = arch_attr->value();
        short        id;
        short        socket_id;
        short        mem_id;

        /// Read id=""
        try {
            // Yes, this conversion is unsafe. However, there will not probably
            // be over 32767 cpu in one machine...
            id = (short)std::stoi(id_attr->value());
        } catch(const std::invalid_argument &e) {
            logger->Error("ID for <cpu> is not a valid integer.");
            return PL_LOGIC_ERROR;
        } catch(const std::out_of_range &e) {
            logger->Error("ID for <cpu> is out-of-range.");
            return PL_LOGIC_ERROR;
        }

        /// Read socket_id=""
        try {
            // Yes, this conversion is unsafe. However, there will not probably
            // be over 32767 cpu in one machine...
            socket_id = (short)std::stoi(socket_id_attr->value());
        } catch(const std::invalid_argument &e) {
            logger->Error("socket_id for <cpu> is not a valid integer.");
            return PL_LOGIC_ERROR;
        } catch(const std::out_of_range &e) {
            logger->Error("socket_id for <cpu> is out-of-range.");
            return PL_LOGIC_ERROR;
        }

        /// Read mem_id=""
        try {
            // Yes, this conversion is unsafe. However, there will not probably
            // be over 32767 memories in one machine...
            mem_id = (short)std::stoi(mem_id_attr->value());
        } catch(const std::invalid_argument &e) {
            logger->Error("mem_id for <cpu> is not a valid integer.");
            return PL_LOGIC_ERROR;
        } catch(const std::out_of_range &e) {
            logger->Error("mem_id for <cpu> is out-of-range.");
            return PL_LOGIC_ERROR;
        }

        std::shared_ptr<pp::PlatformDescription::Memory>
                curr_memory = sys.GetMemoryById(mem_id);

        if (curr_memory == nullptr) {
            logger->Error("<cpu> with memory id %d not found in memories list.", mem_id);
            return PL_LOGIC_ERROR;
        }

        cpu.SetArchitecture(arch);
        cpu.SetId(id);
        cpu.SetSocketId(socket_id);
        cpu.SetMemory(curr_memory);


        // TODO: extract <info> tag

        ///                                                             ///
        ///                             <pe>                            ///
        ///                                                             ///
        node_ptr pe_tag = this->GetFirstChild(cpu_tag,"pe");
        while (pe_tag != NULL) {
            pp::PlatformDescription::ProcessingElement pe;

            attr_ptr id_attr       = this->GetFirstAttribute(pe_tag, "id",       true);
            attr_ptr core_id_attr  = this->GetFirstAttribute(pe_tag, "core_id",  true);
            attr_ptr share_attr    = this->GetFirstAttribute(pe_tag, "share",    true);
            attr_ptr managed_attr  = this->GetFirstAttribute(pe_tag, "managed",  false);

            int id;
            short core_id, share;

            /// Read id=""
            try {
                id = (short)std::stoi(id_attr->value());
            } catch(const std::invalid_argument &e) {
                logger->Error("id for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("id for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            /// Read socket_id=""
            try {
                core_id = (short)std::stoi(core_id_attr->value());
            } catch(const std::invalid_argument &e) {
                logger->Error("core_id for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("core_id for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            /// Read share=""
            try {
                share = (short)std::stoi(share_attr->value());
            } catch(const std::invalid_argument &e) {
                logger->Error("share for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("share for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            // Check that share is in the limits
            if (share < 0 || share > 100) {
                logger->Error("share for <pe> must be between 0 and 100, but it's %i.", share);
                return PL_LOGIC_ERROR;
            }

            // Read managed=""
            if(managed_attr != nullptr) {
                switch(ConstHashString(managed_attr->value())) {
                    case ConstHashString("shared"):
                        pe.SetPartitionType(pp::PlatformDescription::SHARED);
                    break;
                    case ConstHashString("host"):
                        pe.SetPartitionType(pp::PlatformDescription::HOST);
                    break;
                    case ConstHashString("mdev"):
                        pe.SetPartitionType(pp::PlatformDescription::MDEV);;
                    break;
                    default:
                        logger->Error("'%s' is not a valid value for `managed` attribute.", managed_attr->value());
                        return PL_LOGIC_ERROR;
                    break;
                }
            } else {
                // If not specified the default is device-only.
                pe.SetPartitionType(pp::PlatformDescription::MDEV);
            }


            pe.SetId(id);
            pe.SetCoreId(core_id);
            pe.SetShare(share);
            cpu.AddProcessingElement(pe);

            pe_tag = pe_tag->next_sibling("pe");
        }

        sys.AddCPU(cpu);
        cpu_tag = cpu_tag->next_sibling("cpu");
    }

    ///                                                             ///
    ///                             <gpu>                           ///
    ///                                                             ///

    node_ptr gpu_tag = this->GetFirstChild(root,"gpu") ;
    while (gpu_tag != NULL) {
        pp::PlatformDescription::MulticoreProcessor gpu;
        attr_ptr arch_attr     = this->GetFirstAttribute(gpu_tag, "arch",     true);
        attr_ptr id_attr       = this->GetFirstAttribute(gpu_tag, "id",       true);

        const char * arch = arch_attr->value();
        short        id;
        /// Read id=""
        try {
            // Yes, this conversion is unsafe. However, there will not probably
            // be over 32767 cpu in one machine...
            id = (short)std::stoi(id_attr->value());
        } catch(const std::invalid_argument &e) {
            logger->Error("ID for <gpu> is not a valid integer.");
            return PL_LOGIC_ERROR;
        } catch(const std::out_of_range &e) {
            logger->Error("ID for <gpu> is out-of-range.");
            return PL_LOGIC_ERROR;
        }

        gpu.SetArchitecture(arch);
        gpu.SetId(id);

        // TODO: extract <info> tag

        ///                                                             ///
        ///                             <pe>                            ///
        ///                                                             ///
        node_ptr pe_tag = this->GetFirstChild(gpu_tag,"pe");
        while(pe_tag != NULL) {
            pp::PlatformDescription::ProcessingElement pe;
            attr_ptr id_attr       = this->GetFirstAttribute(pe_tag, "id",       true);
            attr_ptr quantity_attr = this->GetFirstAttribute(pe_tag, "quantity", false);
            attr_ptr share_attr    = this->GetFirstAttribute(pe_tag, "share",    true);


            int id;
            short quantity, share;

            /// Read id=""
            try {
                id = (short)std::stoi(id_attr->value());
            } catch(const std::invalid_argument e) {
                logger->Error("id for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("id for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            /// Read quantity=""
            try {
                quantity = std::stoi(quantity_attr->value());
            } catch(const std::invalid_argument &e) {
                logger->Error("quantity for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("quantity for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            /// Read share=""
            try {
                share = (short)std::stoi(share_attr->value());
            } catch(const std::invalid_argument &e) {
                logger->Error("share for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("share for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            // Check that share is in the limits
            if (share < 0 || share > 100) {
                logger->Error("share for <pe> must be between 0 and 100, but it's %i.", share);
                return PL_LOGIC_ERROR;
            }

            pe.SetId(id);
            pe.SetQuantity(quantity);
            pe.SetShare(share);
            gpu.AddProcessingElement(pe);

            pe_tag = pe_tag->next_sibling("pe");
        }

        sys.AddGPU(gpu);
        gpu_tag = gpu_tag->next_sibling("gpu");
    }

    ///                                                             ///
    ///                             <acc>                           ///
    ///                                                             ///

    node_ptr acc_tag = this->GetFirstChild(root,"acc") ;
    while (acc_tag != NULL) {
        pp::PlatformDescription::MulticoreProcessor acc;
        attr_ptr arch_attr     = this->GetFirstAttribute(acc_tag, "arch",     true);
        attr_ptr id_attr       = this->GetFirstAttribute(acc_tag, "id",       true);

        const char * arch = arch_attr->value();
        short        id;
        /// Read id=""
        try {
            // Yes, this conversion is unsafe. However, there will not probably
            // be over 32767 cpu in one machine...
            id = (short)std::stoi(id_attr->value());
        } catch(const std::invalid_argument &e) {
            logger->Error("ID for <acc> is not a valid integer.");
            return PL_LOGIC_ERROR;
        } catch(const std::out_of_range &e) {
            logger->Error("ID for <acc> is out-of-range.");
            return PL_LOGIC_ERROR;
        }

        acc.SetArchitecture(arch);
        acc.SetId(id);

        // TODO: extract <info> tag

        ///                                                             ///
        ///                             <pe>                            ///
        ///                                                             ///
        node_ptr pe_tag = this->GetFirstChild(acc_tag,"pe");
        while (pe_tag != NULL) {
            pp::PlatformDescription::ProcessingElement pe;

            attr_ptr id_attr       = this->GetFirstAttribute(pe_tag, "id",       true);
            attr_ptr quantity_attr = this->GetFirstAttribute(pe_tag, "quantity", false);
            attr_ptr share_attr    = this->GetFirstAttribute(pe_tag, "share",    true);

            int id;
            short quantity, share;

            /// Read id=""
            try {
                id = (short)std::stoi(id_attr->value());
            } catch(const std::invalid_argument &e) {
                logger->Error("id for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("id for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            /// Read quantity=""
            try {
                quantity = std::stoi(quantity_attr->value());
            } catch(const std::invalid_argument &e) {
                logger->Error("quantity for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("quantity for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            /// Read share=""
            try {
                share = (short)std::stoi(share_attr->value());
            } catch(const std::invalid_argument &e) {
                logger->Error("share for <pe> is not a valid integer.");
                return PL_LOGIC_ERROR;
            } catch(const std::out_of_range &e) {
                logger->Error("share for <pe> is out-of-range.");
                return PL_LOGIC_ERROR;
            }

            // Check that share is in the limits
            if (share < 0 || share > 100) {
                logger->Error("share for <pe> must be between 0 and 100, but it's %i.", share);
                return PL_LOGIC_ERROR;
            }

            pe.SetId(id);
            pe.SetQuantity(quantity);
            pe.SetShare(share);
            acc.AddProcessingElement(pe);

            pe_tag = pe_tag->next_sibling("pe");
        }

        sys.AddAccelerator(acc);
        acc_tag = acc_tag->next_sibling("acc");
    }

    pd.AddSystem(sys);
    return is_local ? PL_SUCCESS : PL_SUCCESS_NO_LOCAL;
}



RXMLPlatformLoader::ExitCode_t RXMLPlatformLoader::ParseMemories(
		node_ptr root,
		pp::PlatformDescription::System & sys) {
    // Now get all the memories and save them. This should be perfomed before <cpu> and
    // other tags, due to reference to memories inside them.
    node_ptr memory_tag = this->GetFirstChild(root,"mem",true) ;
    while( memory_tag != NULL ) {
        pp::PlatformDescription::Memory mem;
        attr_ptr id_attr       = this->GetFirstAttribute(memory_tag, "id",       true);
        attr_ptr quantity_attr = this->GetFirstAttribute(memory_tag, "quantity", true);
        attr_ptr unit_attr     = this->GetFirstAttribute(memory_tag, "unit",     true);

        short        id;
        int          quantity;
        int_fast16_t exp;

        // id=""
        try {
            // Yes, this conversion is unsafe. However, there will not probably
            // be over 32767 memories in one machine...
            id = (short)std::stoi(id_attr->value());
        } catch(const std::invalid_argument& e) {
            logger->Error("ID for <memory> is not a valid integer.");
            return PL_LOGIC_ERROR;
        } catch(const std::out_of_range& e) {
            logger->Error("ID for <memory> is out-of-range, please change your unit.");
            return PL_LOGIC_ERROR;
        }

        // quantity=""
        try {
            quantity = std::stoi(quantity_attr->value());
        } catch(const std::invalid_argument &e) {
            logger->Error("Quantity for <memory> is not a valid integer.");
            return PL_LOGIC_ERROR;
        } catch(const std::out_of_range &e) {
            logger->Error("Quantity for <memory> is out-of-range, please increase your unit.");
            return PL_LOGIC_ERROR;
        }

        // unit=""
        switch(ConstHashString(unit_attr->value())) {
            case ConstHashString("B"):
                exp=0;
            break;
            case ConstHashString("KB"):
                exp=10;
            break;
            case ConstHashString("MB"):
                exp=20;
            break;
            case ConstHashString("GB"):
                exp=20;
            break;
            case ConstHashString("TB"):
                exp=40;
            break;
            default:
                logger->Error("Invalid `unit` for <memory>.");
                return PL_LOGIC_ERROR;
            break;
        }

        mem.SetId(id);

#if BBQUE_PP_ARCH_SUPPORTS_INT64
        mem.SetQuantity(((int64_t)quantity) << exp);
#else
        if (exp < 32) {
            mem.SetQuantityLO(((int32_t)quantity) << exp );
            mem.SetQuantityHI(((int32_t)quantity) >> (32-exp) );
        } else {
            mem.SetQuantityLO(0);
            mem.SetQuantityHI(((int32_t)quantity) << (exp-32) );
        }
#endif

        sys.AddMemory(std::make_shared<pp::PlatformDescription::Memory>(mem));

        memory_tag = memory_tag->next_sibling("mem");
    }

    return PL_SUCCESS;
}


}   // namespace plugins
}   // namespace bbque

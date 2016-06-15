#include "rxml_ploader.h"

#include "bbque/utils/logging/logger.h"
#include "bbque/utils/utility.h"
#include "bbque/config.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;


namespace bbque {
namespace plugins {

/** Set true it means the plugin has read its options in the config file*/
bool RXMLPlatformLoader::configured = false;

/** Recipes directory */
std::string RXMLPlatformLoader::platforms_dir = "";

/** Map of options (in the Barbeque config file) for the plugin */
po::variables_map xmlploader_opts_value;



RXMLPlatformLoader::RXMLPlatformLoader() : initialized(false)
{

}

RXMLPlatformLoader::ExitCode_t RXMLPlatformLoader::loadPlatformInfo() noexcept {

    if (this->initialized)
        return PL_SUCCESS;



    this->initialized = true;
    return PL_SUCCESS;
}

const pp::PlatformDescription &RXMLPlatformLoader::getPlatformInfo() const {
    if (!this->initialized) {
        throw new PlatformLoaderEXC("getPlatformInfo() called before initialization.");
    }

    return this->pd;
}

pp::PlatformDescription &RXMLPlatformLoader::getPlatformInfo() {
    if (!this->initialized) {
        throw new PlatformLoaderEXC("getPlatformInfo() called before initialization.");
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




}   // namespace plugins
}   // namespace bbque

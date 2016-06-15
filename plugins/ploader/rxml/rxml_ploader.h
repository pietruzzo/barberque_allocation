#ifndef PLATFORMLOADER_H
#define PLATFORMLOADER_H

#include <stdexcept>
#include "bbque/plugins/platform_loader.h"
#include "bbque/plugin_manager.h"
#include "bbque/plugins/plugin.h"

#define MODULE_NAMESPACE PLATFORM_LOADER_NAMESPACE".rxml"
#define MODULE_CONFIG PLATFORM_LOADER_CONFIG".rxml"


namespace bbque {
namespace plugins {

class RXMLPlatformLoader : PlatformLoaderIF
{
public:

    class PlatformLoaderEXC : public std::runtime_error {
    public:
        PlatformLoaderEXC(const char* x) : std::runtime_error(x) { }
    };

    using PlatformLoaderIF::ExitCode_t;

    /**
     * Default destructor
     */
    virtual ~RXMLPlatformLoader();


    /**
     * @brief Method for creating the static plugin
     */
    static void * Create(PF_ObjectParams *);

    /**
     * @brief Method for destroying the static plugin
     */
    static int32_t Destroy(void *);

    /**
     * @brief  Open the file containing the information about the platform.
     *         Call multiple times this method does not imply a reloading, but
     *         a warning generation.
     * @return PL_SUCCESS in case of success or the error id in case of
     *         failure.
     */
   virtual ExitCode_t loadPlatformInfo() noexcept final;

   /**
    * @brief Return the loaded platform description. You must have successfully
    *        called loadPlatformInfo() before call this method.
    * @throw std::runtime_error in case of non loaded platform.
    * @return The current PlatformDescription.
    */
   virtual const pp::PlatformDescription &getPlatformInfo() const final;
   virtual       pp::PlatformDescription &getPlatformInfo()       final;

private:

    bool initialized;


    /**
     * Set true when the platform loader has been configured.
     * This is done by parsing a configuration file the first time a
     * platform loader is created.
     */
    static bool configured;



    /**
     * The directory path containing all XML files regarding platforms
     */
    static std::string platforms_dir;

    /**
     * @brief Load RecipeLoader configuration
     *
     * @param params @see PF_ObjectParams
     * @return True if the configuration has been properly loaded and object
     * could be built, false otherwise
     */
    static bool Configure(PF_ObjectParams * params);


    RXMLPlatformLoader();

    pp::PlatformDescription pd;
};


}   // namespace plugins
}   // namespace bbque
#endif // PLATFORMLOADER_H

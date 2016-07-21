#ifndef BBQUE_PLATFORM_LOADER_H
#define BBQUE_PLATFORM_LOADER_H

#include "bbque/pp/platform_description.h"

// The prefix for logging statements category
#define PLATFORM_LOADER_NAMESPACE "bq.pl"
// The prefix for configuration file attributes
#define PLATFORM_LOADER_CONFIG "ploader"

#define PLATFORM_MAJOR_VERSION 	0
#define PLATFORM_MINOR_VERSION 	1

namespace bbque {
namespace plugins {

/**
 * @class PlatformLoaderIF
 *
 * @brief Basic interface for platform information loading.
 *
 * This interface contains only two items, a loader to perform the
 * loading of the information (e.g. from the xml file). In this way
 * all the information are saved into the object and provided in
 * read-only manner via the getPlatformInfo() method.
 *
 * @note This representation obviously not includes the dynamic
 * resources like discovered remote barbeque.
 */
class PlatformLoaderIF {

public:

    /**
     * @brief Recipe load exit codes
     */
    enum ExitCode_t {
        //--- Success load
        /** Load completed with success */
        PL_SUCCESS = 0,
        /** One or more files not found */
        PL_NOT_FOUND,
        /** Syntax error in file (e.g. XML unclosed tag) */
        PL_SYNTAX_ERROR,
        /** Logic error in file (e.g. missing mandatory parameter) */
        PL_LOGIC_ERROR,
        /** Other errors */
        PL_GENERIC_ERROR,
    };


    /**
     * @brief  Open the file containing the information about the platform.
     *         Call multiple times this method does not imply a reloading, but
     *         a warning generation.
     * @return PL_SUCCESS in case of success or the error id in case of
     *         failure.
     */
    virtual ExitCode_t loadPlatformInfo() noexcept = 0;

    /**
     * @brief Return the loaded platform description. You must have successfully
     *        called loadPlatformInfo() before call this method.
     * @throw std::runtime_error in case of non loaded platform.
     * @return The current PlatformDescription.
     */
    virtual const bbque::pp::PlatformDescription &getPlatformInfo() const = 0;

    /**
     * @brief Return the loaded platform description. You must have successfully
     *        called loadPlatformInfo() before call this method.
     *        With this method the return type is editable.
     * @throw std::runtime_error in case of non loaded platform.
     * @return The current PlatformDescription.
     */
    virtual bbque::pp::PlatformDescription &getPlatformInfo() = 0;


};


} // namespace plugins
} // namespace bbque
#endif // BBQUE_PLATFORM_LOADER_H

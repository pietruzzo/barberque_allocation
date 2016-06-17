#ifndef BBQUE_LINUX_PLATFORM_PROXY_H
#define BBQUE_LINUX_PLATFORM_PROXY_H


#include "bbque/platform_proxy.h"

namespace bbque {

class LinuxPlatformProxy : public PlatformProxy
{
public:

    LinuxPlatformProxy();

    /**
     * @brief Return the Platform specific string identifier
     */
    virtual const char* GetPlatformID(int16_t system_id=-1) const final;

    /**
     * @brief Return the Hardware identifier string
     */
    virtual const char* GetHardwareID(int16_t system_id=-1) const final;
    /**
     * @brief Platform specific resource setup interface.
     */
    virtual ExitCode_t Setup(AppPtr_t papp) final;

    /**
     * @brief Platform specific resources enumeration
     *
     * The default implementation of this method loads the TPD, is such a
     * function has been enabled
     */
    virtual ExitCode_t LoadPlatformData() final;

    /**
     * @brief Platform specific resources refresh
     */
    virtual ExitCode_t RefreshPlatformData() final;

    /**
     * @brief Platform specific resources release interface.
     */
    virtual ExitCode_t Release(AppPtr_t papp) final;

    /**
     * @brief Platform specific resource claiming interface.
     */
    virtual ExitCode_t ReclaimResources(AppPtr_t papp) final;

    /**
     * @brief Platform specific resource binding interface.
     */
    virtual ExitCode_t MapResources(AppPtr_t papp, UsagesMapPtr_t pres,
            RViewToken_t rvt, bool excl) final;


};

}   // namespace bbque

#endif // LINUX_PLATFORM_PROXY_H

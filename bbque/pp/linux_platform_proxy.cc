#include "bbque/pp/linux_platform_proxy.h"
#include "bbque/config.h"

#include <boost/program_options.hpp>
#include <libcgroup.h>
#include <linux/version.h>

#define BBQUE_LINUXPP_PLATFORM_ID		"org.linux.cgroup"

#define BBQUE_LINUXPP_SILOS 			BBQUE_LINUXPP_CGROUP"/silos"

#define BBQUE_LINUXPP_CPUS_PARAM 		"cpuset.cpus"
#define BBQUE_LINUXPP_CPUP_PARAM 		"cpu.cfs_period_us"
#define BBQUE_LINUXPP_CPUQ_PARAM 		"cpu.cfs_quota_us"
#define BBQUE_LINUXPP_MEMN_PARAM 		"cpuset.mems"
#define BBQUE_LINUXPP_MEMB_PARAM 		"memory.limit_in_bytes"
#define BBQUE_LINUXPP_CPU_EXCLUSIVE_PARAM 	"cpuset.cpu_exclusive"
#define BBQUE_LINUXPP_MEM_EXCLUSIVE_PARAM 	"cpuset.mem_exclusive"
#define BBQUE_LINUXPP_PROCS_PARAM		"cgroup.procs"

#define BBQUE_LINUXPP_SYS_MEMINFO		"/proc/meminfo"

// The default CFS bandwidth period [us]
#define BBQUE_LINUXPP_CPUP_DEFAULT		100000

// Checking for kernel version requirements

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#error Linux kernel >= 2.6.34 required by the Platform Integration Layer
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
#warning CPU Quota management disabled, Linux kernel >= 3.2 required
#endif

#define MODULE_CONFIG "LinuxPlatformProxy"

namespace bb = bbque;
namespace br = bbque::res;
namespace po = boost::program_options;


namespace bbque {
namespace pp {

LinuxPlatformProxy::LinuxPlatformProxy() : controller("cpuset"),
    cfsQuotaSupported(true)  {

    //---------- Get a logger module
    logger = bu::Logger::GetLogger(LINUX_PP_NAMESPACE);
    assert(logger);

    //---------- Loading configuration
    this->LoadConfiguration();

    //---------- Init the CGroups
    if (this->InitCGroups() != PLATFORM_OK) {
        // We have to throw the exception: without cgroups the object should not
        // be created!
        throw new std::runtime_error("Unable to construct LinuxPlatformProxy, "
                                     "InitCGroups() failed.");
    }

#ifdef CONFIG_TARGET_ARM_BIG_LITTLE
    // Initialize the set of ARM "big" cores
    InitCoresType();
#endif

}

LinuxPlatformProxy::~LinuxPlatformProxy() {

}

const char* LinuxPlatformProxy::GetPlatformID(int16_t system_id) const noexcept {
    (void) system_id;
    return BBQUE_LINUXPP_PLATFORM_ID;
}

const char* LinuxPlatformProxy::GetHardwareID(int16_t system_id) const noexcept {
    (void) system_id;
    return BBQUE_TARGET_HARDWARE_ID;    // Defined in bbque.conf
}

LinuxPlatformProxy::ExitCode_t Setup(AppPtr_t papp) noexcept {

}



void LinuxPlatformProxy::LoadConfiguration() noexcept {
    po::options_description opts_desc("Linux Platform Proxy Options");
    opts_desc.add_options()
        (MODULE_CONFIG ".cfs_bandwidth.margin_pct",
         po::value<int>
         (&cfs_margin_pct)->default_value(0),
         "The safety margin [%] to add for CFS bandwidth enforcement");
    opts_desc.add_options()
        (MODULE_CONFIG ".cfs_bandwidth.threshold_pct",
         po::value<int>
         (&cfs_threshold_pct)->default_value(100),
         "The threshold [%] under which we enable CFS bandwidth enforcement");
    po::variables_map opts_vm;
    ConfigurationManager::GetInstance().
        ParseConfigurationFile(opts_desc, opts_vm);

    // Range check
    cfs_margin_pct = std::min(std::max(cfs_margin_pct, 0), 100);
    cfs_threshold_pct = std::min(std::max(cfs_threshold_pct, 0), 100);

    // Force threshold to be NOT lower than (100 - margin)
    if (cfs_threshold_pct < cfs_margin_pct)
        cfs_threshold_pct = 100 - cfs_margin_pct;

    logger->Info("CFS bandwidth control, margin %d, threshold: %d",
            cfs_margin_pct, cfs_threshold_pct);
}

LinuxPlatformProxy::ExitCode_t LinuxPlatformProxy::InitCGroups() noexcept {
    ExitCode_t pp_result;
    char *mount_path = NULL;
    int cg_result;

    // Init the Control Group Library
    cg_result = cgroup_init();
    if (cg_result) {
        logger->Error("PLAT LNX: CGroup Library initializaton FAILED! "
                "(Error: %d - %s)", cg_result, cgroup_strerror(cg_result));
        return PLATFORM_INIT_FAILED;
    }

    cg_result = cgroup_get_subsys_mount_point(controller, &mount_path);
    if (cg_result) {
        logger->Error("PLAT LNX: CGroup Library mountpoint lookup FAILED! "
                "(Error: %d - %s)", cg_result, cgroup_strerror(cg_result));
        return PLATFORM_GENERIC_ERROR;
    }
    logger->Info("PLAT LNX: controller [%s] mounted at [%s]",
            controller, mount_path);


    // TODO: check that the "bbq" cgroup already existis
    // TODO: check that the "bbq" cgroup has CPUS and MEMS
    // TODO: keep track of overall associated resources => this should be done
    // by exploting the LoadPlatformData low-level method
    // TODO: update the MaxCpusCount and MaxMemsCount

    // Build "silos" CGroup to host blocked applications
    pp_result = BuildSilosCG(psilos);
    if (pp_result) {
        logger->Error("PLAT LNX: Silos CGroup setup FAILED!");
        return PLATFORM_GENERIC_ERROR;
    }

    free(mount_path);

    return PLATFORM_OK;


}

LinuxPlatformProxy::ExitCode_t
LinuxPlatformProxy::BuildSilosCG(CGroupDataPtr_t &pcgd) noexcept {
    RLinuxBindingsPtr_t prlb(new RLinuxBindings_t(
                MaxCpusCount, MaxMemsCount));
    ExitCode_t result;
    int error;

    logger->Debug("PLAT LNX: Building SILOS CGroup...");

    // Build new CGroup data
    pcgd = CGroupDataPtr_t(new CGroupData_t(BBQUE_LINUXPP_SILOS));
    result = BuildCGroup(pcgd);
    if (result != PLATFORM_OK)
        return result;

    // Setting up silos (limited) resources, just to run the RTLib
    sprintf(prlb->cpus, "0");
    sprintf(prlb->mems, "0");

    // Configuring silos constraints
    cgroup_set_value_string(pcgd->pc_cpuset,
            BBQUE_LINUXPP_CPUS_PARAM, prlb->cpus);
    cgroup_set_value_string(pcgd->pc_cpuset,
            BBQUE_LINUXPP_MEMN_PARAM, prlb->mems);

    // Updating silos constraints
    logger->Notice("PLAT LNX: Updating kernel CGroup [%s]", pcgd->cgpath);
    error = cgroup_modify_cgroup(pcgd->pcg);
    if (error) {
        logger->Error("PLAT LNX: CGroup resource mapping FAILED "
                "(Error: libcgroup, kernel cgroup update "
                "[%d: %s]", errno, strerror(errno));
        return PLATFORM_MAPPING_FAILED;
    }

    return PLATFORM_OK;
}

LinuxPlatformProxy::ExitCode_t
LinuxPlatformProxy::BuildCGroup(CGroupDataPtr_t &pcgd) noexcept {
    int result;

    logger->Debug("PLAT LNX: Building CGroup [%s]...", pcgd->cgpath);

    // Setup CGroup path for this application
    pcgd->pcg = cgroup_new_cgroup(pcgd->cgpath);
    if (!pcgd->pcg) {
        logger->Error("PLAT LNX: CGroup resource mapping FAILED "
                "(Error: libcgroup, \"cgroup\" creation)");
        return PLATFORM_MAPPING_FAILED;
    }

    // Add "cpuset" controller
    pcgd->pc_cpuset = cgroup_add_controller(pcgd->pcg, "cpuset");
    if (!pcgd->pc_cpuset) {
        logger->Error("PLAT LNX: CGroup resource mapping FAILED "
                "(Error: libcgroup, [cpuset] \"controller\" "
                "creation failed)");
        return PLATFORM_MAPPING_FAILED;
    }

#ifdef CONFIG_BBQUE_LINUX_CG_MEMORY
    // Add "memory" controller
    pcgd->pc_memory = cgroup_add_controller(pcgd->pcg, "memory");
    if (!pcgd->pc_memory) {
        logger->Error("PLAT LNX: CGroup resource mapping FAILED "
                "(Error: libcgroup, [memory] \"controller\" "
                "creation failed)");
        return PLATFORM_MAPPING_FAILED;
    }
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)

    // Add "cpu" controller
    pcgd->pc_cpu = cgroup_add_controller(pcgd->pcg, "cpu");
    if (!pcgd->pc_cpu) {
        logger->Error("PLAT LNX: CGroup resource mapping FAILED "
                "(Error: libcgroup, [cpu] \"controller\" "
                "creation failed)");
        return PLATFORM_MAPPING_FAILED;
    }

#endif

    // Create the kernel-space CGroup
    // NOTE: the current libcg API is quite confuse and unclear
    // regarding the "ignore_ownership" second parameter
    logger->Info("PLAT LNX: Create kernel CGroup [%s]", pcgd->cgpath);
    result = cgroup_create_cgroup(pcgd->pcg, 0);
    if (result && errno) {
        logger->Error("PLAT LNX: CGroup resource mapping FAILED "
                "(Error: libcgroup, kernel cgroup creation "
                "[%d: %s]", errno, strerror(errno));
        return PLATFORM_MAPPING_FAILED;
    }

    return PLATFORM_OK;
}


}   // namespace pp
}   // namespace bbque

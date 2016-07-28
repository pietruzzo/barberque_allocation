#include "bbque/pp/test_platform_proxy.h"

namespace bbque {
namespace pp {


TestPlatformProxy * TestPlatformProxy::GetInstance() {
	static TestPlatformProxy * instance;
	if (instance == nullptr)
		instance = new TestPlatformProxy();
	return instance;
}


TestPlatformProxy::TestPlatformProxy() {
	this->logger = bu::Logger::GetLogger(TEST_PP_NAMESPACE);
}


const char* TestPlatformProxy::GetPlatformID(int16_t system_id) const {
	(void) system_id;
	return "org.test";
}

const char* TestPlatformProxy::GetHardwareID(int16_t system_id) const {
	(void) system_id;
	return "test";
}

TestPlatformProxy::ExitCode_t TestPlatformProxy::Setup(AppPtr_t papp) {
	logger->Info("PLAT TEST: Setup(%s)", papp->StrId());
	return PLATFORM_OK;
}

TestPlatformProxy::ExitCode_t TestPlatformProxy::LoadPlatformData() {
	logger->Info("PLAT TEST: LoadPlatformData()");
	ConfigurationManager &cm(ConfigurationManager::GetInstance());
	ResourceAccounter &ra(ResourceAccounter::GetInstance());

	if (platformLoaded)
		return PLATFORM_OK;

	logger->Warn("Loading TEST platform data");
	logger->Debug("CPUs          : %5d", cm.TPD_CPUCount());
	logger->Debug("CPU memory    : %5d [MB]", cm.TPD_CPUMem());
	logger->Debug("PEs per CPU   : %5d", cm.TPD_PEsCount());
	logger->Debug("System memory : %5d", cm.TPD_SysMem());

	char resourcePath[] = "sys0.cpu256.mem0";
	//                     ........^
	//                        8
	// Registering CPUs, per-CPU memory and processing elements (cores)
	logger->Debug("Registering resources:");
	for (uint8_t c = 0; c < cm.TPD_CPUCount(); ++c) {
		snprintf(resourcePath+8, 8, "%d.mem0", c);
		logger->Debug("  %s", resourcePath);
		ra.RegisterResource(resourcePath, "MB", cm.TPD_CPUMem());

		for (uint8_t p = 0; p < cm.TPD_PEsCount(); ++p) {
			snprintf(resourcePath+8, 8, "%d.pe%d", c, p);
			logger->Debug("  %s", resourcePath);
			ra.RegisterResource(resourcePath, " ", 100);
		}
	}

	// Registering system memory
	char sysMemPath[] = "sys0.mem0";
	logger->Debug("  %s", sysMemPath);
	ra.RegisterResource(sysMemPath, "MB", cm.TPD_SysMem());

	platformLoaded = true;

	return PLATFORM_OK;
}

TestPlatformProxy::ExitCode_t TestPlatformProxy::Refresh() {
	logger->Info("PLAT TEST: Refresh()");
	return PLATFORM_OK;
}

TestPlatformProxy::ExitCode_t TestPlatformProxy::Release(AppPtr_t papp) {
	logger->Info("PLAT TEST: Release(%s)", papp->StrId());
	return PLATFORM_OK;
}

TestPlatformProxy::ExitCode_t TestPlatformProxy::ReclaimResources(AppPtr_t papp)
																		 {
	logger->Info("PLAT TEST: ReclaimResources(%s)", papp->StrId());
	return PLATFORM_OK;
}

TestPlatformProxy::ExitCode_t TestPlatformProxy::MapResources(
		AppPtr_t papp,
		ResourceAssignmentMapPtr_t pres,bool excl) {
	(void) pres;
	(void) excl;
	logger->Info("PLAT TEST: MapResources(%s)", papp->StrId());
	return PLATFORM_OK;
}


}	// namespace pp
}	// namespace bbque

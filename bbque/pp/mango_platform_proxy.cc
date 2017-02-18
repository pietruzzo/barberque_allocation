#include "bbque/pp/mango_platform_proxy.h"
#include "bbque/config.h"
#include "bbque/res/resource_path.h"
#include "bbque/utils/assert.h"
#include "bbque/pp/mango_platform_description.h"

#include "bbque/pp/mango_platform_description.h"

#include <mango-hn/hn.h>

#define BBQUE_MANGOPP_PLATFORM_ID		"org.mango"

namespace bb = bbque;
namespace br = bbque::res;
namespace po = boost::program_options;


namespace bbque {
namespace pp {


MangoPlatformProxy * MangoPlatformProxy::GetInstance() {
	static MangoPlatformProxy * instance;
	if (instance == nullptr)
		instance = new MangoPlatformProxy();
	return instance;
}

MangoPlatformProxy::MangoPlatformProxy() :
	refreshMode(false)
{

	//---------- Get a logger module
	logger = bu::Logger::GetLogger(MANGO_PP_NAMESPACE);
	bbque_assert(logger);

	bbque_assert ( 0 == hn_initialize(MANGO_DEVICE_NAME) ); 

}

MangoPlatformProxy::~MangoPlatformProxy() {

	bbque_assert(0 == hn_end());

}


const char* MangoPlatformProxy::GetPlatformID(int16_t system_id) const noexcept {
	(void) system_id;
	return BBQUE_MANGOPP_PLATFORM_ID;
}

const char* MangoPlatformProxy::GetHardwareID(int16_t system_id) const noexcept {
	(void) system_id;
	return BBQUE_TARGET_HARDWARE_ID;    // Defined in bbque.conf
}

bool MangoPlatformProxy::IsHighPerformance(
		bbque::res::ResourcePathPtr_t const & path) const {
	UNUSED(path);
	return false;
}

MangoPlatformProxy::ExitCode_t MangoPlatformProxy::Setup(AppPtr_t papp) noexcept {
	ExitCode_t result = PLATFORM_OK;

	return result;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::Release(AppPtr_t papp) noexcept {
	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::ReclaimResources(AppPtr_t papp) noexcept {
	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::MapResources(AppPtr_t papp, ResourceAssignmentMapPtr_t pres, bool excl) noexcept {
	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t MangoPlatformProxy::Refresh() noexcept {
	refreshMode = true;
	return PLATFORM_OK;
}


MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::LoadPlatformData() noexcept {

	PlatformDescription &pd = pli->getPlatformInfo();

	struct hn_st_configuration *hn_conf;

	// TODO Get hn_conf from somewhere...

	this->architecture_id = hn_conf->arch_id;

	uint32_t num_tiles;
	if ( 0 != hn_get_num_tiles(&num_tiles) ) {
		logger->Fatal("Unable to get the number of tiles.");
		return PLATFORM_INIT_FAILED;
	}

	logger->Info("Found a total of %d tiles.", num_tiles);

	PlatformDescription::System &sys = pd.GetLocalSystem();

	for ( uint32_t i=0; i < num_tiles; i++) {
		struct hn_st_tile_info tile_info;
		int err = hn_get_tile_info(i, &tile_info);
		if ( 0 != err) {
			logger->Fatal("Unable to get the tile nr.%d [error=%d].", i, err);
			return PLATFORM_INIT_FAILED;
		}

		MangoTile mt((MangoTile::MangoTileType_t)tile_info.tile_type);

		for (int i=0; i < MangoTile::GetCoreNr(mt.GetType()); i++) {
			typedef PlatformDescription pd_t;
			pd_t::ProcessingElement pe(i , 0, 100, pd_t::PartitionType_t::MDEV);
			mt.AddProcessingElement(pe);
		}

		sys.AddAccelerator(mt);

	}

	return PLATFORM_OK;
}


}	// namespace pp
}	// namespace bbque

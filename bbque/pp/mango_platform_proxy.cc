#include "bbque/pp/mango_platform_proxy.h"
#include "bbque/config.h"
#include "bbque/res/resource_path.h"
#include "bbque/utils/assert.h"
#include "bbque/pp/mango_platform_description.h"
#include "bbque/resource_mapping_validator.h"

#include <mango-hn/hn.h>

#define BBQUE_MANGOPP_PLATFORM_ID		"org.mango"
#define MANGO_MEMORY_COUNT_START 10
#define MANGO_PEAKOS_FILE_SIZE 256*1024*1024

namespace bb = bbque;
namespace br = bbque::res;
namespace po = boost::program_options;

typedef hn_st_req hn_st_request_t;
typedef hn_st_response hn_st_response_t;

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


	// We prepare the filter to enable access to registers and statistics
	hn_daemon_socket_filter filter;
	filter.target = TARGET_MANGO;
	filter.mode = APPL_MODE_SYNC_READS;
	filter.tile = 999;
	filter.core = 999;

	int hn_init_err = hn_initialize(filter, UPV_PARTITION_STRATEGY, 1);	// TODO UPV_PARTITION_STRATEGY?

	bbque_assert ( 0 == hn_init_err );

	logger->Debug("Resetting MANGO platform...");

	hn_reset(0);

	logger->Debug("HN Library initialized");

	ResourceMappingValidator &rmv = ResourceMappingValidator::GetInstance();
	rmv.RegisterSkimmer(std::make_shared<MangoPartitionSkimmer>() , 100);

}

MangoPlatformProxy::~MangoPlatformProxy() {

	bbque_assert(0 == hn_end());

}


const char* MangoPlatformProxy::GetPlatformID(int16_t system_id) const noexcept {
	UNUSED(system_id);
	return BBQUE_MANGOPP_PLATFORM_ID;
}

const char* MangoPlatformProxy::GetHardwareID(int16_t system_id) const noexcept {
	UNUSED(system_id);
	return BBQUE_TARGET_HARDWARE_ID;    // Defined in bbque.conf
}

bool MangoPlatformProxy::IsHighPerformance(
		bbque::res::ResourcePathPtr_t const & path) const {
	UNUSED(path);
	return false;
}

MangoPlatformProxy::ExitCode_t MangoPlatformProxy::Setup(AppPtr_t papp) noexcept {
	ExitCode_t result = PLATFORM_OK;
	UNUSED(papp);
	return result;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::Release(AppPtr_t papp) noexcept {

	UNUSED(papp);
	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::ReclaimResources(AppPtr_t papp) noexcept {

	UNUSED(papp);
	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::MapResources(AppPtr_t papp, ResourceAssignmentMapPtr_t pres, bool excl) noexcept {

	UNUSED(papp);
	UNUSED(pres);
	UNUSED(excl);

	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t MangoPlatformProxy::Refresh() noexcept {
	refreshMode = true;

	return PLATFORM_OK;
}


MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::LoadPlatformData() noexcept {

	int err;

	// Get the number of tiles
	err = hn_get_num_tiles(&this->num_tiles, &this->num_tiles_x, &this->num_tiles_y);

	if ( HN_SUCCEEDED != err ) {
		logger->Fatal("Unable to get the number of tiles [err=%d]", err);
		return PLATFORM_INIT_FAILED;
	}

	// Get the number of VNs
	err = hn_get_num_vns(&this->num_vns);
	if ( HN_SUCCEEDED != err ) {
		logger->Fatal("Unable to get the number of VNs [err=%d]", err);
		return PLATFORM_INIT_FAILED;
	}

	logger->Info("Found a total of %d tiles (%dx%d) and %d VNs.", this->num_tiles,
			this->num_tiles_x, this->num_tiles_y, this->num_vns);


	ExitCode_t pp_err;

	// Now we have to register the tiles to the PlatformDescription
	pp_err = RegisterTiles();
	if (PLATFORM_OK != pp_err) {
		return pp_err;
	}

	pp_err = BootTiles();
	if (PLATFORM_OK != pp_err) {
		return pp_err;
	}

	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::BootTiles() noexcept {
	for (unsigned i=0; i < num_tiles; i++) {

		uint32 req_size = MANGO_PEAKOS_FILE_SIZE;
		uint32 tile_memory;
		uint32 base_addr;

		int err = hn_find_memory(i, req_size, &tile_memory, &base_addr);

		if (HN_SUCCEEDED != err) {
			logger->Error("Unable to get memory for tile nr=%d", i);
			return PLATFORM_LOADING_FAILED;
		}

		logger->Info("Booting Tile nr=%d", i);
		err = hn_boot_unit(i, tile_memory, base_addr);

		if (HN_SUCCEEDED != err) {
			logger->Error("Unable to boot Tile nr=%d", i);
			return PLATFORM_LOADING_FAILED;
		}
	}

	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::RegisterTiles() noexcept {
	typedef PlatformDescription pd_t;	// Just for convenience

	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	pd_t &pd = pli->getPlatformInfo();
	pd_t::System &sys = pd.GetLocalSystem();



	for (uint_fast32_t i=0; i < num_tiles; i++) {
		hn_st_tile_info tile_info;
		int err = hn_get_tile_info(i, &tile_info);
		if (HN_SUCCEEDED != err) {
			logger->Fatal("Unable to get the tile nr.%d [error=%d].", i, err);
			return PLATFORM_INIT_FAILED;
		}

		MangoTile mt(i, (MangoTile::MangoTileType_t)(tile_info.tile_type));
		sys.AddAccelerator(mt);
		mt.SetPrefix(sys.GetPath());

		for (int i=0; i < MangoTile::GetCoreNr(mt.GetType()); i++) {
			pd_t::ProcessingElement pe(i , 0, 100, pd_t::PartitionType_t::MDEV);
			pe.SetPrefix(mt.GetPath());
			mt.AddProcessingElement(pe);

			ra.RegisterResource(pe.GetPath(), "", 100);
		}

		// Let now register the memories. Unfortunately, memories are not easy to be
		// retrieved, we have to iterate over all tiles and search memories
		unsigned int mem_attached = 1; // TODO BUG tile_info.memory_attached;
		if (mem_attached != 0) {
			// Register the memory attached if present and if not already registered
			ExitCode_t reg_err = RegisterMemoryBank(i, mem_attached);
			if (reg_err) return reg_err;
			auto mem = sys.GetMemoryById(mem_attached);
			mt.SetMemory(mem);
		}
	}


	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::RegisterMemoryBank(int tile_id, int mem_id) noexcept {
	typedef PlatformDescription pd_t;	// Just for convenience
	ResourceAccounter &ra(ResourceAccounter::GetInstance());

	bbque_assert(mem_id > 0);

	logger->Debug("Registering tile %d, memory %d", tile_id, mem_id);

	if (found_memory_banks.test(mem_id)) {	// Already registered
		return PLATFORM_OK;
	}

	found_memory_banks.set(mem_id);
	uint32_t memory_size;
	int err = hn_get_memory_size(tile_id, &memory_size);
	if (HN_SUCCEEDED != err) {
		logger->Fatal("Unable to get memory information of tile nr.%d, memory %d [error=%d].",
			      tile_id, mem_id, err);
		return PLATFORM_INIT_FAILED;
	}

	pd_t &pd = pli->getPlatformInfo();
	pd_t::System &sys = pd.GetLocalSystem();

	pd_t::Memory mem(MANGO_MEMORY_COUNT_START + mem_id, memory_size);
	mem.SetPrefix(sys.GetPath());

	sys.AddMemory(std::make_shared<pd_t::Memory>(mem));
	ra.RegisterResource(mem.GetPath(), "", memory_size);

	logger->Info("Registered memory id=%d size=%d", tile_id, memory_size);

	return PLATFORM_OK;
}
static uint32 ArchTypeToMangoType(ArchType type, unsigned int nr_thread) {

	switch (type) {
		case ArchType::PEAK:
			if (nr_thread <= 2) return HN_PEAK_TYPE_0;
			if (nr_thread <= 4) return HN_PEAK_TYPE_1;
			return HN_PEAK_TYPE_2;
		break;
		default:
			throw std::runtime_error("Unsupported architecture");
	}

}

static hn_st_request_t FillReq(const TaskGraph &tg) {

	ApplicationManager &am = ApplicationManager::GetInstance();

	hn_st_request_t req;
	req.num_comp_rsc    = tg.TaskCount();

	// We request one more buffer to write the kernel image
	req.num_mem_buffers = tg.BufferCount() + tg.TaskCount();

	// We need the application to get the application bandwidth requirements
	AppPtr_t app = am.GetApplication(tg.GetApplicationId());
	
	unsigned int i=0;
	for (auto t : tg.Tasks()) {
		req.comp_rsc_types[i++] = ArchTypeToMangoType(t.second->GetAssignedArch(), 
							      t.second->GetThreadCount());
	}

	i=0;
	for (auto b : tg.Buffers()) {
		bbque_assert(i < req.num_mem_buffers);
		req.mem_buffers_size[i++] = b.second->Size();
	}


	i=0;
	for (auto t : tg.Tasks()) {
		auto tg_req = app->GetTaskRequirements(t.second->Id());
		unsigned int j=0;
		for (auto b : tg.Buffers()) {
			auto first = t.second->InputBuffers().cbegin();
			auto last = t.second->InputBuffers().cend();

			if ( std::find(first, last, b.second->Id()) != last) {
				req.bw_read_req[i][j]  = 1; // tg_req.InBandwidth();
			} else {
				req.bw_read_req[i][j]  = 0;
			}

			first = t.second->OutputBuffers().cbegin();
			last = t.second->OutputBuffers().cend();

			if ( std::find(first, last, b.second->Id()) != last) {
				req.bw_write_req[i][j]  = 1; // tg_req.OutBandwidth();
			} else {
				req.bw_write_req[i][j]  = 0;
			}



			j++;
		}

		// Extra buffer for kernel and stack memory
		auto arch =  t.second->GetAssignedArch();
		auto ksize = t.second->Targets()[arch]->BinarySize();
		auto ssize = t.second->Targets()[arch]->StackSize();
		req.mem_buffers_size[i] = ksize + ssize;
		req.bw_read_req[i] [j+i]  = 1;
		req.bw_write_req[i][j+i]  = 1;

		i++;
	}

	// TODO Requirements bandwidth

	return req;
}

static Partition GetPartition(const TaskGraph &tg, hn_st_response_t *res,
							  int partition_id) {

	auto it_task  = tg.Tasks().begin();
	int tasks_size = tg.Tasks().size();
	auto it_buff  = tg.Buffers().begin();
	int buff_size = tg.Buffers().size();

	Partition part(partition_id);

	for (int j=0; j < tasks_size; j++) {
		part.MapTask(it_task->second, res->comp_rsc_tiles[j], res->mem_buffers_addr[buff_size + j]);
		it_task++;
	}
	bbque_assert(it_task == tg.Tasks().end());

	for (int j=0; j < buff_size; j++) {
		part.MapBuffer(it_buff->second, res->mem_buffers_tiles[j], res->mem_buffers_addr[j]);
		it_buff++;
	}
	bbque_assert(it_buff == tg.Buffers().end());



	return part;
}

MangoPlatformProxy::MangoPartitionSkimmer::ExitCode_t
MangoPlatformProxy::MangoPartitionSkimmer::SetAddresses(const TaskGraph &tg, const Partition &partition) noexcept {

	for ( auto buffer : tg.Buffers()) {
		buffer.second->SetMemoryBank(partition.GetMemoryBank(buffer.second));
		buffer.second->SetPhysicalAddress(partition.GetBufferAddress(buffer.second));
	}

	for ( auto task : tg.Tasks()) {
		auto arch = task.second->GetAssignedArch();
		task.second->Targets()[arch]->SetAddress(partition.GetKernelAddress(task.second));
	}

	return SK_OK;
}

MangoPlatformProxy::MangoPartitionSkimmer::ExitCode_t
MangoPlatformProxy::MangoPartitionSkimmer::Skim(const TaskGraph &tg,
							std::list<Partition>&part_list) noexcept {
	hn_st_request_t req;
	hn_st_response_t res[MANGO_BASE_NUM_PARTITIONS];
	uint32_t ids[MANGO_BASE_NUM_PARTITIONS];		// TODO multiple partition
	uint32_t num_parts = 1; //MANGO_BASE_NUM_PARTITIONS;
	uint32_t part_id;
	
	auto logger = bu::Logger::GetLogger(MANGO_PP_NAMESPACE ".skm");

	bbque_assert(part_list.empty());

	try {
		req = FillReq(tg);
	}
	catch (const std::runtime_error &err) {
		logger->Error("MangoPartitionSkimmer: %s", err.what());
		return SK_NO_PARTITION;
	}

	if ( (hn_find_partition(req, res, &part_id)) ) {
		return SK_GENERIC_ERROR;
	}

	ids[0] = part_id;

	// No feasible partition found
	if ( num_parts == 0 ) {
		return SK_NO_PARTITION;
	}

	// Fill the partition vector with the partitions returned by HN library
	for (unsigned int i=0; i < num_parts; i++) {
		Partition part = GetPartition(tg, &res[i], ids[i]);
		part_list.push_back(part);
	}


	return SK_OK;

}

MangoPlatformProxy::MangoPartitionSkimmer::ExitCode_t
MangoPlatformProxy::MangoPartitionSkimmer::SetPartition(const TaskGraph &tg,
					  	        const Partition &partition) noexcept {

	UNUSED(tg);


	ExitCode_t err = SetAddresses(tg, partition);

// TODO
/*	uint32_t part_id = partition.GetPartitionId();
	uint32_t ret = hn_allocate_partition(part_id);
	
	if ( HN_SUCCEEDED != ret ) {
		return SK_GENERIC_ERROR;
	}
*/

	return err;
}

MangoPlatformProxy::MangoPartitionSkimmer::ExitCode_t
MangoPlatformProxy::MangoPartitionSkimmer::UnsetPartition(const TaskGraph &tg,
					  	          const Partition &partition) noexcept {

	UNUSED(tg);

	uint32_t part_id = partition.GetPartitionId();
	uint32_t ret;
	ret = hn_delete_partition(part_id);

	if ( HN_SUCCEEDED != ret ) {
		return SK_GENERIC_ERROR;
	}

	ret = hn_delete_partition(part_id);

	if ( HN_SUCCEEDED != ret ) {
		return SK_GENERIC_ERROR;
	}

	return SK_OK;
}

}	// namespace pp
}	// namespace bbque

#include "bbque/pp/mango_platform_proxy.h"
#include "bbque/config.h"
#include "bbque/res/resource_path.h"
#include "bbque/utils/assert.h"
#include "bbque/pp/mango_platform_description.h"
#include "bbque/resource_partition_validator.h"

#include <libhn/hn.h>

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


	// In order to initialize the communication with the HN library we have to
	// prepare the filter to enable access to registers and statistics
	// TODO: this was a copy-and-paste of the example, we have to ask to UPV
	//       how to properly tune these parameters, in particular the meaning of
	//	 UPV_PARTITION_STRATEGY
	hn_daemon_socket_filter filter;
	filter.target = TARGET_MANGO;
	filter.mode = APPL_MODE_SYNC_READS;
	filter.tile = 999;
	filter.core = 999;

	logger->Debug("Initializing communication with MANGO platform...");

	int hn_init_err = hn_initialize(filter, UPV_PARTITION_STRATEGY, 1, 0, 0);

	bbque_assert ( 0 == hn_init_err );

	logger->Debug("Resetting MANGO platform...");

	// We have now to reset the platform
	// This function call may take several seconds to conclude
	hn_reset(0);

	logger->Info("HN Library successfully initialized");

	// Register our skimmer for the incoming partitions (it actually fills the partition list,
	// not skim it)
	// Priority of 100 means the maximum priority, this is the first skimmer to be executed
	ResourcePartitionValidator &rmv = ResourcePartitionValidator::GetInstance();
	rmv.RegisterSkimmer(std::make_shared<MangoPartitionSkimmer>() , 100);

}
 
MangoPlatformProxy::~MangoPlatformProxy() {

	// Just clean up stuffs...
	int hn_err_ret = hn_end();
	bbque_assert(0 == hn_err_ret);

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

	// This method is empty and this is unusual in Barbeque, however in Mango we use Skimmers
	// to get the list of partitions and set them

	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t MangoPlatformProxy::Refresh() noexcept {
	refreshMode = true;

	// TODO (we really need this method?)

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

	// Now we have to register the tiles to the PlatformDescription and ResourceAccounter
	pp_err = RegisterTiles();
	if (PLATFORM_OK != pp_err) {
		return pp_err;
	}

	// The tiles should now be booted by Barbeque using the PeakOS image
	pp_err = BootTiles();
	if (PLATFORM_OK != pp_err) {
		return pp_err;
	}

	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::BootTiles_PEAK(int tile) noexcept {

		uint32_t req_size = MANGO_PEAKOS_FILE_SIZE;
		uint32_t tile_memory;	// This will be filled with the memory id
		uint32_t base_addr;	// This is the starting address selected

		// TODO: This is currently managed by the internal HN find memory, however, we have
		//	 to replace this with an hook to the MemoryManager here.

		int err = hn_find_memory(tile, req_size, &tile_memory, &base_addr);

		if (HN_SUCCEEDED != err) {
			logger->Error("BootTiles_PEAK: Unable to get memory for tile nr=%d", tile);
			return PLATFORM_LOADING_FAILED;
		}

		logger->Debug("Loading PEAK OS in memory bank %d [address=%x]", tile_memory, base_addr);

		logger->Debug("Booting PEAK tile nr=%d [PEAK_OS:%s] [PEAK_PROT:%s]", tile, MANGO_PEAK_OS, MANGO_PEAK_PROTOCOL);
		err = hn_boot_unit(tile, tile_memory, base_addr, MANGO_PEAK_PROTOCOL, MANGO_PEAK_OS);

		if (HN_SUCCEEDED != err) {
			logger->Error("Unable to boot PEAK tile nr=%d", tile);
			return PLATFORM_LOADING_FAILED;
		}
		return PLATFORM_OK;
} 

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::BootTiles() noexcept {

	hn_st_tile_info tile_info;
	for (uint_fast32_t i=0; i < num_tiles; i++) {

		int err = hn_get_tile_info(i, &tile_info);
		if (HN_SUCCEEDED != err) {
			logger->Fatal("Unable to get the tile nr.%d [error=%d].", i, err);
			return PLATFORM_INIT_FAILED;
		}

		if (tile_info.tile_type == HN_PEAK_TYPE_1 || tile_info.tile_type == HN_PEAK_TYPE_2) {

			err = BootTiles_PEAK(i);
			if (PLATFORM_OK != err) {
				logger->Error("Unable to boot tile nr=%d", i);
				return PLATFORM_INIT_FAILED;
			}
		}
	}

	logger->Info("All tiles successfully booted.");

	return PLATFORM_OK;
}

MangoPlatformProxy::ExitCode_t
MangoPlatformProxy::RegisterTiles() noexcept {
	typedef PlatformDescription pd_t;	// Just for convenience

	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	pd_t &pd = pli->getPlatformInfo();
	pd_t::System &sys = pd.GetLocalSystem();



	for (uint_fast32_t i=0; i < num_tiles; i++) {

		// First of all get the information of tiles from HN library
		hn_st_tile_info tile_info;
		int err = hn_get_tile_info(i, &tile_info);
		if (HN_SUCCEEDED != err) {
			logger->Fatal("Unable to get the tile nr.%d [error=%d].", i, err);
			return PLATFORM_INIT_FAILED;
		}

		logger->Info("Loading tile %d of type %d...", i, tile_info.tile_type);

		MangoTile mt(i, (MangoTile::MangoTileType_t)(tile_info.tile_type));
		sys.AddAccelerator(mt);
		mt.SetPrefix(sys.GetPath());

		// For each tile, we register how many PE how may cores the accelerator has, this
		// will simplify the tracking of ResourceAccounter resources
		for (int i=0; i < MangoTile::GetCoreNr(mt.GetType()); i++) {
			pd_t::ProcessingElement pe(i , 0, 100, pd_t::PartitionType_t::MDEV);
			pe.SetPrefix(mt.GetPath());
			mt.AddProcessingElement(pe);

			ra.RegisterResource(pe.GetPath(), "", 100);
		}

		// Let now register the memories. Unfortunately, memories are not easy to be
		// retrieved, we have to iterate over all tiles and search memories

		// FIXME: the tile_info.memory_attached field is bugged and it does not
		//        returns the correct number, ask UPV to fix it
		unsigned int mem_attached = 1; // should be tile_info.memory_attached;
		if (mem_attached != 0) {
			// Register the memory attached if present and if not already registered
			ExitCode_t reg_err = RegisterMemoryBank(i, mem_attached);
			if (reg_err) {
				return reg_err;
			}

			// And now assign in PD the correct memory pointer
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

	logger->Debug("Registering tile %d, memory %d", tile_id, mem_id);

	bbque_assert(mem_id > 0 && mem_id < MANGO_MAX_MEMORIES);

	if (found_memory_banks.test(mem_id)) {
		// We have already registered this memory, nothing to do
		return PLATFORM_OK;
	}

	// A bitset is used to keep track the already registered memory (we could also use the
	// ResourceAccounter methods, but this is faster)
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

	// In order to distinguish the system memory with the accelerator memory we start counting
	// the memory from MANGO_MEMORY_COUNT_START (10).
	// TODO: this is very ugly

	pd_t::Memory mem(MANGO_MEMORY_COUNT_START + mem_id, memory_size);
	mem.SetPrefix(sys.GetPath());

	sys.AddMemory(std::make_shared<pd_t::Memory>(mem));
	ra.RegisterResource(mem.GetPath(), "", memory_size);

	logger->Info("Registered memory id=%d size=%d", tile_id, memory_size);

	return PLATFORM_OK;
}


/* It follows some static local function not member of any class: we selected this approach in order
   to avoid to add the hn.h in the header file mango_platform_proxy.h. This was done in order to
   avoid the pollution of the global namespace of Barbeque with all the mess of the HN library.
   This is definetely not elegant, so maybe we want to FIXME this in the future. */

/**
 * @brief From the TG we have the information of threads number and the architecture type, but
 *	  we have to pass in aggregated form to the HN library, so this function will convert
 *	  to HN library type
 */
static uint32_t ArchTypeToMangoType(ArchType type, unsigned int nr_thread) {

	UNUSED(nr_thread);

	switch (type) {
		case ArchType::PEAK:
			// TODO Selection of the subtype of architecture (who has to do this?
			// the policy?)
			return HN_PEAK_TYPE_1;	// Fix this, should go to policy
		break;
		case ArchType::GN:
			return HN_PEAK_TYPE_1;	// In GN emulation case we are not interested in the real time
		break;


		// TODO add other architectures
		default:
			throw std::runtime_error("Unsupported architecture");
	}

}

/**
 * @brief This function basically converts a TaskGraph object in the request struct of HN library
 */
static hn_st_request_t FillReq(const TaskGraph &tg) {

	ApplicationManager &am = ApplicationManager::GetInstance();

	hn_st_request_t req;
	req.num_comp_rsc    = tg.TaskCount();

	// We request the number of shared buffer + the number of task, since we have to load also
	// the kernels in memory
	req.num_mem_buffers = tg.BufferCount() + tg.TaskCount();

	// We need the application to get the application bandwidth requirements
	AppPtr_t app = am.GetApplication(tg.GetApplicationId());
	
	unsigned int i;


	// Fill the computing resources requested
	i=0;
	for (auto t : tg.Tasks()) {
		req.comp_rsc_types[i++] = ArchTypeToMangoType(t.second->GetAssignedArch(), 
							      t.second->GetThreadCount());
	}

	// Fill the buffer requested (note: we will add other buffers next)
	i=0;
	for (auto b : tg.Buffers()) {
		bbque_assert(i < req.num_mem_buffers);
		req.mem_buffers_size[i++] = b.second->Size();
	}

	unsigned int filled_buffers = i;

	// We now register the mapping task-buffer and we allocate a per-task buffer to allocate the
	// kernel image.
	// The bandwitdth is not currently managed by the HN library, so we use only a boolean value
	// to indicate that the buffer uses that task and vice versa.
	i=0;
	for (auto t : tg.Tasks()) {
		auto tg_req = app->GetTaskRequirements(t.second->Id());
		unsigned int j=0;
		for (auto b : tg.Buffers()) {
			auto first = t.second->InputBuffers().cbegin();
			auto last = t.second->InputBuffers().cend();

			if ( std::find(first, last, b.second->Id()) != last) {
				req.bw_read_req[i][j]  = 1; // TODO tg_req.InBandwidth();
			} else {
				req.bw_read_req[i][j]  = 0;
			}

			first = t.second->OutputBuffers().cbegin();
			last = t.second->OutputBuffers().cend();

			if ( std::find(first, last, b.second->Id()) != last) {
				req.bw_write_req[i][j]  = 1; // TODO  tg_req.OutBandwidth();
			} else {
				req.bw_write_req[i][j]  = 0;
			}

			j++;
		}

		// Extra buffer for kernel and stack memory
		auto arch =  t.second->GetAssignedArch();
		auto ksize = t.second->Targets()[arch]->BinarySize();
		auto ssize = t.second->Targets()[arch]->StackSize();
		int kimage_index = filled_buffers + i;
		req.mem_buffers_size[kimage_index] = ksize + ssize;
		req.bw_read_req[kimage_index] [j+i]  = 1;	// TODO
		req.bw_write_req[kimage_index][j+i]  = 1;	// TODO

		i++;
	}

	return req;
}

/**
 * @brief This function convert a TG + HN response to a Partition object used in Barbeque
 */
static Partition GetPartition(const TaskGraph &tg, hn_st_response_t *res, int partition_id) noexcept {

	auto it_task  = tg.Tasks().begin();
	int tasks_size = tg.Tasks().size();
	auto it_buff  = tg.Buffers().begin();
	int buff_size = tg.Buffers().size();

	Partition part(partition_id);

	for (int j=0; j < tasks_size; j++) {
		part.MapTask(it_task->second, res->comp_rsc_tiles[j],
			res->mem_buffers_tiles[buff_size +j], res->mem_buffers_addr[buff_size + j]);
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

		uint32_t memory_bank = partition.GetMemoryBank(buffer.second);
		uint32_t phy_addr    = partition.GetBufferAddress(buffer.second);
		logger->Debug("Buffer %d is located at bank %d [address=0x%x]",
				buffer.second->Id(), memory_bank, phy_addr);
		buffer.second->SetMemoryBank(memory_bank);
		buffer.second->SetPhysicalAddress(phy_addr);
	}

	for ( auto task : tg.Tasks()) {
		auto arch = task.second->GetAssignedArch();
		uint32_t phy_addr    = partition.GetKernelAddress(task.second);
		uint32_t mem_tile    = partition.GetKernelBank(task.second);

		logger->Debug("Task %d allocated space for kernel %s [address=0x%x]",
				task.second->Id(), GetStringFromArchType(arch), phy_addr);
		task.second->Targets()[arch]->SetMemoryBank(mem_tile);
		task.second->Targets()[arch]->SetAddress(phy_addr);
	}

	return SK_OK;
}

MangoPlatformProxy::MangoPartitionSkimmer::ExitCode_t
MangoPlatformProxy::MangoPartitionSkimmer::Skim(const TaskGraph &tg,
							std::list<Partition>&part_list) noexcept {
	hn_st_request_t req;
	hn_st_response_t res[MANGO_BASE_NUM_PARTITIONS];
	uint32_t ids[MANGO_BASE_NUM_PARTITIONS];		// TODO multiple partition
	uint32_t num_parts = 1;					// TODO multiple partition;
	uint32_t part_id;

	bbque_assert(part_list.empty());

	try {
		// Convert TG to a hn_st_request_t
		// It may generate exception if the architecture is not supported
		// (in that case we should not be here ;-))
		logger->Debug("Converting TG into HN struct...");
		req = FillReq(tg);
	}
	catch (const std::runtime_error &err) {
		logger->Error("MangoPartitionSkimmer: %s", err.what());
		return SK_NO_PARTITION;
	}

	logger->Debug("Request summary:");
	for (unsigned int i=0; i < req.num_comp_rsc; i++) {
		logger->Debug("  -> Computing Resource %d, HN type %d", i, req.comp_rsc_types[i]);
	}
	for (unsigned int i=0; i < req.num_mem_buffers; i++) {
		logger->Debug("  -> Memory buffer %d, size %d", i, req.mem_buffers_size[i]);
	}

	int err;
	// TODO: Change to multiple-partitions version of this function
	if ( (err = hn_find_partitions(req, res, &part_id, 1, &num_parts)) ) {
		logger->Error("hn_find_partitions FAILED with error %d", err);
		return SK_GENERIC_ERROR;
	}

	ids[0] = part_id;	// Just an hack for the single find_partition version

	logger->Debug("Found %d partitions", num_parts);

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
MangoPlatformProxy::MangoPartitionSkimmer::SetPartition(TaskGraph &tg,
					  	        const Partition &partition) noexcept {

	UNUSED(tg);

	// We set the mapping of buffers-addresses based on the selected partition
	ExitCode_t err = SetAddresses(tg, partition);


	for ( auto task : tg.Tasks()) {
		task.second->SetMappedProcessor( partition.GetUnit(task.second) );
	}

	// TODO: with the find_partitions we should allocate the selected partition, but
	//	 unfortunately this is not currently supported by HN library
/*	uint32_t part_id = partition.GetId();
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

	uint32_t part_id = partition.GetId();
	uint32_t ret;

	logger->Debug("Deallocating partition [id=%d]...", part_id);

/* TODO: no more supported by HN library, we have to check if in the future we need this or not

	ret = hn_deallocate_partition(part_id);

	if ( HN_SUCCEEDED != ret ) {
		return SK_GENERIC_ERROR;
	}
*/
	ret = hn_delete_partition(part_id);

	if ( HN_SUCCEEDED != ret ) {
		return SK_GENERIC_ERROR;
	}

	return SK_OK;
}

MangoPlatformProxy::MangoPartitionSkimmer::MangoPartitionSkimmer() : PartitionSkimmer(SKT_MANGO_HN)
{
	logger = bu::Logger::GetLogger(MANGO_PP_NAMESPACE ".skm");
	bbque_assert(logger);
}

}	// namespace pp
}	// namespace bbque

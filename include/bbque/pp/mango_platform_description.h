#ifndef BBQUE_MANGO_PLATFORM_DESCRIPTION_H_
#define BBQUE_MANGO_PLATFORM_DESCRIPTION_H_

#include <chrono>
#include <cstdint>
#include <vector>

#include "bbque/pp/platform_description.h"

namespace bbque {
namespace pp {

class MangoTile {

public:

	typedef enum MangoTileType_e {
		HN_PEAK_0_TYPE,
		HN_PEAK_1_TYPE,
		HN_PEAK_2_TYPE,
		HN_NUPLUS_0_TYPE,
		HN_NUPLUS_1_TYPE,
		HN_ARM,
		HN_HWACC_DCT
	} MangoTileType_t;

	typedef struct MangoTileStats_s {
		uint_fast32_t unit_utilization;
		uint_fast32_t flits_ejected_local;
		uint_fast32_t flits_ejected_north;
		uint_fast32_t flits_ejected_east;
		uint_fast32_t flits_ejected_west;
		uint_fast32_t flits_ejected_south;
	} MangoTileStats_t;


private:

	MangoTileType_t type;

	std::chrono::time_point<std::chrono::system_clock> last_stats_update;
	MangoTileStats_t stats;

	int nr_core;

	std::chrono::time_point<std::chrono::system_clock> last_temp_update;
	uint_fast32_t temperature;
	

};

class MangoVN {

};

/**
 * @class MangoPlatformDescription
 *
 * @brief A MangoPlatformDescription object includes the description of the
 * underlying platform of the server and of the Mango HN node.
 *
 * @warning Non thread safe.
 */
class MangoPlatformDescription : public PlatformDescription {

public:
	


private:
	std::vector<MangoTile> tiles;
	std::vector<MangoVN> vns;


}; // class PlatformDescription

}   // namespace pp
}   // namespace bbque

#endif // BBQUE_PLATFORM_DESCRIPTION_H_

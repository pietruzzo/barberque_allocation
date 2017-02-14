#ifndef BBQUE_MANGO_PLATFORM_DESCRIPTION_H_
#define BBQUE_MANGO_PLATFORM_DESCRIPTION_H_

#include <chrono>
#include <cstdint>
#include <vector>
#include <boost/thread/shared_mutex.hpp>

#include "bbque/pp/platform_description.h"

namespace bbque {
namespace pp {

class MangoTile : public PlatformDescription::ProcessingElement {

public:
	/**
	 * @brief This enumeration represents the tile type in the Mango platform.
	 * This values must agree with the Mango specification and low-level
	 * runtime
	 */
	typedef enum MangoTileType_e {
		HN_PEAK_0_TYPE,
		HN_PEAK_1_TYPE,
		HN_PEAK_2_TYPE,
		HN_NUPLUS_0_TYPE,
		HN_NUPLUS_1_TYPE,
		HN_ARM,
		HN_HWACC_DCT
	} MangoTileType_t;


	/**
	 * @brief Constructor for a MangoTile object.
	 */
	MangoTile(MangoTileType_t type) : type(type) {}

	/**
	 * @brief Returns the type of this tile
	 */
	inline MangoTileType_t GetType() const {
		return this->type;
	}

	/**
	 * @brief Returns the type of this tile
	 */
	inline void SetType(MangoTileType_t type) {
		this->type = type;
	}

private:
	MangoTileType_t type;

};


}   // namespace pp
}   // namespace bbque

#endif // BBQUE_PLATFORM_DESCRIPTION_H_

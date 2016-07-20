/*
 * Copyright (C) 2015  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BBQUE_RESOURCE_BITSET_H_
#define BBQUE_RESOURCE_BITSET_H_

#include <bitset>

#include "bbque/res/identifier.h"

namespace bbque { namespace res {

/**
 * @class ResourceBitset
 *
 * This is a utility class, providing most of the common features of the
 * std::bitset class, plus some facilities to support an efficient access to
 * the information.
 * This is commonly exploited to keep track of the IDs of a specific resource
 * type, from a set of resource assignments or resource descriptors.
 */
class ResourceBitset {

public:

	enum ExitCode_t {
		OK       = 0,
		OUT_OF_RANGE
	};

	ResourceBitset();

	~ResourceBitset();

	ExitCode_t Set(BBQUE_RID_TYPE pos);

	ExitCode_t Reset();

	ExitCode_t Reset(BBQUE_RID_TYPE pos);

	inline bool Test(BBQUE_RID_TYPE pos) const {
		return bit_set.test(pos);
	}

	inline BBQUE_RID_TYPE Count() const {
		return count;
	}

	inline BBQUE_RID_TYPE FirstSet() const {
		return first_set;
	}

	inline BBQUE_RID_TYPE LastSet() const {
		return last_set;
	}

	inline std::string ToString() const {
		return bit_set.to_string();
	}

	inline std::string const & ToStringCG() const {
		return cg_str;
	}

	inline unsigned long ToULong() const {
		return bit_set.to_ulong();
	}

	/*****************************************************************
	 *                        Operators                              *
	 *****************************************************************/

	bool operator== (ResourceBitset const & rbs) {
		return bit_set == rbs.bit_set;
	}

	bool operator!= (ResourceBitset const & rbs) {
		return bit_set != rbs.bit_set;
	}

	bool operator[] (BBQUE_RID_TYPE pos) {
		return bit_set[pos];
	}

	ResourceBitset operator|= (const ResourceBitset & rbs);

	ResourceBitset operator&= (const ResourceBitset & rbs);

private:

	std::bitset<BBQUE_MAX_R_ID_NUM> bit_set;

	BBQUE_RID_TYPE first_set;

	BBQUE_RID_TYPE last_set;

	BBQUE_RID_TYPE count;

	bool none;

	std::string cg_str;
};

} // namespace res

} // namespace bbque

#endif // BBQUE_RESOURCE_BITSET_H_


/*
 * Copyright (C) 2017  Politecnico di Milano
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

#ifndef BBQUE_RESOURCE_MAPPING_VALIDATOR_H_
#define BBQUE_RESOURCE_MAPPING_VALIDATOR_H_

#include <functional>
#include <map>
#include <memory>

#include "bbque/utils/logging/logger.h"
#include "bbque/tg/partition.h"

namespace bu = bbque::utils;

namespace bbque {



/**
 * @class ResourceMappingValidator
 *
 * @brief Provides the list of available partitions, validating them across the registered callback
 *	  function.
 */
class ResourceMappingValidator {

public:

	typedef enum ExitCode_t {
		PMV_OK,
		PMV_GENERIC_ERROR,
		PMV_NO_PARTITION,
		PMV_SKIMMER_FAIL
	} ExitCode_t;

	typedef enum SkimmerType_t {
		SKT_NONE,
		SKT_MANGO_HN,
		SKT_MANGO_MEMORY_MANAGER,
		SKT_MANGO_POWER_MANAGER
	} SkimmerType_t;

	typedef std::function<int(std::list<Partition>&)> SkimmerFun_t;
	typedef std::pair<SkimmerFun_t, SkimmerType_t> SkimmerPairType_t;

	static ResourceMappingValidator & GetInstance();


	virtual ~ResourceMappingValidator()  {};

	/**
	 * @brief Load the feasible partitions according to registered callbacks. 
	 *
	 */
	ExitCode_t LoadPartitions(std::list<Partition> &partitions);


	void RegisterSkimmer(SkimmerFun_t skimmer, int priority, SkimmerType_t type) noexcept;

	/**
	 * @brief If LoadPartitions failed with `PMV_NO_PARTITION` or `PMV_SKIMMER_FAIL`, this
	 * 	  method returns the skimmer that have not found a list of feasible partitions or
	 *	  fails to skim it. 
	 * @note  If the LoadPartitions is never called or returned with other
	 *	  value the return of this method is undefined.
	 */
	inline SkimmerType_t GetLastFailed() const noexcept { 
		return failed_skimmer;
	}

private:

	/* ******* ATTRIBUTES ******* */
	std::unique_ptr<bu::Logger> logger;

	std::multimap<int, SkimmerPairType_t> skimmers;

	SkimmerType_t failed_skimmer;

	/* ******* METHODS ******* */
	ResourceMappingValidator();

};

}

#endif // BBQUE_RESOURCE_MAPPING_VALIDATOR_H_


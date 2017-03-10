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
#include "bbque/tg/task_graph.h"
#include "bbque/tg/partition.h"

namespace bu = bbque::utils;

namespace bbque {

/**
 * @class PartitionSkimmer
 *
 * @brief The interface for declaration of a PartitionSkimmer object.
 */
class PartitionSkimmer {

public:

	typedef enum SkimmerType_t {
		SKT_NONE,
		SKT_MANGO_HN,
		SKT_MANGO_MEMORY_MANAGER,
		SKT_MANGO_POWER_MANAGER
	} SkimmerType_t;

	typedef enum ExitCode_t {
		SK_OK = 0,
		SK_GENERIC_ERROR,
		SK_NO_PARTITION
	} ExitCode_t;

    virtual ~PartitionSkimmer() { }
    virtual ExitCode_t Skim(const TaskGraph &tg, std::list<Partition>&) noexcept = 0;

	PartitionSkimmer(SkimmerType_t type) : type(type) {}

	SkimmerType_t GetType() { return type; }

private:

	SkimmerType_t type;
};

typedef std::shared_ptr<PartitionSkimmer> PartitionSkimmerPtr_t;

/**
 * @class ResourceMappingValidator
 *
 * @brief Provides the list of available partitions, validating them across the registered callback
 *	  function.
 */
class ResourceMappingValidator {

public:

	typedef enum ExitCode_t {
		PMV_OK = 0,
		PMV_GENERIC_ERROR,
		PMV_NO_PARTITION,
		PMV_SKIMMER_FAIL
	} ExitCode_t;


	static ResourceMappingValidator & GetInstance();


	virtual ~ResourceMappingValidator()  {};

	/**
	 * @brief Load the feasible partitions according to registered callbacks. 
	 *
	 */
	ExitCode_t LoadPartitions(const TaskGraph &tg, std::list<Partition> &partitions);


	void RegisterSkimmer(PartitionSkimmerPtr_t skimmer, int priority) noexcept;

	/**
	 * @brief If LoadPartitions failed with `PMV_NO_PARTITION` or `PMV_SKIMMER_FAIL`, this
	 * 	  method returns the skimmer that have not found a list of feasible partitions or
	 *	  fails to skim it. 
	 * @note  If the LoadPartitions is never called or returned with other
	 *	  value the return of this method is undefined.
	 */
	inline PartitionSkimmer::SkimmerType_t GetLastFailed() const noexcept { 
		return failed_skimmer;
	}

private:

	/* ******* ATTRIBUTES ******* */
	std::unique_ptr<bu::Logger> logger;

	std::multimap<int, PartitionSkimmerPtr_t> skimmers;

	PartitionSkimmer::SkimmerType_t failed_skimmer;

	/* ******* METHODS ******* */
	ResourceMappingValidator();

};

}

#endif // BBQUE_RESOURCE_MAPPING_VALIDATOR_H_


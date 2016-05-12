/*
 * Copyright (C) 2016  Politecnico di Milano
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

#ifndef BBQUE_ATTRIBUTES_H_
#define BBQUE_ATTRIBUTES_H_

#include <map>
#include <memory>
#include <string>


namespace bbque { namespace utils {


/**
 * struct PluginData
 *
 * The structure must be inherited in order to implement a specific
 * attribute to manage. This provides just a string to classify the
 * attribute, and the identifying key string.
 */
typedef struct PluginData {
	/** Namespace */
	std::string plugin_name;
	/** ID key  */
	std::string key;
	/** Constructor */
	PluginData(std::string const & _pname, std::string const & _key):
		plugin_name(_pname),
		key(_key) {}
} PluginData_t;

/** Shared pointer to PluginData_t */
typedef std::shared_ptr<PluginData_t> PluginDataPtr_t;

/**
 * @brief A coontainer of module-specific attributes
 *
 * The class provide an interface for setting and getting specific attributes.
 * We expect to use this class for extending classes as Recipe, Application
 * and WorkingMode, in order to manage information specific for each plugin in
 * a flexible way.
 *
 * Considering a scheduler module, the extension provided by this class could
 * be exploit to append information to a descriptor object.
 *
 * For
 * instance how many times it has been re-scheduled, or in the WorkingMode
 * descriptor (e.g. How much that working mode is "good" for the scheduling).
 */
class AttributesContainer {

public:

	/** Data type for the structure storing the plugin-specific data */
	typedef std::map<std::string, PluginDataPtr_t> PluginDataMap_t;


	/**
	 * @brief Constructor
	 */
	AttributesContainer();

	/**
	 * @brief Destructor
	 */
	virtual ~AttributesContainer();

	/******************************************************************
	 *                   Plugin-specific data                         *
	 ******************************************************************/

	/**
	 * @brief Insert/Set a plugin specific data
	 *
	 * @param pdata Shared pointer to the PluginData object (commonly a
	 * derived class)
	 */
	inline void SetPluginData(PluginDataPtr_t pdata) {
		plugin_data.emplace(pdata->plugin_name, pdata);
	}

	/**
	 * @brief Get a plugin specific data
	 *
	 * @param plugin_name The namespace of the attribute
	 * @param key The ID key of the attribute
	 *
	 * @return A shared pointer to the PluginData object
	 */
	PluginDataPtr_t GetPluginData(
			std::string const & plugin_name, std::string const & key) const;

	/**
	 * @brief Clear a plugin specific data
	 *
	 * Remove all the attributes under a given namespace or a specific one
	 * if the key is specified.
	 *
	 * @param plugin_name The attribute namespace
	 * @param key The attribute key
	 */
	void ClearPluginData(
			std::string const & plugin_name, std::string const & key = "");
	 * @param key The attribute key
	 */

protected:

	/**
	 * @brief Multi-map storing the plugin-specific data
	 *
	 * The first level key is the plugin name. The value is a
	 * PluginDataPtr_t storing the ID key for the specific data.
	 * Users should must provide a proper extension of the PluginData
	 * class in order to store the data value.
	 */
	PluginDataMap_t plugin_data;
};

} // namespace utils

} // namespace bbque

#endif // BBQUE_ATTRIBUTES_H_

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
#ifndef MPIRUN_CONFIG_H
#define MPIRUN_CONFIG_H

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <memory>

namespace mpirun {

/**
 * @brief This class manages the configuration options and the command line
 *        arguments.
 */
class Config {

public:
    /**
     * @brief Getter for list of nodes in the config
     */
    inline const std::string& get_addresses      () const noexcept {
        return this->addresses;
    }

    /**
     * @brief Getter for the list of slots associated to nodes in the config
     */
    inline const std::string& get_slots          () const noexcept {
        return this->slots_per_addr;
    }

    /**
     * @brief Getter for time interval between two Exc
     */
    inline int                get_updatetime_res () const noexcept {
        return this->updatetime_resources;
    }

    /**
     * @brief Getter for time to start migration (testing)
     * @return 0 if migration is not active
     */
    inline int                get_mig_time       () const noexcept {
        return this->mig_time;
    }

    /**
     * @brief Getter for source node to migrate
     */
    inline const std::string& get_mig_source     () const noexcept {
        return this->mig_source;
    }

    /**
     * @brief Getter for destination node in migration
     */
    inline const std::string& get_mig_destination() const noexcept {
        return this->mig_destination;
    }

    /**
     * @brief Getter for mpirun arguments
     */
    inline const std::vector<const char *>& get_mpirun_args() const noexcept {
        return this->mpirun_arguments;
    }

    /**
     * @brief Getter for barbeque arguments
     */
    inline const std::vector<const char *>& get_bbque_args() const noexcept {
        return this->barbeque_arguments;
    }

    /**
     * @brief Get the singleton instance of this class.
     * @note  Thread-sage
     */
    static Config& get();

    /**
     * @brief Initialize the command line arguments parsing.
     */
    void set_arguments(int argc, char *argv[]);

private:
    /**
     * @brief The private constructor, hidden in order to implement
     *        the singleton pattern
     */
    Config();
    ~Config() = default;

    Config(Config const &)         = delete;
    void operator=(Config const &) = delete;

    void desc_initialization() noexcept;
    void parse_config_file();

    std::string addresses;              /** @brief Addresses per mpirun            */
    std::string slots_per_addr;         /** @brief Slots per addresses per mpirun  */
    int         updatetime_resources=0; /** @brief Time to wait to check new
                                                   resources and reply to ompi requests   */
    int         mig_time=0;             /** @brief Time when start migration
                                                         (for testing purpose)            */
    std::string mig_source;             /** @brief Source address for migration
                                                         (for testing purpose)            */
    std::string mig_destination;        /** @brief Destination address for migration
                                                         (for testing purpose)            */

    std::string conf_file;

    std::unique_ptr<boost::program_options::options_description> opts_desc;
    boost::program_options::variables_map opts_varmap;
    std::vector<const char *> mpirun_arguments;
    std::vector<const char *> barbeque_arguments;

};

}


#endif // MPIRUN_CONFIG_H

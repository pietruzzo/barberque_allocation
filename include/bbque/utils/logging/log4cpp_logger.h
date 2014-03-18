/*
 * Copyright (C) 2012  Politecnico di Milano
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

#ifndef BBQUE_UTILS_LOG4CPP_LOGGER_H_
#define BBQUE_UTILS_LOG4CPP_LOGGER_H_

#include "bbque/config.h"
#include "bbque/utils/logging/logger.h"

#include <memory>
#include <string>

#include <log4cpp/Category.hh>

#define MODULE_NAMESPACE LOGGER_NAMESPACE ".log4cpp"
#define MODULE_CONFIG LOGGER_CONFIG ".log4cpp"

namespace bbque { namespace utils {

/**
 * @brief A Log4CPP based Logger
 */
class Log4CppLogger : public Logger {

public:

	/**
	 * @brief Build a new Log4CPP based logger
	 * @param conf the logger configuration
	 */
	static std::unique_ptr<Logger>
	GetInstance(Configuration const & conf);

	/**
	 * \brief Logger dtor
	 */
	virtual ~Log4CppLogger() {};

//----- Logger module interface

#ifdef BBQUE_DEBUG
	/**
	 * \brief Send a log message with the priority DEBUG
	 * \param fmt the message to log
	 */
	void Debug(const char *fmt, ...);
#endif

	/**
	 * \brief Send a log message with the priority INFO
	 * \param fmt the message to log
	 */
	void Info(const char *fmt, ...);

	/**
	 * \brief Send a log message with the priority NOTICE
	 * \param fmt the message to log
	 */
	void Notice(const char *fmt, ...);

	/**
	 * \brief Send a log message with the priority WARN
	 * \param fmt the message to log
	 */
	void Warn(const char *fmt, ...);

	/**
	 * \brief Send a log message with the priority ERROR
	 * \param fmt the message to log
	 */
	void Error(const char *fmt, ...);

	/**
	 * \brief Send a log message with the priority CRIT
	 * \param fmt the message to log
	 */
	void Crit(const char *fmt, ...);

	/**
	 * \brief Send a log message with the priority ALERT
	 * \param fmt the message to log
	 */
	void Alert(const char *fmt, ...);

	/**
	 * \brief Send a log message with the priority FATAL
	 * \param fmt the message to log
	 */
	void Fatal(const char *fmt, ...);

private:

	/**
	 * Set true to use colors for logging
	 */
	bool use_colors = true;

	/**
	 * Set true when the logger has been configured.
	 * This is done by parsing a configuration file the first time a Logger is created.
	 */
	static bool configured;

	/**
	 * The Log4CPP configuration file to use
	 */
	static std::string conf_file_path;

	/**
	 * @brief The logger reference
	 * Use this logger reference, related to the 'log' category, to log your messages
	 */
	log4cpp::Category & logger;

	/**
	 * \brief Build a logger with the specified configuration
	 */
	Log4CppLogger(Configuration const & conf);

	/**
	 * @brief   Load Logger configuration
	 * @return  true if the configuration has been properly loaded and a Log4CPP
	 * 			logger could be successfully build, false otherwise.
	 */
	static bool Configure(Configuration const & conf);

};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_UTILS_LOG4CPP_LOGGER_H_

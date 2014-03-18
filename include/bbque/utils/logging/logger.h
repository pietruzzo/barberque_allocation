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

#ifndef BBQUE_UTILS_LOGGER_H_
#define BBQUE_UTILS_LOGGER_H_

#include "bbque/config.h"

#include <memory>
#include <string>

// The prefix for logging statements category
#define LOGGER_NAMESPACE "bq.log"
// The prefix for configuration file attributes
#define LOGGER_CONFIG "logger"

/**
 * Prepend file and line number to the logMessage
 */
#define FORMAT_DEBUG(fmt) "%25s:%05d - " fmt, __FILE__, __LINE__

namespace bbque { namespace utils {

/**
 * @brief The basic class for each Barbeque component
 *
 * This defines the basic logging services which are provided to each Barbeque
 * components. The object class defines logging and modules name.
 */
class Logger {

public:

//----- Objects initialization data

	typedef enum Priority {
		DEBUG,
		INFO,
		NOTICE,
		WARN,
		ERROR,
		CRIT,
		ALERT,
		FATAL
	} Priority;

	struct Configuration {
		Configuration(
				const char * category,
				const char * identity = "undef",
				Priority priority = WARN) :
			category(category),
			identity(identity),
			priority(priority) {};
		char const * category;
		char const * identity;
		Priority priority;
	};

	static std::unique_ptr<Logger>
	GetLogger(Configuration const & conf);

	virtual ~Logger() {};

//----- Objects interface

#ifdef BBQUE_DEBUG
	/**
	 * \brief Send a log message with the priority DEBUG
	 * \param fmt the message to log
	 */
	virtual void Debug(const char *fmt, ...) = 0;
#else
	void Debug(const char *fmt, ...) {(void)fmt;};
#endif
	/**
	 * \brief Send a log message with the priority INFO
	 * \param fmt the message to log
	 */
	virtual void Info(const char *fmt, ...) = 0;

	/**
	 * \brief Send a log message with the priority NOTICE
	 * \param fmt the message to log
	 */
	virtual void Notice(const char *fmt, ...) = 0;

	/**
	 * \brief Send a log message with the priority WARN
	 * \param fmt the message to log
	 */
	virtual void Warn(const char *fmt, ...) = 0;

	/**
	 * \brief Send a log message with the priority ERROR
	 * \param fmt the message to log
	 */
	virtual void Error(const char *fmt, ...) = 0;

	/**
	 * \brief Send a log message with the priority CRIT
	 * \param fmt the message to log
	 */
	virtual void Crit(const char *fmt, ...) = 0;

	/**
	 * \brief Send a log message with the priority ALERT
	 * \param fmt the message to log
	 */
	virtual void Alert(const char *fmt, ...) = 0;

	/**
	 * \brief Send a log message with the priority FATAL
	 * \param fmt the message to log
	 */
	virtual void Fatal(const char *fmt, ...) = 0;

protected:

	Configuration const & configuration;

	/**
	 * \brief Logger ctor
	 *
	 * The Logger constructor is private since direct instantiations are not
	 * allowed, while the GetLogger factory method must be used to properly
	 * configure the logger.
	 */
	Logger(Configuration const & conf);

};

} // namespace utils

} // namespace bbque

#endif // BBQUE_UTILS_LOGGER_H_

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

#ifndef BBQUE_VERSION_H_
#define BBQUE_VERSION_H_

extern "C" {

/**
 * A string representing the GIT version of the compiled binary
 */
extern const char *g_git_version;

}

#define BBQUE_VERSION_MAJOR     1
#define BBQUE_VERSION_MINOR     1
#define BBQUE_VERSION_REVISION  1
#define BBQUE_VERSION_STATUS    "dev"

#define BBQUE_VERSION_STR \
	BBQUE_STR(BBQUE_VERSION_MAJOR) "." \
	BBQUE_STR(BBQUE_VERSION_MINOR) "." \
	BBQUE_STR(BBQUE_VERSION_REVISION) "-" \
	BBQUE_VERSION_STATUS

#define BBQUE_STR(__A)       BBQUE_MAKE_STR(__A)
#define BBQUE_MAKE_STR(__A)  #__A


#endif // BBQUE_VERSION_H_

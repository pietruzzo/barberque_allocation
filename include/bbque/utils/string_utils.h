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

#include <list>
#include <string>

#ifndef BBQUE_STRING_UTILS_H_
#define BBQUE_STRING_UTILS_H_

namespace bbque {

namespace utils {

/**
 * @brief Split a text into separate strings
 */
inline void SplitString(
	std::string const & str, std::list<std::string> & lst, std::string const sep = ".") {
	size_t _e, _b = 0;
	while ((_e = str.find_first_of(sep, _b)) != std::string::npos) {
		lst.push_back(str.substr(_b, (_e - _b)));
		_b = _e + 1;
	}
	lst.push_back(str.substr(_b, std::string::npos));
}

/**
 * @brief Convert a string into uppercase
 */
inline std::string UpperString(std::string const & in_str) {
	std::string out_str;
	for (auto & c: in_str)
		out_str += std::toupper(c);
	return out_str;
}

}


} // namespace utils

} // namespace bbque

#endif // BBQUE_STRING_UTILS_H_

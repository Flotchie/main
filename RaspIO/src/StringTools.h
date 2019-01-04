/*
 * StrFormatter.h
 *
 *  Created on: 18 d�c. 2018
 *      Author: Bertrand
 */

#ifndef STRINGTOOLS_H_
#define STRINGTOOLS_H_

#include <string>
#include <set>

std::string form(const char *fmt,...);
bool ListDir( const std::string& path, std::set<std::string>& files );

#endif /* STRINGTOOLS_H_ */

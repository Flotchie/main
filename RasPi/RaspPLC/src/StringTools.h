/*
 * StrFormatter.h
 *
 *  Created on: 18 déc. 2018
 *      Author: Bertrand
 */

#ifndef STRINGTOOLS_H_
#define STRINGTOOLS_H_

#include <string>
#include <vector>
#include <set>

std::string form(const char *fmt,...);
bool ListDir( const std::string& path, std::set<std::string>& files );

void Tokenize(const std::string& str, const std::string& sep, std::vector<std::string>& tokens, bool emptyAsToken=false);

#endif /* STRINGTOOLS_H_ */

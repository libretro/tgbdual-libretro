/**
 * $Id: resource.cc,v 1.14 2003/07/24 10:53:10 i Exp $
 *
 * Copyright (C) shinichiro.h <g940455@mail.ecc.u-tokyo.ac.jp>
 *  http://user.ecc.u-tokyo.ac.jp/~g940455/wp/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "resource.h"
#include "cpputil.h"

#include <SDL.h>

#include <fstream>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

Resource::Resource() {}

void Resource::addResourceFile(const std::string& file) {
	std::ifstream is(file.c_str());
	check(is.is_open(), file + ": resource file not found.");

	addResourceStream(is);
}

void Resource::addResourceStream(std::istream& is) {
    static const int MAXBUF = 256;
    char ln[MAXBUF];
	std::string opt = "";

    while (is.getline(ln, MAXBUF)) {
		std::string line(ln);
		unsigned int s;

		// コメントの除去
		if ((s = line.find("#")) != std::string::npos)
			line.erase(s);

		// [ が先頭にある行は無視
		if ((s = line.find("[")) == 0) {
			s = line.find("]");
			opt = line.substr(1, s-1);
			line = "";
		}

		// 行頭空白の除去
		if (!line.empty() && line[0] == ' ')
			line.erase(0, line.find_first_not_of(' '));

		s = line.find("=");
		std::string key, val;
		key = line.substr(0, s);
		val = line.substr(s+1, std::string::npos);

/*
		while (ss) {
			std::string add;
			ss >> add;
			if (add != "") val += " " + add;
		}
*/

		if (key != "") {
			if (opt != "") {
//				printf("%s => %s\n", (opt+":"+key).c_str(), val.c_str());
				setString(opt+":"+key, val);
			}
			else {
				setString(key, val);
			}
		}
    }
}

void Resource::addArgument(int argc, char** argv) {
#ifdef HAVE_GETOPT_H
	int optc;
    while ((optc = getopt(argc, argv, "m:c")) != -1) {
        switch (optc) {
		default:
			break;
		}
	}
	if (optind < argc) {
	}
#else
	if (1 < argc) {
	}
#endif
}


/*
void Resource::dump() const {
	for (RcMap::const_iterator ite = resource_.begin();
		 ite != resource_.end(); ++ite)
	{
		std::cout << ite->first << " => " << ite->second.first << std::endl;
	}
}

std::istream& operator>> (std::istream& is, Resource& rc) {
	rc.addResourceStream(is);
	return is;
}

std::ostream& operator<< (std::ostream& os, const Resource& rc) {
	for (Resource::RcMap::const_iterator ite = rc.resource_.begin();
		 ite != rc.resource_.end(); ++ite)
	{
		os << ite->first << " " << ite->second.first << std::endl;
	}
	return os;
}

*/

bool Resource::has(const std::string& key) const {
	try {
		RcMap::const_iterator ite = resource_.find(key);
		return ite != resource_.end();
	}
	catch (...) {
		error("broken config file: key = " + key);
	}
	return false; // dummy...
}

std::string Resource::get_string(const std::string& key) const {
	try {
		RcMap::const_iterator ite = resource_.find(key);
		check(ite != resource_.end(), key + ": unknow key in resource.");
		return ite->second.first;
	}
	catch (...) {
		error("broken config file: key = " + key);
	}
	return ""; // dummy...
}

int Resource::get_int(const std::string& key) const {
	try {
		RcMap::const_iterator ite = resource_.find(key);
		check(ite != resource_.end(), key + ": unknow key in resource.");
		return atoi(ite->second.first.c_str());
	}
	catch (...) {
		error("broken config file: key = " + key);
	}
	return 0; // dummy...
}


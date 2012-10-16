/**
 * $Id: resource.h,v 1.8 2003/07/24 10:53:10 i Exp $
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

#ifndef resource_h_
#define resource_h_

#include "cpputil.h"

#include <string>
#include <map>
#include <utility>
#include <fstream>

#include <stdlib.h>

class Resource {
public:
	Resource();

	void addDefault();
	void addResourceFile();
	void addAdditionalResourceFile();
	void addResourceFile(const std::string& file);
	void addResourceStream(std::istream& is);
	void addArgument(int argc, char** argv);

	template <class Iterator_>
	void saveResource(const std::string& file, Iterator_ begin, Iterator_ end);

	std::string get_string(const char* key) const {
		return get_string(std::string(key));
	}
	std::string get_string(const std::string& key) const;
	int get_int(const char* key) const {
		return get_int(std::string(key));
	}
	int get_int(const std::string& key) const;

	bool has(const char* key) const {
		return has(std::string(key));
	}
	bool has(const std::string& key) const;

	void set(const std::string& key, const std::string& val, bool save =true) {
		setString(key, val, save);
	}
	void setString(const std::string& key,
				   const std::string& val, bool save =true) {
		resource_[key] = std::make_pair(val, save);
	}

	void dump() const;

public:
	typedef std::pair<std::string, bool> RcVal;
	typedef std::map<std::string, RcVal> RcMap;

public:
	typedef RcMap::iterator iterator;
	typedef RcMap::const_iterator const_iterator;
	iterator begin() { return resource_.begin(); }
	iterator end() { return resource_.end(); }
	const_iterator begin() const { return resource_.begin(); }
	const_iterator end() const { return resource_.end(); }

private:
	RcMap resource_;

private:
	friend std::istream& operator>> (std::istream& is, Resource& rc);
	friend std::ostream& operator<< (std::ostream& os, const Resource& rc);

};

template <class Iterator_>
void Resource::saveResource(const std::string& file,
							Iterator_ begin, Iterator_ end)
{
	std::ofstream os(file.c_str());
	for (Iterator_ ite = begin; ite != end; ++ite) {
		os << *ite << " " << get_string(*ite) << std::endl;
	}
}

#endif // ! resource_h_

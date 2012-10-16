#include "zlibwrap.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <vector>
#include <string>
#include <algorithm>

#define HAVE_LIBZ

#ifdef HAVE_LIBZ
#include <zlib.h>
#include "unzip.h"
#endif

typedef unsigned char byte;

namespace {
	std::string lc(std::string s) {
		std::string ret;
		std::transform(s.begin(), s.end(), std::back_inserter(ret), tolower);
		return ret;
	}

	std::string get_suffix(std::string name) {
		size_t i = name.rfind(".");
		return name.substr(i+1);
	}
	std::string get_suffix_lc(std::string name) {
		return lc(get_suffix(name));
	}

	std::string get_prefix(std::string name) {
		size_t i = name.rfind(".");
		return name.substr(0, i);
	}

	bool is_allowable(std::string name, const char** exts) {
		std::string ext = get_suffix_lc(name);
		for (; exts; exts++) {
			if (ext == *exts) return true;
		}
		return false;
	}

	byte* raw_file_read(const char* n, int* size) {
		byte* dat = 0;
		FILE* file=fopen(n,"rb");
		if (!file) return NULL;
		fseek(file,0,SEEK_END);
		*size=ftell(file);
		fseek(file,0,SEEK_SET);
		dat=(byte*)malloc(*size);
		fread(dat,1,*size,file);
		fclose(file);
		return dat;
	}

#ifdef HAVE_LIBZ
	byte* gz_file_read(const char* n, int* size) {
		byte* dat = 0;
		static const int GZSIZE = 4096;
		byte buf[GZSIZE];
		int s;
		gzFile file=gzopen(n,"r");

		*size = 0;

		if (!file) return 0;
		while (!gzeof(file)) {
			s = gzread(file,buf,GZSIZE);
			dat = (byte*)realloc(dat, *size+s);
			memcpy(dat+(*size),buf,s);
			(*size) += s;
		}
		gzclose(file);
		return dat;
	}

	// from zgnuboy
	byte *load_zipfile(const char *path, int *len)
	{
		int size;
		byte *buf = NULL;
		unzFile zipf;
		unz_file_info zipf_info;

		zipf = unzOpen(path);
		if (!zipf) {
			unzClose(zipf);
			return NULL;
		}

		/* TODO: better file handling */
		unzGoToFirstFile(zipf);

		if (unzGetCurrentFileInfo(zipf, &zipf_info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK) {
			unzClose(zipf);
			return NULL;
		}
		size = zipf_info.uncompressed_size;

		if (size <= 0) {
			unzClose(zipf);
			return NULL;
		}

		if (unzOpenCurrentFile(zipf) != UNZ_OK) {
			unzClose(zipf);
			return NULL;
		}

		buf = (byte*)malloc(size);
		if (unzReadCurrentFile(zipf, buf, size) != size) {
			free(buf);
			unzCloseCurrentFile(zipf);
			unzClose(zipf);
			return NULL;
		}

		unzCloseCurrentFile(zipf);
		unzClose(zipf);

		*len = size;
		return buf;
	}

#endif
}

byte* file_read(const char* n, const char** exts, int* size) {
	std::string name(n);

	byte* dat = 0;

#ifdef HAVE_LIBZ
	if (get_suffix_lc(name) == "zip") {
		if (!is_allowable(get_prefix(name), exts)) return NULL;
		dat = load_zipfile(n, size);
	}
	else if (get_suffix_lc(name) == "gz") {
		if (!is_allowable(get_prefix(name), exts)) return NULL;
		dat = gz_file_read(n, size);
	}
	else {
#endif
		if (!is_allowable(name, exts)) return NULL;
		dat = raw_file_read(n, size);
#ifdef HAVE_LIBZ
	}
#endif

	return dat;
}


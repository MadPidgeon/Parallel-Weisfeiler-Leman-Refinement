#pragma once
#include <iostream>
#include <set>
#include <vector>
#include <array>
#include <cmath>
#include <map>
#include <algorithm>
#include <sstream>
#include <string>
#include <cstring>
#include "city.h"

const uint k = 2;

struct hash_t {
	std::pair<uint64_t,uint64_t> value;

	hash_t();
	hash_t( uint64_t v );
	hash_t( const char* str, size_t len );
	bool operator==( const hash_t& other ) const;
	bool operator<( const hash_t& other ) const;
	void assign( uint64_t v );
	void assign( const char* str, size_t len );
	size_t raw( char* str ) const;
};

namespace std {
	template <> struct hash<hash_t> {
		size_t operator()( const hash_t& H ) const {
			return H.value.second;
		}
	};
}

std::ostream& operator<<( std::ostream& os, hash_t H );

struct colour_t;

typedef std::vector<std::array<colour_t,k>> colour_pattern_t; 

struct colour_t {
	hash_t hash_value;
	unsigned char original_colour : 2, is_symmetrical : 1;

	colour_t();
	colour_t( uint64_t v );
	colour_t( colour_t col, bool sym, colour_pattern_t pat );
	bool operator==( const colour_t& other ) const;
	bool operator<( const colour_t& other ) const;
	void assign( uint64_t v );
	void assign( colour_t col, bool sym, colour_pattern_t pat );
};

namespace std {
	template <> struct hash<colour_t> {
		size_t operator()( const colour_t& C ) const {
			return hash<hash_t>()( C.hash_value );
		}
	};
}

std::ostream& operator<<( std::ostream& os, colour_t C );

typedef std::array<uint,k> vertex_tuple_t;
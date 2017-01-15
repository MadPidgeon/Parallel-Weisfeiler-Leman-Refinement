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
#include <iomanip>
#include "city.h"

const uint k = 2;

// type describing the google city hash of a string
struct hash_t {
	std::pair<uint64_t,uint64_t> value;

	// default constructor
	hash_t();
	// initialise hash analogous to assign function
	hash_t( uint64_t v );
	hash_t( const char* str, size_t len );
	// compare hashes
	bool operator==( const hash_t& other ) const;
	bool operator<( const hash_t& other ) const;
	// set hash to (0,v)
	void assign( uint64_t v );
	// set hash to hash of string
	void assign( const char* str, size_t len );
	// copy raw hash data to array pointed at by str
	size_t raw( char* str ) const;
};

// register hash_t to std to allow it to be used for hashmaps
namespace std {
	template <> struct hash<hash_t> {
		size_t operator()( const hash_t& H ) const {
			return H.value.second;
		}
	};
}

// prints hash to stream
std::ostream& operator<<( std::ostream& os, hash_t H );

struct colour_t;

// type describing a `new colour'
typedef std::vector<std::array<colour_t,k>> colour_pattern_t; 

// type describing the colour of a pair
struct colour_t {
	hash_t hash_value[2];
	unsigned char original_colour;

	// default constructor
	colour_t();
	// initialise colour analogous to assign function
	colour_t( uint64_t v );
	colour_t( colour_t col, colour_pattern_t pat );
	// obtain colour of reverse of pair, i.e. if this colour describes C(x,y), the result describes C(y,x)
	colour_t reverse() const;
	// compare colours for sorting
	bool operator==( const colour_t& other ) const;
	bool operator<( const colour_t& other ) const;
	// set original colour to v
	void assign( uint64_t v );
	// compute colour using the previous colour col and the colour pattern
	void assign( colour_t col, colour_pattern_t pat );
	// describes a different order
	bool less_than( const colour_t& other, int i ) const;
};

// register colour_t to std to allow it to be used for hashmaps
namespace std {
	template <> struct hash<colour_t> {
		size_t operator()( const colour_t& C ) const {
			return hash<hash_t>()( C.hash_value[0] );
		}
	};
}

// prints colour to stream
std::ostream& operator<<( std::ostream& os, colour_t C );

typedef std::array<uint,k> vertex_tuple_t;
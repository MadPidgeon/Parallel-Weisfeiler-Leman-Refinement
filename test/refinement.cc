/* 
g++ -o refinement refinement.cc lib/libmcbsp1.2.0.a -pthread -std=c++14
*/
#include <iostream>
#include <set>
#include <vector>
#include <array>
#include <cmath>
#include <map>
#include <algorithm>
#include <sstream>
#include "ext.h"
#include "include/mcbsp.hpp"
extern "C" {
	#include "mcbsp-affinity.h"
}

using namespace std;

typedef unsigned int colour_t;

int P;
int N;
int T;
vector<ostringstream> pout;

void matrix_print( ostream& os, const vector<colour_t>& matrix, bool newline ) {
	for( int j = 0; j < N; ++j ) {
		for( int i = 0; i < N; ++i )
			os.put( matrix[i+j*N] + '0' );
		if( newline )
			os.put( '\n' );
	}
}

void matrix_dump( ostream& os, const vector<colour_t>& matrix ) {
	os << "[ ";
	for( int j = 0; j < N; ++j ) {
		os << "[ ";
		for( int i = 0; i < N; ++i )
			os << matrix[i+j*N] << " ";
		os << "] ";
	}
	os << "]";
}

void debug_output( int p ) {
	bsp_sync();
	cout << pout[p].str() << flush;
	pout[p].str(string());
	pout[p] << "---- PROC " << p << " ----\n";
	bsp_sync();
}

void matrix_broadcast( int p, vector<colour_t>& matrix ) {
	int B = (T+P-1)/P;
	if( p == 0 )
		for( int i = 1; i < P; ++i )
			bsp_put( i, matrix.data()+B*i, matrix.data(), B*i*sizeof(colour_t), min( B, T-B*i )*sizeof(colour_t) );
	bsp_sync();
	for( int i = 1; i < P; ++i )
		if( i != p )
			bsp_put( i, matrix.data()+B*p, matrix.data(), B*p*sizeof(colour_t), min( B, T-B*p )*sizeof(colour_t) );	
}

bool input_matrix( int p, vector<colour_t>& M, bool& error ) {
	if( p == 0 ) {
		bool occ[2] = {false,false};
		int v;
		for( int i = 0; i < T; ++i ) {
			v = getchar() - '0';
			occ[v] = true;
			M[i] = v+1;
		}
		if( !(occ[0]&occ[1]) ) {
			error = true;
			for( int i = 1; i < P; ++i )
				bsp_put( i, &error, &error, 0, sizeof(bool) );
		} 
	}
	for( int i = 0; i < N; ++i )
		M[i*(N+1)] = 0;
	matrix_broadcast( p, M );
	bsp_sync();
	return error;
}

void relation_reassignment1( vector<vector<array<int,2>>>& colour_to_tuple, vector<int>& local_work ) {
	int expected_work = N*N / P;
	int bla;
}

void processor_main() {
	bsp_begin( P );
	unsigned int p = bsp_pid();
	bool unstable = true;
	bool error = false;
	bsp_push_reg( &error, sizeof(bool) );
	int u, v, t, r;
	vector<int> local_work( P, 0 );
	bsp_push_reg( local_work.data(), sizeof(int)*P );
	int lrel_count, new_lrel_count, grel_count = 3;
	vector<colour_t> M( T ); // stores the colouring of the graph
	bsp_push_reg( M.data(), sizeof(colour_t)*T );
	vector<int> relation_bloat( T, -1 ); // stores relation bloat
	vector<int> cum_rel_bloat = {0}; // stores cummulative relation bloat
	bsp_push_reg( relation_bloat.data(), sizeof(int)*T );
	vector<colour_t> local_relations; // relations of processor p
	vector<colour_t> new_local_relations;
	vector<vector<array<int,2>>> colour_to_tuple; // stores local colour to tuple mapping
	vector<vector<array<int,2>>> new_colour_to_tuple;
	vector<array<colour_t,2>> temp_col_pattern; // stores new colour pattern
	map<vector<array<colour_t,2>>,vector<array<int,2>>> col_patterns; // stores all colour patterns
	vector<vector<vector<array<int,2>>>> D; // compressed storage of all colour patterns

	#define INDEX( x, y ) (x+N*y)

	bsp_sync();
	debug_output( p );
	if( input_matrix( p, M, error ) ) {
		bsp_end();
		return;
	}

	//matrix_print( pout[p], M, true );
	//debug_output( p );

	// temporary intializations of variables
	local_relations = {p};
	lrel_count = local_relations.size();
	colour_to_tuple.resize( lrel_count );
	for( int i = 0; i < N; ++i ) // can be done more efficiently
		for( int j = 0; j < N; ++j )
			for( int s = 0; s < lrel_count; ++s )
				if( M[INDEX(i,j)] == local_relations[s] ){
					local_work[p]++;
					colour_to_tuple[s].push_back( {i,j} );
				}
	for( int q = 0; q < P; q++ )
		if( q != p )
			bsp_put( q, &local_work[p], local_work.data(), p*sizeof(int), sizeof(int) );
	bsp_sync();
	pout[p] << "local work: " << local_work << endl;
	debug_output( p );

	// main iteration loop
	int counter = 0;
	while( unstable ) {
		counter++;
		relation_reassignment1( colour_to_tuple, local_work );

		lrel_count = local_relations.size();
		D.resize( lrel_count );
		new_lrel_count = 0;
		// pout[p] << "colour_to_tuple = " << colour_to_tuple << endl;
		// debug_output( p );
		for( int s = 0; s < lrel_count; ++s ) {
			r = local_relations[s];
			for( auto tup : colour_to_tuple[s] ) {
				// create colour pattern for this tuple
				u = tup[0];
				v = tup[1];
				temp_col_pattern.reserve( N );
				for( int x = 0; x < N; ++x )
					temp_col_pattern.push_back( { M[INDEX(x,v)], M[INDEX(u,x)] } );
				sort( temp_col_pattern.begin(), temp_col_pattern.end() );
				// store colour pattern
				col_patterns[move(temp_col_pattern)].push_back( tup );
				temp_col_pattern.clear();
			}
			// pout[p] << "col_patterns = " << col_patterns << endl;
			// count number of new relations through relation bloat
			relation_bloat[r] = col_patterns.size();
			new_lrel_count += relation_bloat[r];
			for( unsigned int q = 0; q < P; ++q )
				if( q != p )
					bsp_put( q, relation_bloat.data() + r, relation_bloat.data(), r*sizeof(int), sizeof(int) );
			D[s].clear();
			for( auto new_colouring : col_patterns )
				D[s].push_back( move( new_colouring.second ) );
			col_patterns.clear();
		}
		
		// debug_output( p );

		bsp_sync();
		cum_rel_bloat.resize( grel_count+1 );
		for( int i = 0; i < grel_count; ++i )
			cum_rel_bloat[i+1] = cum_rel_bloat[i] + relation_bloat[i];
		new_local_relations.resize( new_lrel_count );
		new_colour_to_tuple.resize( new_lrel_count );
		t = 0;
		for( int s = 0; s < lrel_count; ++s ) {
			for( int k = 0; k < relation_bloat[local_relations[s]]; ++k ) {
				new_local_relations[t] = cum_rel_bloat[local_relations[s]] + k;
				new_colour_to_tuple[t].clear();
				for( auto tup : D[s][k] ) {
					u = tup[0];
					v = tup[1];
					M[INDEX(u,v)] = new_local_relations[t];
					new_colour_to_tuple[t].push_back( move( tup ) );
					for( int q = 0; q < P; ++q )
						if( q != p )
							bsp_put( q, M.data() + INDEX(u,v), M.data(), INDEX(u,v)*sizeof(colour_t), sizeof(colour_t) );
				}
				++t;
			}
		}
		swap( local_relations, new_local_relations );
		swap( colour_to_tuple, new_colour_to_tuple );
		bsp_sync();
		// check if number of relations has changed
		if( grel_count == cum_rel_bloat[grel_count] )
			unstable = false;
		grel_count = cum_rel_bloat[grel_count];
	}
	//matrix_print( pout[p], M, true );
	//debug_output( p );
	if( p == 0 ) {
		//matrix_dump( cout, M );
		cout << endl;
	}
	cout << p << counter << endl;
	bsp_end();
}

int main( int argc, char ** argv ) {
	bsp_init( processor_main, argc, argv );
	mcbsp_set_affinity_mode( COMPACT );
	scanf( "%d %d %*[ \n\t]", &P, &N );
	// usage: processors nodes matrix
	T = N*N;
	P = 3; // temp
	pout.resize(P);
	processor_main();
	return 0;
}
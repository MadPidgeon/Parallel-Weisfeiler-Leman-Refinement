// gcc -o refinement refinement.c lib/libmcbsp1.2.0.a -pthread -lrt -lm -std=c99
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <set>
#include <vector>
#include <array>
#include "include/mcbsp.hpp"
extern "C" {
	#include "mcbsp-affinity.h"
}

typedef unsigned int colour_t;

int P;
int N;
int T;


int min( int a, int b ) {
	return a < b ? a : b;
}

void matrix_print( color_t* matrix, bool newline ) {
	for( int j = 0; j < N; ++j ) {
		for( int i = 0; i < N; ++i )
			putchar( matrix[i+j*N] + '0' );
		if( newline )
			putchar('\n');
	}
}

void matrix_broadcast( int p, color_t* matrix ) {
	int B = (T+P-1)/P;
	if( p == 0 )
		for( int i = 1; i < P; ++i )
			bsp_put( i, matrix+B*i, matrix, B*i, min( B, T-B*i ) );
	bsp_sync();
	for( int i = 1; i < P; ++i )
		if( i != p )
			bsp_put( i, matrix+B*p, matrix, B*p, min( B, T-B*p ) );
}


void processor_main() {
	bsp_begin( P );
	int p = bsp_pid();
	vector<colour_t> M( T );
	bsp_push_reg( matrix, sizeof(colour_t)*T );
	int begin = ((N+P-1)/P)*p;
	int end = min( ((N+P-1)/P)*(p+1), N );
	bool unstable = true;
	int u, v;
	vector<int> E( T, -1 ); // stores relation bloat
	vector<int> I; // relations of processor p
	vector<array<int,2>> A; // stores new colour pattern
	map<vector<array<int,2>>,vector<int>> B; // stores all colour patterns
	bsp_sync();

	// get matrix from stdin
	#define INDEX( x, y ) (x+N*y)
	if( p == 0 )
		for( int i = 0; i < T; ++i )
			matrix[i] = getchar()-'0'+1;
	for( int i = 0; i < N; ++i )
		matrix[i*(N+1)] = 0;
	int R = 3;

	matrix_broadcast( p, matrix );
	bsp_sync();

	// temp distribution
	I = {p};

	for( r : Ip ) {
		for( auto tup : C[r] ) {
			tie( u, v ) = tup;
			A.reserve( N );
			for( int x = 0; x < N; ++x )
				A.push_back( { M[INDEX(x,v)], M[INDEX(u,x)] } );
			sort( A.begin(), A.end() );
			B[move(A)].push_back( tup );
		}
		for( auto new_colouring : B ) {

		}
	}

	


	// computation step
	/*while( unstable ) {
		unstable = false;
		for( int u = begin; u < end; ++u ) {
			for( int v = 0; v < N; ++v ) {
				for( int x = 0; x < N; ++x ) {

				}
			}
		}
	}*/

	if( p == 0 )
		matrix_print( matrix, true );
	bsp_end();
}

int main( int argc, char ** argv ) {
	bsp_init( processor_main, argc, argv );
	mcbsp_set_affinity_mode( COMPACT );
	// TODO: catch complete graph and empty graph
	scanf( "%d %d %*[ \n\t]", &P, &N );
	T = N*N;
	P = 3; // temp
	processor_main();
	return 0;
}
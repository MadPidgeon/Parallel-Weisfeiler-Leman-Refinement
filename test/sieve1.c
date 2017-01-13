// gcc -o sieve1 sieve1.c lib/libmcbsp1.2.0.a -pthread -lrt -std=c99
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include/mcbsp.h"

typedef int prime_t;
typedef char byte;

int logP;
int P;
int M;
int N;
int W;

int min( int a, int b ) {
	return a<b?a:b;
}

int XGCD(int a, int b, int* x, int* y) {
    if( b == 0 ) {
       *x = 1;
       *y = 0;
       return a;
    }
    int x1, y1, gcd = XGCD( b, a % b, &x1, &y1 );
    *x = y1;
    *y = x1 - (a / b) * y1;
    return gcd;
}

void spmd() {
	// init
	bsp_begin( P );
	int proc = bsp_pid();
	byte* sieve = malloc( sizeof(byte)*M );
	memset( sieve, 0, sizeof(byte)*M );
	prime_t* primes = malloc( sizeof(prime_t)*N );
	int num_primes = 2;
	primes[0] = 2;
	primes[1] = 3;
	int p = 3;
	prime_t* prime_candidates = malloc( sizeof(prime_t)*P );
	for( int i = 0; i < P; i+=1 )
		prime_candidates[i] = 2*i+1;
	prime_candidates[0] = W+1;
	bsp_push_reg( prime_candidates, sizeof(prime_t)*P ); 	
	int candidate_index = 0;
	int base = 2*proc+1;
	int base_inverse;
	int temp;
	XGCD( W, base, &temp, &base_inverse );
	base_inverse &= W-1;
	if( proc == 0 ) {
		sieve[0] = 1;
		candidate_index = 1;
	}
	bsp_sync();

	#define VAL(z) (base+(z)*W)
	
	// program
	while( p < N ) {
		int y;
		XGCD( W, p*base_inverse, &temp, &y );
		int k = ((y & (W-1))*p-base) / W;

		// ugly: remove all multiples of p
		/*int k;
		for( k = candidate_index; k < M; k += 1 )
			if( VAL(k) % p == 0 )
				break;
		if( proc == 1 )
			printf( "%d; %d ?= %d @ %d\n", p, q, k, VAL(k) ); // note that this does and should not make sense when k >= M*/
		while( k < M ) {
			sieve[k] = 1;
			k += p;
		}

		// find new candidate
		while( candidate_index < M && sieve[candidate_index] )
			candidate_index += 1;
		prime_candidates[proc] = VAL( candidate_index );

		// share candidates
		//bsp_sync();
		for( int i = 0; i < P; i += 1 )
			if( i != proc )
				bsp_put( i, prime_candidates+proc, prime_candidates, proc*sizeof(prime_t), sizeof(prime_t) );
		bsp_sync();
		//printf("HALP %d\n",proc);

		// find prime from candidates
		p = prime_candidates[0];
		for( int i = 1; i < P; i += 1 )
			p = min( p, prime_candidates[i] );
		primes[num_primes] = p;
		//printf( "p=%d\n", p );
		num_primes += 1;
	}

	// output
	while( num_primes>0 && primes[num_primes-1] > N  )
		num_primes -= 1;
	if( proc == 0 ){
		/*for( int i = 0; i < num_primes; i += 1 )
			printf( "%d ", primes[i] );
		printf("\n");*/
		printf("%d\n",num_primes);
	}

	// deinit
	free( sieve );
	free( prime_candidates );
	free( primes );
	bsp_end();
}

int main( int argc, char ** argv ) {
	bsp_init( &spmd, argc, argv );
	if( argc != 3 ) {
		printf("usage: sieve n p\n  n: find primes up to n\n  p: use 2^p processors\n");
		return EXIT_FAILURE;
	}
	N = atoi( argv[1] );
	logP = atoi( argv[2] );
	P = 1 << logP;
	W = 2*P;
	M = ( N+W-1 ) / W;
	spmd();
	return EXIT_SUCCESS;
}
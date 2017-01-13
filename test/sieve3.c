// gcc -o sieve1 sieve1.c lib/libmcbsp1.2.0.a -pthread -lrt -std=c99
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "include/mcbsp.h"

typedef long long prime_t;
typedef char byte;

prime_t logP;
prime_t P;
prime_t M;
prime_t N;
prime_t W;

prime_t min( prime_t a, prime_t b ) {
	return a<b?a:b;
}

prime_t XGCD(prime_t a, prime_t b, prime_t* x, prime_t* y) {
    if( b == 0 ) {
       *x = 1;
       *y = 0;
       return a;
    }
    prime_t x1, y1, gcd = XGCD( b, a % b, &x1, &y1 );
    *x = y1;
    *y = x1 - (a / b) * y1;
    return gcd;
}

void spmd() {
	// init
	bsp_begin( P );
	prime_t proc = bsp_pid();
	byte* sieve = malloc( sizeof(byte)*M );
	memset( sieve, 0, sizeof(byte)*M );
	prime_t* primes = malloc( sizeof(prime_t)*N );
	prime_t num_primes = 2;
	primes[0] = 2;
	primes[1] = 3;
	prime_t p = 3;
	prime_t* prime_candidates = malloc( sizeof(prime_t)*P );
	for( prime_t i = 0; i < P; i+=1 )
		prime_candidates[i] = 2*i+1;
	prime_candidates[0] = W+1;
	bsp_push_reg( prime_candidates, sizeof(prime_t)*P ); 	
	prime_t candidate_index = 0;
	prime_t base = 2*proc+1;
	prime_t base_inverse;
	prime_t temp;
	prime_t private_primes = 0;
	XGCD( W, base, &temp, &base_inverse );
	base_inverse &= W-1;
	if( proc == 0 ) {
		sieve[0] = 1;
		sieve[1] = (P==1);
		candidate_index = 1 + (P==1);
	} else if( P > 1 && proc == 1 ) {
		sieve[0] = 1;
		candidate_index = 1;
	}
	bsp_sync();

	#define VAL(z) (base+(z)*W)
	
	// program
	prime_t bound = sqrt(N);
	while( p <= bound ) {
		prime_t y;
		XGCD( W, p*base_inverse, &temp, &y );
		prime_t k = ((y & (W-1))*p-base) / W;
		//sieve[k] = 1;
		//k += p*(p-1);
		//printf("%lld:%lld,%lld!\n",proc,p,k);

		// ugly: remove all multiples of p
		/*prime_t k;
		for( k = candidate_index; k < M; k += 1 )
			if( VAL(k) % p == 0 )
				break;
		if( proc == 1 )
			prprime_tf( "%d; %d ?= %d @ %d\n", p, q, k, VAL(k) ); // note that this does and should not make sense when k >= M*/
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
		for( prime_t i = 0; i < P; i += 1 )
			if( i != proc )
				bsp_put( i, prime_candidates+proc, prime_candidates, proc*sizeof(prime_t), sizeof(prime_t) );
		bsp_sync();

		// find prime from candidates
		p = prime_candidates[0];
		for( prime_t i = 1; i < P; i += 1 )
			p = min( p, prime_candidates[i] );
		if( p == prime_candidates[proc] ) {
			sieve[candidate_index] = 1;
			private_primes++;
		}
		primes[num_primes] = p;
		/*if( proc == 0 )
			printf("%lld\n",p);*/
		num_primes += 1;
	}
	//printf("!!!\n");

	for( prime_t i = candidate_index; VAL(i) <= N; ++i ) {
		if( sieve[i] == 0 ) {
			//printf("%lld\n",VAL(i));
			private_primes++;
		}
	}

	prime_candidates[proc] = private_primes;
	bsp_put( 0, prime_candidates+proc, prime_candidates, proc*sizeof(prime_t), sizeof(prime_t) );
	bsp_sync();

	// output
	if( proc == 0 ) {
		prime_t total = 2; // prime number 2,3
		for( prime_t i = 0; i < P; ++i )
			total += prime_candidates[i];
		printf("%lld\n",total);
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
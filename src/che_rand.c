#include <che_rand.h>
#include <stdlib.h>

/* TODO: This is just for PC, make a platform independent random */

int che_rand_init(che_rand_t *r, unsigned int seed)
{
	return 0;
}

int che_rand(che_rand_t *r)
{
	return rand();
}

#include <che_rand.h>
#include <stdlib.h>

int che_rand_init(che_rand_t *r, unsigned int seed)
{
	return 0;
}

int che_rand(che_rand_t *r)
{
	return rand();
}

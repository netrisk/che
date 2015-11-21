#ifndef CHE_RAND_H
#define CHE_RAND_H

typedef struct che_rand_t
{
} che_rand_t;

int che_rand_init(che_rand_t *r, unsigned int seed);

int che_rand(che_rand_t *r);

#endif /* CHE_RAND_H */

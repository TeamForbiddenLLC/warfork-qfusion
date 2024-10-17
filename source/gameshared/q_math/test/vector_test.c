#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../q_math.h"
#include "../q_arch.h"


void test_vec3_vec3_mul() {
	struct {
		struct vec3_s v1, v2;
		struct vec3_s expected;
	} test[] = {
		{q_vec3_create(1, 0, 0), q_vec3_create(1, 1, 1), q_vec3_create(1, 0, 0)},
		{q_vec3_create(0, 0, 0), q_vec3_create(1, 1, 1), q_vec3_create(0, 0, 0)},
		{q_vec3_create(-1, -1, -1), q_vec3_create(1, 1, 1), q_vec3_create(-1, -1, -1)}
	};
	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		struct vec3_s res = q_vec_mul(test[i].v1, test[i].v2);
		assert_float_equal( 
			q_vec3_ele(0,res), 
			q_vec3_ele(0,test[i].expected ), 0.9999f );
		assert_float_equal( 
			q_vec3_ele(1,res), 
			q_vec3_ele(1,test[i].expected), 0.9999f );
		assert_float_equal( 
			q_vec3_ele( 2,res ), 
			q_vec3_ele( 2,test[i].expected), 0.9999f );
	}

}

void test_vec3_scalar_sub() {
	struct {
		struct vec3_s v1;
		float scalar;
		struct vec3_s expected;
	} test[] = {
		{q_vec3_create(1, 0, 0), 1, q_vec3_create(0, -1, -1)},
		{q_vec3_create(0, 0, 0), 1, q_vec3_create(-1, -1, -1)},
		{q_vec3_create(-1, -1, -1), 1, q_vec3_create(-2, -2, -2)}
	};

	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		struct vec3_s res = q_vec_sub(test[i].v1, test[i].scalar);
			assert_float_equal( 
				q_vec3_ele(0, res), 
				q_vec3_ele(0, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(1, res ), 
				q_vec3_ele(1, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(2, res ), 
				q_vec3_ele(2, test[i].expected ), 0.9999f );
	}

}

void test_vec3_scalar_add() {
	struct {
		struct vec3_s v1;
		float scalar;
		struct vec3_s expected;
	} test[] = {
		{q_vec3_create(1, 0, 0), 1, q_vec3_create(2, 1, 1)},
		{q_vec3_create(0, 0, 0), 1, q_vec3_create(1, 1, 1)},
		{q_vec3_create(-1, -1, -1), 1, q_vec3_create(0, 0, 0)}
	};

	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		struct vec3_s res = q_vec3_add_scalar(test[i].v1, test[i].scalar);
			assert_float_equal( 
				q_vec3_ele(0, res ), 
				q_vec3_ele(0, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(1, res ), 
				q_vec3_ele(1, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(2, res ), 
				q_vec3_ele(2, test[i].expected ), 0.9999f );
	}

}

void test_vec3_slerp() {
	struct {
		struct vec3_s v1, v2;
		float slerp;
		struct vec3_s expected;
	} test[] = {
  	{q_vec3_create(1.0f, 0.0f, 0.0f), q_vec3_create(0.0f, 1.0f, 0.0f), 0.5f, q_vec3_create(0.7071f, 0.7071f, 0.0f)},
	};
	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		struct vec3_s res = q_vec3_slerp(test[i].slerp,test[i].v1, test[i].v2);
			assert_float_equal( 
				q_vec3_ele(0, res ), 
				q_vec3_ele(0, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(1, res), 
				q_vec3_ele(1, test[i].expected), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(2, res ), 
				q_vec3_ele(2, test[i].expected ), 0.9999f );
	}

}

void test_vec3_cross() {
	struct {
		struct vec3_s v1, v2;
		struct vec3_s expected;
	} test[] = {
  	{q_vec3_create(1.0f, 2.0f, 3.0f), q_vec3_create(3.0f, 1.0f, -1.0f), q_vec3_create(-5.0f, 10.0f, -5.0f)},
	};

	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		struct vec3_s res = q_vec3_cross(test[i].v1, test[i].v2);
			assert_float_equal( 
				q_vec3_ele(0, res ), 
				q_vec3_ele(0, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(1, res ), 
				q_vec3_ele(1, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(2, res ), 
				q_vec3_ele(2, test[i].expected ), 0.9999f );
	}
}

void test_vec3_min_per_element() {
	struct {
		struct vec3_s v1, v2;
		struct vec3_s expected;
	} test[] = {
  	{q_vec3_create(1, 0, 0), q_vec3_create(0, 1, 1), q_vec3_create(0, 0, 0)},
    {q_vec3_create(1, 0, 0), q_vec3_create(0, 1, 0), q_vec3_create(0, 0, 0)}, // Orthogonal vectors
    {q_vec3_create(1, 1, 1), q_vec3_create(1, 1, 1), q_vec3_create(1, 1, 1)}, // Non-orthogonal, positive dot product
    {q_vec3_create(1, 0, 0), q_vec3_create(-1, 0, 0), q_vec3_create(-1, 0, 0)}, // Non-orthogonal, negative dot product
    {q_vec3_create(2, 3, 4), q_vec3_create(1, 0, 0), q_vec3_create(1, 0, 0)}, // Vectors with different lengths
    {q_vec3_create(0, 0, 0), q_vec3_create(0, 0, 0), q_vec3_create(0, 0, 0)}, // Zero vectors
	};

	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		struct vec3_s res = q_vec3_min_per_element(test[i].v1, test[i].v2);
			assert_float_equal( 
				q_vec3_ele(0, res ), 
				q_vec3_ele(0, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(1, res ), 
				q_vec3_ele(1, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(2, res ), 
				q_vec3_ele(2, test[i].expected ), 0.9999f );
	}

}

void test_vec3_max_per_element() {
	struct {
		struct vec3_s v1, v2;
		struct vec3_s expected;
	} test[] = {
  	{q_vec3_create(1, 0, 0), q_vec3_create(0, 1, 1), q_vec3_create(1, 1, 1)},
    {q_vec3_create(1, 0, 0), q_vec3_create(0, 1, 0), q_vec3_create(1, 1, 0)}, // Orthogonal vectors
    {q_vec3_create(1, 1, 1), q_vec3_create(1, 1, 1), q_vec3_create(1, 1, 1)}, // Non-orthogonal, positive dot product
    {q_vec3_create(1, 0, 0), q_vec3_create(-1, 0, 0), q_vec3_create(1, 0, 0)}, // Non-orthogonal, negative dot product
    {q_vec3_create(2, 3, 4), q_vec3_create(1, 0, 0), q_vec3_create(2, 3, 4)}, // Vectors with different lengths
    {q_vec3_create(0, 0, 0), q_vec3_create(0, 0, 0), q_vec3_create(0, 0, 0)}, // Zero vectors
	};

	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		struct vec3_s res = q_vec3_max_per_element(test[i].v1, test[i].v2);
			assert_float_equal( 
				q_vec3_ele(0, res ), 
				q_vec3_ele(0, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(1, res ), 
				q_vec3_ele(1, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(2, res ), 
				q_vec3_ele(2, test[i].expected ), 0.9999f );
	}

}


void test_vec3_dot() {
	struct {
		struct vec3_s v1, v2;
		float expected;
	} test[] = {
  	{q_vec3_create(1, 0, 0), q_vec3_create(0, 1, 1), 0},
    {q_vec3_create(1, 0, 0), q_vec3_create(0, 1, 0), 0}, // Orthogonal vectors
    {q_vec3_create(1, 1, 1), q_vec3_create(1, 1, 1), 3}, // Non-orthogonal, positive dot product
    {q_vec3_create(1, 0, 0), q_vec3_create(-1, 0, 0), -1}, // Non-orthogonal, negative dot product
    {q_vec3_create(2, 3, 4), q_vec3_create(1, 0, 0), 2}, // Vectors with different lengths
    {q_vec3_create(0, 0, 0), q_vec3_create(0, 0, 0), 0}, // Zero vectors
	};

	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		assert_float_equal(q_vec3_dot(test[i].v1, test[i].v2), test[i].expected, 0.9999f);
	}

}

void test_vec3_vec3_add() {
	struct {
		struct vec3_s v1, v2;
		struct vec3_s expected;
	} test[] = {
		{q_vec3_create(1, 0, 0), q_vec3_create(1, 1, 1), q_vec3_create(2, 1, 1)},
		{q_vec3_create(0, 0, 0), q_vec3_create(1, 1, 1), q_vec3_create(1, 1, 1)},
		{q_vec3_create(-1, -1, -1), q_vec3_create(1, 1, 1), q_vec3_create(0, 0, 0)}
	};

	for(size_t i = 0; i < Q_ARRAY_COUNT(test); i++){
		struct vec3_s res = q_vec_add(test[i].v1, test[i].v2);
			assert_float_equal( 
				q_vec3_ele(0, res ), 
				q_vec3_ele(0, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(1, res ), 
				q_vec3_ele(1, test[i].expected ), 0.9999f );
			assert_float_equal( 
				q_vec3_ele(2, res ), 
				q_vec3_ele(2, test[i].expected ), 0.9999f );
	}
}

int main( void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test( test_vec3_vec3_add ),
		cmocka_unit_test( test_vec3_vec3_mul ),
		cmocka_unit_test( test_vec3_scalar_add ),
		cmocka_unit_test( test_vec3_scalar_sub ),
		cmocka_unit_test( test_vec3_dot ),
		cmocka_unit_test( test_vec3_max_per_element ),
		cmocka_unit_test( test_vec3_min_per_element ),
		cmocka_unit_test( test_vec3_cross),
		cmocka_unit_test( test_vec3_slerp),
	};

	return cmocka_run_group_tests( tests, NULL, NULL );
}

#include "unity/unity.h"
#include "coop.h"
#include <string.h>
#include <stdbool.h>

$struct(Animal)
  int legs;
  int (*count_legs)(Animal *self);
  void (*destroy)(Animal *self);
$end

$struct(AnimalImpl, Animal)
  int bonus_legs;
$end

static int Animal_count_legs(Animal *self) {
  return self->legs + ((AnimalImpl *) self)->bonus_legs;
}

// Interface with two embedded interface structs to test owning-struct pattern
$struct(Walker)
  int (*step)(Walker *self);
$end

$struct(Talker)
  void (*speak)(Talker *self);
$end

$struct(CreatureImpl, Walker)
  Talker talker;
  int steps;
  int words;
$end

// For double-evaluation test
static int call_count = 0;
static Animal *get_animal(Animal *a) {
  call_count++;
  return a;
}

// ============================================================
// $struct
// ============================================================

void test_interface_ptr_cast_to_void_ptr_gives_same_address(void) {
  Animal *a = malloc(sizeof(Animal));
  TEST_ASSERT_EQUAL_PTR(a, (void *)a);
  free(a);
}

void test_interface_unspecified_fields_are_zero_initialized(void) {
  $auto Animal *a = $new(Animal);
  TEST_ASSERT_EQUAL_INT(0, a->legs);
  TEST_ASSERT_NULL(a->count_legs);
  TEST_ASSERT_NULL(a->destroy);
}

// ============================================================
// $struct with extension
// ============================================================

void test_implementation_first_member_is_BASE_MEMBER(void) {
  AnimalImpl impl;
  TEST_ASSERT_EQUAL_PTR(&impl, &impl.$BASE);
}

void test_implementation_BASE_MEMBER_has_correct_interface_type(void) {
  AnimalImpl impl;
  TEST_ASSERT_EQUAL_INT(sizeof(Animal), sizeof(impl.$BASE));
}

void test_implementation_ptr_cast_to_interface_gives_same_address(void) {
  AnimalImpl *impl = malloc(sizeof(AnimalImpl));
  TEST_ASSERT_EQUAL_PTR(impl, (Animal *)impl);
  free(impl);
}

void test_implementation_mutation_through_interface_reflected_in_impl(void) {
  $auto AnimalImpl *impl = $new(AnimalImpl, .$BASE.legs=4);
  Animal *a = (Animal *)impl;
  a->legs = 6;
  TEST_ASSERT_EQUAL_INT(6, impl->$BASE.legs);
}

// ============================================================
// $ / $call
// ============================================================

void test_call_invokes_method_with_no_extra_args(void) {
  $auto AnimalImpl *impl = $new(
    AnimalImpl,
    .$BASE.legs=4,
    .$BASE.count_legs=Animal_count_legs,
    .bonus_legs=0,
  );
  Animal *a = (Animal *)impl;
  int result = $(a, count_legs);
  TEST_ASSERT_EQUAL_INT(4, result);
}

$struct(Adder)
  int (*add)(Adder *self, int n);
$end

static int Adder_add(Adder *self, int n) { (void)self; return n + 1; }

void test_call_invokes_method_with_one_arg(void) {
  $auto Adder *a = $new(Adder, .add=Adder_add);
  TEST_ASSERT_EQUAL_INT(6, $(a, add, 5));
}

void test_call_return_value_is_propagated(void) {
  $auto AnimalImpl *impl = $new(
    AnimalImpl,
    .$BASE.legs=3,
    .$BASE.count_legs=Animal_count_legs,
    .bonus_legs=2,
  );
  Animal *a = (Animal *)impl;
  int result = $(a, count_legs);
  TEST_ASSERT_EQUAL_INT(5, result);
}

void test_call_obj_is_evaluated_exactly_once(void) {
  $auto AnimalImpl *impl = $new(
    AnimalImpl,
    .$BASE.count_legs=Animal_count_legs,
  );
  Animal *a = (Animal *)impl;
  call_count = 0;
  $(get_animal(a), count_legs);
  TEST_ASSERT_EQUAL_INT(1, call_count);
}

// ============================================================
// $new
// ============================================================

void test_new_unspecified_fields_are_zero_initialized(void) {
  $auto AnimalImpl *impl = $new(AnimalImpl);
  TEST_ASSERT_EQUAL_INT(0, impl->$BASE.legs);
  TEST_ASSERT_EQUAL_INT(0, impl->bonus_legs);
}

void test_new_returns_correct_pointer_type(void) {
  $auto Animal *a = $new(Animal);
  // compiles only if $new returns Animal * — pointer arithmetic confirms type
  Animal *next = a + 1;
  TEST_ASSERT_EQUAL_PTR((char *)a + sizeof(Animal), (char *)next);
}

typedef struct { int x; int y; } Point;
$struct(NestedStruct)
  Point point ;
$end

void test_new_nested_struct_can_be_initialized_with_positional_values(void) {
  $auto NestedStruct *obj = $new(NestedStruct, 0, NULL,1, 2);
  TEST_ASSERT_EQUAL_INT(1, obj->point.x);
  TEST_ASSERT_EQUAL_INT(2, obj->point.y);
}

void test_new_nested_struct_can_be_initialized_with_fully_qualified_named_values(void) {
  $auto NestedStruct *obj = $new(NestedStruct, .point.x=1, .point.y=2);
  TEST_ASSERT_EQUAL_INT(1, obj->point.x);
  TEST_ASSERT_EQUAL_INT(2, obj->point.y);
}

void test_new_nested_struct_can_be_initialized_with_positional_nested_struct_literal(void) {
  $auto NestedStruct *obj = $new(NestedStruct, 0, NULL, {1, 2});
  TEST_ASSERT_EQUAL_INT(1, obj->point.x);
  TEST_ASSERT_EQUAL_INT(2, obj->point.y);
}

void test_new_nested_struct_can_be_initialized_with_named_nested_struct_literal(void) {
  $auto NestedStruct *obj = $new(NestedStruct, .point={1, 2});
  TEST_ASSERT_EQUAL_INT(1, obj->point.x);
  TEST_ASSERT_EQUAL_INT(2, obj->point.y);
}

void test_new_nested_struct_can_be_initialized_with_nested_struct_literal(void) {
  $auto NestedStruct *obj = $new(NestedStruct, .point={1, 2});
  TEST_ASSERT_EQUAL_INT(1, obj->point.x);
  TEST_ASSERT_EQUAL_INT(2, obj->point.y);
}

$struct(NestedFixedArray)
  int values[2];
$end

void test_new_nested_fixed_array_can_be_initialized_with_positional_values(void) {
  $auto NestedFixedArray *obj = $new(NestedFixedArray, 0, NULL, 1, 2);
  TEST_ASSERT_EQUAL_INT(1, obj->values[0]);
  TEST_ASSERT_EQUAL_INT(2, obj->values[1]);
}

void test_new_nested_fixed_array_can_be_initialized_with_fully_qualified_named_values(void) {
  $auto NestedFixedArray *obj = $new(NestedFixedArray, .values[0]=1, .values[1]=2);
  TEST_ASSERT_EQUAL_INT(1, obj->values[0]);
  TEST_ASSERT_EQUAL_INT(2, obj->values[1]);
}

void test_new_nested_fixed_array_can_be_initialized_with_positional_nested_array_literal(void) {
  $auto NestedFixedArray *obj = $new(NestedFixedArray, 0, NULL, { 1, 2});
  TEST_ASSERT_EQUAL_INT(1, obj->values[0]);
  TEST_ASSERT_EQUAL_INT(2, obj->values[1]);
}

void test_new_nested_fixed_array_can_be_initialized_with_named_nested_array_literal(void) {
  $auto NestedFixedArray *obj = $new(NestedFixedArray, .values = { 1, 2});
  TEST_ASSERT_EQUAL_INT(1, obj->values[0]);
  TEST_ASSERT_EQUAL_INT(2, obj->values[1]);
}

// ============================================================
// $_init
// ============================================================

void test_init_unspecified_fields_are_zero_initialized(void) {
  Animal *a = malloc(sizeof(Animal));
  $_init(a, Animal, .legs=4);
  TEST_ASSERT_NULL(a->count_legs);
  TEST_ASSERT_NULL(a->destroy);
  free(a);
}

void test_init_does_not_allocate_new_memory(void) {
  Animal *a = malloc(sizeof(Animal));
  Animal *result = $_init(a, Animal);
  TEST_ASSERT_EQUAL_PTR(a, result);
  free(a);
}

void test_init_works_with_arena_allocated_memory(void) {
  // Minimal inline arena to avoid pulling in the full arena header
  void *pool = malloc(1024);
  Animal *a = pool;
  $_init(a, Animal, .legs=6);
  TEST_ASSERT_EQUAL_INT(6, a->legs);
  free(pool);
}

typedef struct { int length; int data[]; } NestedFlexibleArray;

void test_init_nested_flexible_array_can_be_initialized_with_injected_literal(void) {
  int data[] = { 1, 2, 3, 4, 5 };
  NestedFlexibleArray *obj = malloc(sizeof(NestedFlexibleArray) + sizeof(data));
  $_init(obj, NestedFlexibleArray, 5);
  memcpy(&obj->data, &data, sizeof(data));
  TEST_ASSERT_EQUAL_INT(5, obj->length);
  TEST_ASSERT_EQUAL_INT_ARRAY(data, obj->data, 5);
  free(obj);
}

// ============================================================
// $retain / $release
// ============================================================

typedef struct {
  bool destroyed;
} RcTestData;

$struct(RcObj)
  char *msg;
  RcTestData *data;
  void (*destroy)(RcObj *self);
$end

void RcObj_destroy(void *self_) {
  RcObj *self = self_;
  if (self->data) self->data->destroyed = true;
}

void test_retain_increments_ref_counter(void) {
  RcObj *obj = $new(RcObj, .$DESTROY=RcObj_destroy);
  TEST_ASSERT_EQUAL_INT(1, obj->$_REF_COUNT_MEMBER);
  $retain(obj);
  TEST_ASSERT_EQUAL_INT(2, obj->$_REF_COUNT_MEMBER);
  $release(obj);
  $release(obj);
}

void test_retain_arg_is_evaluated_once(void) {
  RcObj *obj = $retain($new(RcObj, .$DESTROY=RcObj_destroy));
  TEST_ASSERT_EQUAL_INT(2, obj->$_REF_COUNT_MEMBER);
  $release(obj);
  $release(obj);
}

void test_release_decrements_ref_counter(void) {
  RcObj *obj = $new(RcObj, .$DESTROY=RcObj_destroy);
  $retain(obj);
  $retain(obj);
  TEST_ASSERT_EQUAL_INT(3, obj->$_REF_COUNT_MEMBER);
  $release(obj);
  TEST_ASSERT_EQUAL_INT(2, obj->$_REF_COUNT_MEMBER);
  $release(obj);
  $release(obj);
}

void test_release_destroys_object(void) {
  RcTestData data = {};
  RcObj *obj = $new(RcObj, .$DESTROY=RcObj_destroy, .data=&data);
  TEST_ASSERT_EQUAL_INT(false, data.destroyed);
  $release(obj);
  TEST_ASSERT_EQUAL_INT(true, data.destroyed);
}

 void test_release_does_not_destroy_retained_object(void) {
   RcTestData data = {};
   RcObj *obj = $new(RcObj, .$DESTROY=RcObj_destroy, .data=&data);
   $retain(obj);
   $release(obj);
   TEST_ASSERT_EQUAL_INT(false, data.destroyed);
   $release(obj);
 }

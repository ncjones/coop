#include "unity/unity.h"
#include "coop.h"
#include <signal.h>
#include <stdio.h>

void setUp(void) {
  $rc_retains = 0;
  $rc_releases = 0;
}

void tearDown(void) {
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    $rc_retains,
    $rc_releases,
    "Unreleased Object Reference Detected!"
  );
}

static void crash_handler(int sig) {
  if (Unity.CurrentTestName) {
    fprintf(
      stderr,
      "%s:%ld:%s:FAIL: Crash (Sig %d)\n",
      Unity.TestFile,
      Unity.CurrentTestLineNumber,
      Unity.CurrentTestName,
      sig
    );
  }
  signal(sig, SIG_DFL);
  raise(sig); // re-raise so the process exits with the correct signal status
}

static void install_crash_handler(void) {
  signal(SIGSEGV, crash_handler);
  signal(SIGABRT, crash_handler);
  signal(SIGBUS,  crash_handler);
}

void test_interface_ptr_cast_to_void_ptr_gives_same_address(void);
void test_interface_unspecified_fields_are_zero_initialized(void);
void test_implementation_first_member_is_BASE_MEMBER(void);
void test_implementation_BASE_MEMBER_has_correct_interface_type(void);
void test_implementation_ptr_cast_to_interface_gives_same_address(void);
void test_implementation_mutation_through_interface_reflected_in_impl(void);
void test_call_invokes_method_with_no_extra_args(void);
void test_call_invokes_method_with_one_arg(void);
void test_call_return_value_is_propagated(void);
void test_call_obj_is_evaluated_exactly_once(void);
void test_new_unspecified_fields_are_zero_initialized(void);
void test_new_returns_correct_pointer_type(void);
void test_new_nested_struct_can_be_initialized_with_positional_values(void);
void test_new_nested_struct_can_be_initialized_with_fully_qualified_named_values(void);
void test_new_nested_struct_can_be_initialized_with_positional_nested_struct_literal(void);
void test_new_nested_struct_can_be_initialized_with_named_nested_struct_literal(void);
void test_new_nested_struct_can_be_initialized_with_nested_struct_literal(void);
void test_new_nested_fixed_array_can_be_initialized_with_positional_values(void);
void test_new_nested_fixed_array_can_be_initialized_with_fully_qualified_named_values(void);
void test_new_nested_fixed_array_can_be_initialized_with_positional_nested_array_literal(void);
void test_new_nested_fixed_array_can_be_initialized_with_named_nested_array_literal(void);
void test_init_unspecified_fields_are_zero_initialized(void);
void test_init_does_not_allocate_new_memory(void);
void test_init_works_with_arena_allocated_memory(void);
void test_init_nested_flexible_array_can_be_initialized_with_injected_literal(void);
void test_retain_increments_ref_counter(void);
void test_retain_arg_is_evaluated_once(void);
void test_release_decrements_ref_counter(void);
void test_release_destroys_object(void);
void test_release_does_not_destroy_retained_object(void);
void test_release_is_statement_safe_in_unbraced_if_else(void);

int main(void) {
  install_crash_handler();
  UNITY_BEGIN();
  RUN_TEST(test_interface_ptr_cast_to_void_ptr_gives_same_address);
  RUN_TEST(test_interface_unspecified_fields_are_zero_initialized);
  RUN_TEST(test_implementation_first_member_is_BASE_MEMBER);
  RUN_TEST(test_implementation_BASE_MEMBER_has_correct_interface_type);
  RUN_TEST(test_implementation_ptr_cast_to_interface_gives_same_address);
  RUN_TEST(test_implementation_mutation_through_interface_reflected_in_impl);
  RUN_TEST(test_call_invokes_method_with_no_extra_args);
  RUN_TEST(test_call_invokes_method_with_one_arg);
  RUN_TEST(test_call_return_value_is_propagated);
  RUN_TEST(test_call_obj_is_evaluated_exactly_once);
  RUN_TEST(test_new_unspecified_fields_are_zero_initialized);
  RUN_TEST(test_new_returns_correct_pointer_type);
  RUN_TEST(test_new_nested_struct_can_be_initialized_with_positional_values);
  RUN_TEST(test_new_nested_struct_can_be_initialized_with_fully_qualified_named_values);
  RUN_TEST(test_new_nested_struct_can_be_initialized_with_positional_nested_struct_literal);
  RUN_TEST(test_new_nested_struct_can_be_initialized_with_named_nested_struct_literal);
  RUN_TEST(test_new_nested_struct_can_be_initialized_with_nested_struct_literal);
  RUN_TEST(test_new_nested_fixed_array_can_be_initialized_with_positional_values);
  RUN_TEST(test_new_nested_fixed_array_can_be_initialized_with_fully_qualified_named_values);
  RUN_TEST(test_new_nested_fixed_array_can_be_initialized_with_positional_nested_array_literal);
  RUN_TEST(test_new_nested_fixed_array_can_be_initialized_with_named_nested_array_literal);
  RUN_TEST(test_init_unspecified_fields_are_zero_initialized);
  RUN_TEST(test_init_does_not_allocate_new_memory);
  RUN_TEST(test_init_works_with_arena_allocated_memory);
  RUN_TEST(test_init_nested_flexible_array_can_be_initialized_with_injected_literal);
  RUN_TEST(test_retain_increments_ref_counter);
  RUN_TEST(test_retain_arg_is_evaluated_once);
  RUN_TEST(test_release_decrements_ref_counter);
  RUN_TEST(test_release_destroys_object);
  RUN_TEST(test_release_does_not_destroy_retained_object);
  RUN_TEST(test_release_is_statement_safe_in_unbraced_if_else);
  return UNITY_END();
}

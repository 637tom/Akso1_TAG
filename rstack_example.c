#ifdef NDEBUG
#undef NDEBUG
#endif

#include "rstack.h"
#include "memory_tests.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/** MAKRA SKRACAJĄCE IMPLEMENTACJĘ TESTÓW **/

// To są możliwe wyniki testu.
#define PASS 0
#define FAIL 1
#define WRONG_TEST 2

// Oblicza liczbę elementów tablicy x.
#define SIZE(x) (sizeof x / sizeof x[0])

#define ASSERT(f)            \
  do {                       \
    if (!(f))                \
      return FAIL;           \
  } while (0)

#define ASSERT_RESULT(c, f, ...)          \
  do {                                    \
    result_t r = c;                       \
    if (r.flag != (f))                    \
      return FAIL;                        \
    if ((f) && r.value != __VA_ARGS__ -0) \
      return FAIL;                        \
  } while (0)

#define CHECK_IF_NO_ERROR(f) \
  do {                       \
    if ((f) != 0)            \
      return FAIL;           \
  } while (0)

#define V(code, where) (((unsigned long)code) << (3 * where))

/** WŁAŚCIWE TESTY **/

static int zero(void) {
  rstack_t *rs0 = rstack_new();
  assert(rs0);

  ASSERT(rstack_empty(rs0) == true);
  ASSERT_RESULT(rstack_front(rs0), false);
  CHECK_IF_NO_ERROR(rstack_write("file_zero.out", rs0));
  rstack_delete(rs0);

  return PASS;
}

static int one(void) {
  rstack_t *rs0 = rstack_new();
  assert(rs0);
  rstack_t *rs1 = rstack_new();
  assert(rs1);

  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 1));
  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 2));
  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 0));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs1, rs0));
  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 3));
  ASSERT(rstack_empty(rs0) == false);
  ASSERT_RESULT(rstack_front(rs0), true, 3);
  rstack_delete(rs0);
  CHECK_IF_NO_ERROR(rstack_push_value(rs1, 4));
  CHECK_IF_NO_ERROR(rstack_write("file_one.out", rs1));
  ASSERT(rstack_empty(rs1) == false);
  ASSERT_RESULT(rstack_front(rs1), true, 4);
  rstack_delete(rs1);

  return PASS;
}

static int two(void) {
  rstack_t *rs1 = rstack_new();
  assert(rs1);
  rstack_t *rs2 = rstack_new();
  assert(rs2);

  CHECK_IF_NO_ERROR(rstack_push_rstack(rs1, rs2));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs1));
  ASSERT(rstack_empty(rs1) == true);
  ASSERT(rstack_empty(rs2) == true);
  ASSERT_RESULT(rstack_front(rs1), false);
  ASSERT_RESULT(rstack_front(rs2), false);
  rstack_delete(rs1);
  rstack_delete(rs2);

  rs2 = rstack_new();
  assert(rs2);

  CHECK_IF_NO_ERROR(rstack_push_value(rs2, 4));
  CHECK_IF_NO_ERROR(rstack_push_value(rs2, 5));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs2, rs2));
  ASSERT(rstack_empty(rs2) == false);
  ASSERT_RESULT(rstack_front(rs2), true, 5);
  CHECK_IF_NO_ERROR(rstack_push_value(rs2, 6));
  CHECK_IF_NO_ERROR(rstack_write("file_two.out", rs2));
  ASSERT(rstack_empty(rs2) == false);
  ASSERT_RESULT(rstack_front(rs2), true, 6);
  rstack_delete(rs2);

  return PASS;
}

static int three(void) {
  rstack_t *rs0 = rstack_new();
  assert(rs0);
  rstack_t *rs3 = rstack_new();
  assert(rs3);

  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 7));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs3, rs0));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs3, rs0));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs3, rs0));
  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 8));
  CHECK_IF_NO_ERROR(rstack_push_value(rs0, 9));
  ASSERT(rstack_empty(rs3) == false);
  ASSERT(rstack_empty(rs0) == false);
  ASSERT_RESULT(rstack_front(rs3), true, 9);
  ASSERT_RESULT(rstack_front(rs0), true, 9);
  rstack_pop(rs3);
  rstack_pop(rs0);
  CHECK_IF_NO_ERROR(rstack_write("file_three.out", rs3));
  ASSERT(rstack_empty(rs3) == false);
  ASSERT(rstack_empty(rs0) == false);
  ASSERT_RESULT(rstack_front(rs3), true, 8);
  ASSERT_RESULT(rstack_front(rs0), true, 8);
  rstack_delete(rs3);
  rstack_delete(rs0);

  return PASS;
}

static int four(void) {
  rstack_t *rs4 = rstack_read("file_four.in");
  assert(rs4);

  CHECK_IF_NO_ERROR(rstack_write("file_four.out", rs4));
  rstack_delete(rs4);

  return PASS;
}

static int five(void) {
  rstack_t *rs5 = rstack_new();
  assert(rs5);
  rstack_t *rs6 = rstack_new();
  assert(rs6);
  rstack_t *rs7 = rstack_new();
  assert(rs7);

  CHECK_IF_NO_ERROR(rstack_push_value(rs5, 5));
  CHECK_IF_NO_ERROR(rstack_push_value(rs6, 6));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs6, rs6));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs7, rs7));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs6, rs7));
  CHECK_IF_NO_ERROR(rstack_push_rstack(rs5, rs6));
  ASSERT(rstack_empty(rs5) == false);
  ASSERT(rstack_empty(rs6) == false);
  ASSERT(rstack_empty(rs7) == true);
  ASSERT_RESULT(rstack_front(rs5), true, 6);
  ASSERT_RESULT(rstack_front(rs6), true, 6);
  ASSERT_RESULT(rstack_front(rs7), false);
  CHECK_IF_NO_ERROR(rstack_write("file_five.out", rs5));
  rstack_delete(rs5);
  rstack_delete(rs6);
  rstack_delete(rs7);

  return PASS;
}

// Testuje reakcję implementacji na niepowodzenie alokacji pamięci.
// Błąd alokacji jest zgłaszany raz. Druga próba powinna się udać.
static unsigned long alloc_fail_test(void) {
  unsigned long visited = 0;
  rstack_t *rs;

  errno = 0;
  if ((rs = rstack_new()) != nullptr)
    visited |= V(1, 0);
  else if (errno == ENOMEM && (rs = rstack_new()) != nullptr)
    visited |= V(2, 0);
  else
    return visited |= V(4, 0); // To nie powinno się wykonać.

  errno = 0;
  if (rstack_push_value(rs, 1) == 0)
    visited |= V(1, 1);
  else if (errno == ENOMEM && rstack_push_value(rs, 1) == 0)
    visited |= V(2, 1);
  else
    return visited |= V(4, 1); // To nie powinno się wykonać.

  errno = 0;
  if (rstack_write("file_memory.out", rs) == 0)
    visited |= V(1, 2);
  else if (errno != 0 && rstack_write("file_memory.out", rs) == 0)
    visited |= V(2, 2);
  else
    return visited |= V(4, 2); // To nie powinno się wykonać.

  rstack_delete(rs);

  return visited;
}

// Testuje reakcję implementacji na niepowodzenie alokacji pamięci.
static int memory_test(unsigned long (* test_function)(void)) {
  memory_test_data_t *mtd = get_memory_test_data();

  unsigned fail = 0, pass = 0;
  mtd->call_total = 0;
  mtd->fail_counter = 1;
  while (fail < 3 && pass < 3) {
    mtd->call_counter = 0;
    mtd->alloc_counter = 0;
    mtd->free_counter = 0;
    mtd->function_name = NULL;
    unsigned long visited_points = test_function();
    if (mtd->alloc_counter != mtd->free_counter ||
        (visited_points & 0444444444444444444444UL) != 0) {
      fprintf(stderr,
              "fail_counter %u, alloc_counter %u, free_counter %u, "
              "function_name %s, visited_point %lo\n",
              mtd->fail_counter, mtd->alloc_counter, mtd->free_counter,
              mtd->function_name, visited_points);
      ++fail;
    }
    if (mtd->function_name == NULL)
      ++pass;
    else
      pass = 0;
    mtd->fail_counter++;
  }

  return mtd->call_total > 0 && fail == 0 ? PASS : FAIL;
}

// Testuje reakcję implementacji na niepowodzenie alokacji pamięci.
static int memory(void) {
  memory_tests_check();
  return memory_test(alloc_fail_test);
}

/** URUCHAMIANIE TESTÓW **/

typedef struct {
  char const *name;
  int (*function)(void);
} test_list_t;

#define TEST(t) {#t, t}

static const test_list_t test_list[] = {
  TEST(zero),
  TEST(one),
  TEST(two),
  TEST(three),
  TEST(four),
  TEST(five),
  TEST(memory),
};

static int do_test(int (*function)(void)) {
  int result = function();
  puts("quite long magic string");
  return result;
}

int main(int argc, char *argv[]) {
  if (argc == 2)
    for (size_t i = 0; i < SIZE(test_list); ++i)
      if (strcmp(argv[1], test_list[i].name) == 0)
        return do_test(test_list[i].function);

  fprintf(stderr, "Użycie:\n%s nazwa_testu\n", argv[0]);
  return WRONG_TEST;
}

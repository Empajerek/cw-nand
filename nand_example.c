#ifdef NDEBUG
#undef NDEBUG
#endif

#include "nand.h"
#include "memory_tests.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>

/** MAKRA SKRACAJĄCE IMPLEMENTACJĘ TESTÓW **/

// To są możliwe wyniki testu.
#define PASS 0
#define FAIL 1
#define WRONG_TEST 2

// Oblicza liczbę elementów tablicy x.
#define SIZE(x) (sizeof x / sizeof x[0])

#define TEST_ECANCELED(f)                \
  do {                                   \
    errno = 0;                           \
    if ((f) != -1 || errno != ECANCELED) \
      return FAIL;                       \
  } while (0)

#define TEST_PASS(f)                     \
  do {                                   \
    if ((f) != 0)                        \
      return FAIL;                       \
  } while (0)

#define ASSERT(f)                        \
  do {                                   \
    if (!(f))                            \
      return FAIL;                       \
  } while (0)

#define V(code, where) (((unsigned long)code) << (3 * where))

/** WŁAŚCIWE TESTY **/

// To jest przykładowy test udostępniony studentom.
int testy();

static int example(void) {
  nand_t *g[3];
  ssize_t path_len;
  bool s_in[2], s_out[3];

  g[0] = nand_new(2);
  g[1] = nand_new(2);
  g[2] = nand_new(2);
  assert(g[0]);
  assert(g[1]);
  assert(g[2]);

  TEST_PASS(nand_connect_nand(g[2], g[0], 0));
  TEST_PASS(nand_connect_nand(g[2], g[0], 1));
  TEST_PASS(nand_connect_nand(g[2], g[1], 0));

  TEST_PASS(nand_connect_signal(s_in + 0, g[2], 0));
  TEST_PASS(nand_connect_signal(s_in + 1, g[2], 1));
  TEST_PASS(nand_connect_signal(s_in + 1, g[1], 1));

  ASSERT(0 == nand_fan_out(g[0]));
  ASSERT(0 == nand_fan_out(g[1]));
  ASSERT(3 == nand_fan_out(g[2]));

  int c[3] = {0};
  for (ssize_t i = 0; i < 3; ++i) {
    nand_t *t = nand_output(g[2], i);
    for (int j = 0; j < 3; ++j)
      if (g[j] == t)
        c[j]++;
  }
  ASSERT(c[0] == 2 && c[1] == 1 && c[2] == 0);

  s_in[0] = false, s_in[1] = false;
  path_len = nand_evaluate(g, s_out, 3);
  ASSERT(path_len == 2 && s_out[0] == false && s_out[1] == true && s_out[2] == true);

  s_in[0] = true, s_in[1] = false;
  path_len = nand_evaluate(g, s_out, 3);
  ASSERT(path_len == 2 && s_out[0] == false && s_out[1] == true && s_out[2] == true);

  s_in[0] = false, s_in[1] = true;
  path_len = nand_evaluate(g, s_out, 3);
  ASSERT(path_len == 2 && s_out[0] == false && s_out[1] == false && s_out[2] == true);

  s_in[0] = true, s_in[1] = true;
  path_len = nand_evaluate(g, s_out, 3);
  ASSERT(path_len == 2 && s_out[0] == true && s_out[1] == true && s_out[2] == false);

  nand_delete(g[0]);
  nand_delete(g[1]);
  nand_delete(g[2]);
  return PASS;
}

// Testuje proste przypadki.
static int simple(void) {
  bool s[1];

  nand_t *g0 = nand_new(0);
  nand_t *g1 = nand_new(1);
  nand_t *g2 = nand_new(100);
  assert(g0);
  assert(g1);
  assert(g2);

  TEST_PASS(nand_connect_nand(g1, g1, 0));

  ASSERT(0 == nand_evaluate(&g0, s, 1));
  ASSERT(s[0] == false);

  TEST_ECANCELED(nand_evaluate(&g1, s, 1));
  TEST_ECANCELED(nand_evaluate(&g2, s, 1));

  nand_delete(NULL);
  nand_delete(g0);
  nand_delete(g1);
  nand_delete(g2);
  return PASS;
}

// Testuje reakcję implementacji na niepowodzenie alokacji pamięci.
static unsigned long alloc_fail_test(void) {
  unsigned long visited = 0;
  nand_t *nand1, *nand2;
  ssize_t len;
  int result;
  bool s_in[2], s_out[1];

  errno = 0;
  if ((nand1 = nand_new(2)) != NULL)
    visited |= V(1, 0);
  else if (errno == ENOMEM && (nand1 = nand_new(2)) != NULL)
    visited |= V(2, 0);
  else
    return visited |= V(4, 0);

  errno = 0;
  if ((nand2 = nand_new(1)) != NULL)
    visited |= V(1, 1);
  else if (errno == ENOMEM && (nand2 = nand_new(1)) != NULL)
    visited |= V(2, 1);
  else
    return visited |= V(4, 1);

  errno = 0;
  if ((result = nand_connect_nand(nand2, nand1, 0)) == 0)
    visited |= V(1, 2);
  else if (result == -1 && errno == ENOMEM && nand_connect_nand(nand2, nand1, 0) == 0)
    visited |= V(2, 2);
  else
    return visited |= V(4, 2);

  errno = 0;
  if ((result = nand_connect_signal(&s_in[1], nand1, 1)) == 0)
    visited |= V(1, 3);
  else if (result == -1 && errno == ENOMEM && nand_connect_signal(&s_in[1], nand1, 1) == 0)
    visited |= V(2, 3);
  else
    return visited |= V(4, 3);

  errno = 0;
  if ((result = nand_connect_signal(&s_in[0], nand2, 0)) == 0)
    visited |= V(1, 4);
  else if (result == -1 && errno == ENOMEM && nand_connect_signal(&s_in[0], nand2, 0) == 0)
    visited |= V(2, 4);
  else
    return visited |= V(4, 4);

  s_in[0] = false;
  s_in[1] = false;
  errno = 0;
  if ((len = nand_evaluate(&nand1, s_out, 1)) == 2 && s_out[0] == true)
    visited |= V(1, 5);
  else if (len == -1 && (errno == ENOMEM || errno == ECANCELED) && nand_evaluate(&nand1, s_out, 1) == 2 && s_out[0] == true)
    visited |= V(2, 5);
  else
    return visited |= V(4, 5);

  s_in[0] = false;
  s_in[1] = true;
  errno = 0;
  if ((len = nand_evaluate(&nand1, s_out, 1)) == 2 && s_out[0] == false)
    visited |= V(1, 6);
  else if (len == -1 && (errno == ENOMEM || errno == ECANCELED) && nand_evaluate(&nand1, s_out, 1) == 2 && s_out[0] == false)
    visited |= V(2, 6);
  else
    return visited |= V(4, 6);

  nand_delete(nand1);
  nand_delete(nand2);

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
  TEST(example),
  TEST(simple),
  TEST(memory),
  TEST(testy),
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

// #define TEST1
// #define TEST2
// #define TEST3
// #define TEST4
// #define TEST5
// #define TEST6
#define TEST7
#define TEST8
#define TEST9
#define TEST10
#define TEST11
#define TEST12
#define TEST13
#define TEST14
#define TEST15
#define TEST16
#define TEST17
#define TEST18

int testy() {

    // TESTY NA POPRAWNOŚĆ POŁĄCZEŃ, ZAKŁADAJĄC BRAK BŁĘDÓW ////////////////////////////////////////////////////////////////////////////////////////////

    printf("Uwaga! Najlepiej uruchamiac z valgrindem\n");

    // TEST 1
#ifdef TEST1
    {
        nand_t* g0 = nand_new(7);
        nand_t* g1 = nand_new(0);
        nand_t* g2 = nand_new(1);

        bool b = false;

        assert(g0 != NULL && g1 != NULL && g2 != NULL);

        errno = 1;
        bool uninitialed_input = nand_input(g0, 6) != NULL || nand_input(g2, 0) != NULL;
        bool errno_not_set_to_zero = errno != 0; // nand_input() ma ustawiać errno na zero w przypadku gdy zwraca pustą wartość

        assert(!uninitialed_input && !errno_not_set_to_zero);

        // krok 1
        nand_connect_signal(&b, g0, 0);
        nand_connect_nand(g1, g0, 2);
        nand_connect_nand(g1, g0, 4);
        nand_connect_nand(g2, g0, 3);
        nand_connect_nand(g0, g0, 5);
        nand_connect_nand(g0, g0, 6);

        assert(nand_input(g0, 0) == &b);
        assert(nand_input(g0, 1) == NULL);
        assert(nand_input(g0, 2) == g1);
        assert(nand_input(g0, 3) == g2);
        assert(nand_input(g0, 4) == g1);
        assert(nand_input(g0, 5) == g0);
        assert(nand_input(g0, 6) == g0);

        assert(nand_output(g1, 0) == g0);
        assert(nand_output(g1, 1) == g0);
        assert(nand_output(g2, 0) == g0);
        assert(nand_output(g0, 0) == g0);
        assert(nand_output(g0, 1) == g0);

        assert(nand_fan_out(g0) == 2);
        assert(nand_fan_out(g1) == 2);
        assert(nand_fan_out(g2) == 1);

        // krok 2
        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);

        printf("przeszedl TEST 1\n");
    }
#endif

    // TEST 2
#ifdef TEST2
    {
        nand_t* g0 = nand_new(7);
        nand_t* g1 = nand_new(0);
        nand_t* g2 = nand_new(1);

        bool b = false;

        // krok 1
        nand_connect_signal(&b, g0, 0);
        nand_connect_nand(g1, g0, 2);
        nand_connect_nand(g1, g0, 4);
        nand_connect_nand(g2, g0, 3);
        nand_connect_nand(g0, g0, 5);
        nand_connect_nand(g0, g0, 6);

        // krok 2
        nand_connect_signal(&b, g0, 0);
        nand_connect_signal(&b, g0, 1);
        nand_connect_signal(&b, g0, 2);
        nand_connect_signal(&b, g0, 3);
        nand_connect_signal(&b, g0, 4);
        nand_connect_signal(&b, g0, 5);
        nand_connect_signal(&b, g0, 6);

        assert(nand_input(g0, 0) == &b);
        assert(nand_input(g0, 1) == &b);
        assert(nand_input(g0, 2) == &b);
        assert(nand_input(g0, 3) == &b);
        assert(nand_input(g0, 4) == &b);
        assert(nand_input(g0, 5) == &b);
        assert(nand_input(g0, 6) == &b);

        assert(nand_fan_out(g0) == 0);
        assert(nand_fan_out(g1) == 0);
        assert(nand_fan_out(g2) == 0);

        // krok 3
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g0);

        printf("przeszedl TEST 2\n");
    }
#endif

    // TEST 3
#ifdef TEST3
    {
        nand_t* g0 = nand_new(7);
        nand_t* g1 = nand_new(0);
        nand_t* g2 = nand_new(1);

        bool b = false;

        // krok 1
        nand_connect_signal(&b, g0, 0);
        nand_connect_nand(g1, g0, 2);
        nand_connect_nand(g1, g0, 4);
        nand_connect_nand(g2, g0, 3);
        nand_connect_nand(g0, g0, 5);
        nand_connect_nand(g0, g0, 6);

        // krok 2
        nand_connect_nand(g2, g0, 2);
        nand_connect_nand(g2, g0, 4);

        assert(nand_fan_out(g1) == 0);

        nand_connect_nand(g2, g0, 3);
        nand_connect_nand(g2, g0, 5);
        nand_connect_nand(g2, g0, 6);

        assert(nand_fan_out(g2) == 5);

        nand_connect_nand(g1, g0, 0);
        nand_connect_nand(g1, g0, 1);

        assert(nand_fan_out(g1) == 2);

        // krok 3
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g0);

        printf("przeszedl TEST 3\n");
    }
#endif

    // TEST 4
#ifdef TEST4
    {
        nand_t** g = malloc(sizeof(nand_t*) * 100);

        g[0] = nand_new(99);

        for (int i = 1; i < 100; i++) {
            g[i] = nand_new(1);
            nand_connect_nand(g[i], g[0], i - 1);
            nand_connect_nand(g[0], g[i], 0);
        }

        assert(nand_fan_out(g[0]) == 99);

        for (int i = 0; i < 99; i++)
            assert(nand_input(g[0], i) == g[i + 1]);

        for (int i = 1; i < 100; i++)
            assert(nand_input(g[i], 0) == g[0]);

        for (int i = 0; i < 100; i++)
            nand_delete(g[i]);

        free(g);
        printf("przeszedl TEST 4\n");
    }
#endif

    // TESTY NA POPRAWNOŚĆ POŁĄCZEŃ, ZAKŁADAJĄC BŁĘDNE ARGUMENTY ////////////////////////////////////////////////////////////////////////////////////////////

    // TEST 5
#ifdef TEST5
    {
        nand_t* g0 = nand_new(0);
        nand_t* g1 = nand_new(2);
        nand_t* g2 = nand_new(2);

        bool b = false;

        nand_connect_nand(g1, g1, 1);
        nand_connect_nand(g1, g2, 0);
        nand_connect_nand(g1, g2, 1);

        errno = 0;
        assert(nand_connect_nand(NULL, g1, 0) == -1 && errno == EINVAL);
        errno = 0;
        assert(nand_connect_nand(g1, NULL, 0) == -1 && errno == EINVAL);
        errno = 0;
        assert(nand_connect_nand(g1, g2, 3) == -1 && errno == EINVAL);
        errno = 0;

        errno = 0;
        assert(nand_connect_signal(NULL, g1, 0) == -1 && errno == EINVAL);
        errno = 0;
        assert(nand_connect_signal(&b, NULL, 0) == -1 && errno == EINVAL);
        errno = 0;
        assert(nand_connect_signal(&b, g2, 3) == -1 && errno == EINVAL);
        errno = 0;
        assert(nand_fan_out(NULL) == -1 && errno == EINVAL);
        errno = 0;
        assert(nand_input(NULL, 0) == NULL && errno == EINVAL);
        errno = 0;
        assert(nand_input(g1, 2) == NULL && errno == EINVAL);
        errno = 0;

        // dla nand_output(), wynik ma być niezdefiniowany dla niepoprawnego inputu wedle specyfikacji
        //assert(nand_output(NULL, 0) == NULL && errno == EINVAL);
        //errno = 0;
        //assert(nand_output(g1, 2) == NULL && errno == EINVAL);
        //errno = 0;
        //assert(nand_output(g1, -1) == NULL && errno == EINVAL); // problem z porównaniem (ssize_t)(-1) < (size_t)(2) == 0 ?
        //errno = 0;

        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);

        printf("przeszedl TEST 5\n");
    }
#endif

    // TESTY NA POPRAWNOŚĆ POŁĄCZEŃ, ZAKŁADAJĄC PROBLEM Z ALOKACJĄ PAMIĘCI ////////////////////////////////////////////////////////////////////////////////////////////

    // TEST ?
    {} // nie ma, student debil nie potrafieć
    printf("Uwaga! Testerka nie sprawdza, czy program poprawnie reaguje na problemy z alokacja pamieci.\nUpewnij sie, ze spojnosc programu jest zachowana nawet gdy malloc zwroci NULL\n");

    // TESTY NA EWALUACJĘ ////////////////////////////////////////////////////////////////////////////////////////////

    // TEST 6
#ifdef TEST6
    {
        nand_t* g = nand_new(0);
        bool b;

        errno = 0;
        assert(nand_evaluate(NULL, &b, 1) == -1 && errno == EINVAL);
        errno = 0;
        assert(nand_evaluate(&g, NULL, 1) == -1 && errno == EINVAL);
        errno = 0;
        assert(nand_evaluate(NULL, NULL, 0) == -1 && errno == EINVAL);
        errno = 0;

		assert(nand_evaluate(&g, &b, 0) == -1 && errno == EINVAL);
		// Kwestia sporna, no bo co oznacza ścieżka krytyczna dla żadnych bramek?
		// Ponadto przyznanie, że nand_evaluate(&g, &b, 0) coś zwraca, pociąga za sobą przyznanie, że nand_evaluate(NULL, NULL, 0) również.
		// Oba wywołania dotyczą braku tablicy
		// Tak czy siak na forum piszą, że m = 0 => błąd

        errno = 0;

        nand_delete(g);

        printf("przeszedl TEST 6\n");
    }
#endif

    // TEST 7
#ifdef TEST7
    {
        nand_t** g = malloc(sizeof(nand_t*) * 2);
        g[0] = NULL;
        g[1] = NULL;
        bool b;

        errno = 0;
        assert(nand_evaluate(g, &b, 2) == -1 && errno != 0);
        errno = 0;

        free(g);

        printf("przeszedl TEST 7\n");
    }
#endif

    // TEST 8
#ifdef TEST8
    {
        nand_t* g0 = nand_new(0);
        nand_t* g1 = nand_new(1);
        nand_t* g2 = nand_new(1);
        nand_t* g3 = nand_new(1);

        bool b0 = 0;
        bool b1 = 1;

        nand_connect_signal(&b0, g1, 0);
        nand_connect_signal(&b1, g2, 0);

        bool b = 1;

        errno = 0;
        assert(nand_evaluate(&g0, &b, 1) == 0);
        assert(b == 0);
        assert(nand_evaluate(&g1, &b, 1) == 1);
        assert(b == 1);
        assert(nand_evaluate(&g2, &b, 1) == 1);
        assert(b == 0);
        assert(nand_evaluate(&g3, &b, 1) == -1 && errno != 0);
        errno = 0;


        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g3);

        printf("przeszedl TEST 8\n");
    }
#endif

    // TEST 9
#ifdef TEST9
    {
        nand_t* g0 = nand_new(3);
        nand_t* g1 = nand_new(3);
        nand_t* g2 = nand_new(3);
        nand_t* g3 = nand_new(3);

        bool b0 = 0;
        bool b1 = 1;

        bool b = 0;

        nand_connect_signal(&b0, g0, 0);
        nand_connect_signal(&b0, g0, 1);
        nand_connect_signal(&b0, g0, 2);

        nand_connect_signal(&b1, g1, 0);
        nand_connect_signal(&b0, g1, 1);
        nand_connect_signal(&b0, g1, 2);

        nand_connect_signal(&b0, g2, 0);
        nand_connect_signal(&b1, g2, 1);
        nand_connect_signal(&b0, g2, 2);

        nand_connect_signal(&b1, g3, 0);
        nand_connect_signal(&b1, g3, 1);
        nand_connect_signal(&b1, g3, 2);

        assert(nand_evaluate(&g0, &b, 1) == 1);
        assert(b == 1);
        assert(nand_evaluate(&g1, &b, 1) == 1);
        assert(b == 1);
        assert(nand_evaluate(&g2, &b, 1) == 1);
        assert(b == 1);
        assert(nand_evaluate(&g3, &b, 1) == 1);
        assert(b == 0);

        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g3);

        printf("przeszedl TEST 9\n");
    }
#endif

    // TEST 10
#ifdef TEST10
    {
        nand_t* g0 = nand_new(1);
        nand_t* g1 = nand_new(2);

        bool b0 = 0;

        bool b;

        nand_connect_nand(g0, g0, 0);

        nand_connect_signal(&b0, g1, 0);
        nand_connect_nand(g1, g1, 1);

        errno = 0;
        assert(nand_evaluate(&g0, &b, 1) == -1 && errno == ECANCELED);
        errno = 0;
        assert(nand_evaluate(&g1, &b, 1) == -1 && errno == ECANCELED);
        errno = 0;

        nand_delete(g0);
        nand_delete(g1);

        printf("przeszedl TEST 10\n");
    }
#endif

    // TEST 11
#ifdef TEST11
    {
        nand_t* g0 = nand_new(2);
        nand_t* g1 = nand_new(1);

        bool b0 = 0;
        bool b;

        nand_connect_nand(g0, g1, 0);

        nand_connect_signal(&b0, g0, 0);
        nand_connect_nand(g1, g0, 1);

        errno = 0;
        assert(nand_evaluate(&g0, &b, 1) == -1 && errno == ECANCELED);
        errno = 0;

        nand_delete(g0);
        nand_delete(g1);

        printf("przeszedl TEST 11\n");
    }
#endif

    // TEST 12
#ifdef TEST12
    {
        nand_t* g0 = nand_new(1);
        nand_t* g1 = nand_new(2);

        bool b0 = 0;
        bool b;

        nand_connect_nand(g1, g0, 0);
        nand_connect_signal(&b0, g1, 0);

        errno = 0;
        assert(nand_evaluate(&g0, &b, 1) == -1 && errno == ECANCELED);
        errno = 0;

        nand_delete(g0);
        nand_delete(g1);

        printf("przeszedl TEST 12\n");
    }
#endif

    // TEST 13
#ifdef TEST13
    {
        nand_t* g0 = nand_new(2);
        nand_t* g1 = nand_new(1);
        nand_t* g2 = nand_new(1);
        nand_t* g3 = nand_new(1);

        bool b;

        nand_connect_nand(g1, g0, 0);
        nand_connect_nand(g2, g0, 1);

        nand_connect_nand(g3, g1, 0);

        nand_connect_nand(g3, g2, 0);

        nand_connect_nand(g2, g3, 0);

        errno = 0;
        assert(nand_evaluate(&g0, &b, 1) == -1 && errno == ECANCELED);
        errno = 0;

        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g3);

        printf("przeszedl TEST 13\n");
    }
#endif

    // TEST 14
#ifdef TEST14
    {
        nand_t* g0 = nand_new(2);
        nand_t* g1 = nand_new(1);
        nand_t* g2 = nand_new(1);
        nand_t* g3 = nand_new(1);

        bool b1 = 1;
        bool b = 1;

        nand_connect_nand(g1, g0, 0);
        nand_connect_nand(g2, g0, 1);

        nand_connect_nand(g3, g1, 0);

        nand_connect_nand(g3, g2, 0);

        nand_connect_signal(&b1, g3, 0);

        assert(nand_evaluate(&g0, &b, 1) == 3);
        assert(b == 0);

        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g3);

        printf("przeszedl TEST 14\n");
    }
#endif

    // TEST 15
#ifdef TEST15
    {
        nand_t* g0 = nand_new(4);
        nand_t* g1 = nand_new(1);

        bool b0 = 0;
        bool b1 = 1;
        bool b = 1;

        nand_connect_signal(&b0, g1, 0);
        nand_connect_signal(&b1, g0, 0);
        nand_connect_nand(g1, g0, 1);
        nand_connect_nand(g1, g0, 2);
        nand_connect_nand(g1, g0, 3);

        assert(nand_evaluate(&g0, &b, 1) == 2);
        assert(b == 0);

        nand_delete(g0);
        nand_delete(g1);

        printf("przeszedl TEST 15\n");
    }
#endif

    // TEST 16
#ifdef TEST16
    {
        nand_t* g0 = nand_new(3);
        nand_t* g1 = nand_new(1);
        nand_t* g2 = nand_new(1);
        nand_t* g3 = nand_new(1);

        bool b0 = 0;
        bool b = 1;

        nand_connect_nand(g1, g0, 0);
        nand_connect_nand(g2, g0, 1);
        nand_connect_nand(g3, g0, 2);

        nand_connect_signal(&b0, g1, 0);
        nand_connect_signal(&b0, g2, 0);
        nand_connect_signal(&b0, g3, 0);

        assert(nand_evaluate(&g0, &b, 1) == 2);
        assert(b == 0);

        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g3);

        printf("przeszedl TEST 16\n");
    }
#endif

    // TEST 17
#ifdef TEST17
    {
        nand_t* g0 = nand_new(2);
        nand_t* g1 = nand_new(1);
        nand_t* g2 = nand_new(1);
        nand_t* g3 = nand_new(1);
        nand_t* g4 = nand_new(1);
        nand_t* g5 = nand_new(1);
        nand_t* g6 = nand_new(1);
        nand_t* g7 = nand_new(0);
        nand_t* g8 = nand_new(1);
        nand_t* g9 = nand_new(1);
        nand_t* g10 = nand_new(1);
        nand_t* g11 = nand_new(1);

        bool b0 = 0;
        bool b;

        nand_connect_nand(g1, g0, 1);
        nand_connect_nand(g2, g1, 0);
        nand_connect_signal(&b0, g2, 0);

        nand_connect_nand(g4, g3, 1);
        nand_connect_nand(g5, g4, 0);
        nand_connect_nand(g6, g5, 0);
        nand_connect_nand(g7, g6, 0);

        nand_connect_nand(g3, g0, 0);
        nand_connect_nand(g8, g3, 0);
        nand_connect_nand(g9, g8, 0);
        nand_connect_nand(g10, g9, 0);
        nand_connect_nand(g11, g10, 0);
        nand_connect_nand(g5, g11, 0);

        assert(nand_evaluate(&g0, &b, 1) == 8);
        assert(b == 1);

        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g3);
        nand_delete(g4);
        nand_delete(g5);
        nand_delete(g6);
        nand_delete(g7);
        nand_delete(g8);
        nand_delete(g9);
        nand_delete(g10);
        nand_delete(g11);

        printf("przeszedl TEST 17\n");
    }
#endif

    // TEST 18
#ifdef TEST18
    {
        nand_t* g0 = nand_new(2);
        nand_t* g1 = nand_new(1);
        nand_t* g2 = nand_new(1);
        nand_t* g3 = nand_new(2);
        nand_t* g4 = nand_new(1);
        nand_t* g5 = nand_new(1);
        nand_t* g6 = nand_new(1);
        nand_t* g7 = nand_new(0);
        nand_t* g8 = nand_new(1);
        nand_t* g9 = nand_new(1);
        nand_t* g10 = nand_new(1);
        nand_t* g11 = nand_new(1);

        bool b0 = 0;

        nand_connect_nand(g1, g0, 1);
        nand_connect_nand(g2, g1, 0);
        nand_connect_signal(&b0, g2, 0);

        nand_connect_nand(g4, g3, 1);
        nand_connect_nand(g5, g4, 0);
        nand_connect_nand(g6, g5, 0);
        nand_connect_nand(g7, g6, 0);

        nand_connect_nand(g3, g0, 0);
        nand_connect_nand(g8, g3, 0);
        nand_connect_nand(g9, g8, 0);
        nand_connect_nand(g10, g9, 0);
        nand_connect_nand(g11, g10, 0);
        nand_connect_nand(g5, g11, 0);

        nand_t** g = malloc(sizeof(nand_t*) * 3);

        g[0] = g0;
        g[1] = g4;
        g[2] = g8;

        bool b[3] = {false, false, true};

        assert(nand_evaluate(g, b, 3) == 8);
        assert(b[0] == 1);
        assert(b[1] == 1);
        assert(b[2] == 0);

        free(g);

        nand_delete(g0);
        nand_delete(g1);
        nand_delete(g2);
        nand_delete(g3);
        nand_delete(g4);
        nand_delete(g5);
        nand_delete(g6);
        nand_delete(g7);
        nand_delete(g8);
        nand_delete(g9);
        nand_delete(g10);
        nand_delete(g11);

        printf("przeszedl TEST 18\n");
    }
#endif

    printf("Zakonczono testowanie.");
    return 0;
}

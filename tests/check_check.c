#include "check_check.h"

Suite * make_master_suite (void) {
  Suite * s = suite_create ("Master");
  return s;
}

int main (void) {
  int number_failed;
  SRunner * sr = srunner_create (make_master_suite ());
  srunner_add_suite (sr, make_float_suite ());
  srunner_add_suite (sr, make_line_suite ());
  srunner_add_suite (sr, make_turtle_suite ());
//   srunner_add_suite (sr, make_list_suite ());
  srunner_add_suite (sr, make_entity_suite ());
  srunner_add_suite (sr, make_video_suite ());

  srunner_add_suite (sr, make_timer_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

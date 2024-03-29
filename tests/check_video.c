#include "check_check.h"
#include "../src/video.h"
#include "../src/fcmp.h"

void video_setup (void) {
  objClass_init (video_handler, NULL, NULL, NULL);
}

void video_teardown (void) {
  objClass_destroy ("video");
}

START_TEST (test_video_create) {
  Object * ev = obj_create ("video", NULL, NULL, NULL);
  VIDEO * v = NULL;
  fail_unless (
    ev != NULL,
    "Video handler must create entity");
  v = obj_getClassData (ev, "video");
  fail_unless (
    v != NULL,
    "Video handler must add video data on creation.");
  obj_message (ev, OM_DESTROY, NULL, NULL);
}
END_TEST

START_TEST (test_video_initialize) {
  Object * ev = obj_create ("video", NULL, NULL, NULL);
  VIDEO * v = obj_getClassData (ev, "video");

  obj_message (ev, OM_START, NULL, NULL);
  fail_unless (
    v->screen != NULL,
    "Video start must create a SDL screen.");

  // ... other stuff

  obj_message (ev, OM_DESTROY, NULL, NULL);
}
END_TEST

START_TEST (test_video_resize) {
  Object * ev = obj_create ("video", NULL, NULL, NULL);
  VIDEO * v = obj_getClassData (ev, "video");
  float x = 0, y = 0;
  obj_message (ev, OM_START, NULL, NULL);

  x = video_getXResolution ();
  y = video_getYResolution ();
  fail_unless (
    fcmp (x, 67.2)
      && fcmp (y, 37.8),
    "Video default resolution should be 960x540 with a scale of 0.07"
  );
  video_setScaling (v, 1.0);
  video_setResolution (v, 500.0, 500.0);
  x = video_getXResolution ();
  y = video_getYResolution ();
  fail_unless (
    fcmp (x, 500.0)
      && fcmp (y, 500.0),
    "Video dimensions should be resizable on the fly (got %.2f, %.2f; expecting %.2f,%2.f)",
    x, y, 500.0, 500.0
  );
  video_setScaling (v, 2.0);
  x = video_getXResolution ();
  y = video_getYResolution ();
  fail_unless (
    fcmp (x, 1000.0)
      && fcmp (y, 1000.0),
    "Video resolution should be resizable on the fly (got %.2f,%.2f; expecting %.2f,%.2f)",
    x, y, 1000.0, 1000.0
  );
  video_setScaling (v, 0.5);
  x = video_getXResolution ();
  y = video_getYResolution ();
  fail_unless (
    fcmp (x, 250.0)
      && fcmp (y, 250.0),
    "Video resolution should be resizable on the fly (got %.2f,%.2f; expecting %.2f,%.2f)",
    x, y, 250.0, 250.0
  );

  obj_message (ev, OM_DESTROY, NULL, NULL);
}
END_TEST

Suite * make_video_suite (void) {
  Suite * s = suite_create ("Video");
  TCase
    * tc_video = tcase_create ("Base");
  tcase_add_checked_fixture (tc_video, video_setup, video_teardown);
  tcase_add_test (tc_video, test_video_create);
  tcase_add_test (tc_video, test_video_initialize);
  tcase_add_test (tc_video, test_video_resize);
  suite_add_tcase (s, tc_video);
  return s;
}

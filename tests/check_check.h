#ifndef CHECK_CHECK_H
#define CHECK_CHECK_H
#include <check.h>
#include <stdlib.h>

Suite * make_master_suite (void);
Suite * make_float_suite (void);
Suite * make_line_suite (void);
//Suite * make_list_suite (void);
//Suite * make_entity_suite (void);
Suite * make_turtle_suite (void);
Suite * make_video_suite (void);

Suite * make_timer_suite (void);

#endif /* CHECK_CHECK_H */
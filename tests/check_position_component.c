START_TEST (test_position_component_init) {
  Entity * e = entity_create ();
  Component * cd = NULL;
  struct position_data * p = NULL;
  entity_registerComponentAndSystem (position_component);
  component_instatiateOnEntity ("position", e);
  cd = entity_getAs (e, "position");
  fail_unless (cd->comp_data != NULL);
  p = cd->comp_data;
}

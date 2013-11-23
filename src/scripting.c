#include "scripting.h"

#include "commands.h"
#include "util.h"

// typedef void* arg_t;

SCM it_quit_wrapper(){
  return scm_from_bool(it_quit());
}

SCM it_switch_mode_wrapper(){
  return scm_from_bool(it_switch_mode());
}

SCM it_toggle_fullscreen_wrapper(){
  return scm_from_bool(it_toggle_fullscreen());
}

SCM it_toggle_bar_wrapper(){
  return scm_from_bool(it_toggle_bar());
}

SCM t_reload_all_wrapper(){
  return scm_from_bool(t_reload_all());
}

SCM it_reload_image_wrapper(){
  return scm_from_bool(it_reload_image());
}

SCM it_remove_image_wrapper(){
  return scm_from_bool(it_remove_image());
}

SCM i_navigate_wrapper(SCM n){
  return scm_from_bool(i_navigate(scm_to_long(n)));
}

SCM i_alternate_wrapper(){
  return scm_from_bool(i_alternate());
}

SCM it_first_wrapper(){
  return scm_from_bool(it_first());
}

SCM it_n_or_last_wrapper(SCM n){
  return scm_from_bool(it_n_or_last(scm_to_int(n)));
}

SCM i_navigate_frame_wrapper(SCM n){
  return scm_from_bool(i_navigate_frame(scm_to_long(n)));
}

SCM i_toggle_animation_wrapper(){
  return scm_from_bool(i_toggle_animation());
}

SCM it_scroll_move_wrapper(SCM dir, SCM n){
  return scm_from_bool(it_scroll_move(scm_to_int(dir), scm_to_int(n)));
}

SCM it_scroll_screen_wrapper(SCM dir){
  return scm_from_bool(it_scroll_screen(scm_to_int(dir)));
}

SCM i_scroll_to_edge_wrapper(SCM dir){
  return scm_from_bool(i_scroll_to_edge(scm_to_int(dir)));
}

// i_drag

SCM i_zoom_wrapper(SCM scale){
  return scm_from_bool(i_zoom(scm_to_long(scale)));
}

SCM i_set_zoom_wrapper(SCM scale){
  return scm_from_bool(i_set_zoom(scm_to_long(scale)));
}

SCM i_fit_to_win_wrapper(SCM scalemode){
  return scm_from_bool(i_fit_to_win(scm_to_int(scalemode)));
}

SCM i_fit_to_img_wrapper(){
  return scm_from_bool(i_fit_to_img());
}

SCM i_rotate_wrapper(SCM degree){
  return scm_from_bool(i_rotate(scm_to_int(degree)));
}

SCM i_flip_wrapper(SCM fdir){
  return scm_from_bool(i_flip(scm_to_int(fdir)));
}

SCM i_toggle_antialias_wrapper(){
  return scm_from_bool(i_toggle_antialias());
}

SCM it_toggle_alpha_wrapper(){
  return scm_from_bool(it_toggle_alpha());
}

// it_open_with
// it_shell_cmd

SCM p_set_bar_left_wrapper(SCM str){
  return scm_from_bool(p_set_bar_left(scm_to_locale_string(str)));
}

SCM p_set_bar_right_wrapper(SCM str){
  return scm_from_bool(p_set_bar_right(scm_to_locale_string(str)));
}

SCM it_add_image_wrapper(SCM str){
  return scm_from_bool(it_add_image(scm_to_locale_string(str)));
}

SCM p_get_file_count_wrapper(){
  return scm_from_int(p_get_file_count());
}

SCM p_get_file_index_wrapper(){
  return scm_from_int(p_get_file_index());
}

SCM it_redraw_wrapper(){
  return scm_from_bool(it_redraw());
}



bool init_guile(const char *script) {
  int i;

  static const funcmap_t functions[] = {
    {"it-quit", 0, 0, 0, it_quit_wrapper},
    {"it-switch-mode", 0, 0, 0, it_switch_mode_wrapper},
    {"it-toggle-fullscreen", 0, 0, 0, it_toggle_fullscreen_wrapper},
    {"it-toggle-bar", 0, 0, 0, it_toggle_bar_wrapper},
    {"t-reload-all", 0, 0, 0, t_reload_all_wrapper},
    {"it-reload-image", 0, 0, 0, it_reload_image_wrapper},
    {"it-remove-image", 0, 0, 0, it_remove_image_wrapper},
    {"i-navigate", 1, 0, 0, i_navigate_wrapper},
    {"i-alternate", 0, 0, 0, i_alternate_wrapper},
    {"it-first", 0, 0, 0, it_first_wrapper},
    {"it-n-or-last", 1, 0, 0, it_n_or_last_wrapper},
    {"i-navigate-frame", 1, 0, 0, i_navigate_frame_wrapper},
    {"i-toggle-animation", 0, 0, 0, i_toggle_animation_wrapper},
    {"it-scroll-move", 2, 0, 0, it_scroll_move_wrapper},
    {"it-scroll-screen", 1, 0, 0, it_scroll_screen_wrapper},
    {"i-scroll-to-edge", 1, 0, 0, i_scroll_to_edge_wrapper},
    {"i-zoom", 1, 0, 0, i_zoom_wrapper},
    {"i-set-zoom", 1, 0, 0, i_set_zoom_wrapper},
    {"i-fit-to-win", 1, 0, 0, i_fit_to_win_wrapper},
    {"i-fit-to-img", 0, 0, 0, i_fit_to_img_wrapper},
    {"i-rotate", 1, 0, 0, i_rotate_wrapper},
    {"i-flip", 1, 0, 0, i_flip_wrapper},
    {"i-toggle-antialias", 0, 0, 0, i_toggle_antialias_wrapper},
    {"it-toggle-alpha", 0, 0, 0, it_toggle_alpha_wrapper},
    {"p-set-bar-left", 1, 0, 0, p_set_bar_left_wrapper},
    {"p-set-bar-right", 1, 0, 0, p_set_bar_right_wrapper},
    {"it-add-image", 1, 0, 0, it_add_image_wrapper},
    {"p-get-file-index", 0, 0, 0, p_get_file_index_wrapper},
    {"p-get-file-count", 0, 0, 0, p_get_file_count_wrapper},
    {"it-redraw", 0, 0, 0, it_redraw_wrapper},
  };

  scm_init_guile();
  for (i = 0; i < ARRLEN(functions); i++) {
    scm_c_define_gsubr(functions[i].name,
                       functions[i].req,
                       functions[i].opt,
                       functions[i].rst,
                       functions[i].func);
  }
  
  scm_c_primitive_load(script);
  return true;
}

bool call_guile_keypress(char key, bool ctrl, bool mod1) {
  return scm_to_bool (scm_eval (scm_list_n (scm_from_locale_symbol ("on-key-press"),
                                            scm_from_char(key),
                                            scm_from_bool(ctrl),
                                            scm_from_bool(mod1),
                                            SCM_UNDEFINED
                                            ), scm_interaction_environment()));
}

bool call_guile_buttonpress(unsigned int button, bool ctrl, int x, int y) {
  return scm_to_bool (scm_eval (scm_list_n (scm_from_locale_symbol ("on-button-press"),
                                            scm_from_int(button),
                                            scm_from_bool(ctrl),
                                            scm_from_int(x),
                                            scm_from_int(y),
                                            SCM_UNDEFINED
                                            ), scm_interaction_environment()));
}

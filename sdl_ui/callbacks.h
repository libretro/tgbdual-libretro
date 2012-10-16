#ifndef callbacks_h_
#define callbacks_h_

// この名前で良いかは微妙だが、callback で実現されていた関数を集積する。

void cb_save_state(int lParam);
void cb_load_state(int lParam);
void cb_pause(int lParam);
void cb_fullscreen(int lParam);
void cb_reset(int lParam);
void cb_quit(int lParam);

#endif // ! callbacks_h_


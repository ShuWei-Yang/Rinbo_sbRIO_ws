#ifndef __CONSOLE_H
#define __CONSOLE_H

#define BKGD_PAIR 1
#define CYAN_PAIR 2
#define NCURSES_NOMACROS

#include "fpga_handler.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <ncurses.h>
#include <locale.h>
#include <vector>
#undef OK

using namespace std;

class Panel
{
public:
  Panel(string title, string type, int org_x, int org_y, int height_, int width_, bool box_on);
  Panel()
  {}

  WINDOW *win_;

  string title_;

  string type_;
  int org_x_;
  int org_y_;
  int height_;
  int width_;
  bool box_on_;

  std::mutex *main_mtx_;
  std::vector<bool> *powerboard_state_;

  mutex mutex_;
  void infoDisplay();
  void infoDisplay(FpgaHandler *fpga, bool power_switch, bool signal_switch, bool digital_switch);
  void infoDisplay(FpgaHandler *fpga);
  void resetPanel();
  void panelTitle();
};

// handle user input and update registers
class InputPanel
{
public:
  InputPanel(){}
  FpgaHandler *fpga_;
  void init(FpgaHandler *fpga, bool *if_resetPanel, int term_max_x, int term_max_y);
  void start();
  void inputHandler(WINDOW *win_, std::mutex &input_mutex);
  void reset_input_window(WINDOW *win);
  void commandDecode(std::string buf);
  vector<string> tokenizer(std::string s);
  auto getValue(string str);

  WINDOW *win_;
  std::mutex mutex_;

  bool *if_resetPanel;
  std::mutex *main_mtx_;
  std::vector<bool> *powerboard_state_;

private:
  std::thread *thread;
};

// Console class to manage ncurses panels and refreshes window
class Console
{
public:
  Console(){}

  void init(FpgaHandler *fpga_, std::vector<bool> *pb_state_, std::mutex *mtx_);
  bool is_running_ = false; 

  ~Console();
  void refreshWindow();

  int term_max_x_;
  int term_max_y_;
  int debug_cons_h = 27;
  int power_cons_h = 27;

  FpgaHandler *fpga_;

  Panel p_cmain_;
  Panel p_debug_;
  Panel p_dcmotor_;
  Panel p_servo_;
  InputPanel input_panel_;

  std::mutex *main_mtx_;
  std::vector<bool> *powerboard_state_;

  mutex input_mutex_;
  thread t_frontend_;
  int frontend_rate_;

  bool if_resetPanel;
};

#endif

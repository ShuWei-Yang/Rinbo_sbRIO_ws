#include "console.hpp"
#include <curses.h>
#include <iterator>

using namespace std;

mutex cons_mtx_;
int refresh_flag;
void Console::init(FpgaHandler *fpga, std::vector<bool> *pb_state_ptr_, std::mutex *mtx_ptr_)
{
    fpga_ = fpga;
    setlocale(LC_ALL, "");
    initscr();
    getmaxyx(stdscr, term_max_y_, term_max_x_);
    start_color();
    init_pair(BKGD_PAIR, COLOR_WHITE, COLOR_BLACK);
    wbkgd(stdscr, COLOR_PAIR(BKGD_PAIR));
    init_pair(CYAN_PAIR, COLOR_CYAN, COLOR_BLACK);

    frontend_rate_ = 3;
    input_panel_.init(fpga, &if_resetPanel, term_max_x_, term_max_y_);
    input_panel_.main_mtx_ = mtx_ptr_;
    input_panel_.powerboard_state_ = pb_state_ptr_;

    main_mtx_ = mtx_ptr_;
    powerboard_state_ = pb_state_ptr_;

    if_resetPanel = false;
    is_running_ = true;
    t_frontend_ = thread(&Console::refreshWindow, this);
    refresh_flag = 1;
}

Console::~Console()
{
    is_running_ = false;

    if (t_frontend_.joinable()) {
        t_frontend_.join();
    }
    endwin(); 

}

void Console::refreshWindow()
{
    clear();
    int refresh_period_ = (int)(1 / frontend_rate_) * 1000000;
    
    Panel p_power_("[P] Power Board ", "power", 1, 9, 60, 40, true);
    Panel p_dcmotor_("[M] DC Motor ", "motor", 41, 1, (term_max_y_ - 2) / 2 - 1, 60, true);
    Panel p_servo_("[S] Servo", "servo", 41, (term_max_y_ - 2) / 2 + 1, (term_max_y_ - 2) / 2 - 1, 60, true);
    p_power_.powerboard_state_ = powerboard_state_;
    
    while (is_running_) 
    {
        cons_mtx_.lock(); 

        p_power_.infoDisplay(fpga_, powerboard_state_->at(0), powerboard_state_->at(1), powerboard_state_->at(2));
        p_dcmotor_.infoDisplay(fpga_);
        p_servo_.infoDisplay(fpga_);
        
        cons_mtx_.unlock(); 
        
        usleep(100000); 
    }

}


void InputPanel::init(FpgaHandler *fpga, bool *if_resetPanel, int term_max_x, int term_max_y)
{
    fpga_ = fpga; 
    win_ = newwin(3, term_max_x - 1, term_max_y - 3, 1);

    thread = new std::thread(&InputPanel::inputHandler, this, win_, std::ref(mutex_));
}
void InputPanel::inputHandler(WINDOW *win_, std::mutex &input_mutex)
{
    while (1)
    {
        int ch = 0;
        int x = 1;
        do
        {
            ch = mvwgetch(win_, 1, x);
            if (ch == 'r')
            {
                reset_input_window(win_);
            }
            if (ch == 'e')
            {
                endwin();
                std::cout << "Normal Mode" << std::endl;
                refresh_flag = 0;
            }
            if (ch == 'E')
            {
                refresh_flag = 1;
                refresh();
            }

        } while (ch != ':');

        string input_buf;
        keypad(win_, true);
        do
        {
            ch = mvwgetch(win_, 1, x);

            if (ch == KEY_BACKSPACE || ch == KEY_DC || ch == 127)
            {
                mvwdelch(win_, 1, x);
                mvwdelch(win_, 1, x + 1);
                mvwdelch(win_, 1, x - 1);
                wclrtoeol(win_);
                wrefresh(win_);

                if (input_buf.size() > 0)
                {
                    input_buf.erase(input_buf.size() - 1, 1);
                    x--;
                }
            }
            else
            {
                if (ch != '\n')
                {
                    input_buf.append(1, ch);
                }
                x++;
            }
        } while (ch != '\n');

        reset_input_window(win_);
        commandDecode(input_buf);
    }
}

void InputPanel::reset_input_window(WINDOW *win)
{
    werase(win);
    wclear(win);
    wrefresh(win);
}

void InputPanel::commandDecode(string buf)
{
    bool syntax_err = false;
    bool pb_selected = false;
    bool motor_selected = false;
    bool servo_selected = false;

    vector<string> bufs;
    bufs = tokenizer(buf);

    if (bufs.size() >= 1){
        if (bufs[0] == "P"){
            pb_selected = true;
        }
        else if (bufs[0] == "M"){
            motor_selected = true;
        }
        else if (bufs[0] == "S"){
            servo_selected = true;
        }
        else{
            syntax_err = true;
        }
        
        mvwprintw(win_, 2, 1, bufs[0].c_str());
        mvwprintw(win_, 2, 5, bufs[2].c_str());
        wrefresh(win_);
    }

    main_mtx_->lock();
    cons_mtx_.lock();

    if (bufs.size() == 3){
        mvwprintw(win_, 2, 3, bufs[1].c_str());
        mvwprintw(win_, 2, 5, bufs[2].c_str());
        wrefresh(win_);

        if (pb_selected){
            if (bufs[1] == "D"){
                try{
                    powerboard_state_->at(0) = stoi(bufs[2]);
                }
                catch (exception &e){
                    syntax_err = true;
                    mvwprintw(win_, 2, 1, "err");
                }
            }
            else if (bufs[1] == "S"){
                try{
                    powerboard_state_->at(1) = stoi(bufs[2]);
                }
                catch (exception &e){
                    syntax_err = true;
                    mvwprintw(win_, 2, 1, "err");
                }
            }
            else if (bufs[1] == "P"){
                try{
                    powerboard_state_->at(2) = stoi(bufs[2]);
                }
                catch (exception &e){
                    syntax_err = true;
                    mvwprintw(win_, 2, 1, "err");
                }
            }
            else{
                syntax_err = true;
            }
        }
    }
    else if (motor_selected && bufs.size() == 4){
        mvwprintw(win_, 2, 3, bufs[1].c_str());
        mvwprintw(win_, 2, 5, bufs[2].c_str());
        mvwprintw(win_, 2, 7, bufs[3].c_str());
        wrefresh(win_);

        int i = stoi(bufs[1]); 

        if (bufs[2] == "E"){
            try{
                bool enable = (stoi(bufs[3]) != 0);
                fpga_->moduleIO.write_en_(i, enable);
            }
            catch (exception &e){
                syntax_err = true;
                mvwprintw(win_, 2, 1, "err");
            }
        }
        else if (bufs[2] == "D"){
            try{
                bool direction = (stoi(bufs[3]) != 0);
                fpga_->moduleIO.write_dir_(i, direction);
            }
            catch (exception &e){
                syntax_err = true;
                mvwprintw(win_, 2, 1, "err");
            }
        }
        else if (bufs[2] == "I"){
            try{
                uint16_t voltage = stoi(bufs[3]);
                fpga_->moduleIO.write_iv_(i, voltage);
            }
            catch (exception &e){
                syntax_err = true;
                mvwprintw(win_, 2, 1, "err");
            }
        }
        else if (bufs[2] == "R"){
            try{
                fpga_->moduleIO.write_rp_(i, NiFpga_True);
                fpga_->moduleIO.write_rp_(i, NiFpga_False);
            }
            catch (exception &e){
                syntax_err = true;
                mvwprintw(win_, 2, 1, "err");
            }
        }
        else if (bufs[2] == "S"){
            try{
                bool state = (stoi(bufs[3]) != 0);
                fpga_->moduleIO.write_state_(i, state);
            }
            catch (exception &e){
                syntax_err = true;
                mvwprintw(win_, 2, 1, "err");
            }
        }
        else{
            syntax_err = true;
        }
    }
    else if (servo_selected && bufs.size() == 3){
        mvwprintw(win_, 2, 3, bufs[1].c_str());
        mvwprintw(win_, 2, 5, bufs[2].c_str());
        wrefresh(win_);

        if (bufs[1] == "C"){
            try{
                uint16_t cm_value = (uint16_t)stoi(bufs[2]);
                mvwprintw(win_, 1, 20, "SENT: %d", cm_value);
                wrefresh(win_);
                fpga_->moduleIO.write_cm_(cm_value);
            }
            catch (exception &e){
                syntax_err = true;
                mvwprintw(win_, 2, 1, "err");
            }
        }
    }
    else if (servo_selected && bufs.size() == 4){
        mvwprintw(win_, 2, 3, bufs[1].c_str());
        mvwprintw(win_, 2, 5, bufs[2].c_str());
        mvwprintw(win_, 2, 7, bufs[3].c_str());
        wrefresh(win_);

        int i = stoi(bufs[1]); 

        if (bufs[2] == "P"){
            try{
                uint16_t position_bus = stoi(bufs[3]);
                fpga_->moduleIO.write_position_bus_(i, position_bus);
            }
            catch (exception &e){
                syntax_err = true;
                mvwprintw(win_, 2, 1, "err");
            }
        }
    }
    else{
        syntax_err = true;
    }

    cons_mtx_.unlock();
    main_mtx_->unlock();

    wrefresh(win_);
}

vector<string> InputPanel::tokenizer(string s)
{
    stringstream ss(s);
    string word;
    vector<string> bufs;
    while (ss >> word)
    {
        // cout << word << endl;
        bufs.push_back(word);
    }
    return bufs;
}
Panel::Panel(string title, string type, int org_x, int org_y, int height, int width, bool box_on)
{
    org_x_ = org_x;
    org_y_ = org_y;
    height_ = height;
    width_ = width;
    // box_on_ = // box_on;
    type_ = type;
    title_ = title;

    win_ = newwin(height_, width_, org_y_, org_x_);
    refresh();

    string tag_(title.c_str(), title.c_str() + 3);
    title.erase(0, 3);

    wattron(win_, COLOR_PAIR(CYAN_PAIR));
    wattron(win_, A_BOLD);
    wattron(win_, A_STANDOUT);
    mvwprintw(win_, 0, (width_ / 2 - title.size() / 2 - 2), tag_.c_str());
    wattroff(win_, COLOR_PAIR(CYAN_PAIR));

    mvwprintw(win_, 0, (width_ / 2 - title.size() / 2 + 1), title.c_str());
    wattroff(win_, A_BOLD);
    wattroff(win_, A_STANDOUT);
    wrefresh(win_);
    // refresh();
}

void Panel::infoDisplay(FpgaHandler *fpga_, bool digital_switch, bool signal_switch, bool power_switch)
{
    mvwprintw(win_, 2, 1, "HARDWARE POWER SWITCH ----------------");
    mvwprintw(win_, 3, 1, "[D] Digital:   %4d", digital_switch);
    mvwprintw(win_, 4, 1, "[S] Signal:    %4d", signal_switch);
    mvwprintw(win_, 5, 1, "[P] Power:     %4d", power_switch);

    mvwprintw(win_, 6, 1, "Voltage Current ADC ------------------");
    mvwprintw(win_, 7, 1, "Voltage: %5.5f, Current: %5.5f", fpga_->powerboard_V_list_[0], fpga_->powerboard_I_list_[0]);
    mvwprintw(win_, 8, 1, "Voltage: %5.5f, Current: %5.5f", fpga_->powerboard_V_list_[1], fpga_->powerboard_I_list_[1]);
    mvwprintw(win_, 9, 1, "Voltage: %5.5f, Current: %5.5f", fpga_->powerboard_V_list_[2], fpga_->powerboard_I_list_[2]);
    mvwprintw(win_, 10, 1, "Voltage: %5.5f, Current: %5.5f", fpga_->powerboard_V_list_[3], fpga_->powerboard_I_list_[3]);
    mvwprintw(win_, 11, 1, "Voltage: %5.5f, Current: %5.5f", fpga_->powerboard_V_list_[4], fpga_->powerboard_I_list_[4]);
    mvwprintw(win_, 12, 1, "Voltage: %5.5f, Current: %5.5f", fpga_->powerboard_V_list_[5], fpga_->powerboard_I_list_[5]);
    mvwprintw(win_, 13, 1, "Voltage: %5.5f, Current: %5.5f", fpga_->powerboard_V_list_[6], fpga_->powerboard_I_list_[6]);
    mvwprintw(win_, 14, 1, "Voltage: %5.5f, Current: %5.5f", fpga_->powerboard_V_list_[7], fpga_->powerboard_I_list_[7]);


    wrefresh(win_);
}

void Panel::infoDisplay(FpgaHandler *fpga_)
{
    if (type_ == "motor") 
    {
        int row = 2;
        
        for (int i = 0; i < 6; ++i) {
            int32_t position = fpga_->moduleIO.read_ep_(i);
            uint32_t tick_count = fpga_->moduleIO.read_tc_(i);
            bool hall_effect = fpga_->moduleIO.read_he_(i);
            
            if (i % 2 == 0 && i > 0) {
                row += 2;  
            }
            
            mvwprintw(win_, row, 1, "M%d Pos:%8d | TC:%8u | HE:%d", 
                      i, position, tick_count, hall_effect);
            
            row++;
        }
        
        row += 2;
        mvwprintw(win_, row,   1, "Commands: :M <id> <CMD> <VAL>");
        mvwprintw(win_, row+1, 1, "E, D, I, S, R");
    }
    else if (type_ == "servo") 
    {
        int row = 2;

        uint16_t current_cm = fpga_->moduleIO.read_cm_();
        mvwprintw(win_, row, 1, "Control Mode (CM): %u", current_cm);
        row += 2;

        for (int i = 0; i < 6; ++i) {
            uint16_t encoder_pos = fpga_->moduleIO.read_position_encoder_(i);

            if (i % 2 == 0 && i > 0) {
                row += 1;
            }

            mvwprintw(win_, row, 1, "S%d Enc:%8u", i, encoder_pos);

            row++;
        }

        row += 2;
        mvwprintw(win_, row,   1, "Commands: :S <id> <CMD> <VAL>");
        mvwprintw(win_, row+1, 1, "P (Position), C (Mode All)");
    }

    wrefresh(win_);
}


void Panel::panelTitle()
{
    string tag_ = title_.substr(0, std::min<size_t>(3, title_.size()));

    wattron(win_, COLOR_PAIR(CYAN_PAIR));
    wattron(win_, A_BOLD);
    wattron(win_, A_STANDOUT);
    mvwprintw(win_, 0, (width_ / 2 - title_.size() / 2 - 2), tag_.c_str());
    wattroff(win_, COLOR_PAIR(CYAN_PAIR));

    mvwprintw(win_, 0, (width_ / 2 - title_.size() / 2 + 1), title_.c_str());
    wattroff(win_, A_BOLD);
    wattroff(win_, A_STANDOUT);

    wrefresh(win_);
}

void Panel::resetPanel()
{
    werase(win_);
    wclear(win_);
    wrefresh(win_);
}

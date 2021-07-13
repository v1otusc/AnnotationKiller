/*
 * @Description: 利用状态机删除代码中所有注释
                 目前仅支持 Linux/Unix 系统下的 C++ 文件
 * @LastEditors: zzh
 * @LastEditTime: 2021-07-13 19:44:32
 * @FilePath: /AnnotationKiller/test.cpp
 * @history:      -- 2021/07/12, v1.0.0 实现了基本功能
 *                -- 2021/07/13,
 */

#include <string.h>
#include <sys/time.h>  // only support unix
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// const int BUFFER_SIZE = 1024;

/**
 * @brief: 删除文件中的注释
 */
class AnnotionKiller {
 public:
  /**
   * @brief: core function
   * @param {string&} 待处理字符串
   * @return {string} 处理之后字符串
   */
  string delete_annotion(string &str) {
    State state = CODE;
    string ret{""};
    for (auto &c : str) {
      switch (state) {
        case CODE:
          if (c == '/') {
            state = SLASH;
          } else {
            ret += c;
            if (c == '\'') {
              state = CODE_CHAR;
            } else if (c == '\"') {
              state = CODE_STRING;
            }
          }
          break;
        case SLASH:
          if (c == '*') {
            state = NOTE_MULTILINE;
          } else if (c == '/') {
            state = NOTE_SINGLELINE;
          } else {
            state = CODE;
            ret += '/';
            ret += c;
          }
          break;
        case NOTE_MULTILINE:
          if (c == '*') {
            state = NOTE_MULTILINE_STAR;
          } else {
            if (c == '\n') {
              // 这里选择保留空行，也可以去掉
              ret += '\n';
            }
            state = NOTE_MULTILINE;
          }
          break;
        case NOTE_MULTILINE_STAR:
          if (c == '/') {
            state = CODE;
          } else if (c == '*') {
            state = NOTE_MULTILINE_STAR;
          } else {
            state = NOTE_MULTILINE;
          }
          break;
        case NOTE_SINGLELINE:
          if (c == '\\') {
            state = BACK_SLASH;
          } else if (c == '\n') {
            // 这里选择保留空行，也可以去掉
            ret += '\n';
            state = CODE;
          } else {
            state = NOTE_SINGLELINE;
          }
          break;
        case BACK_SLASH:
          if (c == '\\' || c == '\n') {
            if (c == '\n') {
              // 这里选择保留空行，也可以去掉
              ret += '\n';
            }
            state = BACK_SLASH;
          } else {
            state = NOTE_SINGLELINE;
          }
          break;
        case CODE_CHAR:
          ret += c;
          if (c == '\\') {
            state = CHAR_ESCAPE_SEQUENCE;
          } else if (c == '\'') {
            state = CODE;
          } else {
            state = CODE_CHAR;
          }
          break;
        case CHAR_ESCAPE_SEQUENCE:
          ret += c;
          state = CODE_CHAR;
          break;
        case CODE_STRING:
          ret += c;
          if (c == '\\') {
            state = STRING_ESCAPE_SEQUENCE;
          } else if (c == '\"') {
            state = CODE;
          }
          break;
        case STRING_ESCAPE_SEQUENCE:
          ret += c;
          state = CODE_STRING;
          break;
        default:
          break;
      }
    }
    return ret;
  }

  /**
   * @brief: utils-估计文件中的行数
   * @param {istream} 输入流
   * @return {long long} 估计得到的行数就
   */
  long long estimate_lines_num(istream &is) {
    // char buffer[BUFFER_SIZE];
    // estimate num of lines using 50 records
  }

  /**
   * @brief: utils-从指定文件名中删除所有注释，包括读-删-写三个步骤
   * @param {orig_file} 输入文件名称
   * @param {output_file} 输出文件名称
   * @return {bool} 此操作是否成功
   */
  bool read_del_write(const char *orig_file, const char *output_file) {
    ifstream is(orig_file);
    ofstream os(output_file);
    if (!is.is_open()) {
      cerr << "[Error]: fail to open source file: " << orig_file << endl;
      return false;
    }
    if (!os.is_open()) {
      cerr << "[Error]: fail to write file: " << output_file << endl;
      return false;
    }

    // estimated lines number
    int iall_record = 100;
    int icout = 0;

    // ostringstream oss;
    vector<string> lines;
    vector<string> strip_lines;
    string buffer;

    cout << "1. Now read data to memory ..." << endl;
    ProcessNotify notify(5);
    while (getline(is, buffer)) {
      if (is.fail()) {
        break;
      }
      lines.push_back(buffer);
      ++icout;
      notify.push(icout * 100 / iall_record, "  read");
    }

    cout << "2. delete annotation ..." << endl;
    for (auto &line : lines) {
      string s_lines;
      s_lines = delete_annotion(line);
      strip_lines.push_back(s_lines);
    }

    icout = 0;
    cout << "3. Now write to file ..." << endl;
    ProcessNotify notify1(5);
    for (auto &l : strip_lines) {
      os << l << endl;
      ++icout;
      notify1.push(icout * 100 / iall_record, "  write");
    }
    is.close();
    os.close();
    return true;
  }

 protected:
  enum State {
    CODE,                   // 正常代码
    SLASH,                  // 斜杠
    NOTE_MULTILINE,         // 多行注释
    NOTE_MULTILINE_STAR,    // 多行注释遇到*
    NOTE_SINGLELINE,        // 单行注释
    BACK_SLASH,             // 折行注释
    CODE_CHAR,              // 字符
    CHAR_ESCAPE_SEQUENCE,   // 字符中的转义字符
    CODE_STRING,            // 字符串
    STRING_ESCAPE_SEQUENCE  // 字符串中的转义字符
  };
};

/**
 * @brief: 进度条打印
 */
class ProcessNotify {
 public:
  // ? is necessary function
  void init(const int step = 5) {
    old_percent_ = -1;
    step_ = step;
  }

  void push(int percent, const string &notify = " processed") {
    if (percent % step_ == 0 && percent != old_percent_) {
      cout << notify << " : " << percent << "%..." << endl;
      old_percent_ = percent;
    }
  }

 public:
  explicit ProcessNotify(const int step = 5) : old_percent_(-1), step_(step) {}

 protected:
  // 显示步长的间隔
  int step_;
  int old_percent_;
};

/**
 * @brief: 时间统计功能
 *         from Song Ho Ahn(song.ahn@gmail.com)
 */
class Timer {
 public:
  Timer() {}
  ~Timer();

 public:
  void start();
  void stop();
  inline double get_elapsed_time() {}

 protected:
  double start_time_in_microsec;
  double end_time_in_microsec;
  bool stopped;

  timeval start_count;
  timeval end_count;
};

/**
 * @brief: 打印使用说明
 * @param {int} argc
 * @param {char const} *argv
 * @return {void}
 */
void helper(int argc, char const *argv[]) {
  struct help_str {
    const char *item;
    const char *notice;
  };

  help_str notice[] = {{"from "}, {""}, {""}};

  cout << "============================" << endl;
  cout << argv[0] << " delete annotions from original file" << endl;
  for (int i = 0; i < sizeof(notice) / sizeof(notice[0]); ++i) {
    cout << "-" << notice[i].item << notice[i].notice << endl;
  }
  cout << "============================" << endl;
}

int main(int argc, char const *argv[]) {
  if (argc < 2) {
    helper(argc, argv);
    return -1;
  }
  const char *orig_file = argv[1];
  string des_file{""};
  if (argc == 3) {
    des_file = const_cast<char *>(argv[2]);

  } else {
    char *temp = new char[strlen(orig_file) + 1];
    memcpy(temp, orig_file, strlen(orig_file) - 4);
    temp[strlen(orig_file) - 4] = 0;
    des_file = temp;
    delete[] temp;

    des_file += "_deleted.cpp";
    cout << "[OK] ouput file name: " << des_file << endl;
  }

  AnnotionKiller ak;
  if (ak.read_del_write(orig_file,
                        static_cast<const char *>(des_file.c_str()))) {
    cout << "[Error] failed to delete annotion ..." << endl;
  } else {
    cout << "[OK] successfully delete annotion ..." << endl;
  }
  return 0;
}

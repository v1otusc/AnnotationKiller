/*
 * @Description: 利用状态机删除代码中所有注释
                 目前仅支持 Linux/Unix 系统下的 C++ 文件
 * @LastEditors: zzh
 * @LastEditTime: 2021-07-13 22:34:25
 * @FilePath: /AnnotationKiller/main.cpp
 * @history:      -- 2021/07/12, v1.0 实现了基本功能
 *                -- 2021/07/13, v1.1 添加了显示文件读取进度功能
 *                -- 2021/07/13, v1.2 添加了时间统计功能
 */

#include <string.h>
#include <sys/time.h>  // only support unix
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const int BUFFER_SIZE = 10024;

/**
 * @brief: utils -- 用于显示读取文件进度
 */
class ProcessNotify {
 public:
  void push(int percent, const string &notify = " processed") {
    if (percent % step_ == 0 && percent != old_percent_) {
      cout << notify << " : " << percent << "%" << endl;
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
 * @brief: 时间统计
 *         from Song Ho Ahn(song.ahn@gmail.com)
 */
class Timer {
 public:
  Timer() {
    start_count.tv_sec = start_count.tv_usec = 0;
    end_count.tv_sec = end_count.tv_usec = 0;
    stopped = false;
    start_time_in_microsec = 0;
    end_time_in_microsec = 0;
  }
  ~Timer();

 public:
  void start() {
    stopped = false;
    gettimeofday(&start_count, NULL);
  }

  void stop() {
    stopped = true;
    gettimeofday(&end_count, NULL);
  }

  double get_elapsed_time_in_microsec() {
    if (!stopped) gettimeofday(&end_count, NULL);

    start_time_in_microsec =
        (start_count.tv_sec * 1000000.0) + start_count.tv_usec;
    end_time_in_microsec = (end_count.tv_sec * 1000000.0) + end_count.tv_usec;
    return end_time_in_microsec - start_time_in_microsec;
  }

  double get_elapsed_time_in_millisec() {
    return this->get_elapsed_time_in_microsec() * 0.001;
  }

  double get_elapsed_time_in_sec() {
    return this->get_elapsed_time_in_microsec() * 0.000001;
  }

  double get_elapsed_time() { return this->get_elapsed_time_in_sec(); }

 protected:
  double start_time_in_microsec;
  double end_time_in_microsec;
  bool stopped;

  timeval start_count;
  timeval end_count;
};
// -- utils

/**
 * @brief: 删除文件中的注释
 */
class AnnotionKiller {
 public:
  /**
   * @brief: core function 删除文件注释
   * @param {string&} 待处理字符串
   * @return {string} 处理之后字符串
   */
  string delete_annotion(string &str) {
    state_t state = CODE;
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
            ret += '/';
            ret += c;
            state = CODE;
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
            // 保持当前状态
            state = NOTE_MULTILINE;
          }
          break;
        case NOTE_MULTILINE_STAR:
          if (c == '/') {
            state = CODE;
          } else if (c == '*') {
            // 保持当前状态
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
   * @brief: utils-估计文件中的行数，不是那么准确！
   * @param {istream} 输入流
   * @return {long long} 估计得到的行数
   */
  long long estimate_lines_num(istream &is) {
    char buffer[BUFFER_SIZE];
    // estimate num of lines using 50 records，行数越大越准确
    is.seekg(0, ios::end);
    long long file_size = is.tellg();
    is.seekg(0, ios::beg);
    size_t i = 0;
    for (i = 0; i < 50; ++i) {
      if (is.fail()) break;
      is.getline(buffer, BUFFER_SIZE - 2);
    }
    if (i == 0) {
      cout << "[Error] bad file format" << endl;
      return 0;
    }
    int size = is.tellg();
    int iall = (file_size / size) * i;
    is.clear();
    is.seekg(0, ios::beg);
    return iall;
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
    int iall_record = estimate_lines_num(is);
    int icout = 0;

    // ostringstream oss;
    string lines;
    string strip_lines;
    string buffer;

    cout << "1. Now read data to memory ..." << endl;
    ProcessNotify notify(5);
    while (getline(is, buffer)) {
      if (is.fail()) {
        break;
      }
      // 加换行符
      lines += '\n';
      lines += buffer;
      ++icout;
      notify.push(icout * 100 / iall_record, "  reading");
    }
    cout << "[OK] all data have been read to memory" << endl;

    cout << "2. delete annotion ..." << endl;
    string s_lines;
    strip_lines = delete_annotion(lines);
    cout << "[OK] annotion have been deleted ..." << endl;
    cout << "3. Now write to file ..." << endl;
    os << strip_lines;
    cout << "[OK] file have been writed";
    is.close();
    os.close();
    return true;
  }

 protected:
  typedef enum State {
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
  } state_t;
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

  help_str notice[] = {
      {"<source file name>", " cpp file, annotion to be deleted"},
      {"[destination file name]", " you can specific by yourself"}};

  cout << "==============================================================="
       << endl;
  cout << argv[0] << " delete annotions from source file and output a new file."
       << endl;
  for (int i = 0; i < sizeof(notice) / sizeof(notice[0]); ++i) {
    cout << "- " << notice[i].item << notice[i].notice << endl;
  }
  cout << "==============================================================="
       << endl;
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
    cout << "[OK] user-defined ouput file name: " << des_file << endl;
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
  if (!ak.read_del_write(orig_file,
                         static_cast<const char *>(des_file.c_str()))) {
    cout << "[Error] failed to delete annotion ..." << endl;
  }
  return 0;
}

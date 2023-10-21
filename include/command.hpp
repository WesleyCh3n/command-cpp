#pragma once
#include <filesystem>
#include <initializer_list>
#include <string>
#include <vector>

#include <windows.h>

namespace fs = std::filesystem;

namespace process {

class ChildStdin {
  HANDLE r_;
  HANDLE w_;
  friend class Command;

public:
  ChildStdin() = default;
  ~ChildStdin() {
    CloseHandle(r_);
    CloseHandle(w_);
  };
};

class ChildStdout {
  HANDLE r_;
  HANDLE w_;
  friend class Command;

public:
  ChildStdout() = default;
  ~ChildStdout() {
    CloseHandle(r_);
    CloseHandle(w_);
  };
  void close_write() { CloseHandle(w_); }
  HANDLE get_read() { return r_; }
};

class ChildStderr {
  HANDLE r_;
  HANDLE w_;
  friend class Command;

public:
  ChildStderr() = default;
  ~ChildStderr() {
    CloseHandle(r_);
    CloseHandle(w_);
  };
  void close_write() { CloseHandle(w_); }
  HANDLE get_read() { return r_; }
};

class StdioPipes {
public:
  std::unique_ptr<ChildStdin> stdin_;
  std::unique_ptr<ChildStdout> stdout_;
  std::unique_ptr<ChildStderr> stderr_;
};

struct Output {
  uint32_t status;
  std::string stdout_str;
  std::string stderr_str;
};

class Process {
public:
  HANDLE handle_;
  HANDLE thread_handle_;
  ~Process() {
    CloseHandle(handle_);
    CloseHandle(thread_handle_);
  }
};

class Child {
public:
  std::unique_ptr<Process> process_;
  StdioPipes pipes_;
};

inline Output wait_with_output(std::unique_ptr<Process> proc,
                               StdioPipes pipes) {
  while (WaitForSingleObject(proc->handle_, 100) != WAIT_OBJECT_0) {
  }
  DWORD exit_code = 0;
  GetExitCodeProcess(proc->handle_, &exit_code);
  pipes.stdout_->close_write();
  pipes.stderr_->close_write();

  // parse output
  std::string stdout_str;
  std::string stderr_str;
  {
    uint32_t len;
    char msg[4096];
    bool success = false;
    while (true) {
      success = ReadFile(pipes.stdout_->get_read(), msg, 4096, (DWORD *)&len,
                         nullptr);
      if (success || len == 0)
        break;
    }
    stdout_str = std::string(msg, msg + len);
  }
  {
    uint32_t len;
    char msg[4096];
    bool success = false;
    while (true) {
      success = ReadFile(pipes.stderr_->get_read(), msg, 4096, (DWORD *)&len,
                         nullptr);
      if (success || len == 0)
        break;
    }
    stderr_str = std::string(msg, msg + len);
  }

  return Output{exit_code, stdout_str, stderr_str};
}

class Command {
public:
  class Impl {
    friend class Command;
    std::string program_;
    fs::path cwd_;
    std::vector<std::string> args_;

    Impl(std::string program) : program_{std::move(program)} {}

  public:
    Child spawn() {
      std::string cmd_str{};
      cmd_str.append(program_);
      for (const auto &arg : args_) {
        cmd_str.append(" ");
        cmd_str.append(arg);
      }
      std::string cwd = fs::absolute(cwd_).string();

      PROCESS_INFORMATION pi;
      pi.hProcess = nullptr;
      pi.hThread = nullptr;
      pi.dwProcessId = 0;
      pi.dwThreadId = 0;

      auto stdout_ = std::make_unique<ChildStdout>();
      auto stderr_ = std::make_unique<ChildStderr>();
      SECURITY_ATTRIBUTES sa;
      sa.nLength = sizeof(sa);
      sa.lpSecurityDescriptor = NULL;
      sa.bInheritHandle = TRUE;
      if (!CreatePipe(&stdout_->r_, &stdout_->w_, &sa, 0)) {
        throw std::runtime_error{"CreatePipe failed"};
      }
      if (!CreatePipe(&stderr_->r_, &stderr_->w_, &sa, 0)) {
        throw std::runtime_error{"CreatePipe failed"};
      }
      if (!SetHandleInformation(stdout_->r_, HANDLE_FLAG_INHERIT, 0)) {
        throw std::runtime_error{"SetHandleInformation failed"};
      }
      if (!SetHandleInformation(stderr_->r_, HANDLE_FLAG_INHERIT, 0)) {
        throw std::runtime_error{"SetHandleInformation failed"};
      }

      STARTUPINFO si{0};
      si.cb = sizeof(si);
      si.dwFlags |= STARTF_USESTDHANDLES;
      si.hStdInput = NULL;
      si.hStdOutput = stdout_->w_;
      si.hStdError = stderr_->w_;
      if (!CreateProcess(nullptr, (char *)cmd_str.c_str(), nullptr, nullptr,
                         true, 0, nullptr, cwd.c_str(), &si, &pi)) {
        throw std::runtime_error{"CreateProcess failed"};
      }
      auto proc = std::make_unique<Process>();
      proc->handle_ = pi.hProcess;
      proc->thread_handle_ = pi.hThread;
      // output
      return Child{
          std::move(proc),
          StdioPipes{nullptr, std::move(stdout_), std::move(stderr_)},
      };
    }

    Output output() {
      Child child = this->spawn();
      return wait_with_output(std::move(child.process_),
                              std::move(child.pipes_));
    }
  };

  static Command create(std::string program) { return Command(program); }
  Command &current_dir(fs::path currnet_dir) {
    impl_->cwd_ = currnet_dir;
    return *this;
  }
  Command &arg(std::string arg) {
    impl_->args_.emplace_back(arg);
    return *this;
  }
  Command &args(std::initializer_list<std::string> args) {
    impl_->args_.insert(impl_->args_.end(), args.begin(), args.end());
    return *this;
  }
  std::unique_ptr<Impl> build() { return std::move(impl_); }

private:
  Command(std::string program) : impl_(new Impl(program)) {}
  std::unique_ptr<Impl> impl_;
};

} // namespace process

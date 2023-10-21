#include <fmt/core.h>

#include "../include/command.hpp"

int main(int argc, char *argv[]) {
  auto output = process::Command::create("cmd.exe")
                    .current_dir("./")
                    .arg("/c")
                    .arg("echo")
                    .args({"Hello", "World"})
                    .build()
                    ->output();
  fmt::println("status: {}", output.status);
  fmt::print("stdout: {}", output.stdout_str);
  fmt::print("stderr: {}", output.stderr_str);
  return 0;
}

#include <iostream>
#include <string>
#include <unistd.h>   // for access(), fork(), execv()
#include <sstream>
#include <cstdlib>    // for getenv
#include <vector>
#include <sys/wait.h> // for waitpid
#include <cstring>    // for strerror

int main()
{
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // Prompt
  std::cout << "$ ";

  std::string input;
  std::getline(std::cin, input);

  if (input == "exit 0")
    return 0;
  else if (input.substr(0, 5) == "echo ")
    std::cout << input.substr(5) << "\n";
  else if (input.substr(0, 5) == "type ")
  {
    std::string arg = input.substr(5);

    // Builtins first
    if (arg == "echo" || arg == "exit" || arg == "type")
    {
      std::cout << arg << " is a shell builtin\n";
    }
    else
    {
      bool found = false;
      const char *pathEnv = std::getenv("PATH");

      if (pathEnv)
      {
        std::istringstream iss{pathEnv};
        std::string directory;

        while (std::getline(iss, directory, ':'))
        {
          if (directory.empty()) continue;

          std::string fullPath = directory + "/" + arg;

          // check if file exists and is executable
          if (access(fullPath.c_str(), X_OK) == 0)
          {
            std::cout << arg << " is " << fullPath << std::endl;
            found = true;
            break;
          }
        }
      }

      if (!found)
        std::cout << arg << ": not found\n";
    }
  }
  else
  {
    // Attempt to run an external program with arguments.
    // Parse input into tokens (simple whitespace split).
    std::istringstream iss(input);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok)
      tokens.push_back(tok);

    if (tokens.empty())
    {
      std::cout << input << ": command not found" << std::endl;
      main();
      return 0;
    }

    std::string prog = tokens[0];

    // Search PATH for executable (same as in 'type')
    std::string fullPath;
    const char *pathEnv = std::getenv("PATH");
    if (pathEnv)
    {
      std::istringstream piss{pathEnv};
      std::string directory;
      while (std::getline(piss, directory, ':'))
      {
        if (directory.empty()) continue;
        std::string candidate = directory + "/" + prog;
        if (access(candidate.c_str(), X_OK) == 0)
        {
          fullPath = candidate;
          break;
        }
      }
    }

    // If not found in PATH, try if prog itself is an executable path (relative or absolute)
    if (fullPath.empty())
    {
      if (access(prog.c_str(), X_OK) == 0)
        fullPath = prog;
    }

    if (fullPath.empty())
    {
      std::cout << prog << ": command not found" << std::endl;
      main();
      return 0;
    }

    // Build argv for execv
    std::vector<char*> argv;
    for (size_t i = 0; i < tokens.size(); ++i)
      argv.push_back(const_cast<char*>(tokens[i].c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0)
    {
      // child
      execv(fullPath.c_str(), argv.data());
      // if execv returns, it failed
      std::cerr << prog << ": " << strerror(errno) << std::endl;
      _exit(127);
    }
    else if (pid > 0)
    {
      // parent: wait for child to finish so child's output is visible to tester
      int status = 0;
      waitpid(pid, &status, 0);
    }
    else
    {
      std::cerr << "Fork failed" << std::endl;
    }
  }

  main(); // recursion for next command
}

#include <iostream>
#include <string>
#include <unistd.h>     // access(), fork(), execv(), getcwd(), chdir(), getuid()
#include <sstream>
#include <cstdlib>      // getenv()
#include <vector>
#include <sys/wait.h>   // waitpid()
#include <cstring>      // strerror()
#include <cerrno>       // errno
#include <pwd.h>        // getpwuid()
#include <sys/types.h>  // for uid_t

int main()
{
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // Prompt
  std::cout << "$ ";

  std::string input;
  if (!std::getline(std::cin, input))
    return 0; // EOF or error

  // exit builtin (exact match "exit 0" ends program for tests)
  if (input == "exit 0")
    return 0;

  // echo builtin: print everything after "echo " (or just newline if no args)
  else if (input.size() >= 5 && input.substr(0, 5) == "echo ")
  {
    std::cout << input.substr(5) << "\n";
  }
  else if (input == "echo")
  {
    std::cout << "\n";
  }

  // type builtin: identify builtins or locate program in PATH
  else if (input.size() >= 5 && input.substr(0, 5) == "type ")
  {
    std::string arg = input.substr(5);

    // Builtins recognized by this shell
    if (arg == "echo" || arg == "exit" || arg == "type" || arg == "pwd" || arg == "cd")
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

  // pwd builtin: print absolute current working directory
  else if (input == "pwd")
  {
    // reasonable buffer size
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
      std::cout << cwd << "\n";
    else
      std::cerr << "pwd: error retrieving current directory\n";
  }

  // cd builtin: handles absolute, relative and "~" expansion (including "~/subpath")
  else if (input.size() >= 3 && input.substr(0, 3) == "cd ")
  {
    std::string path = input.substr(3);

    // Trim leading/trailing whitespace (defensive)
    if (!path.empty())
    {
      size_t first = path.find_first_not_of(' ');
      if (first == std::string::npos) path.clear();
      else
        path = path.substr(first, path.find_last_not_of(' ') - first + 1);
    }

    // Resolve ~ to home directory. Use HOME env var first; if not set, use getpwuid.
    if (!path.empty() && path[0] == '~')
    {
      const char* home_env = std::getenv("HOME");
      std::string homeStr;
      if (home_env && home_env[0] != '\0')
      {
        homeStr = std::string(home_env);
      }
      else
      {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_dir)
          homeStr = std::string(pw->pw_dir);
      }

      if (!homeStr.empty())
      {
        if (path == "~")
        {
          path = homeStr;
        }
        else if (path.size() > 1 && path[1] == '/')
        {
          // "~/something" => replace leading '~' with home
          path = homeStr + path.substr(1);
        }
        // else leave as-is (rare)
      }
      // if homeStr empty, we leave path as "~" so chdir will fail and produce an error
    }

    if (path.empty())
    {
      // No argument or after trimming nothing left
      std::cerr << "cd: missing argument" << std::endl;
    }
    else
    {
      if (chdir(path.c_str()) != 0)
      {
        std::cerr << "cd: " << path << ": No such file or directory" << std::endl;
      }
      // On success, cd prints nothing
    }
  }

  // External command: parse arguments, search PATH, fork + exec, wait
  else
  {
    // Tokenize input by whitespace (no quote handling)
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

    // Search PATH for executable
    std::string fullPath;
    const char *pathEnv = std::getenv("PATH");
    if (pathEnv)
    {
      std::istringstream pathIss{pathEnv};
      std::string directory;
      while (std::getline(pathIss, directory, ':'))
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

    // If not found in PATH, check if prog is a direct path (relative or absolute)
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

    // Build argv array for execv (char* array, null terminated)
    std::vector<char*> argv;
    for (size_t i = 0; i < tokens.size(); ++i)
      argv.push_back(const_cast<char*>(tokens[i].c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0)
    {
      // child: replace process image with external program
      execv(fullPath.c_str(), argv.data());
      // if execv returns, it failed
      std::cerr << prog << ": " << strerror(errno) << std::endl;
      _exit(127);
    }
    else if (pid > 0)
    {
      // parent: wait for child so child's output is visible
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

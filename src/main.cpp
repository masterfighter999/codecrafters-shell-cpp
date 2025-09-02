#include <iostream>
#include <string>
#include <unistd.h>   // for access()
#include <sstream>
#include <cstdlib>    // for getenv

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
    std::cout << input << ": command not found" << std::endl;
  }

  main(); // recursion for next command
}

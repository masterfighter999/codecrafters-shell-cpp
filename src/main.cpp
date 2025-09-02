#include <iostream>
#include <string>

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // Uncomment this block to pass the first stage
  std::cout << "$ ";

  std::string input;
  std::getline(std::cin, input);
  if (input == "exit 0")
    return 0;
  else if (input.substr(0, 5) == "echo ")
    std::cout << input.substr(5) << "\n";
  else if (input.substr(0, 5) == "type ")
  {
    if (input.substr(5, 4) == "echo" || input.substr(5, 4) == "exit" || input.substr(5, 4) == "type")
      std::cout << input.substr(5) << " is a shell builtin\n";

    else
      std::cout << input.substr(5) << ": not found\n";
  }
  else
    std::cout << input << ": command not found" << std::endl;

  main();
}

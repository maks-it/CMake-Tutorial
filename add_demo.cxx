#include "MathFunctions.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " number" << std::endl;
    return 1;
  }

  const int input = std::stoi(argv[1]);
  std::cout << input << " + 9 = " << mathfunctions::add(input, 9) << std::endl;
  return 0;
}

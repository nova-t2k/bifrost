#include "Bifrost.h"

int main()
{
  std::cout << "Inside: trying to create bifrost" << std::endl;
  Bifrost bf;

  std::cout << "Inside: trying to receive an int" << std::endl;
  int i;
  bf >> i;
  std::cout << "Inside: Got " << i << std::endl;

  std::cout << "Inside: trying to receive a double" << std::endl;
  double d;
  bf >> d;
  std::cout << "Inside: Got " << d << std::endl;

  std::cout << "Inside: returning the square" << std::endl;
  bf << d*d;
  std::cout << "Inside: all done" << std::endl;

  return 0;
}

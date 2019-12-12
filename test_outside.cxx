#include "Bifrost.h"

int main()
{
  Bifrost bf("/home/bckhouse/containers/slf6.7-ssh.simg");

  std::cout << "Outside: sending an int" << std::endl << std::flush;
  bf << 42;
  std::cout << "Outside: sending a double" << std::endl;
  bf << 42.3;

  std::cout << "Outside: retrieving result" << std::endl;
  double d;
  bf >> d;
  std::cout << "Ouside: got " << d << std::endl;

  std::cout << "Outside: all done" << std::endl;

  return 0;
}

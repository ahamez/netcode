#include <iostream>
int main (int argc, char const *argv[])
{
  std::cout << __clang_major__ << " " << __clang_minor__ << '\n';
  return 0;
}
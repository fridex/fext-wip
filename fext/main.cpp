
#include <iostream>

#include "eheapq.hpp"

using namespace std;

int main(int argv, char * argc[]) {
  auto a = EHeapQ<int>(10);

  cout << "default size: " << EHEAPQ_DEFAULT_SIZE << std::endl;
  cout << "Hello, world!\n" << a.get_size() << std::endl;

  auto l = {35222, 33108, -24189, 50489, -64703};
  for (auto i: l)
      a.push(i);

  while (a.get_size() != 0)
      std::cout << a.pop() << std::endl;

//  std::cout << "max: " << a.get_max() << " last: " << a.get_last() << " top: " << a.get_top() << std::endl;
//  a.push(2);
//  std::cout << "max: " << a.get_max() << " last: " << a.get_last() << " top: " << a.get_top() << std::endl;
//  a.push(-1);
//  std::cout << "max: " << a.get_max() << " last: " << a.get_last() << " top: " << a.get_top() << std::endl;

//  for (auto i: *(a.get_items())) {
//    std::cout << i << std::endl;
//  }

  return 0;
}

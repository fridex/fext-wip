##########################

import eheapq

a = eheapq.ExtHeapQueue()

a.push(1)
a.push(3)
print("top:\t", a.get_top())
print("last:\t", a.get_last())
print("max:\t", a.get_max())
print("len:\t", len(a))
print("size:\t", a.size)
print(a.pop())
a.replace(4)
print(a.get_top())
print("len:\t", len(a))
print(a.get_top())
print(a.get_top())
print(a.get_top())
print(a.get_top())
print(a.get_top())
a.pop()
print(a.pushpop(10))
##########################

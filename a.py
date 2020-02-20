import edict
import eheapq
import random

a = eheapq.ExtHeapQueue(weakref=False)

a.push(97)
a.pushpop(-38)
a.pop()
a.pop()


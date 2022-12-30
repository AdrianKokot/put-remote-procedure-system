import sys

arglist = sys.argv[1:]

result = 1;
for x in [int(x) for x in arglist]:
  result *= x;

print(result)
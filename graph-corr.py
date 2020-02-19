#!/usr/bin/env python3

import matplotlib.pyplot as plt
import csv
import sys

x = []
y = []
firstRow = True

with open(sys.argv[1]) as f:
  spamreader = csv.reader(f)
  for row in spamreader:
    if firstRow:
      firstRow = False
      continue
    (x_local,y_local) = row
    x.append(float(x_local))
    y.append(float(y_local))
# print(len(x))
# print(len(y))
plt.title("Correlation vs Frame Length")
plt.plot(x,y)
plt.show()

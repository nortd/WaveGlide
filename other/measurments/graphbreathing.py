import matplotlib.pyplot as plt



with open("values.txt") as f:
    vals = f.readlines()



x_series = range(len(vals))
y_series = [int(x) for x in vals]
plt.plot(x_series, y_series)
plt.savefig("graph.png")

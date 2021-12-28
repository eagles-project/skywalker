import os, sys, importlib
import matplotlib.pyplot as plt
import matplotlib.tri as tri
import numpy as np

# Look for data in whatever directory we're running in.
sys.path.append(os.getcwd())

def plot_isotherms(data_module):
    """Plot the contours of the temperature as a function of volume and pressure."""

    data = importlib.import_module(data_module)
    p, V, T = data.input.p, data.input.V, data.output.T

    # Interpolate the data onto a triangulated grid.
    nx, ny = 100, 100
    Vi = np.linspace(min(V), max(V), nx)
    pi = np.linspace(min(p), max(p), ny)
    triang = tri.Triangulation(V, p)
    interpolator = tri.CubicTriInterpolator(triang, T)
    Xi, Yi = np.meshgrid(Vi, pi)
    Ti = interpolator(Xi, Yi)

    # Plot contours. We get a little fancy in order to explicitly set log levels
    # because the ticker.LogLocator doesn't "get it."
    levels = [273, 373, 473, 573, 673]
    cs = plt.contour(Vi, pi, Ti, levels)
    plt.clabel(cs, inline=1, fontsize=10)

    plt.xlabel('Volume [m^3]')
    plt.ylabel('Pressure [Pa]')
    plt.title('Temperature [K]')
    plt.savefig(data_module + '.png')
    plt.clf()

def usage():
    print('plot_isotherms.py: generates plots of Van der Waals isotherms.')
    print('usage: python3 plot_isotherms.py <module_name>')
    print('Here, <module_name> is the name of a Python data module generated')
    print('by Skywalker, excluding the .py suffix (e.g. \'n2_gas\').')
    exit(0)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        usage()
    else:
        data_module = sys.argv[1]
    plot_isotherms(data_module)


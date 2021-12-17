import os, sys, importlib
import matplotlib.pyplot as plt
from matplotlib import colors
import matplotlib.tri as tri
import numpy as np

# Look for data in whatever directory we're running in.
sys.path.append(os.getcwd())

# This function returns true if x and y are "equal" for plotting purposes,
# false if not
def equal(x, y):
    return abs(x - y) < 1e-6

def plot_isotherms(mod_file):
    """Plot the contours of the temperature as a function of volume and pressure."""

    data = importlib.import_module(mod_file)
    p, V, T = data.input.p, data.input.V, data.output.T

    # Interpolate the data onto a triangulated grid.
    nx, ny = 100, 100
    pi = np.linspace(min(p), max(p), nx)
    Vi = np.linspace(min(V), max(V), ny)
    triang = tri.Triangulation(RH, T)
    interpolator = tri.LinearTriInterpolator(triang, J)
#    interpolator = tri.CubicTriInterpolator(triang, J)
    Xi, Yi = np.meshgrid(pi, Vi)
    Ti = interpolator(Xi, Yi)

    # Plot contours. We get a little fancy in order to explicitly set log levels
    # because the ticker.LogLocator doesn't "get it."
#    plt.suptitle('Nucleation rate [#/cc/s]')
    fig, ax = plt.subplots()
    lev_exp = np.arange(-3, 11)
    levels = np.power(10., lev_exp)
    contours = ax.contour(RHi, Ti, Ji, levels, colors='k')
    fills = ax.contourf(RHi, Ti, Ji, levels, cmap='jet',
                        norm=colors.LogNorm(), extend='min')

    ax.set_xlabel('Volume [m^3]')
    ax.set_ylabel('Pressure [Pa]')
    ax.set_title('Temperature [K]')
    fig.colorbar(fills)
    plt.savefig(prefix + fig_name + '.png')
    plt.clf()

def usage():
    print('generate_plots.py: generates plots for MAM nucleation parameterizations.')
    print('usage: python3 generate_plots.py <prefix>')
    print('Here, <prefix> is prepended to each Python module containing data')
    print('computed using a Skywalker-powered driver program. The Python modules')
    print('should be named <prefix>_<figure_name>.py, where <figure_name> is')
    print('the name of a figure as represented by the YAML files in this directory.')
    print('(e.g. haero_vehkamaki2002_contour.py, for vehkamaki2002_contour.yaml)')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        usage()
    else:
        mod_file = sys.argv[1]
    plot_isotherms(mod_file)


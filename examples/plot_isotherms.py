import os, sys, importlib
import matplotlib.pyplot as plt
import matplotlib.tri as tri
import numpy as np

# Look for data in whatever directory we're running in.
sys.path.append(os.getcwd())

def approx_equal(x, y):
    return abs(x - y) < 1e-6

def plot_isotherms(data_module):
    """Plot isothermal processes on a V-p diagram."""

    data = importlib.import_module(data_module)
    V, T, p = data.input.V, data.input.T, data.output.p

    temps = [273, 373, 473, 573, 673]
    for temp in temps:
        # extract points that match the desired temperature
        indices = [i for i in range(0, len(V)) if approx_equal(T[i], temp)]
        Vi = [V[i] for i in indices]
        pi = [p[i] for i in indices]
        plt.plot(Vi, pi, label='T = %g K'%temp)
    plt.legend(fontsize='x-small')
    plt.xlabel('Volume [m^3]')
    plt.ylabel('Pressure [Pa]')
    plt.ylim(0, 1e7)
    plt.title('Isotherms (%s)'%data_module)
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


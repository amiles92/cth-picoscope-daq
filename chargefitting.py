import numpy as np
import matplotlib.pyplot as plt
from scipy import stats, optimize
import math


##### PARAMETERS #########

#inputfilepath = 'plots/20Jun24_80V_1.41V_Diffuser_Sheet_MPPC3_LightsOn_Rotated_chA_minimumMaChData.npy' 
#inputfilepath = 'plots/20Jun24_81V_1.44V_Diffuser_Sheet_MPPC3_LightsOn_chA_minimumMaChData.npy' 
#inputfilepath = 'plots/24Jun24_80V_1.35V_MPPC1_chB_minimumMaChData.npy'
#inputfilepath = 'plots/24Jun24_80V_1.36V_MPPC1_chB_minimumMaChData.npy'
inputfilepath = 'npys/24Jun24/24Jun24_80V_Dark_MPPC10_chA_minimumMaChData.npy'

nbins = 300
p0_norm = [5, 0, 0.1, -2.7, 0.1] ## likely fit parameters
p0bounds_norm =([0, -4, 0, -10, 0], [15, 0.5, 2, 0, 2]) ## fit parameters bounds
i1, i2 = 0, nbins-1 ## fitting range, eg: int(nbins/3), nbins-1
#i1, i2 = int(nbins/3), nbins-1
kmax = 30 ## max of poisson sum

##### FUNCTIONS ##########

def normalize(x, y):
    return sum( [ y[i]*(x[i+1]-x[i]) for i in range(len(x)-1) ] )

def binx(bins):
    x = []
    for i in range(len(bins)-1):
        x.append( (bins[i]+bins[i+1])/2 )
    return np.array(x)

def normal(x, mu, sg):
    return np.exp( -0.5*(x-mu)**2 / sg**2 ) / ( sg*np.sqrt(2*np.pi) )

def chargeDist(q, lb, q0, sg0, mu, sg):
    ### charge distribution 
    ### following a poisson number of pe (lb), with normal gain (mu, sg)
    ### and noise factor noisefac
    P = 0
    for k in range(kmax):
        P += ( lb**k * np.exp(-lb) / math.factorial(k) ) * normal( q, k*mu+q0, np.sqrt( (k*sg)**2 + sg0**2 ) )
    return P  

##### FITTING ############

minima = np.load(inputfilepath)

hist, bins = np.histogram(minima, nbins)
x = binx(bins)

histnorm = hist/normalize(x, hist)

par, cov = optimize.curve_fit(chargeDist, x[i1:i2], histnorm[i1:i2], p0=p0_norm, bounds=p0bounds_norm)

print("[lambda, q0, sigma0, mu, sigma] =", par)

yfit = chargeDist(x[i1:i2], *par)

plt.hist(minima, nbins, histtype='step', density=1, label='Charge distribution'
    + '\n' + r'$N_\text{bins}$ = ' + str(nbins))
plt.plot(x[i1:i2], yfit, ls='-', label='Poissonian-gaussian fit' 
    + '\n' + r'[$\lambda$, $Q_0$, $\sigma_0$, $\mu$, $\sigma$] =' 
    + '\n' + '[{:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}]'.format(*par))
plt.xlabel("Minimum charge [mV]")
plt.ylabel("Normalised frequency")
plt.legend()

plt.show()

import numpy as np
import matplotlib.pyplot as plt
from scipy import stats, optimize
import math


##### PARAMETERS #########

inputfilepath = '20Jun24_81V_1.44V_Diffuser_Sheet_MPPC3_LightsOn_chA_minimumMaChData.npy' 
# inputfilepath = '20Jun24_81V_1.41V_Diffuser_Sheet_MPPC3_LightsOn_chA_minimumMaChData.npy' 

nbins = 300
# p0 = [1, 5, 3, 1, 1, 1000]
# p0_normed = [-3, 6, -3, 2, 1] ## likely fit parameters
p0_norm = [0, 5, -2.7, 0.1, 0.1] ## likely fit parameters
p0bounds_norm =([-4, 0, -10, 0, 0], [0.5, 15, 0, 2, 2]) ## fit parameters bounds
i1, i2 = 0, nbins-1 ## fitting range, eg: int(nbins//5), nbins-1
# fit_range = [-15, -3] ## in mV
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
    return np.exp( -0.5*(x-mu)**2/sg**2 ) / (sg*np.sqrt(2*np.pi))

def chargeDist(q, q0, lb, mu, sg, sg0, A):
    ### charge distribution 
    ### following a poisson number of pe (lb), with normal gain (mu, sg)
    ### and normalization factor A
    # kmax = 20
    P = 0
    for k in range(kmax):
        if k==0:
            P += np.exp(-lb) * normal(q, q0, sg0)
        if k==1:
            P += lb*np.exp(-lb) * normal(q, mu+q0, sg+sg0)
        else:
            P += ( lb**k*np.exp(-lb)/math.factorial(k) ) * normal(q, k*mu+q0, k*sg+sg0)
    P *= A
    return P 

def chargeDist_noped(q, q0, lb, mu, sg, sg0, A):
    ### charge distribution 
    ### following a poisson number of pe (lb), with normal gain (mu, sg)
    ### and normalization factor A
    # kmax = 20
    P = 0
    for k in range(1, kmax):
        if k==1:
            P += lb*np.exp(-lb) * normal(q, mu+q0, sg+sg0)
        else:
            P += ( lb**k*np.exp(-lb)/math.factorial(k) ) * normal(q, k*mu+q0, k*sg+sg0)
    P *= A
    return P

def chargeDist_norm(q, q0, lb, mu, sg, sg0):
    ### charge distribution 
    ### following a poisson number of pe (lb), with normal gain (mu, sg)
    ### and noise factor noisefac
    # kmax = 20
    P = 0
    for k in range(kmax):
        if k==0:
            P += np.exp(-lb) * normal(q, q0, sg0)
        if k==1:
            P += lb*np.exp(-lb) * normal(q, mu+q0, sg+sg0)
        else:
            P += ( lb**k*np.exp(-lb)/math.factorial(k) ) * normal(q, k*mu+q0, k*sg+sg0)
    return P  


##### FITTING ############

minima = np.load(inputfilepath)

hist, bins = np.histogram(minima, nbins)
x = binx(bins)

norm = normalize(x, hist)
histnorm = hist/norm

# par, cov = optimize.curve_fit(chargeDist, x, hist, p0=p0)
par, cov = optimize.curve_fit(chargeDist_norm, x[i1:i2], histnorm[i1:i2], p0=p0_norm, bounds=p0bounds_norm)
# par, cov = optimize.curve_fit(chargeDist_norm, x[i1:i2], histnorm[i1:i2], p0=p0_norm)

print("[q0, lambda, mu, sigma, sigma0] = ")
print(par)

# y = chargeDist(x, *par)
yfit = chargeDist_norm(x[i1:i2], *par)

plt.hist(minima, nbins, histtype='step', density=1, label='Charge distribution'
    + '\n' + r'$N_\text{bins}$ = ' + str(nbins))
# plt.plot(x, histnorm, 'o', ms=3)
plt.plot(x[i1:i2], yfit, ls='-', label='Poissonian-gaussian fit' 
    + '\n' + r'[$Q_0$, $\lambda$, $\mu$, $\sigma$, $\sigma_0$] =' 
    + '\n' + '[{:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}]'.format(*par))
plt.xlabel("Minimum charge [mV]")
plt.ylabel("Frequency (normalized)")
plt.legend()

plt.show()

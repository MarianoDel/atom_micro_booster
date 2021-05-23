# -*- coding: utf-8 -*-
#usar python3
import numpy as np
from sympy import *
import matplotlib.pyplot as plt
from scipy.signal import lti, bode, lsim, dbode, zpk2tf, tf2zpk, step2, cont2discrete, dstep, freqz, freqs, dlti, TransferFunction
from tc_udemm import sympy_to_lti, lti_to_sympy, plot_argand
from pid_tf import PID_int
from recursive_tf import RecursiveTF

"""
        MicroInverter - Booster stage
        as is a push pull device the duty is doubled
        CCM mode only
        Output:
        Sensed by an optocoupler 3000pts adc @ 350V

"""

##########################################
# Graphics I want to show on the script. #
##########################################
Bode_Filter_Analog = False

Bode_Filter_Digital = False
Poles_Zeros_Digital = False

Step_Filter_Digital = True
Step_Filter_Digital_Recursive = True


#TF without constant
s = Symbol('s')

# Boost Stage
# From Vinput to Voutput
Lout = 1.8e-3
Rout = 816    #150W on 350V
Cout = 33e-6

# VCC for low-signal graphics
Vinput = 1
# Vinput = 40

#    ---Z1---+--------+
#            |        |
#           Z2       Z3
#            |        |
#    --------+--------+
Z1 = s * Lout
Z2 = 1 / (s * Cout)
Z3 = Rout

# Boost out
print (f'Boost out small signal no opto Vinput = {Vinput}V:')
Transf_Z3_Vinput = (Z2 * Z3 /(Z2 + Z3)) / (Z1 + (Z2 * Z3 /(Z2 + Z3))) * Vinput
Filter_out_sim = Transf_Z3_Vinput.simplify()
print (Filter_out_sim)

# Opamp
# print (f'Filtro T con Opamp en Rsense Vinput = {Vinput}V: ')
# Transf_Z4_Vinput_Opamp = Transf_Z4_Vinput * Amp_gain
# Filter_Opamp_out_sim = Transf_Z4_Vinput_Opamp.simplify()
# print (Filter_Opamp_out_sim)



################################################
# Algunas confirmaciones de la parte Analogica #
################################################
filter_TF = sympy_to_lti(Filter_out_sim)
if Bode_Filter_Analog == True:
    wfreq = np.arange(2*np.pi, 2*np.pi*100000, 1)
    w, mag_p, phase_p = bode(filter_TF, wfreq)

    fig, (ax1, ax2) = plt.subplots(2,1)
    ax1.semilogx (w/6.28, mag_p, 'b-', linewidth="1")
    # ax1.semilogx (w/6.28, mag_s, 'g-', linewidth="1")
    ax1.set_title(f'Magnitude Input to Output Filter Analog Vinput = {Vinput}V')

    ax2.semilogx (w/6.28, phase_p, 'b-', linewidth="1")
    # ax2.semilogx (w/6.28, phase_s, 'g-', linewidth="1")
    ax2.set_title('Phase')

    plt.tight_layout()
    plt.show()


##########################################
# Conver Plant by zoh over sampling freq #
# so no to affect zeroes and poles       #
##########################################
Fsampling = 24000
Tsampling = 1 / Fsampling

filter_dig_zoh_n, filter_dig_zoh_d, td = cont2discrete((filter_TF.num, filter_TF.den), Tsampling, method='zoh')

#normalized with TransferFunction
print ("Digital Filter Zoh:")
filter_dig_zoh = TransferFunction(filter_dig_zoh_n, filter_dig_zoh_d, dt=td)
print (filter_dig_zoh)

####################################
# Some tests over the digital part #
####################################
if Bode_Filter_Digital == True:
    w, mag_zoh, phase_zoh = dbode(filter_dig_zoh, n = 10000)

    fig, (ax1, ax2) = plt.subplots(2,1)

    ax1.semilogx(w/(2*np.pi), mag_zoh, 'y')
    ax1.set_title(f'Filter Digital Bode ZOH Vinput = {Vinput}V')
    ax1.set_xlabel('Frequency [Hz]')

    ax2.semilogx(w/(2*np.pi), phase_zoh, 'y')
    ax2.set_xlabel('Frequency [Hz]')

    plt.tight_layout()
    plt.show()
        

########################################
# Poles and Zeros on the Digital Plant #
########################################
if Poles_Zeros_Digital == True:
    plot_argand(filter_dig_zoh)

    
##############################
# Step Response verification #
##############################
simulation_time = 0.1
t = np.linspace(0, simulation_time, num=(simulation_time*Fsampling))


if Step_Filter_Digital == True:
    tout, yout_zoh = dstep([filter_dig_zoh.num, filter_dig_zoh.den, td], t=t)
    yout1 = np.transpose(yout_zoh)
    yout0 = yout1[0]
    yout_zoh = yout0[:tout.size]

    fig, ax = plt.subplots()
    ax.set_title('Step Filter Digital ZOH')
    ax.set_ylabel('Plant Voltage')
    ax.set_xlabel('Time [s]')
    ax.grid()

    ax.plot(tout, yout_zoh, 'y')
    # ax.set_ylim(ymin=-20, ymax=100)

    plt.tight_layout()
    plt.show()


##############################################
# Plant Step Response converted to Recursive #
##############################################
if Step_Filter_Digital_Recursive == True:
    # ZOH
    b_planta = np.transpose(filter_dig_zoh.num)
    a_planta = np.transpose(filter_dig_zoh.den)

    vin_plant = np.ones(t.size)
    vout_plant_method2 = np.zeros (t.size)    
    recur_planta = RecursiveTF(b_planta, a_planta)
    for i in range(t.size):
        vout_plant_method2[i] = recur_planta.newOutput(vin_plant[i])
    

    fig, ax = plt.subplots()
    ax.set_title('Step Plant Digital Recursive ZOH')
    ax.set_ylabel('Plant Voltage')
    ax.set_xlabel('Time [s]')
    ax.grid()
    ax.plot(t, vout_plant_method2, 'y')    
    plt.tight_layout()
    plt.show()
    

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
Bode_Analog_Plant = False

Bode_Digital_Plant = False
Poles_Zeros_Digital_Plant = False

Step_Digital_Plant = False
Step_Digital_Plant_Recursive = False

Bode_Digital_Controller = True
Bode_Digital_Plant_Controller_OpenLoop_CloseLoop = True
Poles_Zeros_Digital_CloseLoop = False

Step_Digital_CloseLoop = True

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



##################################
# Some checks on the Analog part #
##################################
filter_TF = sympy_to_lti(Filter_out_sim)
if Bode_Analog_Plant == True:
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
if Bode_Digital_Plant == True:
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
if Poles_Zeros_Digital_Plant == True:
    plot_argand(filter_dig_zoh)

    
##############################
# Step Response verification #
##############################
simulation_time = 0.1
t = np.linspace(0, simulation_time, num=(simulation_time*Fsampling))


if Step_Digital_Plant == True:
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
if Step_Digital_Plant_Recursive == True:
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



#####################################
# USED IN CASE OF CUSTOM CONTROLLER #
#####################################
# cont_zeros = [-0.89+0.29j, -0.89-0.29j]
# cont_poles = [0.]
# cont_const = 0.01
# cont_zpk_b, cont_zpk_a = zpk2tf(cont_zeros, cont_poles, cont_const)

# controller_tf = TransferFunction(cont_zpk_b, cont_zpk_a, dt=td)
# print ("Digital Custom Controller:")
# print (controller_tf)

##################################
# USED IN CASE OF PID CONTROLLER #
##################################
""" 
    Only for PI dig:
    w0 ~= ki_dig / kp_dig * Fsampling or Fundersampling
    plateau gain ~= 20 log kp_dig

    Only for PD dig:
    w0 ~= kp_dig / kd_dig * Fsampling or Fundersampling
    plateau gain ~= 20 log kp_dig

"""

# kp_dig = 100 / 256
# ki_dig = 1 / 256
# kd_dig = 200 / 256

kp_dig = 50 / 128
ki_dig = 1 / 128
kd_dig = 100 / 128

k1 = kp_dig + ki_dig + kd_dig
k2 = -kp_dig - 2*kd_dig
k3 = kd_dig

b_pid = [k1, k2, k3]
a_pid = [1, -1]

controller_tf = TransferFunction(b_pid, a_pid, dt=td)    

b_cont = controller_tf.num
a_cont = controller_tf.den
print("Controller Parameters:")
print(f"b[2]: {b_cont[0]} b[1]: {b_cont[1]} b[0]: {b_cont[2]}")
print(f"a[1]: {a_cont[0]} a[0]: {a_cont[1]}")


if Bode_Digital_Controller == True:
    w, mag, phase = dbode(controller_tf, n = 10000)
    fig, (ax1, ax2) = plt.subplots(2,1)
    ax1.semilogx(w/(2*np.pi), mag, 'c')
    ax1.set_title('Digital Controller')
    ax1.set_ylabel('Amplitude [dB]', color='c')
    ax1.set_xlabel('Frequency [Hz]')

    ax2.semilogx(w/(2*np.pi), phase, 'c')
    ax2.set_ylabel('Phase', color='c')
    ax2.set_xlabel('Frequency [Hz]')

    plt.tight_layout()
    plt.show()

    
contr_dig = lti_to_sympy(controller_tf)
plant_dig = lti_to_sympy(filter_dig_zoh)
ol_dig = contr_dig * plant_dig

open_loop_dig = sympy_to_lti(ol_dig)
close_loop_dig = sympy_to_lti(ol_dig/(1+ol_dig))
    
#normalize with TransferFunction
open_loop_dig = TransferFunction(open_loop_dig.num, open_loop_dig.den, dt=td)    
close_loop_dig = TransferFunction(close_loop_dig.num, close_loop_dig.den, dt=td)


if Bode_Digital_Plant_Controller_OpenLoop_CloseLoop == True:    
    w, mag_ol, phase_ol = dbode(open_loop_dig, n = 100000)
    w, mag_cl, phase_cl = dbode(close_loop_dig, n = 100000)
    
    fig, (ax1, ax2) = plt.subplots(2,1)
    ax1.semilogx(w/(2*np.pi), mag_ol, 'b')
    ax1.semilogx(w/(2*np.pi), mag_cl, 'c')    
    ax1.set_title('Open Loop Blue - Close Loop Cyan')
    ax1.set_ylim(ymin=-50, ymax=50)

    ax2.semilogx(w/(2*np.pi), phase_ol, 'b')
    ax2.semilogx(w/(2*np.pi), phase_cl, 'c')    
    ax2.set_title('Phase')
    ax2.set_xlabel('Frequency [Hz]')

    plt.tight_layout()
    plt.show()


if Poles_Zeros_Digital_CloseLoop == True:
    plot_argand(close_loop_dig)



##################################
# Check Close Loop Step Response #
##################################
tout, yout_zoh = dstep([close_loop_dig.num, close_loop_dig.den, td], t=t)
yout1 = np.transpose(yout_zoh)
yout0 = yout1[0]
yout_zoh = yout0[:tout.size]


if Step_Digital_CloseLoop == True:
    fig, ax = plt.subplots()
    ax.set_title('Step Planta Digital Realimentada')
    ax.set_ylabel('Tension del Sensor')
    ax.set_xlabel('Tiempo [s]')
    ax.grid()
    # ax.plot(tout, yout_ef, 'g')
    ax.plot(tout, yout_zoh, 'y')
    # ax.set_ylim(ymin=-20, ymax=100)

    plt.tight_layout()
    plt.show()

    

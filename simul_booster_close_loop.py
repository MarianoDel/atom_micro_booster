# -*- coding: utf-8 -*-
#usar python3
import numpy as np
import matplotlib.pyplot as plt
# import sys
# import math

#####################
# Utility Functions #
#####################
def get_vector_lines_float (myfile, line):
    v_str = myfile[line]
    v_str = v_str.strip('\n')
    str_data = str(fl[line + 1])
    v_data = np.fromstring(str_data, dtype=np.float32, sep=' ')
    print(f"vector: {v_str} numpy array size: {v_data.size} first element: {v_data[0]}")
    return v_data

def get_vector_lines_short (myfile, line):
    v_str = myfile[line]
    v_str = v_str.strip('\n')
    str_data = str(fl[line + 1])
    v_data = np.fromstring(str_data, dtype=np.int16, sep=' ')
    print(f"vector: {v_str} numpy array size: {v_data.size} first element: {v_data[0]}")
    return v_data

def get_vector_lines_ushort (myfile, line):
    v_str = myfile[line]
    v_str = v_str.strip('\n')
    str_data = str(fl[line + 1])
    v_data = np.fromstring(str_data, dtype=np.uint16, sep=' ')
    print(f"vector: {v_str} numpy array size: {v_data.size} first element: {v_data[0]}")
    return v_data


################################################
# Open the data file with the vectors to graph #
################################################
file = open ('data.txt', 'r')

###################
# Get the vectors #
###################
#readlines reads the individual line into a list
fl =file.readlines()

vinput = get_vector_lines_float (fl, 0)
voutput = get_vector_lines_float (fl, 2)
reference_v = get_vector_lines_float (fl, 4)

reference = get_vector_lines_ushort (fl, 6)
duty_applied = get_vector_lines_ushort (fl, 8)
vinput_adc = get_vector_lines_ushort (fl, 10)
voutput_adc = get_vector_lines_ushort (fl, 12)



file.close()
###########################
# Armo la senial temporal #
###########################
t = np.linspace(0, vinput.size, num=vinput.size)
# vmax_power = np.ones(t.size) * max_power

fig, ax = plt.subplots()
ax.set_title('Input & Output')
ax.set_xlabel('Tiempo en muestras')
ax.grid()
ax.plot(t, vinput, 'r', label='vinput')
ax.plot(t, voutput, 'y', label='voutput')
ax.plot(t, reference_v, 'g', label='reference_v')
ax.legend(loc='upper right')
plt.tight_layout()
plt.show()


fig, ax = plt.subplots()
ax.set_title('Sampled Input & Output')
ax.set_xlabel('sampled times')
ax.grid()
ax.plot(t, vinput_adc, 'r', label='vinput_adc')
ax.plot(t, voutput_adc, 'y', label='voutput_adc')
ax.plot(t, reference, 'g', label='reference')
ax.plot(t, duty_applied, 'm', label='duty_applied')
ax.legend(loc='upper right')
plt.tight_layout()
plt.show()

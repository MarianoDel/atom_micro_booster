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

adc_output = get_vector_lines_ushort (fl, 4)



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
ax.plot(t, adc_output, 'g', label='adc_output')
ax.legend(loc='upper right')
plt.tight_layout()
plt.show()

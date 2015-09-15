#!/usr/bin/python
#
# Copyright (C) 2012  Politecnico di Milano
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

import getopt
import os
import string
import sys

import numpy as np
import matplotlib.pyplot as plt
from scipy import stats
from math import sqrt

#cases = ["BBQ", "NOBBQ"]
cases       = ["NOBBQ"]
instances   = [1, 2, 3, 4]
iterations  = [300, 2000, 10]
param_value = [32768, 0, 256]
#instances  = [2]

#samples     = [ "nbody", "fluidsimulation2D", "montecarlo", "stereomatch"]
#samples      = [ "nbody", "fluidsimulation2D", "montecarlo"]
samples      = [ "nbody", "fluidsimulation2D", "montecarlo"]
#samples      = [ "nbody", "montecarlo"]
#samples      = ["nbody"]
params       = {"stereomatch": [1, 3, 5, 10, 15, 20], "nbody": [1024, 2048, 4096, 8192, 16384], "montecarlo": [8, 16, 32], "fluidsimulation2D": [0] }
param_labels = {"stereomatch": 'FPS', "nbody": 'particles', "montecarlo": 'steps', "fluidsimulation2D":'' }

#bbq_samples     = [ "nbody", "fluidsimulation2D", "montecarlo", "stereomatch"]
#bbq_samples      = [ "nbody", "fluidsimulation2D", "montecarlo"]
bbq_samples      = [ "nbody", "montecarlo"]

#bbq_samples      = ["nbody"]
bbq_params       = {"stereomatch": [1, 3, 5, 10, 15, 20], "nbody": [1024, 2048, 4096, 8192, 16384], "montecarlo": [8, 16, 32], "fluidsimulation2D": [0] }
bbq_param_labels = {"stereomatch": 'FPS', "nbody": 'particles', "montecarlo": 'steps', "fluidsimulation2D":'' }

amd_samples = [ "NBody", "FluidSimulation2D", "MonteCarloAsian" ]
#amd_samples = [ "NBody", "MonteCarloAsian" ]

#amd_samples      = [ "NBody" ]
amd_params       = {"NBody": [1024, 2048, 4096, 8192, 16384], "MonteCarloAsian": [8, 16, 32], "FluidSimulation2D": [0] }
amd_param_labels = {"NBody": 'particles', "MonteCarloAsian": 'steps', "FluidSimulation2D":'' }

#
legend_cases = [ "BarbequeRTRM managed" , "Unmanaged" ]

#              ...samples...
avg_cmds_s   = [[], [], [], []]
err_cmds_s   = [[], [], [], []]

#              t   G0  T0  G1  T1
avg_metrics = [[[], [], [], [], []], [[], [], [], [], []], [[], [], [], [], []]]
err_metrics = [[[], [], [], [], []], [[], [], [], [], []], [[], [], [], [], []]]

avg_metrics_bbq = [[[], [], [], [], []], [[], [], [], [], []], [[], [], [], [], []]]
err_metrics_bbq = [[[], [], [], [], []], [[], [], [], [], []], [[], [], [], [], []]]

avg_metrics_amd = [[[], [], [], [], []], [[], [], [], [], []], [[], [], [], [], []]]
err_metrics_amd = [[[], [], [], [], []], [[], [], [], [], []], [[], [], [], [], []]]
lab_metrics = [ "Exec time", "GPU0 load ", "GPU0 temperature", "GPU1 load", "GPU1 temperature"]
out_metrics = [ "Time", "GPU0Load", "GPU0Temp", "GPU1Load", "GPU1Temp"]


colorc = ['#FF0080', '#F3B165', '#8000FF', '#16EBD5']
colors = ['r', 'y', 'b', 'g', 'm', 'c', 'k']
colorj = ['#CC0000', '#CC6600', '#CCCC00', '#66CC00', '#0000CC', '#FF9999', '#FFCC99', '#FFFF99', '#CCFF99', '#9999FF' ]


avg_rtime_bbq = {"stereomatch": [], "nbody": [], "montecarlo": [],"fluidsimulation2D": []}
err_rtime_bbq = {"stereomatch": [], "nbody": [], "montecarlo": [],"fluidsimulation2D": []}
avg_rtime_amd = {"NBody": [], "MonteCarloAsian": [], "FluidSimulation2D": [] }
err_rtime_amd = {"NBody": [], "MonteCarloAsian": [], "FluidSimulation2D": [] }

###############################################################################
#                          Parsing functions                                  #
###############################################################################

def getRuntimeStats(bbq, metrics):
	m_i = out_metrics.index(metrics)

	if bbq == "NOBBQ":
		samples = amd_samples
		params  = amd_params
	else:
		samples = bbq_samples
		params  = bbq_params
	for s in samples:
		id_samp = samples.index(s)

		for n in instances:
			nr_inst = instances.index(n)
			nr_iter = iterations[id_samp]
			p_value = param_value[id_samp]
			datafile = "%s-%s-N%d-I%d-P%d-Runtime.dat" % (bbq, s, n, nr_iter, p_value)
			print "Parsing sample data from %s..." % datafile,
			data = np.loadtxt(datafile)
			print "DONE"
			print "Data [time]: ",
			data_col = [col[m_i] for col in data]
			print data_col

			if bbq == "BBQ":
				getStats(data_col, avg_rtime_bbq[s], err_rtime_bbq[s])
			else:
				getStats(data_col, avg_rtime_amd[s], err_rtime_amd[s])

		print " ====== %s =====" % s
		print "%s: " % metrics,
		if bbq == "BBQ":
			print avg_rtime_bbq[s]
			print err_rtime_bbq[s]
		else:
			print avg_rtime_amd[s]
			print err_rtime_amd[s]
		print " ..............."


def getStats(data, avg_out, err_out):
	n, (smin, smax), smean, svar, sskew, skurt = stats.describe(data)
	sd = sqrt(svar)
	se = sd / sqrt(n)
	ci95 = 1.96 * se
	ci99 = 2.58 * se
	avg_out.append(smean)
	err_out.append(ci95)

###############################################################################
#                       Plotting functions                                    #
###############################################################################


def plotRunTimes(bbq, metrics):
	m_i = out_metrics.index(metrics)

	if bbq == "BBQ":
		samples = bbq_samples
		avg_rtime = avg_rtime_bbq
		err_rtime = err_rtime_bbq
	else:
		samples = amd_samples
		avg_rtime = avg_rtime_amd
		err_rtime = err_rtime_amd

	graph_name = "GPU-Coscheduling-{}-Penalty-{}.pdf".format(metrics, bbq)
	print "Plotting running times -> %s" % graph_name

	width = 0.15             # the width of the bars
	x_instances = instances[1:]
	nr_s = len(avg_rtime)
	nr_x = len(x_instances)
	ind = np.arange(nr_x)    # the x locations for the groups

	fig, ax = plt.subplots()
	for s in samples:
		s_i = samples.index(s)
		print "[%s] %s = " % (s, metrics),
		print avg_rtime[s]
		print "base value = %f" % float(avg_rtime[s][0])
		print "base error = %f" % float(err_rtime[s][0])

		if metrics == "Time":
			ydata = [x / float(avg_rtime[s][0]) for x in avg_rtime[s][1:]]
		else:
			ydata = [x - float(avg_rtime[s][0]) for x in avg_rtime[s][1:]]

		if err_rtime[s][0] == 0:
			yerrs = [0] * nr_x
		else:
			yerrs = [e / float(err_rtime[s][0]) for e in err_rtime[s][1:]]
		print "Ydata = ",
		print ydata
		print yerrs

		#ax.bar(ind, ydata, width, color=colors[samples.index(s)], yerr=yerrs)
		ax.bar(ind, ydata, width, color=colors[samples.index(s)])

		#Plot settings
		if metrics == "Time":
			plt.ylim(ymin=0)
			ax.set_ylabel('Execution time increase factor')
		elif metrics == "GPU0Load" or metrics == "GPU1Load":
			ax.set_ylabel("Load increase [%]")
			plt.ylim(0, 25)
		else:
			ax.set_ylabel('Temperature increase [C]')
			plt.ylim(0, 7)


		ax.set_xlabel('Number of running instances')
		ax.set_title('GPU coscheduling penalties ['+ lab_metrics[m_i]+']')
		ax.set_xticklabels(x_instances)
		ax.legend(samples, loc='best')

		ind = ind + width

	plt.xticks(np.arange(nr_x)+(width*nr_s)/2)
	plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
	#plt.show()
	plt.close()


def plotRuntimeLines(bbq, metrics):
	mts = [[],[],[]]
	m_i = out_metrics.index(metrics)

	if bbq == "BBQ":
		samples = bbq_samples
		avg_rtime = avg_rtime_bbq
		err_rtime = err_rtime_bbq
	else:
		samples = amd_samples
		avg_rtime = avg_rtime_amd
		err_rtime = err_rtime_amd

	graph_name = "GPU-Coscheduling-{}-Penalty-{}.pdf".format(metrics, bbq)
	print "Plotting running times -> %s" % graph_name

	width = 0.15             # the width of the bars
	nr_s = len(avg_rtime)
	nr_x = len(instances)
	ind = np.arange(nr_x)    # the x locations for the groups

	fig, ax = plt.subplots()
	for s in samples:
		s_i = samples.index(s)

		if metrics == "GPU0Load" or metrics == "GPU1Load":
			plt.ylim(0,100)
			ax.set_ylabel('GPU load []')
		else:
			plt.ylim(30,50)
			ax.set_ylabel('Temperature [C]')

		#ax.set_xticklabels(ind)
		ax.set_xlabel('Number of instances')
		ax.legend(s, loc='best')
		mts[s_i] = avg_rtime[s]

	# Plotting...
	plt.xlim(1, len(instances))
	plt.xticks(instances)
	plt.plot(ind, mts[0], ind, mts[1], ind, mts[2], linewidth=3)
	plt.show()


def cleanup(bbq):
	print "Cleaning... %s" % bbq
	if bbq == "BBQ":
		for s in bbq_samples:
			del avg_rtime_bbq[s][:]
			del err_rtime_bbq[s][:]
			print avg_rtime_bbq[s]

	else:
		for s in amd_samples:
			del avg_rtime_amd[s][:]
			del err_rtime_amd[s][:]
			print avg_rtime_amd[s]

###############################################################################
#                                  MAIN                                       #
###############################################################################

for b in cases:
	for m in out_metrics:
		print "************************** %s ************************" % m
		getRuntimeStats(b, m)

#		if m == "Time":
		plotRunTimes(b, m)
#		else:
#			plotRuntimeLines(b, m)

		print "**************************************************"
		cleanup(b)


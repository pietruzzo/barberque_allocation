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

from matplotlib import rc, rcParams
rc('font',**{'family':'sans-serif','sans-serif':['Times New Roman']})
#rc('font',**{'family':'sans-serif','sans-serif':['computer modern sans serif']})
# These lines are needed to get type-1 results:
# http://nerdjusttyped.blogspot.com/2010/07/type-1-fonts-and-matplotlib-figures.html
rcParams['ps.useafm'] = True
rcParams['pdf.use14corefonts'] = True
rcParams['text.usetex'] = False
rcParams['text.latex.unicode']=True


from scipy import stats
from math import sqrt

cases_lgd  = [ "BarbequeRTRM managed" , "Unmanaged" ]
cases      = ["BBQ", "NOBBQ"]
#instances  = [1, 2, 3, 4]
instances  = [1, 2, 3]
iterations = {"nbody":300, "stereomatch":100, "fluidsimulation2D":2000, "montecarlo":10}

lab_y         = [ "Time [s]", "Load [%]", "Temperature [C]" ]
title_metrics = [ "Exec time", "GPU0 load ", "GPU0 temp", "GPU1 load", "GPU1 temp"]
out_metrics   = [ "Time", "GPU0Load", "GPU0Temp", "GPU1Load", "GPU1Temp"]


#samples     = [ "nbody", "fluidsimulation2D", "montecarlo", "stereomatch"]
samples      = [ "nbody", "fluidsimulation2D", "montecarlo"]
#samples      = [ "nbody", "montecarlo"]
#samples      = [ "nbody"]
#bbq_samples = [ "nbody", "fluidsimulation2D", "montecarlo", "stereomatch"]
bbq_samples  = [ "nbody", "fluidsimulation2D", "montecarlo"]
#bbq_samples  = [ "nbody", "montecarlo"]
#bbq_samples  = [ "nbody"]
#amd_samples  = [ "NBody", "FluidSimulation2D", "MonteCarloAsian" ]
amd_samples  = [ "NBody", "FluidSimulation2D", "MonteCarloAsian" ]
#amd_samples  = [ "NBody", "MonteCarloAsian" ]
#amd_samples  = [ "NBody" ]

# Parameters
params_nvalues   = 6
params           = {"stereomatch": [20], "nbody": [1024, 2048, 4096, 8192, 16384, 32768], "montecarlo": [8, 16, 32], "fluidsimulation2D": [0] }
#params           = {"stereomatch": [20], "nbody": [32768], "montecarlo": [16, 32], "fluidsimulation2D": [0] }
param_labels     = {"stereomatch": 'FPS', "nbody": 'particles', "montecarlo": 'steps', "fluidsimulation2D":'' }

bbq_params       = {"stereomatch": [20], "nbody": [1024, 2048, 4096, 8192, 16384, 32768], "montecarlo": [8, 16, 32], "fluidsimulation2D": [0] }
#bbq_params       = {"stereomatch": [20], "nbody": [32768], "montecarlo": [16, 32], "fluidsimulation2D": [0] }
bbq_param_labels = {"stereomatch": 'FPS', "nbody": 'particles', "montecarlo": 'steps', "fluidsimulation2D":'' }

amd_params       = {"NBody": [1024, 2048, 4096, 8192, 16384, 32768], "MonteCarloAsian": [8, 16, 32], "FluidSimulation2D": [0] }
#amd_params       = {"NBody": [32768], "MonteCarloAsian": [16, 32], "FluidSimulation2D": [0] }
amd_param_labels = {"NBody": 'particles', "MonteCarloAsian": 'steps', "FluidSimulation2D":'' }

#            STAGES: queue,  submit, exec
times_stage = ["queue", "submit", "exec"]
commands    = ["clEnqueueReadBuffer", "clEnqueueWriteBuffer", "clEnqueueMapBuffer", "clEnqueueNDRangeKernel"]
avg_cmds    = [[] for i in range(len(commands))]
err_cmds    = [[] for i in range(len(commands))]

#                                  t   G0  T0  G1  T1
avg_metrics     = [[[[] for i in range(len(out_metrics)) ] for j in range(params_nvalues)] for p in range(len(instances)) ]
err_metrics     = [[[[] for i in range(len(out_metrics)) ] for j in range(params_nvalues)] for p in range(len(instances)) ]
avg_metrics_bbq = [[[[] for i in range(len(out_metrics)) ] for j in range(params_nvalues)] for p in range(len(instances)) ]
err_metrics_bbq = [[[[] for i in range(len(out_metrics)) ] for j in range(params_nvalues)] for p in range(len(instances)) ]
avg_metrics_amd = [[[[] for i in range(len(out_metrics)) ] for j in range(params_nvalues)] for p in range(len(instances)) ]
err_metrics_amd = [[[[] for i in range(len(out_metrics)) ] for j in range(params_nvalues)] for p in range(len(instances)) ]

colorc = ['#FF0080', '#F3B165', '#8000FF', '#16EBD5']
colors = ['r', 'y', 'b', 'g', 'm', 'c', 'k']
colorj = ['#CC0000', '#CC6600', '#CCCC00', '#66CC00', '#0000CC', '#FF9999', '#FFCC99', '#FFFF99', '#CCFF99', '#9999FF' ]
colorm = ['#CC0000', '#CC6600', '#CCCC00', '#FF9999', '#FFCC99', '#FFFF99' ]
colorm2 = ['#CC0000', '#CC6600', '#CCCC00', '#CC6600', '#CCCC00' ]


###############################################################################
#                       Statistics functions                                  #
###############################################################################

def getRuntimeStats(sample):
	s=""
	s_i = bbq_samples.index(sample)
	i   = iterations[sample]
	for bbq in cases:
		if bbq == "NOBBQ":
			s = amd_samples[s_i]
			params = amd_params
		else:
			s = bbq_samples[s_i]
			params = bbq_params

		for p in params[s]:
			p_i = params[s].index(p)
			for n in instances:
				datafile = "%s-%s-N%d-I%d-P%d-Runtime.dat" % (bbq, s, n, i, p)
				print "# Opening %s..." % (datafile)
				#datafile = "%s-%s-N%d-P%d-Runtime.dat" % (bbq, s, n, p)
				data = np.loadtxt(datafile)
				nr_i = instances.index(n)

				if bbq == "BBQ":
					getStats(data, avg_metrics_bbq[nr_i][p_i], err_metrics_bbq[nr_i][p_i])
					print "BBQ  averages  [S:%s, P:%d, I:%d]: " % (s, p, n),
					print avg_metrics_bbq[nr_i][p_i]
				else:
					getStats(data, avg_metrics_amd[nr_i][p_i], err_metrics_amd[nr_i][p_i])
					print "NOBBQ averages [S:%s, P:%d, I:%d]: " % (s, p, n),
					print avg_metrics_amd[nr_i][p_i]


def getCommandStatsInst(sample, param, orient):
	nr_it = iterations[sample]
	for c in commands:
		a_times = [[[] for x in range(len(times_stage))]  for j in range(len(instances))]
		e_times = [[[] for x in range(len(times_stage))]  for j in range(len(instances))]
		for n in instances:
			datafile = "BBQ-%s-N%d-I%d-P%d-%s-%s.dat" % (sample, n, nr_it, param, c, orient)
			#datafile = "BBQ-%s-N%d-P%d-%s-%s.dat" % (sample, n, param, c, orient)
			print "<< Parsing command data from %s..." % datafile
			n_i = instances.index(n)
			try:
				data = np.loadtxt(datafile)
			except IOError as e:
				continue

			getStats(data, a_times[n_i], e_times[n_i])
		avg_cmds[commands.index(c)] = a_times
		err_cmds[commands.index(c)] = e_times


def getCommandStatsParams(s, nr_i, orient):
	for c in commands:
		a_times = [[[] for x in range(len(times_stage))]  for j in range(params_nvalues)]
		e_times = [[[] for x in range(len(times_stage))]  for j in range(params_nvalues)]
		i = iterations[s]

		for p in params[s]:
			p_i = params[s].index(p)
			#datafile = "BBQ-%s-N%d-P%d-%s-%s.dat" % (s, nr_i, p, c, orient)
			datafile = "BBQ-%s-N%d-I%d-P%d-Runtime.dat" % (s, nr_i, i, p)
			print "<< Parsing command data from %s..." % datafile
			try:
				data = np.loadtxt(datafile)
			except IOError as e:
				print "(E) Unable to load the file %s" % datafile
				continue

			getStats(data, a_times[p_i], e_times[p_i])
		avg_cmds[commands.index(c)] = a_times
		err_cmds[commands.index(c)] = e_times


def getStats(data, avg_out, err_out):
	data_col = []
	for i in range(len(avg_out)):
		data_col = [col[i] for col in data]
		n, (smin, smax), smean, svar, sskew, skurt = stats.describe(data_col)
		sd = sqrt(svar)
		se = sd / sqrt(n)
		ci95 = 1.96 * se
		ci99 = 2.58 * se
		avg_out[i].append(smean)
		err_out[i].append(ci95)
	return avg_out

###############################################################################
#                       Plotting functions                                    #
###############################################################################

def getCommandStatsInstForPie(sample, nr_inst, param, hv):
	for c in commands:
		a_times = [[], [], []]
		e_times = [[], [], []]

		datafile = "BBQ-%s-N%d-P%d-%s-%s.dat" % (sample, nr_inst, param, c, hv)
		print "<- Parsing command data from %s..." % datafile
		try:
			data = np.loadtxt(datafile)
		except IOError as e:
			continue

		getStats(data, a_times, e_times)
		avg_cmds[commands.index(c)] = a_times
		err_cmds[commands.index(c)] = e_times

def plotCommandsPie(sample, param, stage):
	graph_name = "{}-{}.pdf".format(sample, param, "CommandsPie")
	ytimes  = []
	labcmds = []

	# Get the row of the give application parameter value
	st_i = times_stage.index(stage)
	for c_i in range(len(commands)):
		if avg_cmds[c_i] == []:
			continue
		ytimes.append(avg_cmds[c_i][st_i])
		labcmds.append(commands[c_i])

	# Plot the row with queue, submit and exec times
	colors = ['yellowgreen', 'lightskyblue', 'lightcoral']
	explode = (0, 0, 0.2) # only explode the "exec" time
	plt.pie(ytimes, labels=labcmds, colors=colorm,
		autopct='%1.1f%%', shadow=True, startangle=0)

	# Set aspect ratio to be equal so that pie is drawn as a circle
	plt.axis('equal')
	plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
	#plt.show()
	plt.close()

def plotCommandBarsParamsH(sample, nr_inst):
	graph_name  = "{}-N{}-Stages.pdf".format(sample, nr_inst)
	graph_title = "OpenCL per command stages time distribution ["+ sample +"]"

	s_i = samples.index(sample)
	N   = len(params[sample])
	rects   = [0]
	nr_cmds = 0               # track the number of commands profiled
	width   = 0.4             # the width of the bars
	ind     = np.arange(N)    # the x locations for the groups

	# Plot settings
	plt.title(graph_title)
	plt.ylim(0, 180)
	plt.ylabel('Percentage of time spent [%]')
	plt.xlabel(param_labels[sample])
	plt.yticks(np.arange(0, 101, 10))

	for c in commands:
		cmd_bottom = [0 * len(params[sample])]
		c_i = commands.index(c)
		cmd_data     = avg_cmds[c_i]
		cmd_data_err = err_cmds[c_i]
		if  cmd_data == [[[] for x in range(len(times_stage))]  for j in range(params_nvalues)]:
			print "(W) Command %s not profiled in %s" % (c, sample)
			continue
		nr_cmds += 1
		yloc     = 0

		# queue, submit, exec
		for t_i in range(len(times_stage)):
			cmd_means  = []
			cmd_errs   = []
			for p in params[sample]:
				p_i = params[sample].index(p)
				if cmd_data[p_i] == [[], [], []]:
					continue
				cmd_means.append(cmd_data[p_i][t_i][0])
				cmd_errs.append(cmd_data_err[p_i][t_i][0])

			# Bar graph
			rect = plt.bar(ind, cmd_means, width, color=colorc[t_i], bottom=cmd_bottom, yerr=cmd_errs)
			cmd_bottom = [x+y for x,y in zip(cmd_bottom, cmd_means)]

		ind  = ind + width
		for r in rect:
			xloc = ind[rect.index(r)] - width/2
			yloc = 102
			plt.text(xloc, yloc, commands[c_i], horizontalalignment='left',\
				verticalalignment='bottom', color='k', rotation=90, fontsize=9)

	#lab_legend.append(times_stage)
	plt.legend(times_stage, fontsize=12)
	plt.xticks(np.arange(N)+(width*nr_cmds)/2, params[sample])
	plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
	plt.close()
	#plt.show()


def plotCommandBarsParamsV(sample, nr_inst):
	s_i = samples.index(sample)
	N     = len(params[sample])
	rects = [0]
	width = 0.35   		# the width of the bars
	ind   = np.arange(N)    # the x locations for the groups

	# queue, submit, exec
	for t_i in range(len(times_stage)):
		graph_name  = "{}-N{}-{}-{}.pdf".format(sample, nr_inst, "Commands", times_stage[t_i])
		graph_title = "OpenCL commands time distribution '{}' [{}]".format(sample, times_stage[t_i])
		cmd_bottom = [0]*len(ind)
		cmd_labels = []
		rect_attr  = []

		for c in commands:
			c_i = commands.index(c)
			# Data extracted from file
			cmd_data     = avg_cmds[c_i]
			cmd_data_err = err_cmds[c_i]
			if  cmd_data == [[[] for x in range(len(times_stage))]  for j in range(params_nvalues)]:
				continue
			cmd_means = []
			cmd_errs  = []
			for p in params[sample]:
				print "-> Plotting [%s] T:%s, C:%s, P:%d" % (graph_name, times_stage[t_i], c, p)
				p_i = params[sample].index(p)
				cmd_means.append(cmd_data[p_i][t_i][0])
				cmd_errs.append(cmd_data_err[p_i][t_i][0])
#				print cmd_means

			# Bar graph
			rect = plt.bar(ind, cmd_means, width, color=colors[c_i], bottom=cmd_bottom, yerr=cmd_errs)
			cmd_bottom = [x+y for x,y in zip(cmd_bottom, cmd_means)]

			# Plot settings
			plt.ylim(0, 140)
			plt.ylabel('Percentage of time spent [%]')
			plt.xlabel(param_labels[sample])
			plt.title(graph_title)
			plt.xticks(ind+width/2, params[sample])
			plt.yticks(np.arange(0, 101, 10))
			# Legend information
			cmd_labels.append(c)
			rect_attr.append(rect[0])
			rects.append(rect)

		plt.legend(rect_attr, cmd_labels, fontsize=12)
		plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
		plt.close()
		#plt.show()

def plotCommandBarsInstH(sample, param):
	graph_name  = "{}-P{}-Stages.pdf".format(sample, param)
	graph_title = "OpenCL per command stages time distribution ["+ sample +"]\
			varying the number of instances"

	s_i = samples.index(sample)
	N   = len(instances)
	rects   = [0]
	nr_cmds = 0               # track the number of commands profiled
	width   = 0.4             # the width of the bars
	ind     = np.arange(N)    # the x locations for the groups

	# Plot settings
	plt.title(graph_title)
	plt.ylim(0, 180)
	plt.ylabel('Percentage of time spent [%]')
	plt.xlabel('Number of instances')
	plt.yticks(np.arange(0, 101, 10))

	for c in commands:
		cmd_bottom = [0 * len(params[sample])]
		c_i = commands.index(c)
		cmd_data     = avg_cmds[c_i]
		cmd_data_err = err_cmds[c_i]
		if cmd_data == [[[] for x in range(len(times_stage))]  for j in range(len(instances))]:
			print "(W) Command %s not profiled in %s" % (c, sample)
			continue
		nr_cmds += 1
		yloc     = 0

		# queue, submit, exec
		for t_i in range(len(times_stage)):
			cmd_means  = []
			cmd_errs   = []
			for n in instances:
				n_i = instances.index(n)
				print "[%s] command '%s' stage '%s' [N:%d]:" % (sample, c, t_i, n)
			#	print cmd_data[n_i]
				if cmd_data[n_i] == [[], [], []]:
					print "Skipping #instances = %d" % n
					continue
				cmd_means.append(cmd_data[n_i][t_i][0])
				cmd_errs.append(cmd_data_err[n_i][t_i][0])

			# Bar graph
			print "(I) Len of height (cmd_means) = %d" % len(cmd_means)
			rect = plt.bar(ind, cmd_means, width, color=colorc[t_i], bottom=cmd_bottom, yerr=cmd_errs)
			cmd_bottom = [x+y for x,y in zip(cmd_bottom, cmd_means)]

		ind  = ind + width
		for r in rect:
			xloc = ind[rect.index(r)] - width/2
			yloc = 102
			plt.text(xloc, yloc, commands[c_i], horizontalalignment='left',\
				verticalalignment='bottom', color='k', rotation=90, fontsize=9)

	#lab_legend.append(times_stage)
	plt.legend(times_stage, fontsize=12)
	plt.xticks(np.arange(N)+(width*nr_cmds)/2, instances)
	plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
	plt.close()
	#plt.show()


def plotCommandBarsInstV(sample, param):
	s_i   = samples.index(sample)
	N     = len(instances)
	rects = [0]
	width = 0.35      # the width of the bars
	ind   = np.arange(N)    # the x locations for the groups

	# queue, submit, exec
	for t_i in range(len(times_stage)):
		graph_name  = "{}-P{}-{}-{}.pdf".format(sample, param, "Commands", times_stage[t_i])
		graph_title = "OpenCL commands time distribution '{}' [{}] {}={}".format(\
				sample, times_stage[t_i], param_labels[sample], param)
		cmd_bottom = [0]*len(ind)
		cmd_labels = []
		rect_attr  = []

		for c in commands:
			c_i = commands.index(c)
			# Data extracted from file
			cmd_data     = avg_cmds[c_i]
			cmd_data_err = err_cmds[c_i]
			if  cmd_data == [[[] for x in range(len(times_stage))]  for j in range(len(instances))]:
				continue
			cmd_means = []
			cmd_errs  = []
			for n in instances:
				n_i = instances.index(n)
				cmd_means.append(cmd_data[n_i][t_i][0])
				cmd_errs.append(cmd_data_err[n_i][t_i][0])

			# Bar graph
			rect = plt.bar(ind, cmd_means, width, color=colors[c_i], bottom=cmd_bottom, yerr=cmd_errs)
			cmd_bottom = [x+y for x,y in zip(cmd_bottom, cmd_means)]

			# Plot settings
			plt.ylim(0, 140)
			plt.ylabel('Percentage of time spent [%]')
			plt.xlabel("Number of instances")
			plt.title(graph_title)
			plt.xticks(np.arange(N)+width/2, instances)
			plt.yticks(np.arange(0, 101, 10))
			# Legend information
			cmd_labels.append(c)
			rect_attr.append(rect[0])
			rects.append(rect)

		plt.legend(rect_attr, cmd_labels, fontsize=12)
		plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
		plt.close()
		#plt.show()


#######################################################################
#                                                  Runtime statistics #
#######################################################################

def plotMetricsVs(sample):
	print "-> Plotting metrics BBQ vs NoBBQ..."
	width = 0.30          # the width of the bars
	rects     = []
	rect_attr = []
	labels    = []

	nr_s = bbq_samples.index(sample)

	for p in params[sample]:
		p_nr = params[sample].index(p)
		# Runtime metrics
		nr_metrics = len(avg_metrics_bbq[0])
		plt.figure()
		for m_i in range(len(out_metrics)):
			fig, ax = plt.subplots()
			graph_name = "{}-P{}-VS-{}.pdf".format(sample, p, out_metrics[m_i])

			ind = np.arange(len(instances))
			bbq_data, bbq_err, amd_data, amd_err = [], [], [], []

			#Plot settings
			cs = getMetricsColor(m_i)
			print "[%s] metrics: %s" % (sample, out_metrics[m_i])
			if out_metrics[m_i] == "GPU0Load" or out_metrics[m_i] == "GPU1Load":
				plt.ylim(0, 100)
			elif out_metrics[m_i] == "GPU0Temp" or out_metrics[m_i] == "GPU1Temp":
				plt.ylim(0, 60)

			for i in instances:
				i_nr = instances.index(i)
				for c_i in cases:
					c_nr = cases.index(c_i)
					if c_i == "BBQ":
						bbq_data.append(avg_metrics_bbq[i_nr][p_nr][m_i][0])
						bbq_err.append(err_metrics_bbq[i_nr][p_nr][m_i][0])
					else:
						amd_data.append(avg_metrics_amd[i_nr][p_nr][m_i][0])
						amd_err.append(err_metrics_amd[i_nr][p_nr][m_i][0])

			#rect = plt.bar(ind, bbq_data, width, color=colorm[cs], yerr=bbq_err)
			rect = plt.bar(ind, bbq_data, width, color="#AAAAAA", yerr=bbq_err, ecolor='black')
			rect = plt.bar(ind+width, amd_data, width, color="#333333", yerr=amd_err, ecolor='black')
			#rect = plt.bar(ind+width, amd_data, width, color=colorm[cs+3], yerr=amd_err)
			ind  = ind + 2*width

			# Plot settings
			plt.title(title_metrics[m_i] + "(" + bbq_param_labels[sample] + "=" + str(p) + ")",
				fontsize=14)
			ax.set_ylabel(lab_y[cs], size='large')
			ax.set_xlabel("Number of instances", size='large')
			ax.set_xticklabels(instances)
			ax.set_xticks(np.arange(len(instances)+(0.5*width*len(instances))))
			ax.legend(cases_lgd, loc='best')
			#plt.show()
			plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
			plt.close()


def plotMetricsParams(bbq, sample, nr_inst):
	if bbq == "BBQ":
		avg_metrics = avg_metrics_bbq
		err_metrics = err_metrics_bbq
	else:
		avg_metrics = avg_metrics_amd
		err_metrics = err_metrics_amd

	n_i   = instances.index(nr_inst)
	width = 0.15          # the width of the bars
	rects     = []
	rect_attr = []
	labels    = []
	ind = np.arange(len(out_metrics)) # the x locations for the groups
	fig, ax = plt.subplots()
	graph_name = "{}-{}-N{}-Runtime.pdf".format(bbq, sample, nr_inst)

	print "-> Plotting metrics for %s [%s] N:%d P:%d" % (bbq, sample, nr_inst, len(params[sample]))
	for p_i in range(len(params[sample])):
		ydata, yerrs = [], []
		for m_i in range(len(avg_metrics[n_i][p_i])):
			ydata.append(avg_metrics[n_i][p_i][m_i][0])
			yerrs.append(err_metrics[n_i][p_i][m_i][0])
		rect = ax.bar(ind, ydata, width, color=colorm2, yerr=yerrs)

		# Plot settings
		plt.ylim(0, 180)
		ax.set_ylabel('Metrics values')
		ax.set_xticklabels(out_metrics)
		ax.set_title(\
			'Runtime metrics ['+str(nr_inst)\
				+' instances] varying the parameter "'\
				+ param_labels[sample] + '"',
			fontsize=14)
		#ax.legend((rects[i]), (commands[pos]))
		ind = ind + width

		# Vertical labels on top of bars
		for r in rect:
			xloc = ind[rect.index(r)] - width/2
			yloc = ydata[rect.index(r)] + 2
			plt.text(xloc, yloc, params[sample][p_i],
				horizontalalignment='center', verticalalignment='bottom',\
				color='k', rotation=90, fontsize=9)

		#labels.append(title_metrics[m_i])
		rect_attr.append(rect[0])
		rects.append(rect)

	plt.xticks(np.arange(len(out_metrics))+(width*len(avg_metrics[n_i][p_i]))/2)
	plt.legend(rect_attr, labels, loc='upper left', fontsize=14)
	plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
	plt.close()
	#plt.show()


def plotMetricsInst(bbq, sample, param):
	if bbq == "BBQ":
		avg_metrics = avg_metrics_bbq
		err_metrics = err_metrics_bbq
	else:
		avg_metrics = avg_metrics_amd
		err_metrics = err_metrics_amd

	p_i = params[sample].index(param)
	width = 0.15          # the width of the bars
	rects     = []
	rect_attr = []
	labels    = []
	ind = np.arange(len(out_metrics)) # the x locations for the groups
	fig, ax = plt.subplots()
	graph_name = "{}-{}-P{}-Runtime.pdf".format(bbq, sample, param)

	for n_i in range(len(instances)):
		print "-> Plotting metrics (for #instances) for %s [%s] N:%d P:%d = " % (bbq, sample, instances[n_i], len(params[sample])),
		ydata, yerrs = [], []
		for m_i in range(len(avg_metrics[n_i][p_i])):
			ydata.append(avg_metrics[n_i][p_i][m_i][0])
			yerrs.append(err_metrics[n_i][p_i][m_i][0])
		rect = ax.bar(ind, ydata, width, color=colorm2, yerr=yerrs)
		print ydata

		# Plot settings
		plt.ylim(0, 180)
		ax.set_ylabel('Metrics values')
		ax.set_xticklabels(out_metrics)
		ax.set_title('Runtime metrics varying the number of instances ['\
				+ param_labels[sample] + '=' + str(param) + ']')
		#ax.legend((rects[i]), (commands[pos]))
		ind = ind + width

		# Vertical labels on top of bars
		for r in rect:
			xloc = ind[rect.index(r)] - width/2
			yloc = ydata[rect.index(r)] + 2
			plt.text(xloc, yloc, instances[n_i], horizontalalignment='center',\
				verticalalignment='bottom', color='k', rotation=90, fontsize=9)

		#labels.append(title_metrics[m_i])
		rect_attr.append(rect[0])
		rects.append(rect)

	plt.xticks(np.arange(len(out_metrics))+(width*len(avg_metrics[n_i][p_i]))/2)
	plt.legend(rect_attr, labels, loc='upper left', fontsize=12)
	plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
	plt.close()
	#plt.show()


def plotDeltaTemp(sample, param):
	# Get data
	gpus_delta_temp = [[[] for p in range(len(instances)) ] for g in range(2)]
	for p_i in range(len(params[sample])):
		width     = 0.30          # the width of the bars
		rects     = []
		rect_attr = []
		#ind     = np.arange(len(instances)) # the x locations for the groups
		fig, ax = plt.subplots()
		graph_name = "{}-P{}-dTemp.pdf".format(sample, params[sample][p_i])
		for n_i in range(len(instances)):
			# delta(GPU0, GPU1)
			gpus_delta_temp[0][n_i] = abs(avg_metrics_amd[n_i][p_i][2][0] - avg_metrics_amd[n_i][p_i][4][0])
			gpus_delta_temp[1][n_i] = abs(avg_metrics_bbq[n_i][p_i][2][0] - avg_metrics_bbq[n_i][p_i][4][0])

			# Bars
			rect = ax.bar(n_i, gpus_delta_temp[1][n_i], width, color="#AAAAAA")
			rects.append(rect)
			rect_attr.append(rect[0])
			rect = ax.bar(n_i+width,  gpus_delta_temp[0][n_i], width, color="#333333")
			rects.append(rect)
			rect_attr.append(rect[0])

		plt.ylim(0,20)
		ax.legend(rect_attr, cases_lgd, loc='upper right', fontsize=14)
		ax.set_title("Difference of temperature between GPUs", fontsize=14)
#		ax.set_ylabel(u'\u0394'u'\u00B0'"C", size='large')
		ax.set_ylabel(u'\u00B0'"C", size='large')
		ax.set_xlabel("Number of instances", size='large')
		ax.set_xticks(np.arange(len(instances)+(0.5*width*len(instances))))
		ax.set_xticklabels(instances)
#		plt.show()
		#ApplyFont(plt.gca())
		plt.savefig(graph_name, papertype = 'a3', format = 'pdf')
		plt.close()

##########################################################################################
#                                                                           Utilities    #
##########################################################################################


def getMetricsColor(m_i):
	if out_metrics[m_i] == "Time":
		return 0
	elif out_metrics[m_i] == "GPU0Load" or out_metrics[m_i] == "GPU1Load":
		return 1
	else:
		return 2


def cleanup(s):
	s_i = bbq_samples.index(s)
	for p_i in range(len(params[s])):
		for m_i in range(len(out_metrics)):
			while avg_metrics_bbq[s_i][p_i][m_i] != []:
				avg_metrics_bbq[s_i][p_i][m_i].pop()
				err_metrics_bbq[s_i][p_i][m_i].pop()
				if "NOBBQ" in cases:
					avg_metrics_amd[s_i][p_i][m_i].pop()
					err_metrics_amd[s_i][p_i][m_i].pop()
	#	print avg_metrics_bbq[s_i][p_i]
	#	if "NOBBQ" in cases:
	#		print avg_metrics_amd[s_i][p_i]



###############################################################################
#                                                             Fonts settings  #
###############################################################################

def SetPlotRC():
	#If fonttype = 1 doesn't work with LaTeX, try fonttype 42.
	plt.rc('pdf', fonttype = 1)
	plt.rc('ps',  fonttype = 1)

def ApplyFont(ax):
        ticks = ax.get_xticklabels() + ax.get_yticklabels()
	text_size = 12.0
	for t in ticks:
		t.set_fontname('Times New Roman')
		t.set_fontsize(text_size)

	txt = ax.get_xlabel()
        txt_obj = ax.set_xlabel(txt)
	txt_obj.set_fontname('Times New Roman')
	txt_obj.set_fontsize(text_size)

	txt = ax.get_ylabel()
	txt_obj = ax.set_ylabel(txt)
	txt_obj.set_fontname('Times New Roman')
	txt_obj.set_fontsize(text_size)

	txt = ax.get_title()
	txt_obj = ax.set_title(txt)
	txt_obj.set_fontname('Times New Roman')
	txt_obj.set_fontsize(text_size)


###############################################################################
#                                  MAIN                                       #
###############################################################################

for s in samples:
#	SetPlotRC()
	print " ******************** Sample: %s ***************************" % s

	# Runtime metrics trends (time, load, temperature)
	getRuntimeStats(s)

	# BBQ vs NoBBQ
	if len(cases) > 1:
		plotMetricsVs(s)
		plotDeltaTemp(s, p)

#	getCommandStatsInstForPie("fluidsimulation2D", 2, 1024, 'V')
#	plotCommandsPie("fluidsimulation2D", 1024, 'exec')

	for bbq in cases:
		# x: Application parameters and number of instances
		for i in instances:
			plotMetricsParams(bbq, s, i)

		# x: Number of instances
		for p in params[s]:
			plotMetricsInst(bbq, s, p)

			### Plot per command (OpenCL) statistics [horizontal]
       			getCommandStatsInst(s, p, "H")
       			plotCommandBarsInstH(s, p)

			### Plot per command (OpenCL) statistics [vertical]
			getCommandStatsInst(s, p, "V")
			plotCommandBarsInstV(s, p)
	# Cleaning
	cleanup(s)

for i in instances:
	for s in samples:
		## Plot per command (OpenCL) statistics [horizontal]
		getCommandStatsParams(s, i, "H")
		plotCommandBarsParamsH(s, i)

		## Plot per command (OpenCL) statistics [vertical]
		getCommandStatsParams(s, i, "V")
		plotCommandBarsParamsV(s, i)



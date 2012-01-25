import os

log = open("finalLog.txt", 'r')
text = log.readlines()
data = {"make": {}, "dmake": {}}
for line in text:
	words = line.split(" ")

	if words[3] == 'average': continue

	prog = words[0]
	ex = words[1]
	numMachines = int(words[2])
	time = float(words[4])
	try:
		data[prog][ex][numMachines]['time'] += time
		data[prog][ex][numMachines]['numTimes'] += 1
	except KeyError:
		try:
			data[prog][ex].update({
				numMachines: {
					'time': time,
					'numTimes': 1
				}
			})
		except KeyError:
			data[prog].update({ex: {
				numMachines: {
					'time': time,
					'numTimes': 1
				}
			}})
log.close()

p = 'make'
for e in data[p].keys():
	for m in data[p][e].keys():
		info = data[p][e][m]
		info['time'] /= info['numTimes']

p = 'dmake'
for e in data[p].keys():
	plotFilename = "plot_" + e + ".txt" 
	plotFile = open(plotFilename, 'w')
	dataFile = open('benchmark.dat', 'w')
	acDataFile = open('ac.dat', 'w')
	keys = data[p][e].keys()
	keys = sorted(keys)
	for m in keys:
		info = data[p][e][m]
		info['time'] /= info['numTimes']
		info['ac'] = data['make'][e][1]['time'] / info['time']
		dataFile.write("%d\t%lf\n" % (m, info['time']))
		acDataFile.write("%d\t%lf\n" % (m, info['ac']))
	dataFile.close()
	acDataFile.close()
	plotFile.write("set terminal png;\n")
	plotFile.write("set output 'graph_" + e + ".png'\n")
	plotFile.write("set xlabel \"Nombre de Machines\";\n")
	plotFile.write("set ylabel \"Temps\";\n")
	plotFile.write("set title \"" + e + "\";\n")
	plotFile.write("plot 'benchmark.dat' using 1:2 with linespoints title \"dmake\", %lf with line title \"make\"\n" % data['make'][e][1]['time'])
	plotFile.close()
	acFilename = "ac_" + e + ".txt"
	acFile = open(acFilename, "w")
	acFile.write("set terminal png;\n")
	acFile.write("set output 'acceleration_" + e + ".png'\n")
	acFile.write("set xlabel \"Nombre de Machines\";\n")
	acFile.write("set title \"" + e + "\";\n")
	acFile.write("plot 'ac.dat' using 1:2 with linespoints title \"acceleration\", x with lines title \"acceleration ideale\"\n")
	acFile.close()
	os.system("gnuplot " + plotFilename)
	os.system("gnuplot " + acFilename)

print data

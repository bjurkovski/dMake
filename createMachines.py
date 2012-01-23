startMachine = 05
NUM_MACHINES = 15

machinesFile = file("machines.txt", 'w')
for i in range(startMachine, startMachine+NUM_MACHINES):
	machinesFile.write("ensipsys%02d\n" % i)
machinesFile.close()

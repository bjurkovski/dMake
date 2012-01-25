import os

startMachine = 21
NUM_MACHINES = 26
LAST_MACHINE = 48

machinesFile = file("machines.txt", 'w')

def machineIsUp(machineName):
	if not os.system("ping -c 1 " + machineName + " > /dev/null"):
		return True
	return False

numMachines = 0
i = startMachine
while (numMachines < NUM_MACHINES) and (i < LAST_MACHINE):
	machine = "ensipsys%02d" % i
	if machineIsUp(machine):
		machinesFile.write(machine + "\n")
		numMachines += 1
	i += 1

machinesFile.close()

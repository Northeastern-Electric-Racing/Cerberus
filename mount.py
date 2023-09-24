import subprocess as sub
import os

#Function to get BUSID base on device name
def check_for_probe(x):
    if "CMSIS-DAP" in x:
        return True
    else:
        return False

#This opens terminal 1
os.system('start /B start cmd.exe @cmd /k wsl -d ubuntu')

#These helps filter out the BUSID
output = str(sub.getoutput("usbipd wsl list"))
linedvs = output.splitlines() #Split lines of output base on device name

flt_dv = filter(check_for_probe, linedvs) 
for x in flt_dv:
    dvline = x #Obtain the line of specific device

dvid = dvline.split(" ", 1)[0] #Keep only the BUSID in the line
busid = str(dvid) #BUSID turn for list to single string

#This opens Terminal 2
# Terminal might need to be elevated to admin privileges for this
sub.check_call('cmd /K usbipd wsl attach --distribution=ubuntu --busid=' + busid)
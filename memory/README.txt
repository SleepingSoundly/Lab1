************************************
*
*	README.txt - Memory 
*
*		memory_coordinator 
*
************************************

Description:
-------------------------------------------------------------------------------------------------

	This tool utilizes the libvirt C library to make hypervisor calls to control guests' memory availability.
	This is done through use of the balloon driver on each guest, which allows us to dynamically control how much
	memory a domain has available to it. When a particular domain starts requesting more memory, the tool will
	take either some memory from the other domains by deflating their balloon drivers, or by taking it from the 
	memory not assigned to any one guest, but still associated with the hypervisor. Additionally, if a domain is 
	greedy, the tool will take memory away from that domain if they aren't doing anything with it. Eventually,
	either the tool will help reach a steady state, or more load will be introduced and the tool will pick up again.
	This program will run until it is terminated by a ctrl-c. 
	

USAGE:
-------------------------------------------------------------------------------------------------
Input -> takes a floating point time for the number of seconds you'd like to wait in between checking scheduling


Output-> print statements


Side effects -> Memory tracker that will utilize virDomainSetMemory to give or take away as much memory as is 
		necessary to make sure the systems have enough memory to run on, but not so much that they hord


To run this program, type 

make


in this directory, and once that's been completed, (with warnings) input the following on the CLI


./memory_coordinator <f>




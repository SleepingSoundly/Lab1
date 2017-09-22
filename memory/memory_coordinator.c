/*
 *	Lab 1: memory_coordinator.c
 *		utilizes APIs to manage virtual memory usage of 
 *		physical resources within the hypervisor
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <libvirt/libvirt.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>


unsigned long long* virtMemDiffArray; // last timestamp value per vcpu of mem 

int* percentUsed;


void printNodeInfo(virNodeInfo info){
	printf(" n : model == %s\n", info.model);
	printf(" n : memory size  == %lu kb\n", info.memory);

}

void printDomainInfo(virDomainInfo info){
	printf(" d : running state == %d\n", info.state);
	printf(" d : maximum Memory == %lu Kb\n", info.maxMem);
	printf(" d : memory in use == %lu Kb\n", info.memory);
}



// main, controls the scheduler and several data structures
// related to the Node and its domains
int main(int argc, char *argv[]){


	int itr, itl, ret = 0; // iterators and return values
	virConnectPtr conn; // connection structure to hypervisor

	virNodeInfo Ninfo;
	double sleepVal;
	unsigned long totPMem;
	virDomainPtr dom = NULL; // pointer to a virtual domain, obtained by name or ID
	virDomainPtr* domains = NULL; // for list of all domains returned by ListAll API	
	int numDomains = 0;
	virDomainMemoryStatStruct memStats[VIR_DOMAIN_MEMORY_STAT_NR];
	unsigned int flags;

	if(argc < 2){
		printf(" Please give number of seconds for the scheduler to sleep\n");
		return 1;
	}
	sleepVal = (double)(atof(argv[1]))* 1000 * 1000;
	printf(" Starting, sleeps %f seconds\n", sleepVal);

	conn = virConnectOpen("qemu:///system"); // connect to the hypervisor
	if ( conn == NULL ){
		fprintf(stderr, "Failed to open connection to qemu\n");
		return 1;
	}
	else{
		printf("Success! Connected to the qemu:///system hypervisor\n");
	}

	
	// get node info about the host machine
	ret = virNodeGetInfo(conn, &Ninfo);
	if ( ret != 0 ){
		fprintf(stderr, "Failed to get Node info\n");
		return 1;
	}
	totPMem = Ninfo.memory;
	
	printNodeInfo(Ninfo);

	// can now allocate space for the amount of VCPUS we'll deal with
	// ASSUMPTION -> according to the staff, one VCPU per domain
		// list all connected domains in the domain variable
	flags = VIR_CONNECT_LIST_DOMAINS_RUNNING;
	ret = virConnectListAllDomains(conn, &domains, flags);
	if( ret < 0){
		fprintf(stderr, "Failed to obtain domains, exiting\n");
		return 1;
	}		
 
	// have data for Nodes, now need Domain Data
	printf("\n\n Starting Domain Data\n");
	numDomains = virConnectNumOfDomains(conn);
	printf("+ %d domains connected\n\n", numDomains);


	// initialize the data for the memory usage 
	//percentUsed = malloc(numPcpus * sizeof(int));
	//memset(&percentPCPUUsed, 0, numPcpus*sizeof(int));
	virtMemDiffArray = malloc(sizeof(unsigned long)*numDomains);
	memset(&virtMemDiffArray, 0, sizeof(unsigned long)*numDomains);	
	flags = VIR_DOMAIN_AFFECT_LIVE;
	for(itr = 0; itr< numDomains;itr++){// have every domain take stats every second
		ret = virDomainSetMemoryStatsPeriod(domains[itr], 1, flags); 
	
	}

	sleep(1);
		
	// take a look at inital values for Memory Stats

	while(1){
		printf(" ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		for(itr = 0; itr< numDomains;itr++){// have every domain take stats every second
			ret = virDomainMemoryStats(domains[itr], memStats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
			if (ret < 0){
				fprintf(stderr, " !! Error, no memory stats, exiting\n");
				return 1;
			}

			printf("\n + Analysing domain %d, with available memstats %d/%d requested\n", itr, ret, VIR_DOMAIN_MEMORY_STAT_NR);

			for(itl = 0; itl<ret; itl++){
				if(memStats[itl].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED){
					printf(" ++ %d: Unused Memory-> %llu\n", itr, memStats[itl].val/ 1024);
				}
				else if(memStats[itl].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE){
					printf(" ++ %d: Available Memory-> %llu\n", itr, memStats[itl].val/ 1024);
				}		
			}	
	
		}
		usleep(sleepVal);
	}




	return 0;

}

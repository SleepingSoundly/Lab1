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


	int itr, itl, itn, ret = 0; // iterators and return values
	virConnectPtr conn; // connection structure to hypervisor

	virNodeInfo Ninfo;
	double sleepVal;
	unsigned long totPMem;
	virDomainPtr dom = NULL; // pointer to a virtual domain, obtained by name or ID
	virDomainPtr* domains = NULL; // for list of all domains returned by ListAll API	
	int numDomains = 0;
	virDomainMemoryStatStruct memStats[VIR_DOMAIN_MEMORY_STAT_NR];
	unsigned int flags;
	unsigned int threashold;
	int notInit = 0;
	unsigned long long currentStat;
	unsigned long long highestMem = 0;
	unsigned long long check;
	int deflate = -1;	
	


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
	virtMemDiffArray = malloc(sizeof(unsigned long long)*numDomains);
	memset(&virtMemDiffArray, 0, sizeof(unsigned long long)*numDomains);	
	flags = VIR_DOMAIN_AFFECT_LIVE;
	for(itr = 0; itr< numDomains;itr++){// have every domain take stats every second
		ret = virDomainSetMemoryStatsPeriod(domains[itr], 1, flags); 
	
	}

	sleep(1);
		
	// take a look at inital values for Memory Stats
	threashold = 150;
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
					currentStat = memStats[itl].val / 1024;
					printf(" ++ %d: Unused Memory-> %llu == %llu / 1024\n", itr, currentStat, memStats[itl].val);
					memcpy((&virtMemDiffArray + sizeof(unsigned long long)*itr), &currentStat, sizeof(unsigned long long));
					printf(" %d: %llu: %llu > %llu\n", itr, (&virtMemDiffArray + sizeof(unsigned long long)*itr), *(&virtMemDiffArray + sizeof(unsigned long long)*itr));
						
					if( currentStat < threashold && notInit==1 ){ // should probably do something about this
	
						// who should we deflate? 
						for(itn = 0; itn<numDomains; itn++){
							check = *(&virtMemDiffArray + sizeof(unsigned long long)*itn);
							if(check > highestMem){

								printf(" %d: %llu: %llu > %llu\n", itn, (&virtMemDiffArray + sizeof(unsigned long long)*itn), check, highestMem);
								highestMem = check;	
								deflate = itn;
							}						
						}			
						// if it doesn't assign something we've goofed
						if( deflate == -1 ){
							printf(" NOPE\n");
							return 1;
						}
						// get defaltes memory usage and cut it in half. 

						currentStat = ((unsigned long long)*(&virtMemDiffArray + sizeof(unsigned long long)*deflate))/ 2;

						printf(" !! %d: Reassigning %llu kb of memory from %d\n", itr, currentStat, deflate);
						ret = virDomainSetMemory(domains[deflate], currentStat);
						// assign that amount to the domain that needs the memory
						ret = virDomainSetMemory(domains[itr], currentStat);
						deflate = -1;	
						highestMem = 0;
						break;
					}

				}
				//else if(memStats[itl].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE){
			//		currentStat = memStats[itl].val / 1024;
			//		printf(" ++ %d: Available Memory-> %llu\n", itr, currentStat);
			//		
			//	}	
			//	else if(memStats[itl].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON){
			//		printf(" ++ %d: Balloon stats-> %llu\n", itr, memStats[itl].val/ 1024);
			//	}	
				//break;
			}	
	
		}
		notInit = 1;
		usleep(sleepVal);
	}




	return 0;

}

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
	unsigned int threashold, Wthreashold;
	int notInit = 0;
	unsigned long long currentUnused, currentStat = 0;
	unsigned long long highestMem = 0;
	unsigned long long check, lesser, more = 0;
		
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
	deflate = -2;
	
	// take a look at inital values for Memory Stats
	threashold = 100;
	Wthreashold = 250;
	while(1){
		printf(" ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		for(itr = 0; itr< numDomains;itr++){// have every domain take stats every second
			deflate = -2; // reset deflate value
			ret = virDomainMemoryStats(domains[itr], memStats, VIR_DOMAIN_MEMORY_STAT_NR, 0);

			if (ret < 0){
				fprintf(stderr, " !! Error, no memory stats, exiting\n");
				return 1;
			}

			printf("\n + Analysing domain %d, with available memstats %d/%d requested\n", itr, ret, VIR_DOMAIN_MEMORY_STAT_NR);
			for(itl = 0; itl<ret; itl++){ 
					
				if(memStats[itl].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED){
					currentUnused = memStats[itl].val / 1024;
					printf(" ++ %d: Unused Memory-> %llu\n", itr, currentUnused);
				}	
				else if(memStats[itl].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE){
					currentStat = memStats[itl].val / 1024;
					printf(" ++ %d: Available Memory stats-> %llu\n", itr, currentStat);
					memcpy((&virtMemDiffArray + sizeof(unsigned long long)*itr), &currentStat, sizeof(unsigned long long));
					// *********** CLAIMING MEMORY FOR A STARVED PROCESS ******************
					if( currentStat < threashold && notInit==1 ){ // should probably do something about this
	
						// who should we deflate? 
						for(itn = 0; itn<numDomains; itn++){
							if(itn == itr){
								continue;
							}
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
						// currentStat is a quarter of the balloon availablility from the thing we're deflating
						currentStat = ((unsigned long long)*(&virtMemDiffArray + sizeof(unsigned long long)*deflate));

						lesser = currentStat - (currentStat/2);
						if(lesser < threashold/2){
							printf(" !! %d: Reassigning memory from the hypervisor, not another vm\n", itr);
							more = Wthreashold/2;	
						}
						else{
							more = ((unsigned long long)*(&virtMemDiffArray + sizeof(unsigned long long)*itr)) + (currentStat/2);
							printf(" !! %d: Reassigning %llu kb/%llu  of %d \n", itr, lesser, currentStat);
		
							ret = virDomainSetMemory(domains[deflate], lesser* 1024);
							if( ret == -1){
								printf(" !!!!! Pin failed, exiting\n");
								return 1;
							}
						}


						// assign that amount to the domain that needs the memory
						ret = virDomainSetMemory(domains[itr], more *1024);
						printf(" !! Assigning %llu to the starved domain --- %d\n", more, ret);
						if( ret == -1){
							printf(" !!!!! Pin failed, exiting\n");
							return 1;
						}
	
						deflate = -1;	
						highestMem = 0;
					}
					// *********** CLAIMING MEMORY FROM A GREEDY PROCESS to the HV
					else if(currentStat > Wthreashold && notInit == 1) {
						printf(" !! %d: Reassigning back to the hypervisor\n", itr);
						ret = virDomainSetMemory(domains[itr], (currentStat - Wthreashold)*1024)  ;
						if( ret == -1){
							printf(" !!!!! Pin failed, exiting\n");
						}
						deflate = -1;	
						highestMem = 0;

					}

				}
				else if(memStats[itl].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON){
					deflate == -2;
					printf(" ++ %d: Balloon stats-> %llu\n", itr, memStats[itl].val/ 1024);
				}// else if	
			} // for items in memInfo	
			if(deflate == -1){// we need to reclaculate everything
				printf(" Deflated, reset\n");
				notInit = 0;
				break;
			}	
	
		} // for domains

		if(deflate != -1){
			printf(" Sleeping, init == %d\n", notInit);
			notInit = 1;
		}

		usleep(sleepVal);
	}// while 1




	return 0;

}

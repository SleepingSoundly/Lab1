/*
 *	Lab 1: vcpu_scheduler.c
 *		utilizes APIs to manage virtual CPU usage of 
 *		physical resources within the hypervisor
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <libvirt/libvirt.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>


unsigned long long* timeDiffArray; // for the number of timestamps we need (# vcpus)
unsigned long long* VcpuDiffArray; // previous VCPU stat values

int* percentPCPUUsed;

void printNodeInfo(virNodeInfo info){
	printf(" n : model == %s\n", info.model);
//	printf(" n : memory size  == %lu kb\n", info.memory);
	printf(" n : # of active CPUs == %d\n", info.cpus);
	printf(" n : expected CPU frequency  == %d mHz\n\n", info.mhz);

}

void printDomainInfo(virDomainInfo info){
	printf(" d : running state == %d\n", info.state);
//	printf(" d : maximum Memory == %lu Kb\n", info.maxMem);
//	printf(" d : memory in use == %lu Kb\n", info.memory);
	printf(" d : # virtual CPUs == %i \n", info.nrVirtCpu);
	printf(" d : CPU time used == %llu nanoseconds\n", info.cpuTime);
	
}

// update PCPU usage statistics
int updateNodePCPUs(virConnectPtr conn, int numCPUS){

	int ret, itr, itl = 0; 
	int nparams = 0;
	virNodeCPUStatsPtr pcpuInfo;
	virNodeCPUStatsPtr temp_pcpuInfo;

	if( virNodeGetCPUStats(conn, VIR_NODE_CPU_STATS_ALL_CPUS, NULL, &nparams, 0) == 0 && nparams != 0){
		pcpuInfo = calloc(nparams, sizeof(virNodeCPUStats));
		temp_pcpuInfo = calloc(nparams*numCPUS, sizeof(virNodeCPUStats));
		printf(" Ncpu+ nparams to collect -> %d\n", nparams);
		for(itr = 0; itr < numCPUS; itr++){
			virNodeGetCPUStats(conn, itr, temp_pcpuInfo, &nparams, 0);
			memcpy((&pcpuInfo + itr*(sizeof(virNodeCPUStatsPtr))), &temp_pcpuInfo, sizeof(virNodeCPUStatsPtr));
			for( itl = 0; itl < nparams; temp_pcpuInfo++, itl++){
				printf(" + Pcpu %d: %s, %llu\n", itr,temp_pcpuInfo->field, temp_pcpuInfo->value); 
			}
			printf("\n\n");
		}

	}
	else
		return 1;


	// pcpu data has been aquired, now to calculate the percent usage 


	return ret;

}
int getPercentPerDomain(virDomainInfo newInfo){
	int result = 0;
	

	return result;
}


// check PCPUs to see if any of them are "overworked"
// return 0 if no, return value of CPU that's highest 
// % usage if yes
int shouldSchedule(){
	int result = 0;


	return result;

}

// the scheduler, which takes the integer value of the lowest
// used CPU and determines which domain needs to be re-pinned
// FROM that PCPU
int scheduler(int starvedCPU){
	int result = 0;

	return result;
}

// main, controls the scheduler and several data structures
// related to the Node and its domains
int main(int argc, char *argv[]){

	int itr, itl, itn, ret = 0; // iterators and return values
	virConnectPtr conn; // connection structure to hypervisor

	virNodeInfo Ninfo;

	// map data
	unsigned char * cpuMaps; // pointer to a bit map of real CPUS on the host Node. Little endian. FREE
	unsigned char * map; // initalized to the first CPU

 
	unsigned int online; // optional number of online CPUs in cpumap, has success for online if necessary
	unsigned int flags;
	int numPcpus = 0 ;

	virDomainPtr dom = NULL; // pointer to a virtual domain, obtained by name or ID
	virDomainPtr* domains = NULL; // for list of all domains returned by ListAll API	
	virDomainInfo Dinfo;
	virTypedParameterPtr params;
	int numDomains = 0;	

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
	else{
		printf("Success!\n");
	}
	numPcpus = Ninfo.cpus;
	printNodeInfo(Ninfo);

	// get CPU maps from Node Data
	// no flags necessary, ret value is number of CPUs present
	ret = virNodeGetCPUMap(conn, &map, &online, 0);
	ret = virNodeGetCPUMap(conn, &cpuMaps, &online, 0);
	if ( ret != numPcpus){
		fprintf(stderr, " ! ret (%d) does not equal the number of CPUS present, weird case, exit\n", ret);
		return 1;
	}

	printf(" + Success, aquired bitmapping for active CPUS\n");

	// each byte would be 8 cpus worth, so
	// we need one bm if there's up to 8 cpus, etc. 
	// wraps around at 9
	for(itr = 0; itr<9%numPcpus; itr++){
		printf(" ++ bm : %d\n", cpuMaps[itr]);

	}



	// have data for Nodes, now need Domain Data
	printf("\n\n Starting Domain Data\n");
	numDomains = virConnectNumOfDomains(conn);
	printf("\n\n + %d domains connected\n", numDomains);

	// can now allocate space for the amount of VCPUS we'll deal with
	// ASSUMPTION -> according to the staff, one VCPU per domain
	timeDiffArray = calloc(numDomains, sizeof(unsigned long long));
	VcpuDiffArray = calloc(numDomains, sizeof(unsigned long long));

	// list all connected domains in the domain variable
	flags = VIR_CONNECT_LIST_DOMAINS_RUNNING;
	ret = virConnectListAllDomains(conn, &domains, flags);
	if( ret < 0){
		fprintf(stderr, "Failed to obtain domains, exiting\n");
		return 1;
	}		
 
	// initially pin all the vcpus associated with a domain 
	// to one of the PCPUS we've found
	
	// initialize the map mover to 1, for the first available PCPU		
	*map &= 0x01;
	for (itr = 0; itr< numDomains; itr++){
		// since there's only one VCPU, the vcpu number will always be 0
		ret = virDomainPinVcpu(domains[itr], 0, map, VIR_CPU_MAPLEN(Ninfo.cpus));
	
		printf(" ++ vcpu %d: set to pcpu %d\n", itr, map[0]);

		*map<<=1;
		// note that the map[0] code is specialized to 8 PCPUs tops, otherwise problems. 	
		if( map[0] > numPcpus+1){
			// ROLL: need to go back to the beginning of the PCPUS
			*map &= 0x00;
			*map |= 0x01; 	
		}

		if( ret != 0 ){
			fprintf(stderr, " Initial Pinning failed, exit\n");
			return 1;
		}

	}

	// initialize the data for the PCPUs
	percentPCPUUsed = calloc( numPcpus, sizeof(int));
	printf("\n\n Initializing Node CPU data\n");	
	ret = updateNodePCPUs(conn, numPcpus);

	
	// enter scheduler

	 


	// free everything you can
	for(itr = 0; itr < numDomains; itr++){
		virDomainFree(domains[itr]);
	}
	free(domains);	
	free(cpuMaps);
	virConnectClose(conn);
	return 0;

}




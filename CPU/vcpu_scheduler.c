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


unsigned long long timeDiffArray; // for the number of timestamps we need (# vcpus)
unsigned long long VcpuDiffArray; // previous VCPU stat values

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

	printNodeInfo(Ninfo);

	// initialize the data for the PCPUs
	percentPCPUUsed = calloc( Ninfo.cpus, sizeof(int));


	ret = updateNodePCPUs(conn, Ninfo.cpus);






	return 0;

}




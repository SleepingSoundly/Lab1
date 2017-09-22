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


unsigned long* timeDiffArray; // for the number of timestamps we need (# vcpus)
unsigned long long* VcpuDiffArray; // previous VCPU stat values
unsigned long long* PcpuDiffArray; // Pcpu diff values per pcpu

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

	int ret, itr = 0; 
	int nparams = 0;
	virNodeCPUStatsPtr pcpuInfo;
	virNodeCPUStatsPtr temp_pcpuInfo;

	if( virNodeGetCPUStats(conn, VIR_NODE_CPU_STATS_ALL_CPUS, NULL, &nparams, 0) == 0 && nparams != 0){
		pcpuInfo = calloc(nparams, sizeof(virNodeCPUStats));
		temp_pcpuInfo = calloc(nparams*numCPUS, sizeof(virNodeCPUStats));
		// printf(" Ncpu+ nparams to collect -> %d\n", nparams);
		for(itr = 0; itr < numCPUS; itr++){
			virNodeGetCPUStats(conn, itr, temp_pcpuInfo, &nparams, 0);
			memcpy((&pcpuInfo + itr*(sizeof(virNodeCPUStatsPtr))), &temp_pcpuInfo, sizeof(virNodeCPUStatsPtr));
			//for( itl = 0; itl < nparams; temp_pcpuInfo++, itl++){
			//	printf(" + Pcpu %d: %s, %llu\n", itr,temp_pcpuInfo->field, temp_pcpuInfo->value); 
			//}
			//printf("\n\n");
		}

	}
	else
		return 1;


	// pcpu data has been aquired, now to calculate the percent usage 


	return ret;

}
// updates the percent CPU usage per domain 
// expects output from params from virDomainGetCPUStats per VCPU (1 in our world)
int updatePercentPerDomain(virDomainPtr domain, int domainIndex){
	int itr, result, ret, percent= 0;
	unsigned long long new_cpuTime = 0;
	unsigned long long old_cpuTime = 0;
	unsigned long long diff = 0;
	unsigned long long Pdiff = 0;
	virVcpuInfoPtr vcpuInfo;
	int PcpuIndex = 0;
	double down  = 0.0;
	vcpuInfo = calloc(1, sizeof(virVcpuInfoPtr));
	

	//printf(" WHERE IS MY NEW_VALUE -> %llu\n", params->value);
	ret = virDomainGetVcpus(domain, vcpuInfo, 1, NULL, 0); // called with 1 pcup, since that's all it will ever have affinity for at this point	
	PcpuIndex = vcpuInfo->cpu; // which PCPU are we using for this domain	
	

	//printf(" ++ %d: Domain VCPU number -> %d\n", domainIndex, vcpuInfo->number);
	printf(" ++ %d: Domain VCPU cpuTime -> %llu\n", domainIndex, vcpuInfo->cpuTime);
	//printf(" ++ %d: Domain VCPU cpu -> %d\n", domainIndex, vcpuInfo->cpu);

	// set up new params
	memset(&new_cpuTime, 0, sizeof(unsigned long long));
	memcpy(&new_cpuTime, &vcpuInfo->cpuTime, sizeof(unsigned long long));


	// grab old value of the vcpu
	memset(&old_cpuTime, 0, sizeof(unsigned long long));
	memcpy(&old_cpuTime, (&VcpuDiffArray + domainIndex*(sizeof(unsigned long long))), sizeof(unsigned long long));


	printf(" WHERE IS MY OLD_VALUE -> %llu\n", old_cpuTime);

	if( old_cpuTime == 0 ){
		printf(" ++ %d : i -> initialized to our first value: %llu\n", domainIndex, new_cpuTime);
	}
	else{
		printf(" ++ Diff -> %llu\n", new_cpuTime - old_cpuTime );
	
		diff = (new_cpuTime - old_cpuTime);
		Pdiff = (double)(1000 * 1000 * 1000);
		if( diff == 0 ) {
			printf(" +++ NO change\n");
			return 0;
		}

		down = (double)( diff * 10.0)/ Pdiff ; // diff in nanoseconds
		
		// printf(" !! change in doubles for this vcpu-> %llu\n", down);
		percent = (int)(down);	
		printf(" ++ %d : Percent usage change since last sample %i ++ %i\n", PcpuIndex, *(&percentPCPUUsed + sizeof(int)*PcpuIndex), percent);
	
		percent = *(&percentPCPUUsed + sizeof(int)*PcpuIndex) + percent; // add percent usage here
		memcpy((&percentPCPUUsed + sizeof(int)*PcpuIndex), &percent, sizeof(int));

	}

	// put new value into array and move on.
	memcpy( (&VcpuDiffArray + domainIndex*(sizeof(unsigned long long))), &new_cpuTime, sizeof(unsigned long long));
	

	return result;
}


// check PCPUs to see if any of them are "overworked"
// return 0 if no, return value of CPU that's highest 
// % usage if yes
int shouldSchedule(int numPcpus){
	int result = 0;
	int itr = 0;
	int flag;
	int highestUsage = 0;
	int index;
	int percent;

	index = -1;

	for(itr = 0; itr < numPcpus; itr++){
		percent = *(&percentPCPUUsed + sizeof(int)*itr);
		printf("  + %d: usage: %d\n", itr, percent);
		if(percent > 50 ) {
			flag++;
			if(percent > highestUsage){
				highestUsage = percent;
				index = itr; 
			}
		}
	}
	if (flag == numPcpus){
		// nothing we can do, the loads are even, but above the high value	
		return -1;
	}
	else{
		// there is a place we can reschedule a vcpu, potentially, return the highest usage one
		return index;
	}

	return -1;

}

// the scheduler, which takes the integer value of the lowest
// used CPU and determines which domain needs to be re-pinned
// FROM that PCPU
int scheduler(int busyPcpu, int numPcpus){
	int ret, result, itr, percent = 0;
	int lowestUsage = 100;
	int index = -1;
	// we need to take one of the VCPUs assigned to the busy PCPU 
	// given and assign it to a more quiet one than the one we're on 
	// find the quietest one
	for(itr = 0; itr < numPcpus; itr++){
		percent = *(&percentPCPUUsed + sizeof(int)*itr);
		if(percent < lowestUsage){
			lowestUsage = percent;
			index = itr; 
		}
	}
	printf(" s : Attempting to alliviate PCPU%d by moving VCPU%d\n", busyPcpu, index);
	return index;
}

// main, controls the scheduler and several data structures
// related to the Node and its domains
int main(int argc, char *argv[]){

	int itr, itl, ret = 0; // iterators and return values
	virConnectPtr conn; // connection structure to hypervisor

	virNodeInfo Ninfo;
	int seconds;
	// map data
	unsigned char * cpuMaps; // pointer to a bit map of real CPUS on the host Node. Little endian. FREE
	unsigned char * map; // initalized to the first CPU

 
	unsigned int online; // optional number of online CPUs in cpumap, has success for online if necessary
	unsigned int flags;
	int numPcpus = 0 ;

	virDomainPtr dom = NULL; // pointer to a virtual domain, obtained by name or ID
	virDomainPtr* domains = NULL; // for list of all domains returned by ListAll API	
	int numDomains = 0;
	virVcpuInfoPtr vcpuInfo;
	unsigned int nparams = 0;
	int busyPcpu = 0;

	if(argc < 2){
		printf(" Please give number of seconds for the scheduler to sleep\n");
		return 1;
	}
	seconds = atoi(argv[1]);
	printf(" Starting, sleeps %d seconds\n", seconds);

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
	

	numPcpus = (int)Ninfo.cpus;
	printNodeInfo(Ninfo);


	// get CPU maps from Node Data
	// no flags necessary, ret value is number of CPUs present
	ret = virNodeGetCPUMap(conn, &map, &online, 0);
	ret = virNodeGetCPUMap(conn, &cpuMaps, &online, 0);
	if ( ret != numPcpus){
		fprintf(stderr, " ! ret (%d) does not equal the number of CPUS present, weird case, exit\n", ret);
		return 1;
	}

//	printf(" + Success, aquired bitmapping for active CPUS\n");

	// each byte would be 8 cpus worth, so
	// we need one bm if there's up to 8 cpus, etc. 
	// wraps around at 9
//	for(itr = 0; itr<9%numPcpus; itr++){
//		printf(" ++ bm : %d\n", cpuMaps[itr]);
//
//	}



	// have data for Nodes, now need Domain Data
	printf("\n\n Starting Domain Data\n");
	numDomains = virConnectNumOfDomains(conn);
	printf("\n\n + %d domains connected\n", numDomains);

	// can now allocate space for the amount of VCPUS we'll deal with
	// ASSUMPTION -> according to the staff, one VCPU per domain
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
	percentPCPUUsed = malloc(numPcpus * sizeof(int));
	memset(&percentPCPUUsed, 0, numPcpus*sizeof(int));
	memset(&VcpuDiffArray, 0, sizeof(unsigned long long)*numDomains);	
	

	
	// initialize the data for the VCPU info
	while(1){

		for(itr = 0; itr< numPcpus; itr++){
			memcpy((&percentPCPUUsed + sizeof(int)*itr), &itl, sizeof(int));
		}

		printf(" ii : Cleared %d CPUS for consumption: %i\n", numPcpus, *(&percentPCPUUsed + sizeof(int)));
	
		for(itr = 0; itr < numDomains; itr++){
			printf(" + Domain %d: Analysing\n", itr);
			ret = updatePercentPerDomain(domains[itr], itr);
		}// iterate through domains to update the % amounts used on a PCPU they're associated with

		// calculated all the changes, should we change a pinning? 
		ret = shouldSchedule(numPcpus); // ret contains the value of the busy cpu
	
		if( ret != -1)
		{
			busyPcpu = ret;
			*map &= 0x01;
			for (itr = 0; itr< ret; itr++){
				*map<<=1;
			}
	
			printf(" ++ PCPU%d map set to pcpu %d\n", busyPcpu, map[0]);
	
			ret = scheduler(busyPcpu, numPcpus); // call the scheduler on the first pcpu we got with usage above 50
		

			// figure out which VCPU to reassign
			for(itr = 0; itr < numDomains; itr++){
				vcpuInfo = calloc(1, sizeof(virVcpuInfoPtr));
				ret = virDomainGetVcpus(domains[itr], vcpuInfo, 1, NULL, 0); // called with 1 pcup, since that's all it will ever have affinity for at this point	
				if(vcpuInfo->cpu == busyPcpu ){
					printf(" s -> rescheduling VCPU/domain %d to PCPU %d \n", itr, map[0]);
					break;
				}
			}
			ret = virDomainPinVcpu(domains[itr], 0, map, VIR_CPU_MAPLEN(Ninfo.cpus));
			
		}
		printf("sleep %d\n", seconds);
		sleep(seconds);
	}


	




	// free everything you can
	for(itr = 0; itr < numDomains; itr++){
		virDomainFree(domains[itr]);
	}

	free(percentPCPUUsed);
	free(domains);	
	free(cpuMaps);
	virConnectClose(conn);
	return 0;

}




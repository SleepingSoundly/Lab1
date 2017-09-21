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

// takes in index, gives diff since last time stamp taken per VCPU
unsigned long getTimeDiff(int itr){
	struct timeval temp_time;
	int ret;
	unsigned long time, timedif, oldTime = 0;

	timedif = 0;
	oldTime = *(&timeDiffArray + sizeof(unsigned long) *itr);
	ret = gettimeofday(&temp_time, NULL);		
 	time = 1000000 * temp_time.tv_sec + temp_time.tv_usec;

	if( oldTime != 0){
		timedif = time - oldTime;
	}
	memcpy((&timeDiffArray + sizeof(unsigned long)*itr), &time, sizeof(unsigned long));
	printf(" t:%lu\n", timedif);
	return timedif;
}


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
int updatePercentPerDomain(virDomainPtr domain, int domainIndex, unsigned int nparams){
	int itr, result, ret, percent= 0;
	unsigned long long new_cpuTime = 0;
	unsigned long long old_cpuTime = 0;
	unsigned long long new_PcpuTime = 0;
	unsigned long long old_PcpuTime = 0;
	double newcpu = 0;
	double oldcpu = 0;
	unsigned long long diff = 0;
	unsigned long long Pdiff = 0;
	unsigned long long timediff = 0;
	virTypedParameterPtr params;
	virVcpuInfoPtr vcpuInfo;
	int PcpuIndex = 0;
	double down  = 0.0;	
 	params = calloc(nparams, sizeof(virTypedParameter));
	vcpuInfo = calloc(1, sizeof(virVcpuInfoPtr));
	ret = virDomainGetCPUStats(domain, params, nparams, 0, 1, 0); // nparams for the whole domain (b/c one VCPU)
	printf(" WHERE IS MY NEW_VALUE -> %llu\n", params->value);
	ret = virDomainGetVcpus(domain, vcpuInfo, 1, NULL, 0); // called with 1 pcup, since that's all it will ever have affinity for at this point	

	printf(" ++ %d: Domain VCPU number -> %d\n", domainIndex, vcpuInfo->number);
	printf(" ++ %d: Domain VCPU cpuTime -> %llu\n", domainIndex, vcpuInfo->cpuTime);
	printf(" ++ %d: Domain VCPU cpu -> %d\n", domainIndex, vcpuInfo->cpu);
	PcpuIndex = domainIndex; // which PCPU are we using for this domain	
	//	*map<<=1;
		// note that the map[0] code is specialized to 8 PCPUs tops, otherwise problems. 	
	//	if( map[0] > numPcpus+1){
			// ROLL: need to go back to the beginning of the PCPUS
	//		*map &= 0x00;
	//		*map |= 0x01; 	
	//	}


	// for this domain, we only care about parameter 1, the cpu_time
	// printf(" memory allocated for vectors at %lu %llu %llu\n", &percentPCPUUsed, &VcpuDiffArray, &timeDiffArray);

	// set up new params
	memset(&new_cpuTime, 0, sizeof(unsigned long long));
	memcpy(&new_cpuTime, &params->value, sizeof(unsigned long long));
	memset(&new_PcpuTime, 0, sizeof(unsigned long long));
	memcpy(&new_PcpuTime, &vcpuInfo->cpuTime, sizeof(unsigned long long));




	//printf(" WHERE IS MY NEW_VALUE -> %llu\n", new_cpuTime);
	//printf(" ++ %d : need another sample to address %llu, which currently looks like.... %llu\n", domainIndex, *(&VcpuDiffArray + domainIndex*(sizeof(unsigned long long))), *(&VcpuDiffArray + domainIndex*(sizeof(unsigned long long)))  );
	
	// grab old value of the vcpu
	memset(&old_cpuTime, 0, sizeof(unsigned long long));
	memcpy(&old_cpuTime, (&VcpuDiffArray + domainIndex*(sizeof(unsigned long long))), sizeof(unsigned long long));
	memset(&old_PcpuTime, 0, sizeof(unsigned long long));
	memcpy(&old_PcpuTime, (&PcpuDiffArray + PcpuIndex*(sizeof(unsigned long long))), sizeof(unsigned long long));


	printf(" WHERE IS MY OLD_VALUE -> %llu\n", old_cpuTime);
	printf(" 		and PCPU -> %llu\n", old_PcpuTime);
//	timediff = (unsigned long long)getTimeDiff(domainIndex);


	if( old_cpuTime == 0 || old_PcpuTime == 0 ){
		printf(" ++ %d : i -> initialized to our first value: %llu\n", domainIndex, new_cpuTime);
		memcpy( (&VcpuDiffArray + domainIndex*(sizeof(unsigned long long))), &new_cpuTime, sizeof(unsigned long long));
		printf(" ++ %d : i -> initialized to our first value: %llu\n", PcpuIndex, new_PcpuTime);
		memcpy( (&PcpuDiffArray + PcpuIndex*(sizeof(unsigned long long))), &new_PcpuTime, sizeof(unsigned long long));
		return 0;
	}
	else{
		printf(" VDiff -> %llu\n", new_cpuTime - old_cpuTime );
		printf(" PDiff -> %llu\n", new_PcpuTime - old_PcpuTime );
	
		diff = (new_cpuTime - old_cpuTime);
		Pdiff = new_PcpuTime - old_PcpuTime;
		if( diff == 0 || Pdiff == 0 ) {
			printf(" +++ NO change\n");
			return 0;
		}

		down = (double)( diff)/ Pdiff ; // diff in nanoseconds
		down = down * 100;
		printf(" !! percent chage in doubles-> %llu\n", down);
		
		percent = (int)(down) ;	

		
		printf(" ++ %d : Percent usage change since last sample %i\n", domainIndex, percent);
	}


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
// TODO:  DO NOT FORGET TO RESET THE PERCENTUSED VALUE, it will NOT be for the 
// right CPU after you pin. 
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
int numDomains = 0;

	unsigned int nparams = 0;

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
	percentPCPUUsed = calloc( numDomains, sizeof(int));
	VcpuDiffArray = calloc( numDomains, sizeof(unsigned long long));
	timeDiffArray = calloc( numDomains, sizeof(unsigned long long));

	memset(&percentPCPUUsed, 0, numDomains*sizeof(int));
	ret = updateNodePCPUs(conn, numPcpus);

	memset(&timeDiffArray, 0, sizeof(unsigned long long)*numDomains);	
	memset(&VcpuDiffArray, 0, sizeof(unsigned long long)*numDomains);	

	PcpuDiffArray = calloc( numDomains, sizeof(unsigned long long));
	memset(&PcpuDiffArray, 0, sizeof(unsigned long long)*numDomains);	



	
	nparams = virDomainGetCPUStats(domains[0], NULL, 0, -1, 1, 0); // nparams for the whole domain (b/c one VCPU)
	// initialize the data for the VCPU info

	while(1){
	
		for(itr = 0; itr < numDomains; itr++){
			printf(" + Domain %d: Analysing\n", itr);
			ret = updatePercentPerDomain(domains[itr], itr, nparams);
		}// iterate through domains

		printf("\n");
		sleep(1);
	}

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




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

int main(int argc, char *argv[]){
	
	int numDomains;
	int ret;
	int flags;
	int itr, itl, itn;
	int nVCPUs = 0;
	int nparams = 0;
	unsigned long long baseTime = 0;
	
	unsigned char *cpumaps;
	
	virConnectPtr conn; // data structure for opening a connection for full r/w access (app creds needed)
	

	virDomainPtr dom = NULL; // pointer to a virtual domain, obtained by name or ID
	
	virDomainPtr* domains = NULL; // for list of all domains returned by ListAll API	
	
	virNodeInfo Ninfo;
	virNodeCPUStatsPtr pcpuInfo;

	virDomainInfo Dinfo;
	virTypedParameterPtr params;	
	virVcpuInfoPtr vcpuInfo;
	
	unsigned long *timestamps;
	unsigned long long *vcpustamps;
 	struct timeval temp_time;
	unsigned long time, timedif;
	unsigned long long vcpudif;
	// open connection to hypervisor
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
	if( virNodeGetCPUStats(conn, VIR_NODE_CPU_STATS_ALL_CPUS, NULL, &nparams, 0) == 0 && nparams != 0){
		pcpuInfo = calloc(nparams, sizeof(virNodeCPUStats));
		virNodeGetCPUStats(conn, VIR_NODE_CPU_STATS_ALL_CPUS, pcpuInfo, &nparams, 0);
	}
	else
		return 1;

	// physical CPU data being printed
	for( itr = 0; itr < nparams; pcpuInfo++, itr++){
		printf(" + %s, %llu\n", pcpuInfo->field, pcpuInfo->value); 
	}

	numDomains = virConnectNumOfDomains(conn);
	printf("\n\n + %d domains connected\n", numDomains);
	timestamps = calloc(numDomains, sizeof(unsigned long));
	flags = VIR_CONNECT_LIST_DOMAINS_RUNNING;
	ret = virConnectListAllDomains(conn, &domains, flags);


	if( ret < 0){
		fprintf(stderr, "Failed to obtain domains, exiting\n");
		return 1;
	}		
 	int numVcpus = 0;	
	for(itr = 0; itr < numDomains; itr++){
		ret = gettimeofday(&temp_time, NULL);		
		time = 1000000 * temp_time.tv_sec + temp_time.tv_usec;
		timestamps[itr] = time;	
		ret = virDomainGetInfo(domains[itr], &Dinfo);
		numVcpus += Dinfo.nrVirtCpu;
		if ( ret == -1){
			fprintf(stderr, "Failed to get Domain info\n");
			return 1;
		}
	}
	vcpustamps = calloc(numVcpus, sizeof(unsigned long long));

	while(1){	


		flags = VIR_CONNECT_LIST_DOMAINS_RUNNING;
		ret = virConnectListAllDomains(conn, &domains, flags);


		if( ret < 0){
			fprintf(stderr, "Failed to obtain domains, exiting\n");
			return 1;
		}		
	
	for(itr = 0; itr < numDomains; itr++){
		dom = domains[itr];

		ret = gettimeofday(&temp_time, NULL);		
		time = 1000000 * temp_time.tv_sec + temp_time.tv_usec;
		timedif = time - timestamps[itr];
		timestamps[itr] = time;		

		printf(" ++ %llu:  %d: Starting analysis\n", timedif, itr);

		ret = virDomainGetInfo(dom, &Dinfo);
		if ( ret == -1){
			fprintf(stderr, "Failed to get Domain info\n");
			return 1;
		}
		printDomainInfo(Dinfo);

		cpumaps = calloc(Dinfo.nrVirtCpu, VIR_CPU_MAPLEN(Ninfo.cpus)); // need 
		vcpuInfo = calloc(Dinfo.nrVirtCpu, sizeof(virVcpuInfoPtr));

		ret = virDomainGetVcpus(dom, vcpuInfo, Dinfo.nrVirtCpu, cpumaps, VIR_CPU_MAPLEN(Ninfo.cpus));

		if( ret == -1 ) {
			fprintf(stderr, "FAIL at get vcpus, exiting\n");
			return 1; 
		}

		for(itl = 0; itl < Dinfo.nrVirtCpu; itl++, vcpuInfo++, cpumaps++){
			//vcpuInfo = (virVcpuInfo *)cpumaps;
			printf(" +++ %d : DomainGetVcpu number-> %d\n", itr, vcpuInfo->number);
			printf(" +++ %d : DomainGetVcpu CPU time -> %llu\n", itr, vcpuInfo->cpuTime);
			printf(" +++ %d : DomainGetVcpu Pcpu -> %llu\n", itr, vcpuInfo->cpu);
			printf(" +++ %d : DomainGetVcpu cpumap # -> %c\n", itr, cpumaps);

			for(itn = 0; itn< 8; cpumaps++, itn++){
				printf("bm: %d\n", cpumaps);
			}
			

			vcpudif = vcpustamps[itr + itl] - vcpuInfo->cpuTime;
			vcpustamps[itr + itl] = vcpuInfo->cpuTime; printf(" +++ %d : vcpu %d time diff == %llu\n", itr, vcpuInfo->number, vcpudif);	
		}


		printf(" ++ %d: freeing domain\n\n", itr);
		virDomainFree(dom);
		}
		sleep(1);
	}
 	// after obtaining domains, they must be freed because the function
	// does an allocation per domain

	free(domains);	
	virConnectClose(conn);


	return 0;
}







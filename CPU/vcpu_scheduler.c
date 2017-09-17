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

void printNodeInfo(virNodeInfo info){
	printf(" n : model == %s\n", info.model);
	printf(" n : memory size  == %lu kb\n", info.memory);
	printf(" n : # of active CPUs == %d\n", info.cpus);
	printf(" n : expected CPU frequency  == %d mHz\n\n", info.mhz);

}

void printDomainInfo(virDomainInfo info){
	printf(" d : running state == %d\n", info.state);
	printf(" d : maximum Memory == %lu Kb\n", info.maxMem);
	printf(" d : memory in use == %lu Kb\n", info.memory);
	printf(" d : # virtual CPUs == %i Kb\n", info.nrVirtCpu);
	printf(" d : CPU time used == %llu nanoseconds\n", info.cpuTime);
	
}

int main(int argc, char *argv[]){
	
	int numDomains;
	int ret;
	int flags;
	int itr, itl;
	int nVCPUs = 0;
	int nparams = 0;
	unsigned char *cpumaps;
	
	virConnectPtr conn; // data structure for opening a connection for full r/w access (app creds needed)
	

	virDomainPtr dom = NULL; // pointer to a virtual domain, obtained by name or ID
	
	virDomainPtr* domains = NULL; // for list of all domains returned by ListAll API	
	
	virNodeInfo Ninfo;
	virNodeCPUStatsPtr pcpuInfo;

	virDomainInfo Dinfo;
	virTypedParameterPtr params;	
	virVcpuInfoPtr vcpuInfo;



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
	if( virNodeGetCPUStats(conn, Ninfo.cpus, NULL, &nparams, 0) == 0 && nparams != 0){
		pcpuInfo = calloc(nparams, sizeof(virNodeCPUStats));
		virNodeGetCPUStats(conn, Ninfo.cpus, pcpuInfo, &nparams, 0);
	}
	else
		return 1;

	// physical CPU data being printed
	for( itr = 0; itr < Ninfo.cpus; pcpuInfo++, itr++){
		printf(" + %s, %llu\n", pcpuInfo->field, pcpuInfo->value); 
	}

	numDomains = virConnectNumOfDomains(conn);
	printf(" + %d domains connected\n", numDomains);

	flags = VIR_CONNECT_LIST_DOMAINS_RUNNING;
	ret = virConnectListAllDomains(conn, &domains, flags);


	if( ret < 0){
		fprintf(stderr, "Failed to obtain domains, exiting\n");
		return 1;
	}	
	for(itr = 0; itr < numDomains; itr++){
		dom = domains[itr];

		printf(" ++ %d: Starting analysis\n", itr);
		ret = virDomainGetInfo(dom, &Dinfo);
		if ( ret == -1){
			fprintf(stderr, "Failed to get Domain info\n");
			return 1;
		}
		printDomainInfo(Dinfo);

		// total statistics, arguments to get-stats allow us to get 
		// number of cpus to query on the VM
		nparams = virDomainGetCPUStats(dom, NULL, 0, -1, 1, 0); 
		printf(" ++ %d: %d parameters to query on this domain\n", itr, nparams);
			

		//  arguments let us return the number of virtual CPUs on 
		// the vm
		nVCPUs = virDomainGetCPUStats(dom, NULL, 0, 0, 1, 0); 
		printf(" ++ %d: %d CPUs to query on this domain\n", itr, nVCPUs);
			
		params = calloc(nparams*nVCPUs, sizeof(virTypedParameter));
		ret = virDomainGetCPUStats(dom, params, nparams, 0, nVCPUs, 0); 

		for(itl = 0; itl < nparams*nVCPUs; itl++, params++){ 
			printf(" +++ %d : %s-> %lu \n", itr, params->field, params->value);
		}

		// now we have number of nanoseconds each domain is using, and the 
		// number of nanoseconds each CPU is using per domain
	
		// gives cpu maps of a single virtual CPU to all the physical CPUs 
		cpumaps = calloc(VIR_CPU_MAPLEN(Ninfo.cpus)*sizeof(unsigned char), sizeof(unsigned char));
		vcpuInfo = calloc(nVCPUs, sizeof(virVcpuInfoPtr));


		ret = virDomainGetVcpus(dom, vcpuInfo, nVCPUs, cpumaps, VIR_CPU_MAPLEN(Ninfo.cpus));

		if( ret == -1 ) {
			fprintf(stderr, "FAIL at get vcpus, exiting\n");
			return 1; 
		}

		for(itl = 0; itl < Ninfo.cpus; itl++, vcpuInfo++){
			//vcpuInfo = (virVcpuInfo *)cpumaps;
			printf(" +++ %d : DomainGetVcpu results -> %d\n", itr, vcpuInfo->number);
			printf(" +++ %d : DomainGetVcpu results -> %llu\n", itr, vcpuInfo->cpuTime);
			printf(" +++ %d : DomainGetVcpu results -> %llu\n", itr, vcpuInfo->cpu);
		}


		printf(" ++ %d: freeing domain\n\n", itr);
		virDomainFree(dom);
	}

 	// after obtaining domains, they must be freed because the function
	// does an allocation per domain

	free(domains);	
	virConnectClose(conn);


	return 0;
}







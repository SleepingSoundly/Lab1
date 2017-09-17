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
	printf(" n : expected CPU frequency  == %d mHz\n", info.mhz);

}

void printDomainInfo(virDomainInfoPtr info){
	printf(" d : running state == %c\n", info->state);
	printf(" d : maximum Memory == %lu Kb\n", info->maxMem);
	printf(" d : memory in use == %lu Kb\n", info->memory);
	printf(" d : # virtual CPUs == %lu Kb\n", info->nrVirtCpu);
	printf(" d : CPU time used == %llu nanoseconds\n", info->cpuTime);
	
}

int main(int argc, char *argv[]){
	
	int numDomains;
	int ret;
	int flags;
	int itr, itl;
	int nVCPUs = 0;
	int nparams = 0;
	
	virConnectPtr conn; // data structure for opening a connection for full r/w access (app creds needed)
	

	virDomainPtr dom = NULL; // pointer to a virtual domain, obtained by name or ID
	
	virDomainPtr* domains = NULL; // for list of all domains returned by ListAll API	
	virNodeInfo info;
	virDomainInfoPtr Dinfo;
	virTypedParameterPtr params;	

	// open connection to hypervisor
	conn = virConnectOpen("qemu:///system"); // connect to the hypervisor
	if ( conn == NULL ){
		fprintf(stderr, "Failed to open connection to qemu\n");
		return 1;
	}
	else{
		printf("Success!\n");
	}

	// get node info about the host machine
	ret = virNodeGetInfo(conn, &info);
	if ( ret != 0 ){
		fprintf(stderr, "Failed to get Node info\n");
		return 1;
	}
	else{
		printf("Success!\n");
	}


	printNodeInfo(info);

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
		ret = virDomainGetInfo(dom, Dinfo);
		if ( ret == -1){
			fprintf(stderr, "Failed to get Domain info\n");
			return 1;
		}

		printf(" ++ %d: Starting analysis\n", itr);
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

		printf(" ++ %d: freeing domain\n\n", itr);
		free(params);	
		virDomainFree(dom);
	}

 	// after obtaining domains, they must be freed because the function
	// does an allocation per domain

	free(domains);	
	virConnectClose(conn);


	return 0;
}







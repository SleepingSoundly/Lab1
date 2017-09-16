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

int main(int argc, char *argv[]){
	
	int numDomains;
	int ret;
	int flags;
	int itr;
	
	virConnectPtr conn; // data structure for opening a connection for full r/w access (app creds needed)
	

	virDomainPtr dom = NULL; // pointer to a virtual domain, obtained by name or ID
	
	virDomainPtr* domains = NULL; // for list of all domains returned by ListAll API	
	int nVCPUs = 0;
	int nparams = 0;
	virTypedParameterPtr params;	

	conn = virConnectOpen("qemu:///system"); // connect to the hypervisor

	if ( conn == NULL ){
		fprintf(stderr, "Failed to open connection to qemu\n");
		return 1;
	}
	else{
		printf("Success!\n");
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
		// total statistics, arguments to get-stats allow us to get 
		// number of cpus to query on the VM
		nparams = virDomainGetCPUStats(dom, NULL, 0, -1, 1, 0); 
		printf(" ++ %d: %d parameters to query on this domain\n", itr, ret);
			

		//  arguments let us return the number of virtual CPUs on 
		// the vm
		nVCPUs = virDomainGetCPUStats(dom, NULL, 0, 0, 1, 0); 
		printf(" ++ %d: %d CPUs to query on this domain\n", itr, ret);
	
		params = calloc(nparams, sizeof(virTypedParameter));
		ret = virDomainGetCPUStats(dom, params, nparams, -1, 1, 0); 
		printf(" +++ %d: %s: %lu \n", itr, params->field, params->value);			

		printf(" ++ %d: freeing domain\n\n", itr);
	
		virDomainFree(dom);
	}

 	// after obtaining domains, they must be freed because the function
	// does an allocation per domain
	free(domains);	
	virConnectClose(conn);


	return 0;
}







*********************************************
*
*	Lab 1 Read Me
*
*
*
*
*********************************************
Project 1

Please read through the entire page, before getting started.

Big Picture

In this project, you are going to implement a vCPU scheduler and a memory coordinator to dynamically manage the resources assigned to each guest machine. Each of these programs will be running in the host machine's user space, collecting statistics for each guest machine through hypervisor calls and taking proper actions.
 
During one interval, the vCPU scheduler should track each guest machine's vCpu utilization, and decide how to pin them to pCpus, so that all pCpus are "balanced", where every pCpu handles similar amount of workload. The "pin changes" can incur overhead, but the vCPU scheduler should try its best to minimize it.
 
Similarly, during one interval, the memory coordinator should track each guest machine's memory utilization, and decide how much extra free memory should be given to each guest machine. The memory coordinator will set the memory size of each guest machine and trigger the balloon driver to inflate and deflate. The memory coordinator should react properly when the memory resource is insufficient.

Tools that you need or might help you

1. qemu-kvm, libvirt-bin, libvirt-dev are packages you need to install, so that you can launch virtual machines with KVM and develop programs to manage virtual machines.
2. libvirt is a toolkit prividing lots of APIs to interact with the virtualization capabilities of Linux.
3. Virtualization is a page you should check.
4. virsh, uvtool, virt-top, virt-clone, virt-manager, virt-install are tools that may help you playing with virtual machines.
5. script command to make a typescript of your terminal session and generate a log file.

Deliverables

You need to implement two separate C/C++ programs, one for vCPU scheduler(vcpu_scheduler.c) and another for memory coordinator(memory_coordinator.c). Both programs should accept one input parameter, the time interval (in seconds) your scheduler or coordinator will trigger. For example, if we want the vCPU scheduler to take action every 12 seconds, we will start your program by doing "./vcpu_scheduler 12" .

You need to submit one compressed folder containing two separate subfolders(CPU and Memory), each containing a Makefile, log files generated through script command for each test case,binary and the corresponding code. We will compile your program by just doing “make”. Therefore, your final submission should be structured as follows after being decompressed. Don’t change the name of files.
 
Project1 
 
  -  CPU
     -  vcpu_scheduler.c 
     -  Makefile 
     -  Readme (Instructions to compile & Run and Code Description)
     -  vcpu_scheduler (binary)
     -  5 vcpu_scheduler.log files - vcpu_scheduler1.log and so on for each test case

  -   Memory
     -  memory_coordinator.c
     -  Makefile 
     -  Readme (Instructions to compile & Run and Code Description)
     -  memory_coordinator (binary)
     -  3 memory_coordinator.log files - memory_coordinator1.log and so on for each test case
   
Grading

This is not performance-oriented project, we will test the functionality only.
The Rubric will be: 
1. vCPU scheduler functionality - 6 points
     - Once the VMs enter into the stable/balanced state, your program shouldn’t do additional jobs.
     - 5 points for implementation and 1 point for Readme.
2. memory coordinator functionality - 6 points
     - Don't kill the guest operating system
     - Don't freeze the host (Give all memory resources to guests).
     - 5 points for implementation and 1 point for Readme.

Step by Step -- Which environment I should use to develop?

1. In general, make sure your machine cpu supports hardware virtualization and the Linux is directly installed (native) on the hardware. Note: you cannot do this project in a virtualbox hosted Linux.
2. Please make sure balloon driver is enabled.
3. Boot Ubuntu from a USB flash drive. Note: it will be a bit slow.
4. Nested virtualization within VMware (communities.vmware.com/docs/DOC-8970).

 
Step by Step -- Where can I find the APIs I might need to use?

1. libvirt-domain provides APIs to monitor and manage the guest virtual machines.
2. libvirt-host provides APIs to query the information regarding host machine.
3. Development Guide libvirt's official guide, you might read to get a general idea of libvirt, but it does not really help a lot. Too many parts are TBD.
4. Attached document HowToCreateVM.md provides examples on creating virtual machines using uvtool in ubuntu.

 
Step by Step -- VCPU Scheduler

1. The first thing you need to do is to connect to the Hypervisor, virConnect* functions in libvirt-host are what you need to check. In our project, please connect to the local one which is "qemu:///system".
2. Next, you need to get all active running virtual machines within "qemu:///system", virConnectList* functions will help you.
3. You are ready to collect VCPU statistics, to do this you need virDomainGet* functions in libvirt-domain. If you also need host pcpu information, there are also APIs in libvirt-host.
4. You are likely to get VCPU time in nanoseconds instead of VCPU usage in % form. Think how to transform or use them.
5. You can also determine the current map (affinity) between VCPU to PCPU through virDomainGet* functions.
6. Write your algorithm, and according to the statistics, find "the best" PCPU to pin each VCPU.
7. Use virDomainPinVcpu to dynamically change the PCPU assigned to each VCPU.
8. Now you have a "one time scheduler", revise it to run periodically.
9. Launch several virtual machines and launch test workloads in every virtual machine to consume CPU resources, then test your VCPU scheduler.

 
Step by Step -- Memory Coordinator

1. The first thing you need to do is to connect to the Hypervisor, virConnect* functions in libvirt-host are what you need to check. In our project, please connect to the local one which is "qemu:///system".
2. Next, you need to get all active running virtual machines within "qemu:///system", virConnectList* functions will help you.
3. Tricky: to get as much memory statistics as possible, you need virDomainSetMemoryStatsPeriod fucntion.
4. Now you are ready to get memory statistics, first think what kind of memory statistics you are expecting to get. Then viewvirDomainGet* and virDomainMemory* functions to find those you need.
5. You can also get the host memory information through virNodeGet* in libvirt-host.
6. Write your algorithm, choose your policy, decide how much extra free memory you should give to each virtual machine according to the numbers you get.
7. Use virDomainSetMemory to dynamically change the memory of each virtual machine, which will indirectly trigger balloon driver.
8. Now you have a "one time scheduler", revise it to run periodically.
9. Launch several virtual machines and launch test workloads in every virtual machine to consume memory resources gradually, then test your memory coordinator.

Step-by-step Testing Process

Refer to the flowchart below to help understand the overall project concept. We are taking the CPU scheduler as an example here.

1. Make sure your system supports this project. 
One way to check is to run "egrep '^flags.*(vmx|svm)' /proc/cpuinfo"  from the shell in your system, and if that works, you are good to go. 

(1) You CAN use
       * Native linux-based system 
       * VMware + Ubuntu ( tested under VMware Fusion 7.1.3 + Ubuntu 16.04 )
          - Once you install VMware, go to "setting->Processors&Memory-> Advanced options", and enable the option "Enable hypervisor applications in this virtual machine"
          - Once you install required libraries(e.g. libvirt) in Ubuntu, you may have to restart the OS to successfully get them working
     * Boot Linux from a flash drive
 (2) You CANNOT use
      * Windows
      * MAC
      * Linux installed in VirtualBox 
 
 2 . Use the link https://github.gatech.edu/zxu330/cs6210Project1_test to download the testing suite.

 (1) For testing cpu scheduler, follow directions in “cpu/HowToDoTest.md”. This includes five test cases; detailed scenario and expected outcome for each case is found in  “cpu/test cases/(test number)/README.md”.
 (2) For testing memory coordinator, follow directions in “memory/HowToDoTest.md”. This describes three test cases; detailed scenario and expected outcome for each case is in “memory/test cases/(test number)/README.md”.

   * FYI, we will use up to 4 VMs for memory part, and 8 VMs, each with 1 vCPU for CPU part when grading.
 
3. Run your program, which is vcpu_scheduler or memory_coordinator.
 
4. Check the outcome of your code by running the monitor in the provided link (monitor.py) for vcpu_scheduler and memory_coordinator respectively. Once satisfied with your results, store your monitor results for all test cases into log files using script. Note: All log files must follow proper naming convention as mentioned in the deliverables.

Generating log file using script

The script command is used to record the terminal session and store in a log file. Students must record the results of the monitor test (monitor.py) while running test cases of the pertaining program on another terminal. Test cases can be found in the testing suite mentioned in the link earlier. In other words, execute the following commands sequentially :-

1. script vcpu_scheduler1.log or memory_coordinator1.log
2. python monitor.py (vcpu or memory)
3. # Run test case for vcpu or memory on another terminal
4. # Run your scheduler/coordinator program on another terminal
5. exit once you observe satisfying results on your monitor and repeat for next test case

On exiting, the log file will be generated and stored in the current directory. More information can be found by running man script on your terminal.

Make sure to generate separate log files for each of the test cases. We recommend you reboot your VMs before running each test for better judgement of monitor results. 

Compiling On Deepthought

Once you've run and tested your code locally, please ensure that the code compiles successfully on deepthought. This will help us make the grading easier.

All of you must be provided with access to deepthought. To access the cluster, make sure to connect to the Georgia Tech VPN. To log into deepthought from your shell :-

ssh <GT username>@deepthought-login.cc.gatech.edu

On entering your GT password, you will have logged into one of the nodes on the cluster. Copy your files from local and compile them. Please let us know if you face any issues logging in or compiling your code. 

Have a look at https://support.cc.gatech.edu/facilities/instructional-labs/deepthought-cluster for more information on accessing deepthought.

Submit your deliverables on a successful compilation. Please note that you do not have to run the binaries on deepthought, just compile them and make sure they are generated.
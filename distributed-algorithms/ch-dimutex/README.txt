General Information
===================
This package contains source code for the application that implements the
Ricart-Agrawala's algorithm [2] for distributed mutual exclusion with the
optimization suggested by Roucairol-Carvalho [1].

The source code is POSIX compliant and uses the standard C99 data types.

The application is aptly named "dimutex", short for distributed mutual
exclusion.

Building
========
1. % git clone https://github.com/corehacker/projects.git
2. Navigate to distributed-algorithms/ch-dimutex.
   % cd distributed-algorithms/ch-dimutex
3. Issue make command after issuing make clean.
   % ./configure
   % make clean
   % make
   After successful execution of the above commands, the executable "ch-dimutex"
   will be created in ch-dimutex directory.
4. Copy the aplication to the desired directoty.
   % cp ch-dimutex <Desired Directory>
   
Execution
=========
1. Requirements:
   a. Current implementation supports execution os algorithm for 10 nodes.
      Although, changing a few macros will enable the application to support
      more nodes.
   b. All 10 nodes should reachable by each other and all firewall for incoming
      connections for the ports that will be used (refer below) should be
      disabled.
   c. The application accepts only DNS host names.

2. Application Usage:
   Usage: 
      ./ch-dimutex <Total Nodes> <Node Index> <Listen Port Range Start> 
         <Leader Host Name> <Unit Time> <Console Logging> 
         [<Port No For Internal Use>]
   
   Parameter Details: 
      <Total Nodes, Range: 2 - 10> - Mandatory
      <Node Index, Range: 0 - 9; 0th index will mapped to leader.> - Mandatory
      <Listen Port Range Start (> 15000 Preferred)> - Mandatory
      <Leader Host Name (Host Name of Node 0)> - Mandatory
      <Unit Time, in milliseconds. This will be used internally by algorithm for 
         various waits conditions> - Mandatory
      <Console Logging, 1 - Enable, 0 - Disable> - Mandatory
      [<Port No For Internal Use (Defaults to 19000)>] - Optional. But required  
         if bind fails for the internal port

3. Example:
   a. Catch hold of 10 machines named machine1-machine10. (10 nodes)
   b. Navigate to the directory where the "ch-dimutex" executable is located.
      Copy the executable to each of the machines. Recompilation may be necessary
	  for each of the machine.
   c. Starting from machine1, execute the following commands, one on
      each node, in sequence. (This sequence is not necessary, but to make the
      executions simpler.). Here machine1 will be used as the leader
      node. Also 100ms will be used for unit time.
      % ./ch-dimutex 10 0 37001 machine1 100 0
      % ./ch-dimutex 10 1 37001 machine2 100 0
      % ./ch-dimutex 10 2 37001 machine3 100 0
      % ./ch-dimutex 10 3 37001 machine4 100 0
      % ./ch-dimutex 10 4 37001 machine5 100 0
      % ./ch-dimutex 10 5 37001 machine6 100 0
      % ./ch-dimutex 10 6 37001 machine7 100 0
      % ./ch-dimutex 10 7 37001 machine8 100 0
      % ./ch-dimutex 10 8 37001 machine9 100 0
      % ./ch-dimutex 10 9 37001 machine10 100 0
   d. Wait till all the nodes enter and exit the critical section 40 times. At 
      the end of 40 executions, a summary of all the executions and statistics
      will be printed on the console of the leader (machine1).
   e. In case of any node going down (crash/network failure) please restart the
      application as described in c above.

4. Data Collection
   a. Each node writes a log file as follows:
      dimutex_log_node_xx.txt, where xx is the node index.
   a. The file dimutex_stats_summary.txt contains the statistics summary for
      all the nodes aggregated for all the nodes.
	  
5. Miscellaneous
	a. The program has been check for memory leaks and found to be none in the
	   "happy path" case, i.e., when the algorithm executes and finishes till
	   the end.
      
Known Issues
============
1. During the execution of the algorithm, if any of the nodes goes down due to
   a crash or network error, the error cases are not handles in the other 
   nodes. Meaning, the socket/task clean-up parts of the code will not be 
   executed, which will lead to memory leaks.
   
Copyright
=========
Copyright Sandeep Prakash (c), 2012
Sandeep Prakash - 123sandy@gmail.com

References
==========
[1] O.S.F. Carvalho and G. Roucairol. On Mutual Exclusion in Computer Networks 
(Technical Correspondence). Communications of the ACM, February 1983.

[2] G. Ricart and A. K. Agrawala. An Optimal Algorithm for Mutual Exclusion in
Computer Networks. Communications of the ACM, 24(1):9{17, January 1981.
# Data-Tree-Management-in-C

## Programming Assignment 1
**Author :** Rayson Lim Jun Kai  | Kimberlyn Loh Cheng Lin <br />
**ID     :** 1002026 | 1002221 <br />
**Date   :** 28/02/2018  

## Purpose of the program
This program utilises UNIX system calls such as fork,exec and wait in C to traverse a direct acyclic graph (DAG) of user programs in parallel. By transversing through a text file, the user program is read and formulated into a DAG. The program then analyses the graph to determine the eligible process and run them. A process is eligible to run if and only if all of its parents have finished executing, ensuring control and data dependencies. Each node can also have it own input and output redirected through its definition.  

It analyses this graph to determine which processes are eligible to run, and then runs those processes. A process is eligible to run only when all of its parents have completed executing, after which it is forked and executed. Each node can also have its input and output redirected through its definition in the graph file. Additionally, nodes which are runnable can be concurrently executed in their own threads.

## Compliation Method

### Setting up
If you wish to utilise the terminal, go to the folder where you wish to clone the file and paste the following in the terminal
``` 
git clone https://github.com/nosyarlin/Data-Tree-Management-in-C.git
```
else, press the download zip file button instead. 

### Running in Ubuntu/Mac Shell

To compile the file, 
```
gcc starter.c -o <name>
```

Finally to run it,
```
./<name> <graph-file>
```


Ensure that the dependent files are in the same folder as where you run the command. If not, the program will throw an error message saying there is something wrong with the graph.

## Program Flow

The program goes through three steps:
1. Parses the input text file into process nodes
2. Topologically sorts the process nodes
3. Run the processes in topological order

### Parsing Input File
This part is the longest part of the code.
The text file has to be written in the following format:

```
program name with arguments:list of children ID's:input file:output file
```

Each line represents one process node. Within each section delimited by colons, the arguments are further delimited by spaces. i.e. the list of children IDs looks like

```
1 2 3 4
```

One example of a valid line is as follows

```
Sleep 1:1 2 3 4:stdin:stdout
```

After parsing the entire text file, the different elements are stored in a process node structure with the following fields:

```id```: holds the node id of the process

```prog```: holds the command to be run

```args```: holds additional arguments for the command

```num_args```: hold number of arguments this process has

```input```: holds name of input file, can be stdin

```output```: holds name of output file, can be stdout

```parents```: holds an array of parents' IDs

```num_parents```: holds number of parents this process has

```children```: holds an array of children's IDs

```status```: holds the current status of the process, can be INELIGIBLE, READY, RUNNING or FINISHED

First, we go through the entire text file and initialise all the nodes. Since the text file only holds the children of each node, we go through all the nodes once more to update who their parents are. 

### Topological Sort
Sorting the processes ensures a process will only be allowed to run after its parents have all completed their commands. This part is fairly straight forward. 

We keep an array ```visited``` which tells us which node has been visited by our sorting algorithm. ```visited[1] == 1``` would mean process 1 has been visited. 

After which, we start from a random node and recursively go down the graph all visit all of its descendants. Once we have reached the last descendant (a leaf node), we add the node id to the sorted list as we go back up the graph. At each recursion, the ```visited``` array is updated. This is repeated for the remaining unvisited nodes if any. 

### Running All Processes
Finally, we go down the sorted list and run each process in order. 

For each process, we check what are its input and output. If necessary, we use ```dup2()``` to redirect the process' input and output to the correct files. 

After all necessary redirections have been completed, a process is executed using ```execvp()```

There is no concurrency in this program. As the processes run, their statuses will be printed to stdout

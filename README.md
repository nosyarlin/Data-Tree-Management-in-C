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


If you want to just run the file without compling, 
```
./raysonisgreat <input_text_file>
```
else, 
```
gcc starter.c -o <name>
./<name> <input_text_file>
```
Ensure that the dependent files are in the same folder as where you run the command. If not, the program will throw an error message saying there is something wrong with the graph.

## Program Flow

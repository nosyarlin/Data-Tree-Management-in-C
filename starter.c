#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<ctype.h>
#include <sys/wait.h>

#define INELIGIBLE 0
#define READY 1
#define RUNNING 2
#define FINISHED 3

#define MAX_LENGTH 1024
#define MAX_PARENTS 10
#define MAX_CHILDREN 10
#define MAX_NODES 50

int **children;
int *tempNum_children;
int *sortedList;
int *visited;
int count = 0;

typedef struct node {
  int id;
  char prog[MAX_LENGTH];
  char *args[MAX_LENGTH/2 + 1];
  int num_args;
  char input[MAX_LENGTH];
  char output[MAX_LENGTH];
  int parents[MAX_PARENTS];
  int num_parents;
  int children[MAX_CHILDREN];
  int num_children;
  int status;
  pid_t pid;
} node_t;

/**
 * Search for tokens in the string s, separated by the characters in 
 * delimiters. Populate the string array at *tokens.
 *
 * Return the number of tokens parsed on success, or -1 and set errno on 
 * failure.
 */
int parse_tokens(const char *s, const char *delimiters, char ***tokens) {
  const char *s_new;
  char *t;
  int num_tokens;
  int errno_copy;

  /* Check arguments */
  if ((s == NULL) || (delimiters == NULL) || (tokens == NULL)) {
    errno = EINVAL;
    return -1;
  }

  /* Clear token array */
  *tokens = NULL;

  /* Ignore initial segment of s that only consists of delimiters */
  s_new = s + strspn(s, delimiters);

  /* Make a copy of s_new (strtok modifies string) */
  t = (char *) malloc(strlen(s_new) + 1);
  if (t == NULL) {
    return -1;    
  }
  strcpy(t, s_new);

  /* Count number of tokens */
  num_tokens = 0;
  if (strtok(t, delimiters) != NULL) {
    for (num_tokens = 1; strtok(NULL, delimiters) != NULL; num_tokens++) ;
  }

  /* Allocate memory for tokens */
  *tokens = (char**) malloc((num_tokens + 1)*sizeof(char *));
  if (*tokens == NULL) {
    errno_copy = errno;
    free(t);  // ignore errno from free
    errno = errno_copy;  // retain errno from malloc
    return -1;
  }

  /* Parse tokens */
  if (num_tokens == 0) {
    free(t);
  } else {
    strcpy(t, s_new);
    **tokens = strtok(t, delimiters);
    for (int i=1; i<num_tokens; i++) {
      *((*tokens) +i) = strtok(NULL, delimiters);      
    }
  }
  *((*tokens) + num_tokens) = NULL;  // end with null pointer

  return num_tokens;
}

void free_parse_tokens(char **tokens) {
  if (tokens == NULL) {
    return;    
  }
  
  if (*tokens != NULL) {
    free(*tokens);    
  }

  free(tokens);
}

/**
 * Parse the input line at line, and populate the node at node, which will
 * have id set to id.
 * 
 * Return 0 on success or -1 and set errno on failure.
 */
int parse_input_line(char *line, int id, node_t *node) {
  char **strings;  // string array
  char **arg_list;  // string array
  char **child_list;  // string array
  int a;

  /* Split the line on ":" delimiters */
  if (parse_tokens(line, ":\r\n", &strings) == -1) {
    perror("Failed to parse node information");
    return -1;
  }

  /* Parse the space-delimited argument list */
  if (parse_tokens(strings[0], " ", &arg_list) == -1) {
    perror("Failed to parse argument list");
    free_parse_tokens(strings);
    return -1;
  }

  /* Parse the space-delimited child list */
  if (parse_tokens(strings[1], " ", &child_list) == -1) {
    perror("Failed to parse child list");
    free_parse_tokens(strings);
    return -1;
  }

  /* Set node id */
  node->id = id;
  fprintf(stderr, "... id = %d\n", node->id);

  /* Set program name */
  strcpy(node->prog, arg_list[0]);
  fprintf(stderr, "... prog = %s\n", node->prog);

  /* Set program arguments */
  for (a = 0; arg_list[a] != NULL; a++) {
    node->args[a] = arg_list[a];
    node->num_args++;
    fprintf(stderr, "... arg[%d] = %s\n", a, node->args[a]);
  }
  node->args[a] = NULL;
  fprintf(stderr, "... arg[%d] = %s\n", a, node->args[a]);

  /* Set input file */
  strcpy(node->input, strings[2]);
  fprintf(stderr, "... input = %s\n", node->input);
  
  /* Set output file */
  strcpy(node->output, strings[3]);
  fprintf(stderr, "... output = %s\n", node->output);
    
  /* Set child nodes */
  node->num_children = 0;
  if (strcmp(child_list[0], "none") != 0) {
    for (int c = 0; child_list[c] != NULL; c++) {
      if (c < MAX_CHILDREN) {
        if (atoi(child_list[c]) != id) {
          node->children[c] = atoi(child_list[c]);
          children[node->id][c] = atoi(child_list[c]);
          fprintf(stderr, "... child[%d] = %d\n", c, node->children[c]);
          node->num_children++;
        } else {
          perror("Node cannot be a child of itself");
          return -1;
        }
      } else {
        perror("Exceeded maximum number of children per node");
        return -1;
      }
    }
    tempNum_children[node->id] = node->num_children;
  }

  fprintf(stderr, "... num_children = %d\n", node->num_children);

  /* Set node status */
  node->status = 0;

  return 0;
}

/**
 * Parse the file at file_name, and populate the array at n.
 * 
 * Return the number of nodes parsed on success, or -1 and set errno on
 * failure.
 */
int parse_graph_file(char *file_name, node_t *node) {
  FILE *f;
  char line[MAX_LENGTH];
  int id = 0;
  int errno_copy;

  /* Open file for reading */
  fprintf(stderr, "Opening file...\n");
  f = fopen(file_name, "r");
  if (f == NULL) {
    perror("Failed to open file");
    return -1;
  }

  /* Read file line by line */
  fprintf(stderr, "Reading file...\n");
  while (fgets(line, MAX_LENGTH, f) != NULL) {
    strtok(line, "\n");  // remove trailing newline

    /* Parse line */
    fprintf(stderr, "Parsing line %d...\n", id);
    if (parse_input_line(line, id, node) == 0) {
      node++;  // increment pointer to point to next node in array
      id++;  // increment node ID
      if (id >= MAX_NODES) {
        perror("Exceeded maximum number of nodes");
        return -1;
      }
    } else {
      perror("Failed to parse input line");
      return -1;
    }
  }

  /* Handle file reading errors and close file */
  if (ferror(f)) {
    errno_copy = errno;
    fclose(f);  // ignore errno from fclose
    errno = errno_copy;  // retain errno from fgets
    perror("Error reading file");
    return -1;
  }

  /* If no file reading errors, close file */
  if (fclose(f) == EOF) {
    perror("Error closing file");
    return -1;  // stream was not successfully closed
  }
  
  /* If no file closing errors, return number of nodes parsed */  
  return id;
}

/**
 * Parses the process tree represented by nodes and determines the parent(s)
 * of each node.
 */
int parse_node_parents(node_t *nodes, int num_nodes) {
  int **parent_list = (int**) malloc(MAX_NODES * sizeof(int *));
  int k, num_parents;

  // initialize parent list
  for (int i = 0; i < num_nodes; ++i)
  {
    // Set all entries of parent list to -1
    parent_list[i] = (int*) malloc(MAX_PARENTS * sizeof(int));
    for (int j = 0; j < num_nodes; ++j)
    {
      parent_list[i][j] = -1;
    }
  }

  // save parents of nodes to parent list
  for (int i = 0; i < num_nodes; ++i)
  {
    for (int j = 0; j < nodes->num_children; ++j)
    {
      // find next empty slot to write in parent_list[child][k]
      k = 0;
      while (parent_list[nodes->children[j]][k] != -1) {
        k++;
      }
      // return error if exceeded the maximum number of parents per node
      if(k>10){
        perror("Exceeded maximum number of parents per node");
        return -1;
      }
      // insert parent's id in parent_list
      parent_list[nodes->children[j]][k] = nodes->id;
    }
    // move forward in list of nodes
    nodes++;
  }

  // save parents of nodes to the nodes themselves
  nodes--;
  for (int i = num_nodes-1; i > -1; --i)
  {
    // save individal node parent count
    k = 0;
    while (parent_list[i][k] != -1) {
      nodes->parents[k] = parent_list[i][k];
      if(nodes->parents[k] == nodes->id){
        // return error if node is inside the list 
        perror("Node cannot be a parent of itself");
        return -1;
      }
      k++;
    }
    // save individal node's parents
    nodes->num_parents = k;
    // move backwards in list of nodes
    nodes--;
  }

  // free parent_list
  for (int i = 0; i < num_nodes; ++i)
  {
      free(parent_list[i]);
  }
  free(parent_list);

  return 0;
}

/**
 * Checks the status of each node in the process tree represented by nodes and 
 * verifies whether it can progress to the next stage in the cycle:
 *
 * INELIGIBLE -> READY -> RUNNING -> FINISHED
 *
 * Returns the number of nodes that have finished running, or -1 if there was 
 * an error.
 */
int parse_node_status(node_t *nodes, int num_nodes) {
    int *statuses = malloc(num_nodes * sizeof(int));
    int tempStatus;
    int num_finished = 0;

    // Saving all current statuses of nodes
    for (int i = 0; i < num_nodes; ++i)
    {
        if(nodes->status >= 0 && nodes->status <=3){
          // Check if node is done
          if (nodes->status == 3)
          {
              num_finished++;
          }
          // Saving statuses
          statuses[i] = nodes->status;
          nodes++;        
        }
        else{
          //return error for unknown status number 
           perror("Unknown status number detected");
           return -1;
        }
    } 

    // Check if nodes' statuses changed
    nodes--;
    for (int i = 0; i < num_nodes; ++i)
    {
        // Only change status if the node was ineligible
        if (nodes->status == 0)
        {
            // If no parents, you are ready
            if (nodes->num_parents == 0)
            {
                statuses[i] = 1;
                nodes->status = 1;
            }
            // Else check if parents are done
            else
            {
                tempStatus = 1;
                // Runs through all parents to look for unfinished parents
                for (int j = 0; j < nodes->num_parents; ++j)
                {   
                    if (statuses[nodes->parents[j]] != 3)
                    {
                        tempStatus = 0;
                    }
                }
                // Setting new status
                nodes->status = tempStatus;
                statuses[i] = tempStatus;
            }
        }
        nodes--;
    }
    free(statuses);
    return num_finished;
}

/**
 * Prints the process tree represented by nodes to standard error.
 *
 * Returns 0 if printed successfully.
 */
int print_process_tree(node_t *nodes, int num_nodes) {
  char *stat = malloc(10 * sizeof(char));
  for (int i = 0; i < num_nodes; ++i)
  {
    printf("process %d: \nparents: ", nodes->id);
    for (int j = 0; j < nodes->num_parents; ++j)
    {
      printf("%d, ", nodes->parents[j]); 
    }
    printf("\nchildren: ");
    for (int j = 0; j < nodes->num_children; ++j)
    {
      printf("%d, ", nodes->children[j]); 
    }
    printf("\ncommand: %s", nodes->prog);
    printf("\ninput: %s", nodes->input);
    printf("\noutput: %s", nodes->output);

    switch (nodes->status){
        case 0:
            stat = "INELIGIBLE";
            break;
        case 1:
            stat = "READY";
            break;
        case 2:
            stat = "RUNNING";
            break;
        case 3:
            stat = "FINISHED";
            break;
    }

    printf("\nstatus: %s", stat);
    printf("\n\n");
    nodes++;
  }
  return 0;
}

int topoSort(int current_node, int num_nodes) {

    if (current_node > num_nodes){
       perror("Current node number exceeds the total number of nodes");
       return -1;
    }else{
        for (int i = 0; i < tempNum_children[current_node]; ++i)
        {
            if (visited[children[current_node][i]] == 0)
            {
                topoSort(children[current_node][i],num_nodes);
            }
        }
        sortedList[count] = current_node;
        visited[current_node] = 1;
        count++;
    }
}

void cleanUP() {
    
    for (int j = 0; j < MAX_CHILDREN; ++j)
    {
        free(children[j]);
    }
    free(children);
    free(tempNum_children);
    free(sortedList);
    free(visited);
}

int exec_command(char *prog, char **args, char *input, char *output){
    pid_t pid, wpid;
    int status;
    int file;
    pid = fork();   // -1 means forking failed
                    // 0 means forking success
                    // + means returned to parent or caller, contains child id
    if (pid == 0) {
        // redirect stdin if needed
        if (strcmp(input, "stdin") != 0)
        {
            //First, we're going to open a file
            file = open(input, O_RDONLY, S_IRUSR | S_IWUSR);
            if(file < 0)    return -1;

            //Now we redirect standard input to the file using dup2
            if(dup2(file, STDIN_FILENO) < 0)    return -1;
        }

        // redirect stdout if needed
        if (strcmp(output, "stdout") != 0)
        {
            //First, we're going to open a file
            file = open(output, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if(file < 0)    return -1;

            //Now we redirect standard output to the file using dup2
            if(dup2(file, STDOUT_FILENO) < 0)    return -1;
        }

        // execute child process
        if (execvp(prog, args) == -1) {
            perror("execvp error");
        }
        exit(EXIT_SUCCESS);

    } else if (pid < 0) { //error forking 
        perror("fork error");

    } else { // parent process
        do { // wait for child process to be done before resuming
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        if (file >= 0)
        {
            close(file);
        }
    }
    return 1;
}

int run_processes(node_t *nodes, int num_nodes) {
    int current;
    int num_nodes_finished;
    int status;

    count = num_nodes-1;
    current = 0;

    // Run through every node in sorted list
    for (int i = 0; i < num_nodes; ++i)
    {
        // Move to correct node
        while (current != sortedList[count]) {
            if (sortedList[count] > current)
            {
                current++;
                nodes++;
            } else {
                current--;
                nodes--;
            }
        }

        // Start running the node
        nodes->status = 2;
        printf("%s%d\n", "running process ", nodes->id);
        if (exec_command(nodes->prog, nodes->args, nodes->input, nodes->output) < 0)
        {
            perror("Error executing processes");
            return EXIT_FAILURE;
        }
        // Change node status to finish, and move down sorted list
        nodes->status = 3;
        count--;

        // Update all statuses
        num_nodes_finished = parse_node_status(nodes, num_nodes);
        if (num_nodes_finished < 0) {
            perror("Error executing processes");
            cleanUP();
            return EXIT_FAILURE;
        }
    }
    printf("\n");
    print_process_tree(nodes, num_nodes);
}

/**
 * Takes in a graph file and executes the programs in parallel.
 */
int main(int argc, char *argv[]) {
  node_t nodes[MAX_NODES];
  int num_nodes;
  int num_nodes_finished;
  char *filename;
  sortedList = malloc(MAX_NODES * sizeof(int));

  /* Check command line arguments */
  if (argc > 1) {
    filename = argv[1];
  }

  /* Parse graph file */
  fprintf(stderr, "Parsing graph file...\n");
  // Prepare global children array
  children = malloc(MAX_NODES * sizeof(int*));
  visited = malloc(MAX_NODES * sizeof(int*));
  tempNum_children = malloc(MAX_NODES * sizeof(int));
  for (int i = 0; i < MAX_CHILDREN; ++i)
  {
      children[i] = malloc(MAX_CHILDREN * sizeof(int));
  }
  // Parse file
  num_nodes = parse_graph_file(filename, nodes);
  if (num_nodes < 0) {
    cleanUP();
    return EXIT_FAILURE;
  }
  
  /* Parse nodes for parents */
  fprintf(stderr, "Parsing node parents...\n");
  parse_node_parents(nodes, num_nodes);

  /* Print process tree */
  fprintf(stderr, "\nProcess tree:\n");
  print_process_tree(nodes, num_nodes);

  // Topologically sorting processes
  topoSort(nodes->id,num_nodes);
  for (int i = 0; i < num_nodes; ++i)
  {
      if (visited[i] == 0)
      {
        topoSort(i,num_nodes);
      }
  }

  // Printing out sorted list
  printf("%s\n", "Sorted list: ");
  for (int i = 0; i < num_nodes; ++i)
  {
      printf("%d ", sortedList[i]);
  }
  printf("\n\n");

  /* Run processes */
  fprintf(stderr, "Running processes...\n");
  run_processes(nodes, num_nodes);
  
  fprintf(stderr, "All processes finished. Exiting.\n");

  cleanUP();
  return EXIT_SUCCESS;
}


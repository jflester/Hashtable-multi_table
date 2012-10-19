#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include "hashtable.h"

static int comparing(const void *, const void *);
static Table* expandTable(Table *);

/* Creates a new table, with buckets and returns the table.
   If dynamic memory allocation doesn't work, exits with EX_OSERR */
Table *create_table(){
  Table *t;
  t = malloc(sizeof(Table));
  if (t == NULL){
    perror("Memory Allocation Error");
    exit(EX_OSERR);
  }
  t->key_ct = 0;
  t->bucket_ct = INIT_BUCKETS;
  t->next = NULL;
  t->buckets = calloc(sizeof(Hash_bucket), INIT_BUCKETS);
  if (t->buckets == NULL){
    perror("Memory Allocation Error");
    exit(EX_OSERR);
  }
  return t;
}

/* Resets the table to the created state.
   If any expanded tables exist, destroys them. */
void reset_table(Table *t){
  Data_pair_node *curr, *prev;
  int i;
  if (t == NULL){
    return;
  }
  t->key_ct = 0;
  for (i = 0; i < t->bucket_ct; i++){
      curr = t->buckets[i];
      while (curr != NULL){
	prev = curr;
	curr = curr->next;
	free(prev->key);
	prev->key = NULL;
	free(prev->value);
	prev->value = NULL;
	free(prev);
	prev = NULL;
      }
      if (t->buckets[i] != NULL){
	t->buckets[i] = NULL;
      } 
  }
 /* Recursively destroys expanded tables */
  if (t->next != NULL){
    reset_table(t->next);
    free(t->next->buckets);
    t->next->buckets = NULL;
    free(t->next);
    t->next = NULL;
  }
  return;
}

/* Inserts key and val into the table.
   If the average chain length is greater than MAX_CHAIN_LENGTH
   then the table gets expanded upon finishing the current insertion.
   If any parameters are NULL or if insertion fails, then returns -1.
   Returns 0 if success.
   If dynamic memory allocation doesn't work, exits with EX_OSERR. */
int insert(Table *t, const char *key, const char *val){
  int hashcode, wcode;
  Data_pair_node *curr;
  char *search_val;
  Table *w;
  if (t == NULL || key == NULL || val == NULL){
    return -1;
  }
  /* If the key is already in the table chain, jumps to that table,
     finds the spot where the keys are equal, and changes the value there
     to the val passed in. */
  search_val = NULL;
  if (search(t, key, &search_val) == 0){
    free(search_val);
    w = t;
    while(w != NULL){
      wcode = hash_code(key) % w->bucket_ct;
      curr = w->buckets[wcode];
      while (curr != NULL){
	if (strcmp(curr->key, key) == 0){
	  free(w->buckets[wcode]->value);
	  w->buckets[wcode]->value = NULL;
	  w->buckets[wcode]->value = malloc(strlen(val)+1);
	  if (w->buckets[wcode]->value == NULL){
	    perror("Memory Allocation Error");
	    exit(EX_OSERR);
	  }
	  strcpy(w->buckets[wcode]->value, val);
	  return 0;
	}
	curr = curr->next;
      }
      w = w->next;
    }
  }
  free(search_val);
  /* If the key does not already exist in the table, and we
     are in the most recent table (ie. the largest table), will
     try to insert. */
  hashcode = hash_code(key) % t->bucket_ct;
  if (t->next == NULL){
    /* If there's nothing in the list yet, will make insertion
       to first element of list at buckets[hashcode]. */
    if (t->buckets[hashcode] == NULL){
      t->buckets[hashcode] = malloc(sizeof(Data_pair_node));
      if (t->buckets[hashcode] == NULL){
	perror("Memory Allocation Error");
	exit(EX_OSERR);
      }
      t->buckets[hashcode]->key = NULL;
      t->buckets[hashcode]->value = NULL;
      t->buckets[hashcode]->next = NULL;
    }
    if (t->buckets[hashcode]->key == NULL){
      t->buckets[hashcode]->key = malloc(strlen(key)+1);
      if (t->buckets[hashcode]->key == NULL){
	perror("Memory Allocation Error");
	exit(EX_OSERR);
      }
      strcpy(t->buckets[hashcode]->key, key);
      t->buckets[hashcode]->value = malloc(strlen(val)+1);
      if (t->buckets[hashcode]->value == NULL){
	perror("Memory Allocation Error");
	exit(EX_OSERR);
      }
      strcpy(t->buckets[hashcode]->value, val);
      t->key_ct++;
      t->buckets[hashcode]->next = NULL;
      /* If table is large enough, will do table expansion. */
      if ((double)key_count(t)/(double)bucket_count(t) > MAX_CHAIN_LENGTH){
	t->next = expandTable(t);
      }
      return 0;
    }
    /* Insert key and val into hashed bucket in most recent table
       at end of list. */
    else {
      curr = t->buckets[hashcode];
      while (curr->next != NULL){
	if (strcmp(curr->key, key) == 0){
	  free(curr->value);
	  t->buckets[hashcode]->value = NULL;
	  curr->value = malloc(strlen(val)+1);
	  if (curr->value == NULL){
	    perror("Memory Allocation Error");
	    exit(EX_OSERR);
	  }
	  strcpy(curr->value, val);
	  return 0;
	}
	curr = curr->next;
      }
      curr->next = malloc(sizeof(Data_pair_node));
      if (curr->next == NULL){
	perror("Memory Allocation Error");
	exit(EX_OSERR);
      }
      curr->next->key = malloc(strlen(key)+1);
      if (curr->next->key == NULL){
	perror("Memory Allocation Error");
	exit(EX_OSERR);
      }
      strcpy(curr->next->key, key);
      curr->next->value = malloc(strlen(val)+1);
      if (curr->next->value == NULL){
	perror("Memory Allocation Error");
	exit(EX_OSERR);
      }
      strcpy(curr->next->value, val);
      curr->next->next = NULL;
      t->key_ct++;
      /* Table expands, if large enough. */
      if ((double)key_count(t)/(double)bucket_count(t) > MAX_CHAIN_LENGTH){
	t->next = expandTable(t);
      }
      return 0;
    }
  }
  /* Recursive call to move to larger table to do insert
     if table has been previously expanded */
  else if (t->next != NULL){   
    return insert(t->next, key, val);
  }
  else {
    return -1;
  }
}

/* Searches through the table for key, if key is found
   then val gets the value associated with the specified key.
   Returns 0 if success.
   Returns -1 if t, key, or val is NULL or if search fails.
   If dynamic memory allocation doesn't work, exits with EX_OSERR. */
int search(Table *t, const char *key, char **val){
  int hashcode;
  Data_pair_node *curr;
  if (t == NULL || key == NULL || val == NULL){
    return -1;
  }
  /* Goes to hashed position, if any key in the list equals the
     key passed in, the val gets the value at that location */
  hashcode = hash_code(key) % t->bucket_ct;
  if (t->buckets[hashcode] != NULL && t->buckets[hashcode]->key != NULL){
    curr = t->buckets[hashcode];
    while (curr != NULL){
      if (strcmp(curr->key, key) == 0){
	*val = malloc(strlen(curr->value)+1);
	if (*val == NULL){
	  perror("Memory Allocation Error");
	  exit(EX_OSERR);
	}
	strcpy(*val, curr->value);
	return 0;
      }
      curr = curr->next;
    }
  }
  /* If the key isn't in the table, recursively checks through
     all the expanded tables. */
  if (t->next != NULL){
    return search(t->next, key, val);
  }
  return -1;
}

/* Deletes the specified key, and corresponding value from the table.
   If t or key is NULL or deletion fails, then returns -1.
   Otherwise, returns 0.
   If dynamic memory allocation doesn't work, exits with EX_OSERR. */
int delete(Table *t, const char *key){
  Data_pair_node *curr, *prev;
  int hashcode;
  if (t == NULL || key == NULL){
    return -1;
  }
  /* Checking the base case, if key found, then delete it. */
  hashcode = hash_code(key) % t->bucket_ct;
  if (t->buckets[hashcode] != NULL && t->buckets[hashcode]->key != NULL){
    if (strcmp(t->buckets[hashcode]->key, key) == 0){
      if (t->buckets[hashcode]->next == NULL){
	free(t->buckets[hashcode]->key);
	t->buckets[hashcode]->key = NULL;
	free(t->buckets[hashcode]->value);
	t->buckets[hashcode]->value = NULL;
	t->key_ct--;
	return 0;
      }
      prev = t->buckets[hashcode]->next;
      free(t->buckets[hashcode]->key);
      t->buckets[hashcode]->key = NULL;
      free(t->buckets[hashcode]->value);
      t->buckets[hashcode]->value = NULL;
      t->buckets[hashcode] = prev;
      t->key_ct--;
      return 0;
    }
    curr = t->buckets[hashcode]->next;
    prev = t->buckets[hashcode];
    while (curr != NULL){
      if (strcmp(curr->key, key) == 0){
	curr = curr->next;
	free(prev->next->key);
	prev->next->key = NULL;
	free(prev->next->value);
	prev->next->value = NULL;
	free(prev->next);
	prev->next = curr;
	t->key_ct--;
	return 0;
      }
      curr = curr->next;
      prev = prev->next;
    }
  }
  /* Recurse upon any expanded tables */
  if (t->next != NULL){
    return delete(t->next, key);
  }
  return -1;
}

/* Returns the key count across all tables */
int key_count(Table *t){
  int total;
  Table *temp;
  if (t != NULL){
    total = t->key_ct;
    if (t->next == NULL){
      return total;
    }
    temp = t;
    while (temp->next != NULL){
      temp = temp->next;
      total += temp->key_ct;
    }
    return total;
  }
  return -1;
}

/* Returns the bucket count across all tables. */
int bucket_count(Table *t){
  int total;
  Table *temp;
  if (t != NULL){
    total = t->bucket_ct;
    if (t->next == NULL){
      return total;
    }
    temp = t;
    while (temp != NULL){
      temp = temp->next;
      total += temp->bucket_ct;
    }
    return total;
  }
  return -1;
}

/* Hashcode algorithm to determine which bucket things get placed.
   If dynamic memory allocation doesn't work, exits with EX_OSERR. */
unsigned long hash_code(const char *str){
  if (str == NULL){
    return 0;
  }
  else if (*str == '\0'){
    return 0;
  }
  else {
    int new_char;
    size_t string_size;
    int last_char;
    unsigned long answer;
    char *new_str = malloc(strlen(str)+1);
    if (new_str == NULL){
      perror("Memory Allocation Error");
      exit(EX_OSERR);
    }
    strcpy(new_str, str);
    string_size = strlen(new_str);
    last_char = string_size - 1;
    new_char = new_str[last_char];
    new_str[last_char] = '\0';
    answer = hash_code(new_str) * 65599 + new_char;
    free(new_str);
    return answer;
  }
}

/* Returns a char** of all the keys across all tables.
   The array is sorted via qsort.
   If the table is NUll, returns NULL.
   If dynamic memory allocation doesn't work, exits with EX_OSERR. */
char **get_keys(Table *t){
  char **p;
  Data_pair_node *curr;
  Table *advance;
  int i, k = 0;
  if (t == NULL){
    return NULL;
  }
  advance = t;
  p = calloc(sizeof(char*), key_count(advance));
  if (p == NULL){
    perror("Memory Allocation Error");
    exit(EX_OSERR);
  }
  while (advance != NULL){
    for (i = 0; i < advance->bucket_ct; i++){
      if (advance->buckets[i] != NULL && advance->buckets[i]->key != NULL){
	curr = advance->buckets[i];
	while (curr != NULL){
	  p[k] = malloc(strlen(curr->key)+1);
	  if (p[k] == NULL){
	    perror("Memory Allocation Error");
	    exit(EX_OSERR);
	  }
	  strcpy(p[k], curr->key);
	  k++;
	  curr = curr->next;
	}
      }
    }
    /* Recurses to expanded tables. */
    advance = advance->next;
  }
  /* Qsort the array (increasing order). */
  qsort(p, key_count(t), sizeof(char*), comparing);
  return p;
}

/* Returns a char** of all the values across the tables.
   Sorted by correspondence to keys sorted by qsort.
   If dynamic memory allocation doesn't work, exits with EX_OSERR.
   If the table is NULL, returns NULL. */
char **get_values(Table *t){
  char **keyset = get_keys(t);
  char **value_array;
  char **val;
  int i;
  if (t == NULL || keyset == NULL){
    return NULL;
  }
  value_array = calloc(sizeof(char *), key_count(t));
  if (value_array == NULL){
    perror("Memory Allocation Error");
    exit(EX_OSERR);
  }
  val = malloc(sizeof(char*));
  if (val == NULL){
    perror("Memory Allocation Error");
    exit(EX_OSERR);
  }
  /* Goes through the array returned by get_keys
     Searches for key, to get value associated with that key
     Copies value to value_array. */
  for (i = 0; i <key_count(t); i++){
    search(t, keyset[i], val);
    value_array[i] = malloc(strlen(*val)+1);
    if (value_array[i] == NULL){
      perror("Memory Allocation Error");
      exit(EX_OSERR);
    }
    strcpy(value_array[i], *val);
  }
  free(*val);
  return value_array;
}

/* Completely destroys the table.
   No table operations are possible after this method has been called. */
void destroy_table(Table *t){
  if (t == NULL){
    return;
  }
  reset_table(t);
  free(t->buckets);
  if (t->next != NULL){
    free(t->next);
  }
  free(t);
}

/* Helper methods */

/* Helper method for qsort */
int comparing(const void *one, const void *two){
  const char **a = (const char **)one;
  const char **b = (const char **)two;
  return strcmp(*a, *b);
}

/* Expands the table to have twice the number of buckets
   as the previous table, and is linked to next of the most
   recent table. */
Table* expandTable(Table *t){
  Table *next_table;
  next_table = malloc(sizeof(Table));
  if (next_table == NULL){
    perror("Memory Allocation Error");
    exit(EX_OSERR);
  }
  next_table->bucket_ct = 2*t->bucket_ct;
  next_table->key_ct = 0;
  next_table->next = NULL;
  next_table->buckets = calloc(sizeof(Hash_bucket), next_table->bucket_ct);
  if (next_table->buckets == NULL){
    perror("Memory Allocation Error");
    exit(EX_OSERR);
  }
  return next_table;
}
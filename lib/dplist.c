#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
	#define DEBUG_PRINTF(...) 									         \
		do {											         \
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	 \
			fprintf(stderr,__VA_ARGS__);								 \
			fflush(stderr);                                                                          \
                } while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition,err_code)\
	do {						            \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");    \
            assert(!(condition));                                    \
        } while(0)


struct dplist_node {
  dplist_node_t * prev, * next;
  void * element;
};

struct dplist {
  dplist_node_t * head;
  void * (*element_copy)(void * src_element);			  
  void (*element_free)(void ** element);
  int (*element_compare)(void * x, void * y);
};


dplist_t * dpl_create (
			  void * (*element_copy)(void * src_element),
			  void (*element_free)(void ** element),
			  int (*element_compare)(void * x, void * y)
			  )
{
  dplist_t * list;
  list = malloc(sizeof(struct dplist));
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
  list->head = NULL;  
  list->element_copy = element_copy;
  list->element_free = element_free;
  list->element_compare = element_compare; 
  return list;
}

void dpl_free(dplist_t ** list, bool free_element)
{
    int size_of_list=dpl_size(*list);
    for(int i=0;i<size_of_list;i++)
    {
        dpl_remove_at_index(*list,0,free_element);
    }
   free(*list);
   *list=NULL;
}

dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{ 
	  dplist_node_t * ref_at_index, * list_node;
	  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
	  list_node = malloc(sizeof(dplist_node_t));
	  DPLIST_ERR_HANDLER(list_node==NULL,DPLIST_MEMORY_ERROR);
	  if(insert_copy){
   		 list_node->element=list->element_copy(element);
   	 }
   	 else{
   		 list_node->element=element;
   	    }
	  if (list->head == NULL)  
	  { // covers case 1 
	    list_node->prev = NULL;
	    list_node->next = NULL;
	    list->head = list_node;
	  } else if (index <= 0)  //add to the head
	  { // covers case 2 
	    list_node->prev = NULL;
	    list_node->next = list->head;
	    list->head->prev = list_node;
	    list->head = list_node;
	  } else 
	  {
	    ref_at_index = dpl_get_reference_at_index(list, index);  
	    assert( ref_at_index != NULL);
	    if (index < dpl_size(list))
	    { // covers case 4
	      list_node->prev = ref_at_index->prev;
	      list_node->next = ref_at_index;
	      ref_at_index->prev->next = list_node;
	      ref_at_index->prev = list_node;
	    } else
	    { // covers case 3 
	      assert(ref_at_index->next == NULL);
	      list_node->next = NULL;
	      list_node->prev = ref_at_index;
	      ref_at_index->next = list_node;    
	    }
          }
  return list;
}

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
	    
DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);
int size=dpl_size(list);

// if no nodes
if(size==0) {return list;}

if(size>0)
{
//if there is node
dplist_node_t *ref=dpl_get_reference_at_index(list, index);
if (free_element)
{
list->element_free(&(ref->element));
}
 
if(size==1){list->head=NULL;} //only one reference
else{  
	if(index<=0) //no prev
	{
	list->head=ref->next;
	ref->prev=NULL;
	}
	else if(index>=size-1)//no next
	{
	(ref->prev)->next=NULL;
	}
	else{
	ref->prev->next=ref->next;
	ref->next->prev=ref->prev;
	}
    }
free(ref);
ref=NULL;
}
    
return list;
  
}

	


int dpl_size( dplist_t * list )
{
  //empty list
  if(list->head==NULL)
  {
    return 0;
  }
  //list with something
  else
  {
     dplist_node_t* l;
     l=list->head;
     int size=1;
     while(l->next!=NULL)
    {
      l=l->next;
      size++;
  }
  return size;
  }
}

void * dpl_get_element_at_index( dplist_t * list, int index )
{
  int size_of_list=dpl_size(list);
  if((list->head)==NULL){return ((void *)0);}
  if(index<=0){
  return (dpl_get_reference_at_index(list,0)->element);
 }
  else if(index<=size_of_list)
  {
    return (dpl_get_reference_at_index(list,index)->element);
  }
  else 
  {
    return (dpl_get_reference_at_index(list,size_of_list-1)->element);
  }

}

int dpl_get_index_of_element( dplist_t * list, void * element )
{ 
  int size_of_list=dpl_size(list);
  dplist_node_t* l;
  l=list->head;
  for(int i=0;i<size_of_list;i++,l=l->next)
  {
    int flag=0;
    flag=list->element_compare(element,l->element);
    if(flag==0){return i;}//match found
    
  }
  return -1;
}

dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
  int count;
  dplist_node_t * dummy;
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if (list->head == NULL ) return NULL;
  for ( dummy = list->head, count = 0; dummy->next != NULL ; dummy = dummy->next, count++) 
  { 
    if (count >= index) return dummy;
  }  
  return dummy;
}
 
void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
    if(list->head==NULL){return NULL;}
    if(reference==NULL){
      int size=dpl_size(list);
      return (dpl_get_reference_at_index(list,size-1)->element);  
    }
    //check if the reference is in the list
    dplist_node_t* dummy=list->head;
    for(int i=0;i<dpl_size(list);i++){
        if(dummy==reference){return dummy->element;}
        if(dummy->next!=NULL)
        {dummy=dummy->next;}
    }
   return NULL;
}



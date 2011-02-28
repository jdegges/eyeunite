#include "debug.h"
#include "list.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


struct list
{
  void **data;
  uint64_t count;
  uint64_t size;
};

struct list *
list_new (void)
{
  struct list *l = calloc (1, sizeof (struct list));

  if (NULL == l)
    {
      print_error ("Out of memory");
      exit (EXIT_FAILURE);
    }

  return l;
}

void
list_add (struct list *l, void *item)
{
  if (NULL == l)
    {
      print_error ("Invalid list");
      exit (EXIT_FAILURE);
    }

  if (l->size <= l->count)
    {
      uint64_t size = l->size ? l->size * 2 : 2;
      void **data = NULL;
      void **old_data;

      if (NULL == (data = malloc (sizeof (void *) * size)))
        {
          print_error ("Out of memory");
          exit (EXIT_FAILURE);
        }

      memcpy (data, l->data, sizeof (void *) * l->count);

      old_data = l->data;
      l->data = data;
      l->size = size;

      free (old_data);
    }

  l->data[l->count++] = item;
}

void *
list_get (struct list *l, uint64_t i)
{
  if (NULL == l)
    {
      print_error ("Invalid list");
      exit (EXIT_FAILURE);
    }
    
  if (NULL == l->data)
    {
      print_error ("Invalid list");
      exit (EXIT_FAILURE);
    }
 
  if (l->count <= i)
    {
      print_error ("Invalid item");
      exit (EXIT_FAILURE);
    }
 
  return l->data[i];
}

void
list_remove_data (struct list *l, void *data)
{
  uint64_t i;

  if (NULL == l)
    {
      print_error ("Invalid list");
      exit (EXIT_FAILURE);
    }

  if (NULL == l->data)
    {
      print_error ("Invalid list");
      exit (EXIT_FAILURE);
    }

  for (i = 0; i < l->count; i++)
    {
      if (data == l->data[i])
        {
          for (; i < l->count - 1; i++)
            l->data[i] = l->data[i + 1];
          l->count--;
        }
    }
}

void
list_remove_item (struct list *l, uint64_t i)
{
  if (NULL == l)
    {
      print_error ("Invalid list");
      exit (EXIT_FAILURE);
    }

  if (NULL == l->data)
    {
      print_error ("Invalid list");
      exit (EXIT_FAILURE);
    }

  if (l->count <= i)
    {
      print_error ("Invalid item");
      exit (EXIT_FAILURE);
    }

  for (; i < l->count; i++)
    l->data[i] = l->data[i + 1];
  l->count--;
}

uint64_t
list_count (struct list *l)
{
  if (NULL == l)
    {
      print_error ("Invalid list");
      exit (EXIT_FAILURE);
    }
    
  return l->count;
}

void
list_free (struct list *l)
{
  if (l)
    free (l->data);
  free (l);
}
